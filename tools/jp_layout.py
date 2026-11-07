#!/usr/bin/env python3
"""Derive a module's JP split from its US split through shared names.

Every US subsegment start is looked up as a label (function, jump table or
data symbol) in the US disassembly, and the label of the same name in the
JP disassembly gives the JP offset. Run it after tools/share_symbols.py has
given JP the US names and tools/fullasm.py has regenerated the JP listing:

    python tools/share_symbols.py us jp <module>
    python tools/fullasm.py jp <module>
    python tools/jp_layout.py <module>            # report
    python tools/jp_layout.py <module> --write    # rewrite config/jp/<module>.yaml

Starts that have no label in US, or whose label is missing in JP, are
reported (give them with --set KIND:NAME=OFF); offset 0 maps to 0. The JP
subsegments are written in JP offset order, since JP may link the objects
in another order. The JP yaml keeps its own header, vram and
end offset; subsegment comments are regenerated.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent


def labels(ver: str, module: str) -> tuple[dict[int, list[str]], dict[str, int]]:
    at: dict[int, list[str]] = {}
    by: dict[str, int] = {}
    for f in (ROOT / "work/fullasm" / ver / module / "asm").rglob("*.s"):
        lines = f.read_text().splitlines()
        for i, line in enumerate(lines):
            m = re.match(r"^\s*(glabel|dlabel|jlabel|nonmatching)\s+(\w+)", line)
            if not m or m.group(1) == "nonmatching":
                continue
            for nxt in lines[i + 1:i + 4]:
                a = re.search(r"/\* (?:[0-9A-F]+ )?([0-9A-F]{8}) ", nxt)
                if a:
                    addr = int(a.group(1), 16)
                    at.setdefault(addr, []).append(m.group(2))
                    by.setdefault(m.group(2), addr)
                    break
    return at, by


def load(ver: str, module: str) -> tuple[dict, int, list]:
    y = yaml.safe_load((ROOT / "config" / ver / f"{module}.yaml").read_text())
    seg = [s for s in y["segments"] if isinstance(s, dict)][0]
    return y, int(seg["vram"]), seg["subsegments"]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("module")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--set", action="append", default=[], metavar="KIND:NAME=OFF",
                    help="JP offset for a start that has no shared label, e.g. c:b_800B3E04=0x4414C")
    a = ap.parse_args()
    manual = {}
    for m in a.set:
        key, off = m.split("=")
        kind, name = key.split(":")
        manual[(kind, name or None)] = int(off, 16)
    us_at, _ = labels("us", a.module)
    _, jp_by = labels("jp", a.module)
    _, us_vram, us_subs = load("us", a.module)
    _, jp_vram, _ = load("jp", a.module)
    rows, bad = [], 0
    for s in us_subs:
        off, kind = s[0], s[1]
        name = s[2] if len(s) > 2 else None
        if (kind, name) in manual:
            rows.append((manual[(kind, name)], kind, name, "manual")); continue
        if off == 0:
            rows.append((0, kind, name, "")); continue
        va = us_vram + off
        cands = us_at.get(va, [])
        hit = next((c for c in cands if c in jp_by), None)
        if hit is None:
            rows.append((None, kind, name, f"US 0x{va:08X} {cands or 'no label'}"))
            bad += 1
            continue
        rows.append((jp_by[hit] - jp_vram, kind, name, hit))
    for off, kind, name, why in rows:
        o = "  ???   " if off is None else f"0x{off:06X}"
        print(f"{o} {kind:8} {name or '':32} {why}")
    if bad:
        print(f"{bad} start(s) unmapped", file=sys.stderr)
        return 1
    if a.write:
        p = ROOT / "config/jp" / f"{a.module}.yaml"
        text = p.read_text()
        i = text.index("    subsegments:\n") + len("    subsegments:\n")
        j = re.search(r"\n  - \[0x[0-9A-F]+\]", text).start() + 1
        # JP may link the objects in another order: subsegments go by offset
        out = []
        for off, kind, name, _ in sorted(rows, key=lambda r: r[0]):
            e = f"      - [0x{off:X}, {kind}" + (f", {name}]" if name else "]")
            out.append(f"{e:<48}# 0x{jp_vram + off:08X}")
        p.write_text(text[:i] + "\n".join(out) + "\n" + text[j:])
        print(f"wrote {p.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
