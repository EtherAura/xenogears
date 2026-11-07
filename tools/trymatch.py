#!/usr/bin/env python3
"""Compile a standalone C file and compare each function in it with retail.

This is the quick loop for matching: nothing in the build is touched, so it
can run while other work is in progress. Functions are found by name:
Names are looked up in config/<ver>/symbols/<module>.txt; a func_XXXXXXXX name
not listed there gives the address directly. Words carrying a relocation (call targets,
%hi/%lo/%gp_rel halves) are compared with the relocated field masked out,
and their targets are checked separately: for jal, %lo and %gp_rel the
symbol+addend the C object refers to must be the address retail's assembly
names at that instruction (%hi halves and branches are left to the build).

    python tools/trymatch.py us main candidate.c
    python tools/trymatch.py us main candidate.c --cc gcc-2.7.2-cdk --diff
"""

from __future__ import annotations

import argparse
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
TC = ROOT / "tools/toolchain"
CROSS = TC / "bin/mips-linux-gnu-"
MASKS = {"R_MIPS_26": 0xFC000000, "R_MIPS_HI16": 0xFFFF0000, "R_MIPS_LO16": 0xFFFF0000,
         "R_MIPS_GPREL16": 0xFFFF0000, "R_MIPS_PC16": 0xFFFF0000}


def run(cmd: list[str], **kw) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def compile_c(src: Path, version: str, cc: str, cflags: str, maspsx: str, work: Path,
              includes: list[Path] = (), as_g: int | None = None) -> Path:
    i, s, o = work / "t.i", work / "t.s", work / "t.o"
    steps = [
        ["cpp", "-nostdinc", "-undef", "-P", "-D_LANGUAGE_C", f"-DVERSION_{version.upper()}",
         *[f"-I{d}" for d in includes], f"-I{ROOT / 'include'}", str(src), "-o", str(i)],
        [str(TC / cc / "cc1"), *cflags.split(), "-o", str(s), str(i)],
    ]
    for cmd in steps:
        p = run(cmd)
        if p.returncode:
            sys.exit(p.stdout + p.stderr)
    gflag = next((f for f in cflags.split() if f.startswith("-G")), "-G0")
    if as_g is not None:  # the preset's asG: the assembler's own -G
        gflag = f"-G{as_g}"
    with open(s) as fh:
        p = subprocess.run([sys.executable, str(TC / "maspsx/maspsx.py"), "--run-assembler",
                            "--gnu-as-path", f"{CROSS}as", *maspsx.split(), "-EL", "-march=r3000",
                            "-mtune=r3000", "-no-pad-sections", f"-I{ROOT / 'include'}",
                            f"-I{ROOT / 'asm' / version}", gflag,
                            "-o", str(o)], stdin=fh, capture_output=True, text=True, cwd=ROOT)
    if p.returncode:
        sys.exit(p.stdout + p.stderr)
    return o


def elf_functions(obj: Path) -> dict[str, tuple[int, int]]:
    out = run([f"{CROSS}readelf", "-sW", str(obj)]).stdout
    funcs = {}
    for line in out.splitlines():
        p = line.split()
        if len(p) >= 8 and p[3] == "FUNC" and p[6] not in ("UND", "ABS"):
            funcs[p[7]] = (int(p[1], 16), int(p[2], 0))
    return funcs


def text_relocs(obj: Path) -> tuple[dict[int, str], dict[int, str]]:
    """Relocation type and symbol name by .text offset."""
    out = run([f"{CROSS}readelf", "-rW", str(obj)]).stdout
    relocs, syms, in_text = {}, {}, False
    for line in out.splitlines():
        if line.startswith("Relocation section"):
            in_text = "'.rel.text'" in line
            continue
        p = line.split()
        if in_text and len(p) >= 3 and re.fullmatch(r"[0-9a-f]+", p[0]):
            relocs[int(p[0], 16)] = p[2]
            if len(p) >= 5:
                syms[int(p[0], 16)] = p[4]
    return relocs, syms


