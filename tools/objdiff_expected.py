#!/usr/bin/env python3
"""Build objdiff's "expected" objects: one per C file, assembled from retail.

For every C object a module's link map loads (build/<ver>/src/<module>/...
.c.o), this takes the retail disassembly of exactly the address ranges that
object's .text/.rodata/.data/.sdata/.sbss/.bss occupy in the linked module
(tools/fullasm.py: splat with disassemble_all, matched functions included),
puts it in sections of the same names and assembles it with the build's GNU
as. objdiff pairs functions by name and diffs instructions, relocations and
data.

Names are the C side's choice and retail fixes only addresses, so the
expected object borrows the C object's naming wherever the address agrees:
- a relocated operand (%hi/%lo/%gp_rel, jal/j, .word) whose retail address
  equals the address the C object's relocation at the same offset resolves
  to (its own sections as linked, other symbols from the module ELF) is
  written as the C object writes it: the same symbol or section, plus
  addend. Otherwise splat's symbol stays and objdiff shows the difference.
  Where the C object has no relocation, the operand is written as its
  number (splat names any address it sees; the C used a constant);
- data labels become assembler-local and the C object's data symbols are
  defined at their offsets instead (GCC's literals and jump tables are
  anonymous), so objdiff compares data bytes and relocations between the
  same symbols on both sides;
- functions keep splat's (symbol file) names, global where the C's are;
- splat gives the padding up to the next object to the last item and can
  drop a data file's last bytes: sections are cut or completed (from the
  retail image) to the C object's size.
Hand-written functions (config/handwritten.yaml) and data inside .text are
marked .hidden, which takes them out of objdiff's report; the C side has them
as INCLUDE_ASM, whose <name>.NON_MATCHING marker objdiff reads as "not
decompiled" (so any other INCLUDE_ASM function reports as unmatched).

    python tools/objdiff_expected.py us [module ...] [--out build/us/expected]

Writes <out>/asm/<module>/ (the disassembly) and <out>/src/<module>/<file>.c.s
and .c.o, mirroring build/<ver>/src/, and prints any object whose expected
sections still differ in size from the C object's. Rerun after a build that
renames symbols: the borrowed names are the C objects' at the time of the run.
"""

from __future__ import annotations

import argparse
import re
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import yaml

ROOT = next(p for p in Path(__file__).resolve().parents if (p / "tools/fullasm.py").is_file())
AS = [str(ROOT / "tools/toolchain/bin/mips-linux-gnu-as"), "-EL", "-march=r3000", "-mtune=r3000",
      "-no-pad-sections", "-Iinclude", "-G0"]

SECTION_FLAGS = {".text": '"ax"', ".rodata": '"a"', ".data": '"wa"', ".sdata": '"wa"',
                 ".sbss": '"wa", @nobits', ".bss": '"wa", @nobits'}
MAP_LINE = re.compile(r" (\.\w+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+) (build/\S+\.c\.o)$")
SECTION = re.compile(r"\s*\.section\s+(\.\w+)")
ADDRESS = re.compile(r"/\* (?:([0-9A-F]+) )?([0-9A-F]{8})(?: [0-9A-F]{8})? \*/ +\S")  # rom, vram
LABEL = re.compile(r"(\s*)(glabel|dlabel|alabel|jlabel|ehlabel|enddlabel) ([^\s,]+)")
SYMBOL_LINE = re.compile(r"\s*([A-Za-z_][\w.]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")
# A relocated operand: %op(sym +- off), a jump to a symbol, a .word of a symbol or an address.
OPERAND = re.compile(
    r"(?P<op>%(?:hi|lo|gp_rel))\((?P<sym>[A-Za-z_][\w.]*)(?: (?P<sign>[+-]) (?P<off>0x[0-9A-F]+))?\)"
    r"|\((?P<lui>0x[0-9A-F]+) >> 16\)"
    r"|(?<=\sj)(?P<al>al)?\s+(?P<jsym>[A-Za-z_]\w*)$"
    r"|\.word (?:(?P<wsym>[A-Za-z_][\w.]*)(?: (?P<wsign>[+-]) (?P<woff>0x[0-9A-F]+))?|(?P<wnum>0x[0-9A-F]{8}))$")
