#!/usr/bin/env python3
"""Let one version fall back to its own assembly for shared C functions that
do not match it (yet).

For every function in the given C files that tools/trymatch.py does not
report OK for <version>, the definition is wrapped as

    #if VERSION_<V>
    INCLUDE_ASM("<module>/nonmatchings/<file>", name);
    #else
    <the C that matches the other version>
    #endif

so the build keeps matching both versions; writing the <version> variant
later replaces the INCLUDE_ASM branch. Functions already wrapped are left
alone. Functions with no address in <version> are only reported.

    python tools/version_fallback.py jp main src/main/*.c
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import splice  # noqa: E402


def not_ok(version: str, module: str, src: Path) -> tuple[list[str], list[str]]:
    p = subprocess.run([sys.executable, "tools/trymatch.py", version, module, str(src)],
                       cwd=ROOT, capture_output=True, text=True)
    diff, missing = [], []
    for line in p.stdout.splitlines():
        m = re.match(r"^(DIFF|\?\?)\s+(\w+)", line)
        if m:
            (diff if m.group(1) == "DIFF" else missing).append(m.group(2))
    if p.returncode != 0 and not diff and not missing:
        sys.exit(f"trymatch failed on {src}:\n{(p.stdout + p.stderr)[-2000:]}")
    return diff, missing


def split_comment(item: str) -> tuple[str, str]:
    """Leading comment block (with its newline) and the rest of an item."""
    m = re.match(r"(\s*(?:/\*.*?\*/\s*|//[^\n]*\n\s*)*)", item, re.S)
    head = m.group(1) if m else ""
    return head, item[len(head):]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("version")
    ap.add_argument("module")
    ap.add_argument("files", nargs="+", type=Path)
    args = ap.parse_args()
    macro = f"VERSION_{args.version.upper()}"

    for src in args.files:
        diff, missing = not_ok(args.version, args.module, src)
        text = src.read_text()
        done = 0
        for item in splice.top_level_items(text):
            name = splice.function_name(item)
            if name not in diff:
                continue
            head, body = split_comment(item)
            wrapped = (f"{head}#if {macro}\n"
                       f'INCLUDE_ASM("{args.module}/nonmatchings/{src.stem}", {name});\n'
                       f"#else\n{body.rstrip()}\n#endif")
            text = text.replace(item, wrapped, 1)
            done += 1
        src.write_text(text)
        note = f"; no {args.version} address: {', '.join(missing)}" if missing else ""
        print(f"{src}: {done} function(s) fall back to {args.version} assembly{note}")


if __name__ == "__main__":
    main()
