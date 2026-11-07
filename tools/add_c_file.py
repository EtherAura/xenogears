#!/usr/bin/env python3
"""Carve C files out of main's assembly in both versions' configs.

Each FILE is given with its US bounds (and optionally JP bounds and a
.rodata range). The script inserts `[off, c, name]` subsegments (plus an
`asm` subsegment after the file if nothing starts there) into
config/us/main.yaml and config/jp/main.yaml, in address order, and the
file's `.rodata` pairing when a rodata range is given. JP bounds default to
the same library object in JP's own signature scan (for src/main/psyq/
files) or can be passed explicitly; JP rodata is found by searching JP's
executable for the US rodata bytes.

    python tools/add_c_file.py psyq/libc2/bzero 0x8003F8E8 0x8003F918
    python tools/add_c_file.py psyq/libc/sprintf 0x8003FBF8 0x80040454 \\
        --rodata 0x80018BF0 0x80018CCC
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VRAM = 0x80010000
HDR = 0x800
ENTRY = re.compile(r"^      - \[0x([0-9A-Fa-f]+), ([.\w]+)(?:, ([\w/]+))?\](.*)$")
RODATA_TYPES = {"rodata", ".rodata"}
TEXT_TYPES = {"asm", "c", "hasm"}


def off(addr: int) -> int:
    return addr - VRAM + HDR


def sdk_table(version: str) -> dict[tuple[str, str, int], tuple[int, int]]:
    """(lib, obj, occurrence) -> (start, end) from tools/psyq_sigs.py."""
    rng = {"us": ("0x8003F8E8", "0x8004EAA0"), "jp": ("0x8003F790", "0x8004E948")}[version]
    out = subprocess.run([sys.executable, "tools/psyq_sigs.py", version, "main", "--range", *rng],
                         cwd=ROOT, capture_output=True, text=True).stdout
    table, seen = {}, {}
    for line in out.splitlines():
        m = re.match(r"^0x([0-9a-f]+) 0x([0-9a-f]+)\s+(\S+)\s+(\S+)", line)
        if m:
            key = (m.group(3), m.group(4))
            k = seen.get(key, 0)
            seen[key] = k + 1
            table[(*key, k)] = (int(m.group(1), 16), int(m.group(2), 16))
    return table


def insert(path: Path, entries: list[tuple[int, str, str | None, str]]) -> None:
    """Insert (offset, type, name, comment) entries into a main.yaml, keeping
    each group (rodata, text) in address order. An entry replaces an existing
    one at the same offset in the same group."""
    lines = path.read_text().splitlines(keepends=True)
    for o, kind, name, comment in entries:
        group = RODATA_TYPES if kind in RODATA_TYPES else TEXT_TYPES
        idx = [i for i, l in enumerate(lines) if (m := ENTRY.match(l)) and m.group(2) in group]
        text = f"      - [0x{o:X}, {kind}" + (f", {name}" if name else "") + "]"
        text = f"{text:40} # 0x{o - HDR + VRAM:08X}{(' ' + comment) if comment else ''}\n"
        replaced = False
        for i in idx:
            if int(ENTRY.match(lines[i]).group(1), 16) == o:
                lines[i] = text
                replaced = True
                break
        if replaced:
            continue
        after = [i for i in idx if int(ENTRY.match(lines[i]).group(1), 16) < o]
        pos = (after[-1] + 1) if after else idx[0]
        lines.insert(pos, text)
    path.write_text("".join(lines))


def starts(path: Path) -> set[int]:
    return {int(m.group(1), 16) for l in path.read_text().splitlines() if (m := ENTRY.match(l))}


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("name", help="file under src/main without .c, e.g. psyq/libgpu/sys")
    ap.add_argument("start", type=lambda s: int(s, 16))
    ap.add_argument("end", type=lambda s: int(s, 16))
    ap.add_argument("--jp", nargs=2, type=lambda s: int(s, 16), metavar=("START", "END"))
    ap.add_argument("--rodata", nargs=2, type=lambda s: int(s, 16), metavar=("START", "END"))
    ap.add_argument("--jp-rodata", nargs=2, type=lambda s: int(s, 16), metavar=("START", "END"),
                    help="JP .rodata range, when it cannot be found by content")
    ap.add_argument("--no-jp", action="store_true", help="the file does not exist in JP")
    ap.add_argument("--only", choices=["us", "jp"], help="edit only this version's config")
    ap.add_argument("--comment", default="")
    args = ap.parse_args()

    bounds = {"us": (args.start, args.end)}
    if not args.no_jp:
        if args.jp:
            bounds["jp"] = tuple(args.jp)
        elif args.name.startswith("psyq/"):
            us, jp = sdk_table("us"), sdk_table("jp")
            key = next((k for k, v in us.items() if v[0] == args.start), None)
            if key is None or key not in jp:
                sys.exit(f"no library object at 0x{args.start:08X} in US, or not in JP; pass --jp")
            bounds["jp"] = jp[key]
        else:
            sys.exit("pass --jp START END (or --no-jp)")

    rodata = {}
    if args.rodata:
        rodata["us"] = tuple(args.rodata)
        if "jp" in bounds and args.jp_rodata:
            rodata["jp"] = tuple(args.jp_rodata)
        elif "jp" in bounds:
            us_exe = (ROOT / "disc/us/main.exe").read_bytes()
            jp_exe = (ROOT / "disc/jp/main.exe").read_bytes()
            chunk = us_exe[off(args.rodata[0]):off(args.rodata[1])]
            i = jp_exe.find(chunk)
            if i < 0 or jp_exe.find(chunk, i + 1) >= 0:
                sys.exit("JP rodata not found uniquely (it may differ); add JP by hand")
            rodata["jp"] = (i - HDR + VRAM, i - HDR + VRAM + len(chunk))

    for ver, (s, e) in bounds.items():
        if args.only and ver != args.only:
            continue
        path = ROOT / "config" / ver / "main.yaml"
        have = starts(path)
        entries = [(off(s), "c", args.name, args.comment)]
        if off(e) not in have:
            entries.append((off(e), "asm", None, ""))
        if ver in rodata:
            rs, re_ = rodata[ver]
            entries.append((off(rs), ".rodata", args.name, ""))
            if off(re_) not in have:
                entries.append((off(re_), "rodata", None, ""))
        insert(path, entries)
        print(f"{ver}: {args.name} at 0x{s:08X}-0x{e:08X}"
              + (f", .rodata 0x{rodata[ver][0]:08X}-0x{rodata[ver][1]:08X}" if ver in rodata else ""))


if __name__ == "__main__":
    main()
