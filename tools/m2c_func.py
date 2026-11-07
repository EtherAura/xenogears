#!/usr/bin/env python3
"""Draft C for one function with m2c, using the project headers as context.

    python tools/m2c_func.py us main func_80019C7C
    python tools/m2c_func.py us main func_80019C7C --headers main/sprite_actor.h

The function's assembly is taken from splat's output (a nonmatchings file if
the function sits in a C file, otherwise the module's asm chunk).
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def find_asm(version: str, module: str, func: str) -> Path:
    base = ROOT / "asm" / version / module
    direct = list(base.glob(f"nonmatchings/**/{func}.s"))
    if direct:
        return direct[0]
    for chunk in sorted(base.glob("*.s")):
        if f"glabel {func}\n" in chunk.read_text():
            return chunk
    sys.exit(f"{func}: not found under {base}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version")
    parser.add_argument("module")
    parser.add_argument("function")
    parser.add_argument("--headers", nargs="*", default=[], help="extra headers (relative to include/)")
    args, extra = parser.parse_known_args()

    asm = find_asm(args.version, args.module, args.function)
    with tempfile.TemporaryDirectory() as work:
        ctx_src = Path(work) / "ctx.c"
        ctx_src.write_text("".join(f'#include "{h}"\n' for h in ["common.h", "psyq.h", *args.headers]))
        ctx = Path(work) / "ctx.i"
        subprocess.run(["cpp", "-nostdinc", "-undef", "-P", "-D_LANGUAGE_C", "-DM2CTX",
                        f"-DVERSION_{args.version.upper()}", f"-I{ROOT / 'include'}", str(ctx_src), "-o", str(ctx)],
                       check=True)
        cmd = [sys.executable, "-m", "m2c.main", "-t", "mipsel-gcc-c", "--context", str(ctx),
               "--function", args.function, *extra, str(asm)]
        return subprocess.run(cmd).returncode


if __name__ == "__main__":
    sys.exit(main())
