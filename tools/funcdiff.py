#!/usr/bin/env python3
"""Compare every function compiled from C in a module against retail.

For each function without a .NON_MATCHING marker (i.e. it came from C), the
bytes in the built image are compared with the retail file at the same
address. Mismatches are shown as a side-by-side disassembly.

    python tools/funcdiff.py us main               # all C functions in main
    python tools/funcdiff.py us main func_80021AD8 # just one, always show asm
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
TC = ROOT / "tools/toolchain/bin/mips-linux-gnu-"


def symbols(elf: Path) -> tuple[dict[str, tuple[int, int]], set[str]]:
    out = subprocess.run([f"{TC}readelf", "-sW", str(elf)], capture_output=True, text=True, check=True).stdout
    funcs, asm = {}, set()
    for line in out.splitlines():
        p = line.split()
        if len(p) < 8 or not p[0].rstrip(":").isdigit() or p[6] in ("ABS", "UND"):
            continue
        if p[7].endswith(".NON_MATCHING"):
            asm.add(p[7].removesuffix(".NON_MATCHING"))
        elif p[3] == "FUNC":
            funcs[p[7]] = (int(p[1], 16), int(p[2], 0))
    return funcs, asm


def disasm(data: bytes, vram: int) -> list[str]:
    tmp = ROOT / "build" / ".funcdiff.bin"
    tmp.write_bytes(data)
    out = subprocess.run([f"{TC}objdump", "-D", "-b", "binary", "-m", "mips:3000", "-EL", "-z",
                          f"--adjust-vma={vram:#x}", str(tmp)], capture_output=True, text=True).stdout
    return [l.split(":", 1)[1].strip() for l in out.splitlines() if l.strip().startswith(f"{vram:x}"[:3])]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version")
    parser.add_argument("module")
    parser.add_argument("functions", nargs="*")
    args = parser.parse_args()

    manifest = yaml.safe_load((ROOT / "config" / args.version / "disc.yaml").read_text())
    entry = next(e for e in manifest["files"] if e["name"] == args.module)
    fname = args.module + (".exe" if "iso" in entry else ".bin")
    header = 0x800 if "iso" in entry else 0
    retail = (ROOT / "disc" / args.version / fname).read_bytes()
    built = (ROOT / "build" / args.version / fname).read_bytes()
    funcs, asm = symbols(ROOT / "build" / args.version / f"{args.module}.elf")

    names = args.functions or sorted((n for n in funcs if n not in asm), key=lambda n: funcs[n][0])
    bad = 0
    for name in names:
        addr, size = funcs[name]
        off = addr - entry["vram"] + header
        want, got = retail[off:off + size], built[off:off + size]
        same = want == got
        bad += not same
        print(f"{'OK  ' if same else 'DIFF'} {name} ({size:#x} bytes at {addr:#010x})")
        if not same or args.functions:
            left, right = disasm(want, addr), disasm(got, addr)
            for i in range(max(len(left), len(right))):
                l = left[i] if i < len(left) else ""
                r = right[i] if i < len(right) else ""
                mark = "  " if l == r else "<>"
                print(f"   {mark} {l:44s} | {r}")
    if not args.functions:
        print(f"{len(names) - bad}/{len(names)} C functions match retail")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