STALE_NOTED: set[tuple[str, str]] = set()


def fresh_fullasm(version: str, module: str) -> Path | None:
    """The module's full disassembly (tools/fullasm.py), unless it is older than the symbol files
    it was made with: its operands then spell old names (renamed or swapped since), and every
    instruction using one would look like a wrong relocation target (xg-1jd)."""
    full = ROOT / "work" / "fullasm" / version / module / "asm"
    if not full.is_dir():
        return None
    made = (full.parent / "paths.yaml").stat().st_mtime if (full.parent / "paths.yaml").exists() else 0
    symbols = [ROOT / "config" / version / "symbols" / f"{m}.txt" for m in (module, "main")]
    if any(f.exists() and f.stat().st_mtime > made for f in symbols):
        if (version, module) not in STALE_NOTED:
            STALE_NOTED.add((version, module))
            print(f"note: {full.relative_to(ROOT)} is older than the symbol files: not used, so only "
                  f"functions still in asm/ get their targets checked (refresh: tools/fullasm.py {version} {module})",
                  file=sys.stderr)
        return None
    return full


def retail_operands(version: str, module: str) -> dict[int, str]:
    """Operand text of every instruction in splat's assembly, by vram: the
    full disassembly (tools/fullasm.py) covers functions already in C."""
    ops = {}
    full = fresh_fullasm(version, module)
    dirs = [full] if full is not None else []
    for f in [p for d in dirs + [ROOT / "asm" / version / module] for p in d.rglob("*.s")]:
        for line in f.read_text().splitlines():
            m = re.match(r"\s*/\* [0-9A-F]+ ([0-9A-F]{8}) [0-9A-F]{8} \*/\s+\S+\s+(.*)$", line)
            if m:
                ops[int(m.group(1), 16)] = m.group(2)
    return ops


def symbol_address(name: str, names: dict[str, int]) -> int | None:
    if name in names:          # the symbol file wins over the name's spelling
        return names[name]
    m = re.fullmatch(r"(?:func|D|jtbl|sym)_([0-9A-F]{8})", name)
    return int(m.group(1), 16) if m else None


def retail_target(operand: str, names: dict[str, int]) -> int | None:
    """Address named by jal X / %lo(X + N) / %gp_rel(X) in an operand."""
    m = re.search(r"%(?:lo|gp_rel)\((\w+)(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?\)", operand)
    if not m:
        m = re.fullmatch(r"(\w+)", operand.strip())
    if not m:
        return None
    base = symbol_address(m.group(1), names)
    if base is None:
        return None
    extra = m.group(2) if m.lastindex and m.lastindex >= 2 else None
    return base + (int(extra, 0) if extra else 0)


def symbol_table(version: str, module: str) -> dict[str, int]:
    table = {}
    for f in (ROOT / "config" / version / "symbols").glob("*.txt"):
        for line in f.read_text().splitlines():
            m = re.match(r"\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", line)
            if m and (f.stem == module or f.stem == "main"):
                table.setdefault(m.group(1), int(m.group(2), 16))
    return table


def retail_sizes(version: str, module: str) -> dict[int, int]:
    """Retail function sizes: from the full disassembly (tools/fullasm.py) when
    there is one, since its boundaries follow the symbol files; otherwise from
    the build's ELF."""
    elf = ROOT / "build" / version / f"{module}.elf"
    sizes = {addr: size for addr, size in elf_functions(elf).values()} if elf.is_file() else {}
    full = fresh_fullasm(version, module)
    for f in full.rglob("*.s") if full is not None else []:
        for m in re.finditer(r"^nonmatching (\w+), (0x[0-9A-Fa-f]+)\n+glabel \1\n\s*/\* \w+ ([0-9A-F]{8}) ",
                             f.read_text(), re.M):
            sizes[int(m.group(3), 16)] = int(m.group(2), 16)
    return sizes