R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16 = 2, 4, 5, 6, 7
STT_OBJECT, STT_SECTION = 1, 3


def modules(ver: str) -> list[str]:
    """The version's modules (as tools/configure.py lists them) that have been linked."""
    files = yaml.safe_load((ROOT / "config" / ver / "disc.yaml").read_text())["files"]
    names = [f["name"] for f in files if (ROOT / "config" / ver / f"{f['name']}.yaml").is_file()]
    for name in names:
        if not (ROOT / "build" / ver / f"{name}.map").is_file():
            print(f"{ver} {name}: not linked yet (no build/{ver}/{name}.map), skipped", file=sys.stderr)
    return [n for n in names if (ROOT / "build" / ver / f"{n}.map").is_file()]


def c_objects(ver: str, module: str) -> dict[str, list[tuple[str, int, int]]]:
    """C object path -> its (section, vram, size) placements, from the link map."""
    objs: dict[str, list[tuple[str, int, int]]] = {}
    for line in (ROOT / "build" / ver / f"{module}.map").read_text().splitlines():
        if line.startswith("LOAD build/") and line.endswith(".c.o"):
            objs.setdefault(line[5:], [])
        elif (m := MAP_LINE.match(line)) and m[1] in SECTION_FLAGS and int(m[3], 16):
            objs.setdefault(m[4], []).append((m[1], int(m[2], 16), int(m[3], 16)))
    return objs


