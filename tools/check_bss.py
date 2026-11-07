#!/usr/bin/env python3
"""Check that every C object's .sbss/.bss landed where config/<ver>/<mod>.yaml
places it.

The sha1 check cannot see this: the symbol files become <mod>.syms.ld
assignments, and an assignment silently overrides an object's global symbol,
so a C static placed at the wrong address can still produce the retail image.
Run it after ninja:

    python tools/check_bss.py              # every built module of every version
    python tools/check_bss.py us main
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
MAP_RE = re.compile(r"^ (\.s?bss)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+) build/\w+/src/(\S+)\.c\.o$", re.M)


def expected(ver: str, mod: str) -> dict[tuple[str, str], int]:
    """(section, C file relative to src/) -> vram, from the module's dict subsegments."""
    cfg = yaml.safe_load((ROOT / f"config/{ver}/{mod}.yaml").read_text())
    out = {}
    for seg in cfg["segments"]:
        if not isinstance(seg, dict):
            continue
        for sub in seg.get("subsegments", []):
            if isinstance(sub, dict) and sub.get("type") in (".sbss", ".bss"):
                out[(sub["type"], f"{mod}/{sub['name']}")] = sub["vram"]
    return out


def check(ver: str, mod: str) -> int:
    mapfile = ROOT / f"build/{ver}/{mod}.map"
    if not mapfile.is_file() or not (ROOT / f"config/{ver}/{mod}.yaml").is_file():
        return 0
    want = expected(ver, mod)
    bad = 0
    for m in MAP_RE.finditer(mapfile.read_text()):
        sec, addr, size, obj = m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4)
        if not size:
            continue
        w = want.get((sec, obj))
        if w != addr:
            bad += 1
            where = f"expected {w:#010x}" if w is not None else "no subsegment places it"
            print(f"{ver} {mod}: {obj}.c {sec} at {addr:#010x} (size {size:#x}): {where}")
    return bad


def main() -> int:
    args = sys.argv[1:]
    if args:
        jobs = [(args[0], m) for m in args[1:]] or [(args[0], p.stem) for p in (ROOT / f"build/{args[0]}").glob("*.map")]
    else:
        jobs = [(v.name, p.stem) for v in sorted((ROOT / "build").iterdir()) if v.is_dir()
                for p in sorted(v.glob("*.map"))]
    bad = sum(check(v, m) for v, m in jobs)
    print(f"check_bss: {len(jobs)} modules, " + ("every C .sbss/.bss where its yaml puts it" if not bad else f"{bad} problems"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