def disasm(words: list[int], vram: int) -> list[str]:
    with tempfile.NamedTemporaryFile(suffix=".bin") as tmp:
        tmp.write(struct.pack(f"<{len(words)}I", *words))
        tmp.flush()
        out = run([f"{CROSS}objdump", "-D", "-b", "binary", "-m", "mips:3000", "-EL", "-z",
                   f"--adjust-vma={vram:#x}", tmp.name]).stdout
    return [l.split(":", 1)[1].strip() for l in out.splitlines() if re.match(r"\s*[0-9a-f]{8}:\t", l)]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version")
    parser.add_argument("module")
    parser.add_argument("source", type=Path)
    parser.add_argument("--cc", help="cc1 build (default: the file's preset in config/build.yaml)")
    parser.add_argument("--cflags", help="override cflags")
    parser.add_argument("--maspsx", help="override maspsx flags")
    parser.add_argument("--resolve", action="store_true",
                        help="print the retail address of each symbol the version does not define")
    parser.add_argument("--diff", action="store_true", help="show disassembly even for matches")
    parser.add_argument("--only", nargs="*", default=[], help="only check these functions")
    parser.add_argument("-I", dest="includes", action="append", type=Path, default=[],
                        help="extra include directory, searched before include/ (repeatable)")
    parser.add_argument("--as", dest="as_file", type=Path,
                        help="use this project file's preset (for a copy of a file under src/)")
    parser.add_argument("--rodata", action="store_true",
                        help="also dump the candidate's .rodata and retail's rodata used by each function")
    args = parser.parse_args()

    build = yaml.safe_load((ROOT / "config/build.yaml").read_text())["default"]
    src = (args.as_file or args.source).resolve()
    if src.is_relative_to(ROOT / "src"):
        # Project files use their preset from config/build.yaml.
        sys.path.insert(0, str(ROOT / "tools"))
        from configure import load_presets, preset_for
        default, presets = load_presets()
        build = preset_for(str(src.relative_to(ROOT)), default, presets, args.version)
    manifest = yaml.safe_load((ROOT / "config" / args.version / "disc.yaml").read_text())
    entry = next(e for e in manifest["files"] if e["name"] == args.module)
    header = 0x800 if "iso" in entry else 0
    retail = (ROOT / "disc" / args.version / (args.module + (".exe" if "iso" in entry else ".bin"))).read_bytes()
    sizes = retail_sizes(args.version, args.module)
    names = symbol_table(args.version, args.module)

    with tempfile.TemporaryDirectory() as work:
        obj = compile_c(args.source.resolve(), args.version, args.cc or build["cc"],
                        args.cflags or build["cflags"], args.maspsx or build["maspsx"], Path(work),
                        [d.resolve() for d in args.includes], build.get("asG"))
        funcs = elf_functions(obj)
        relocs, reloc_syms = text_relocs(obj)
        data = run([f"{CROSS}objcopy", "-O", "binary", "--only-section=.text", str(obj), f"{work}/t.bin"])
        text = Path(f"{work}/t.bin").read_bytes()
        if args.rodata:
            print("== candidate .rodata (relocated words are 0 here; see relocations) ==")
            print(run([f"{CROSS}objdump", "-s", "-j", ".rodata", str(obj)]).stdout.split("Contents of section .rodata:")[-1].rstrip())
            rel = run([f"{CROSS}readelf", "-rW", str(obj)]).stdout
            if "'.rel.rodata'" in rel:
                print(rel[rel.index("'.rel.rodata'"):].split("\n\n")[0])

    operands = retail_operands(args.version, args.module)
    bad = 0
    resolved: dict[str, set[int]] = {}
    for name, (off, size) in sorted(funcs.items(), key=lambda kv: kv[1][0]):
        if name.startswith("__maspsx") or (args.only and name not in args.only):
            continue
        # The symbol file wins: shared C uses one version's address names.
        m = re.fullmatch(r"func_([0-9A-F]{8})", name)
        addr = names.get(name, int(m.group(1), 16) if m else None)
        if addr is None:
            print(f"??   {name}: no address (not func_XXXXXXXX and not in symbols)")
            continue
        want_size = sizes.get(addr)
        roff = addr - entry["vram"] + header
        mine = list(struct.unpack_from(f"<{size // 4}I", text, off))
        theirs = list(struct.unpack_from(f"<{(want_size or size) // 4}I", retail, roff))
        masked_m, masked_t = [], []
        for i, (a, b) in enumerate(zip(mine, theirs)):
            mask = MASKS.get(relocs.get(off + 4 * i), 0xFFFFFFFF)
            masked_m.append(a & mask)
            masked_t.append(b & mask)
        # Check where each relocated jal/%lo/%gp_rel points.
        wrong_targets = []
        last_hi: dict[str, int] = {}  # symbol -> upper addend of its latest %hi
        for i, word in enumerate(mine):
            kind = relocs.get(off + 4 * i)
            if kind == "R_MIPS_HI16":
                last_hi[reloc_syms.get(off + 4 * i, "")] = word & 0xFFFF
            if kind not in ("R_MIPS_26", "R_MIPS_LO16", "R_MIPS_GPREL16"):
                continue
            want = retail_target(operands.get(addr + 4 * i, ""), names)
            sym = reloc_syms.get(off + 4 * i, "")
            # with --resolve, a name this version's symbol file lacks is unknown
            base = None if args.resolve and sym not in names else symbol_address(sym, names)
            addend = 0 if kind == "R_MIPS_26" else ((word & 0xFFFF) ^ 0x8000) - 0x8000
            if kind == "R_MIPS_LO16" and sym in last_hi:
                # REL addends are split: %hi holds the upper half (AHL).
                addend = ((last_hi[sym] << 16) + addend + 0x80000000) % 0x100000000 - 0x80000000
            if base is None and want is not None and args.resolve:
                resolved.setdefault(reloc_syms.get(off + 4 * i, ""), set()).add(want - addend)
            if base is None and want is not None and not args.resolve \
                    and kind == "R_MIPS_26" and sym not in funcs:
                # a call to a name no symbol file knows and this object does
                # not define (e.g. a typo'd, implicitly declared function)
                wrong_targets.append(f"+{4 * i:#x}: call to unknown {sym}, retail {want:#010x}")
                continue
            if want is None or base is None:
                continue
            if base + addend != want:
                wrong_targets.append(f"+{4 * i:#x}: {reloc_syms.get(off + 4 * i)}{addend:+#x} "
                                     f"= {base + addend:#010x}, retail {want:#010x}")
        same = want_size == size and masked_m == masked_t and not wrong_targets
        bad += not same
        note = "" if want_size == size else f" (size {size:#x}, retail {want_size:#x})" if want_size else " (retail size unknown)"
        if wrong_targets:
            note += f" (wrong relocation target: {'; '.join(wrong_targets[:3])})"
        print(f"{'OK  ' if same else 'DIFF'} {name}{note}")
        if args.rodata:
            # retail rodata this function references (by address, from the asm operands)
            refs = sorted({t for i in range(size // 4)
                           if (t := retail_target(operands.get(addr + 4 * i, ""), names)) is not None
                           and entry["vram"] <= t < entry["vram"] + 0x9524 and t < addr})
            for t in refs:
                o = t - entry["vram"] + header
                chunk = retail[o:o + 32]
                print(f"   retail rodata {t:#010x}: {chunk.hex(' ')}  |{''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)}|")
        if not same or args.diff:
            left, right = disasm(theirs, addr), disasm(mine, addr)
            for i in range(max(len(left), len(right))):
                l = left[i] if i < len(left) else ""
                r = right[i] if i < len(right) else ""
                reloc = relocs.get(off + 4 * i)
                eq = i < len(masked_m) and i < len(masked_t) and masked_m[i] == masked_t[i]
                print(f"   {'  ' if eq else '<>'} {l:44s} | {r}{'   [' + reloc + ']' if reloc else ''}")
    for sym, addrs in sorted(resolved.items()):
        print(f"RESOLVE {sym} = " + ", ".join(f"0x{a:08X}" for a in sorted(addrs)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