class Elf:
    """What this needs from a little-endian ELF32 file: sections, symbols, REL relocations."""

    def __init__(self, path: Path):
        data = path.read_bytes()
        shoff, = struct.unpack_from("<I", data, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)
        heads = [struct.unpack_from("<10I", data, shoff + i * shentsize) for i in range(shnum)]
        cstr = lambda off: data[off:data.index(b"\0", off)].decode()
        names = [cstr(heads[shstrndx][4] + h[0]) for h in heads]
        self.sizes = {n: h[5] for n, h in zip(names, heads) if n in SECTION_FLAGS and h[5]}
        self.bytes = {n: data[h[4]:h[4] + h[5]] for n, h in zip(names, heads)}
        self.symbols: list[tuple[str, int, int, int, str]] = []  # name, value, binding, type, section
        self.relocs: dict[str, list[tuple[int, int, int]]] = {}  # section -> (offset, type, symbol index)
        special = {0: "UND", 0xFFF1: "ABS", 0xFFF2: "COM"}
        for h in heads:
            if h[1] == 2:  # SHT_SYMTAB
                for i in range(h[5] // 16):
                    name, value, _, info, _, shndx = struct.unpack_from("<IIIBBH", data, h[4] + i * 16)
                    section = special.get(shndx) or (names[shndx] if shndx < len(names) else "")
                    self.symbols.append((cstr(heads[h[6]][4] + name), value, info >> 4, info & 15, section))
            elif h[1] == 9:  # SHT_REL
                entries = struct.iter_unpack("<II", data[h[4]:h[4] + h[5]])
                self.relocs[names[h[7]]] = [(offset, info & 0xFF, info >> 8) for offset, info in entries]


def s16(x: int) -> int:
    return (x & 0xFFFF) - ((x & 0x8000) << 1)


def hi16(address: int) -> int:  # %hi(): the upper half, rounded for a signed %lo()
    return (address + 0x8000) >> 16 & 0xFFFF


class CObject:
    """What an expected object borrows from its C object."""

    def __init__(self, path: Path, placed: list[tuple[str, int, int]], linked: dict[str, int]):
        elf = Elf(path)
        self.sizes = elf.sizes
        self.globals = {n for n, _, bind, _, sec in elf.symbols if bind in (1, 2) and sec in SECTION_FLAGS}
        # data symbols: section -> (name, offset, is object, is global); INCLUDE_RODATA's are left out,
        # as objdiff leaves them out on the C side
        marked = {n.removesuffix(".NON_MATCHING") for n, *_ in elf.symbols if n.endswith(".NON_MATCHING")}
        self.data: dict[str, list[tuple[str, int, bool, bool]]] = {}
        for name, value, bind, kind, sec in elf.symbols:
            if sec in SECTION_FLAGS and sec != ".text" and kind != STT_SECTION \
                    and name.removesuffix(".NON_MATCHING") not in marked:
                self.data.setdefault(sec, []).append((name, value, kind == STT_OBJECT, bind in (1, 2)))
        # relocations: (section, offset) -> (operand as the C object writes it, address it resolves to)
        vram = {s: v for s, v, _ in placed}
        gp0 = struct.unpack_from("<I", elf.bytes.get(".reginfo", bytes(24)), 20)[0]
        self.relocs: dict[tuple[str, int], tuple[str, int | None]] = {}
        for sec, rels in elf.relocs.items():
            if sec not in vram:
                continue
            last_hi: dict[int, int] = {}
            waiting: dict[int, list[tuple[int, int]]] = {}  # HI16s waiting for their LO16
            for offset, kind, index in rels:
                word, = struct.unpack_from("<I", elf.bytes[sec], offset)
                name, value, _, stype, ssec = elf.symbols[index]
                if kind == R_MIPS_HI16:
                    waiting.setdefault(index, []).append((offset, word & 0xFFFF))
                    last_hi[index] = word & 0xFFFF
                    continue
                if kind == R_MIPS_LO16:
                    found = [(o, (hi << 16) + s16(word)) for o, hi in waiting.pop(index, [])]
                    found.append((offset, (last_hi.get(index, 0) << 16) + s16(word)))
                elif kind == R_MIPS_GPREL16:
                    found = [(offset, s16(word) + (gp0 if stype == STT_SECTION else 0))]
                elif kind == R_MIPS_32:
                    found = [(offset, word - ((word & 0x80000000) << 1))]
                elif kind == R_MIPS_26:
                    found = [(offset, (word & 0x3FFFFFF) << 2)]
                else:
                    continue
                if stype == STT_SECTION:  # one of this object's sections, as linked
                    name, base = f".L_start{ssec}", vram.get(ssec)
                else:
                    base = vram[ssec] + value if ssec in vram else linked.get(name)
                for at, addend in found:
                    expr = name + (f" + {addend:#x}" if addend > 0 else f" - {-addend:#x}" if addend < 0 else "")
                    self.relocs[(sec, at)] = (expr, None if base is None else (base + addend) & 0xFFFFFFFF)


def asm_parts(asm: Path) -> list[tuple[str, int, list[str]]]:
    """(section, vram, lines) for each function (nonmatchings/) and data file (data/).

    Function files can carry migrated rodata, and nonmatchings/ has
    rodata-only files; both duplicate data/, so only .text is taken there.
    """
    parts = []
    for path in sorted(asm.rglob("*.s")):
        top = path.relative_to(asm).parts[0]
        if top not in ("nonmatchings", "data"):
            continue  # whole asm segments: not part of any C file
        chunks: list[tuple[str, list[str]]] = [(".text", [])]
        for line in path.read_text().splitlines():
            if m := SECTION.match(line):
                chunks.append((m[1], []))
            elif not line.lstrip().startswith((".include", "nonmatching")):
                chunks[-1][1].append(line)
        for section, lines in chunks:
            if (section == ".text") != (top == "nonmatchings"):
                continue
            vram = next((int(m[2], 16) for l in lines if (m := ADDRESS.search(l))), None)
            if vram is not None:
                parts.append((section, vram, lines))
    return sorted(parts, key=lambda p: p[1])


def label_addresses(parts) -> dict[str, int]:
    """Every label splat defined in the module, at the address of the item after it."""
    out: dict[str, int] = {}
    for _, _, lines in parts:
        pending = []
        for line in lines:
            if (m := LABEL.match(line)) and m[2] != "enddlabel":
                pending.append(m[3])
            elif pending and (m := ADDRESS.search(line)):
                out.update((n, int(m[2], 16)) for n in pending)
                pending = []
    return out


class Module:
    def __init__(self, ver: str, name: str, out: Path):
        self.ver, self.name, self.out = ver, name, out
        asm = out / "asm" / name
        subprocess.run([sys.executable, str(ROOT / "tools/fullasm.py"), ver, name, "--out", str(asm)],
                       cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
        options = yaml.safe_load((ROOT / f"build/{ver}/splat/{name}.paths.yaml").read_text())["options"]
        self.image = (ROOT / options["target_path"]).read_bytes()
        self.parts = asm_parts(asm / "asm")
        # retail addresses of splat's names, linked addresses of the C objects' names
        self.retail = label_addresses(self.parts)
        for path in [ROOT / p for p in options["symbol_addrs_path"]] + [asm / "undefined_syms.txt", asm / "undefined_funcs.txt"]:
            for m in map(SYMBOL_LINE.match, path.read_text().splitlines() if path.is_file() else []):
                if m:
                    self.retail.setdefault(m[1], int(m[2], 16))
        self.linked = {n: v for n, v, bind, _, sec in Elf(ROOT / f"build/{ver}/{name}.elf").symbols
                       if bind in (1, 2) and sec != "UND"}
        hand = yaml.safe_load((ROOT / "config/handwritten.yaml").read_text()) or {}
        self.hidden = {n for group in hand.get(name, []) for n in group["names"]}

    def section_lines(self, section: str, vram: int, size: int, fix_end: bool) -> list[str]:
        """The retail assembly of [vram, vram + size), ending where the C object's section does."""
        end = vram + size
        lines = [l for p in self.parts if p[0] == section and vram <= p[1] < end for l in p[2]
                 if not ((m := ADDRESS.search(l)) and int(m[2], 16) >= end)]
        items = [i for i, l in enumerate(lines) if ADDRESS.search(l)]
        if not items:
            return lines
        last = items[-1]
        lines[last + 1:] = [l for l in lines[last + 1:] if not l.lstrip().startswith(".align")]
        m = ADDRESS.search(lines[last])
        comment = lines[last][:m.end() - 1]
        if re.search(r"\*/ +\.space ", lines[last]):
            lines[last] = f"{comment}.space {end - int(m[2], 16):#x}"
        elif fix_end and m[1] and not re.search(r"\.word\s+[A-Za-z_.]", lines[last]):
            # the last item crosses the end, or splat dropped bytes after it: retail bytes instead
            rom = int(m[1], 16)
            lines[last] = f"{comment}.byte " + ", ".join(f"{b:#04x}" for b in self.image[rom:rom + end - int(m[2], 16)])
        return lines

    def operand(self, m: re.Match, c: CObject, at: tuple[str, int], local) -> str:
        """A relocated operand at (section, offset), written as the C object writes that address."""
        splat = local.sub(r".L\1", m[0]) if local else m[0]  # splat's operand, its data labels local
        sym, sign, off = (m["sym"], m["sign"], m["off"]) if m["sym"] else (m["wsym"], m["wsign"], m["woff"])
        number = m["wnum"] or m["lui"]
        address = int(number, 16) if number else self.retail.get(sym or m["jsym"])
        if address is None:
            return splat
        address = (address + (int(sign + off, 16) if off else 0)) & 0xFFFFFFFF
        expr, linked = c.relocs.get(at, (None, None))
        if expr is None:  # the C object has a constant here
            if m["op"] in ("%hi", "%lo"):
                return f"{hi16(address):#x}" if m["op"] == "%hi" else str(s16(address))
            return f".word {address:#x}" if m["wsym"] else splat
        if linked is None:
            return splat
        if m["lui"] or m["op"] == "%hi":  # only the upper half is known
            same = (address >> 16 if m["lui"] else hi16(address)) == hi16(linked)
        else:
            same = address == linked
        if not same:
            return splat
        if m["op"] or m["lui"]:
            return f"{m['op'] or '%hi'}({expr})"
        return f"{m['al'] or ''} {expr}" if m["jsym"] else f".word {expr}"

    def source(self, placed, c: CObject, fix_end: set[str]) -> str:
        sections = [(s, vram, self.section_lines(s, vram, size, s in fix_end)) for s, vram, size in placed]
        data_labels = {m[3] for s, _, lines in sections if s != ".text" for l in lines if (m := LABEL.match(l))}
        names = sorted(data_labels, key=len, reverse=True)
        local = re.compile(rf"(?<![\w.])({'|'.join(map(re.escape, names))})(?![\w.])") if names else None
        out = ['.include "macro.inc"', ".set noat", ".set noreorder"]
        for section, vram, lines in sections:
            out += [f"\n.section {section}, {SECTION_FLAGS[section]}", f".L_start{section}:"]
            for line in lines:
                if (m := LABEL.match(line)) and section != ".text":
                    line = "" if m[2] == "enddlabel" else f".L{m[3]}:"  # the C object's symbols come below
                elif m and m[2] != "enddlabel":
                    name = m[3]
                    line = f"{m[1]}{m[2]} {name}, {'global' if name in c.globals else 'local'}"
                    if name in c.globals and (name in self.hidden or m[2] == "dlabel"):
                        line += f"\n.hidden {name}"
                elif not m and (a := ADDRESS.search(line)):
                    at = (section, int(a[2], 16) - vram)
                    line = OPERAND.sub(lambda o: self.operand(o, c, at, local), line)
                out.append(line)
            for name, offset, is_object, is_global in c.data.get(section, []):
                out += [f".globl {name}"] * is_global + [f".type {name}, @object"] * is_object
                out.append(f"{name} = .L_start{section} + {offset:#x}")
        return "\n".join(out) + "\n"

    def build(self, obj: str, placed) -> str | None:
        """Assemble obj's expected object; returns a problem, if any."""
        target = self.out / Path(obj).relative_to(f"build/{self.ver}")
        target.parent.mkdir(parents=True, exist_ok=True)
        c = CObject(ROOT / obj, placed, self.linked)
        fix_end: set[str] = set()
        while True:
            target.with_suffix(".s").write_text(self.source(placed, c, fix_end))
            proc = subprocess.run([*AS, "-o", str(target), str(target.with_suffix(".s"))],
                                  cwd=ROOT, capture_output=True, text=True)
            if proc.returncode:
                return f"{obj}: as failed\n{proc.stderr[-2000:]}"
            got = Elf(target).sizes
            wrong = {s for s in (".rodata", ".data", ".sdata") if got.get(s) != c.sizes.get(s)}
            if fix_end or not wrong:
                break
            fix_end = wrong
        if got != c.sizes:
            diff = ", ".join(f"{s} {c.sizes.get(s, 0):#x}/{got.get(s, 0):#x}"
                             for s in sorted(set(c.sizes) | set(got)) if c.sizes.get(s) != got.get(s))
            return f"{obj}: section sizes differ (C/expected): {diff}"
        return None


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("version")
    ap.add_argument("modules", nargs="*", help="default: every module of the version")
    ap.add_argument("--out", type=Path, help="default: build/<ver>/expected")
    ap.add_argument("-j", "--jobs", type=int, default=8)
    args = ap.parse_args()
    out = (args.out or ROOT / "build" / args.version / "expected").resolve()
    bad = 0
    for name in args.modules or modules(args.version):
        module = Module(args.version, name, out)
        objs = c_objects(args.version, name)
        with ThreadPoolExecutor(args.jobs) as pool:
            problems = [p for p in pool.map(lambda kv: module.build(*kv), objs.items()) if p]
        for problem in problems:
            print(problem)
        print(f"{args.version} {name}: {len(objs) - len(problems)}/{len(objs)} expected objects")
        bad += len(problems)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
