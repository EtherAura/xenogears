#!/usr/bin/env python3
"""Compile a C file like tools/trymatch.py does and compare its .data/.sdata/.rodata
bytes (relocations resolved) with retail.

    python tools/datacheck.py us main kernel_data.c --as src/main/kernel.c \
        --sec .data=0x8004EAA0 --sec .rodata=0x80010000 [--sym NAME=0xADDR ...]

Relocated words: a section-relative relocation (e.g. .rodata+0x104) is
resolved against the --sec base given for that section; a named symbol is
resolved with config/<ver>/symbols (plus --sym overrides, plus the
candidate's own symbols placed at the --sec bases), else D_XXXXXXXX
spelling.
"""
import argparse
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import trymatch  # noqa: E402
import yaml  # noqa: E402

CROSS = trymatch.CROSS


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True).stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("version")
    ap.add_argument("module")
    ap.add_argument("source", type=Path)
    ap.add_argument("--as", dest="as_file", type=Path)
    ap.add_argument("--cc")
    ap.add_argument("--cflags")
    ap.add_argument("--maspsx")
    ap.add_argument("--sec", action="append", default=[], help="section=0xADDR")
    ap.add_argument("--sym", action="append", default=[], help="name=0xADDR")
    ap.add_argument("-v", action="store_true", help="print every word")
    ap.add_argument("-I", dest="includes", action="append", type=Path, default=[],
                    help="extra include directory, searched before include/")
    args = ap.parse_args()

    build = yaml.safe_load((ROOT / "config/build.yaml").read_text())["default"]
    src = (args.as_file or args.source).resolve()
    if src.is_relative_to(ROOT / "src"):
        from configure import load_presets, preset_for
        default, presets = load_presets()
        build = preset_for(str(src.relative_to(ROOT)), default, presets, args.version)
    manifest = yaml.safe_load((ROOT / "config" / args.version / "disc.yaml").read_text())
    entry = next(e for e in manifest["files"] if e["name"] == args.module)
    header = 0x800 if "iso" in entry else 0
    retail = (ROOT / "disc" / args.version / (args.module + (".exe" if "iso" in entry else ".bin"))).read_bytes()
    names = trymatch.symbol_table(args.version, args.module)
    for s in args.sym:
        n, a = s.split("=")
        names[n] = int(a, 0)
    bases = {}
    for s in args.sec:
        n, a = s.split("=")
        bases[n] = int(a, 0)

    with tempfile.TemporaryDirectory() as work:
        obj = trymatch.compile_c(args.source.resolve(), args.version, args.cc or build["cc"],
                                 args.cflags or build["cflags"], args.maspsx or build["maspsx"], Path(work),
                                 [d.resolve() for d in args.includes], build.get("asG"))
        # own symbols -> address
        symtab = run([f"{CROSS}readelf", "-sW", str(obj)])
        secnames = {}
        for line in run([f"{CROSS}readelf", "-SW", str(obj)]).splitlines():
            m = re.match(r"\s*\[\s*(\d+)\]\s+(\S+)", line)
            if m:
                secnames[m.group(1)] = m.group(2)
        own = {}
        for line in symtab.splitlines():
            p = line.split()
            if len(p) >= 8 and p[6] not in ("UND", "ABS") and p[3] in ("OBJECT", "NOTYPE", "FUNC"):
                sec = secnames.get(p[6])
                if sec in bases:
                    own[p[7]] = bases[sec] + int(p[1], 16)
        for n, a in own.items():
            names.setdefault(n, a)
        relout = run([f"{CROSS}readelf", "-rW", str(obj)])
        ok = True
        for sec, base in bases.items():
            binf = Path(work) / "s.bin"
            subprocess.run([f"{CROSS}objcopy", "-O", "binary", f"--only-section={sec}", str(obj), str(binf)])
            mine = binf.read_bytes() if binf.exists() else b""
            binf.unlink(missing_ok=True)
            # relocations of this section
            rels = {}
            block = None
            for line in relout.splitlines():
                if line.startswith("Relocation section"):
                    block = line.split("'")[1]
                    continue
                if block == f".rel{sec}":
                    p = line.split()
                    if len(p) >= 5 and re.fullmatch(r"[0-9a-f]+", p[0]):
                        rels[int(p[0], 16)] = (p[2], p[4])
            data = bytearray(mine)
            notes = {}
            for off, (kind, sym) in rels.items():
                if kind != "R_MIPS_32":
                    notes[off] = f"{kind} {sym}"
                    continue
                addend = struct.unpack_from("<I", data, off)[0]
                if sym in bases:
                    tgt = bases[sym] + addend
                else:
                    a = trymatch.symbol_address(sym, names)
                    if a is None:
                        notes[off] = f"unresolved {sym}"
                        continue
                    tgt = a + addend
                struct.pack_into("<I", data, off, tgt & 0xFFFFFFFF)
                notes[off] = f"-> {sym}+{addend:#x} = {tgt:#010x}"
            roff = base - entry["vram"] + header
            theirs = retail[roff: roff + len(data)]
            bad = [i for i in range(0, len(data), 4) if data[i:i + 4] != theirs[i:i + 4]]
            print(f"== {sec}: {len(data):#x} bytes at {base:#010x}..{base + len(data):#010x}: "
                  f"{'OK' if not bad else f'{len(bad)} words differ'}")
            for i in (range(0, len(data), 4) if args.v else bad[:40]):
                m_ = data[i:i + 4].hex()
                t_ = theirs[i:i + 4].hex()
                print(f"  {base + i:08X} mine {m_} retail {t_} {notes.get(i, '')}")
            ok &= not bad
        print("ALL OK" if ok else "MISMATCH")


if __name__ == "__main__":
    main()
