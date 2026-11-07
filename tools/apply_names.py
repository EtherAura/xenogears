#!/usr/bin/env python3
"""Rename func_XXXXXXXX / D_XXXXXXXX references in C sources and headers to
the names the symbol file gives those addresses.

Agents and m2c work with splat's address names; once an address is named in
config/<ver>/symbols/<module>.txt, every C reference should use the name.

    python tools/apply_names.py us main [files...]   (default: src/**/*.c, include/**/*.h)
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SYM = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("version")
    ap.add_argument("module")
    ap.add_argument("files", nargs="*", type=Path)
    args = ap.parse_args()

    names = {}
    for line in (ROOT / "config" / args.version / "symbols" / f"{args.module}.txt").read_text().splitlines():
        m = SYM.match(line)
        if m and not re.fullmatch(r"(func|D)_[0-9A-F]{8}", m.group(1)):
            addr = int(m.group(2), 16)
            names[f"func_{addr:08X}"] = m.group(1)
            names[f"D_{addr:08X}"] = m.group(1)

    files = args.files or [*ROOT.glob("src/**/*.c"), *ROOT.glob("include/**/*.h")]
    pattern = re.compile(r"\b(?:func|D)_[0-9A-F]{8}\b")
    for f in files:
        text = f.read_text()
        new = pattern.sub(lambda m: names.get(m.group(0), m.group(0)), text)
        if new != text:
            f.write_text(new)
            print(f"{f.relative_to(ROOT) if f.is_absolute() else f}: renamed")


if __name__ == "__main__":
    main()
