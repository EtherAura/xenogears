#!/usr/bin/env python3
"""Disassemble every function of a module, matched ones included.

The build's asm/ tree only has files for functions still pulled in with
INCLUDE_ASM. This runs splat with disassemble_all into work/fullasm/<ver>/
<module>/ (gitignored) without touching asm/, src/ or build/, so the whole
module can be searched and read, e.g. for $gp-vs-%hi/%lo evidence.

    python tools/fullasm.py us main
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("version")
    ap.add_argument("module")
    ap.add_argument("--symbols", type=Path, nargs="+", help="use these symbol files instead of the module's")
    ap.add_argument("--out", help="output folder name under work/fullasm/<ver>/ (default: the module)")
    args = ap.parse_args()

    paths = ROOT / "build" / args.version / "splat" / f"{args.module}.paths.yaml"
    if not paths.is_file():
        sys.exit(f"missing {paths.relative_to(ROOT)}; run tools/configure.py first")
    out = ROOT / "work" / "fullasm" / args.version / (args.out or args.module)
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True)
    rel = out.relative_to(ROOT)

    options = yaml.safe_load(paths.read_text())["options"]
    options.update({
        "asm_path": f"{rel}/asm",
        "src_path": f"{rel}/src",
        "build_path": f"{rel}/build",
        "ld_script_path": f"{rel}/{args.module}.ld",
        "undefined_funcs_auto_path": f"{rel}/undefined_funcs.txt",
        "undefined_syms_auto_path": f"{rel}/undefined_syms.txt",
        "cache_path": f"{rel}/splat.cache",
        "elf_path": f"{rel}/{args.module}.elf",
        "disassemble_all": True,
    })
    if args.symbols:
        options["symbol_addrs_path"] = [str(p.resolve().relative_to(ROOT)) for p in args.symbols]
    override = out / "paths.yaml"
    override.write_text(yaml.safe_dump({"options": options}, sort_keys=False))

    configs = [
        ROOT / "config" / "splat.common.yaml",
        ROOT / "config" / args.version / "splat.yaml",
        override,
        ROOT / "config" / args.version / f"{args.module}.yaml",
    ]
    cmd = [sys.executable, "-m", "splat", "split", *[str(c.relative_to(ROOT)) for c in configs]]
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        sys.exit((proc.stdout + proc.stderr)[-4000:])
    print(f"wrote {rel}/asm")


if __name__ == "__main__":
    main()
