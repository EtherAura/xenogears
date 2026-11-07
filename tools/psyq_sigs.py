#!/usr/bin/env python3
"""Find PSY-Q library objects in a module using the published signature sets
(tools/toolchain/psyq-signatures, fetched by tools/setup.sh).

Each signature is the byte pattern of one library object's .text with
relocations wildcarded, plus the offsets of its symbols. A match therefore
names functions *and* gives the object's exact bounds (one C file / TU).

    python tools/psyq_sigs.py us main                 # report
    python tools/psyq_sigs.py us main --symbols       # splat symbol_addrs lines
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
SIGS = ROOT / "tools/toolchain/psyq-signatures"
# Signature labels include the generator's local branch labels; skip them.
LOCAL_LABEL = re.compile(r"(loc|text)_[0-9A-F]+")


def load_module(version: str, name: str) -> tuple[bytes, int]:
    manifest = yaml.safe_load((ROOT / "config" / version / "disc.yaml").read_text())
    entry = next(e for e in manifest["files"] if e["name"] == name)
    data = (ROOT / "disc" / version / (name + (".exe" if "iso" in entry else ".bin"))).read_bytes()
    if "iso" in entry:
        data = data[0x800:]  # PS-X EXE header
    return data, entry["vram"]


def compile_sig(sig: str) -> tuple[re.Pattern, int, int]:
    parts, fixed = [], 0
    tokens = sig.split()
    for t in tokens:
        if t == "??":
            parts.append(b".")
        else:
            parts.append(re.escape(bytes([int(t, 16)])))
            fixed += 1
    return re.compile(b"".join(parts), re.DOTALL), len(tokens), fixed


def scan(image: bytes, min_fixed: int):
    """Yield (offset, length, sdk, lib, obj, labels) for every aligned match."""
    cache: dict[str, list[int]] = {}
    for sdk_dir in sorted(p for p in SIGS.iterdir() if p.is_dir() and p.name.isdigit()):
        for lib_file in sorted(sdk_dir.glob("*.json")):
            lib = lib_file.name.removesuffix(".json")
            for obj in json.loads(lib_file.read_text()):
                sig = obj.get("sig")
                if not sig:
                    continue
                if sig not in cache:
                    pattern, length, fixed = compile_sig(sig)
                    if fixed < min_fixed:
                        cache[sig] = []
                    else:
                        hits, pos = [], 0
                        while (m := pattern.search(image, pos)) is not None:
                            if m.start() % 4 == 0:
                                hits.append(m.start())
                            pos = m.start() + 1
                        cache[sig] = hits
                length = len(sig.split())
                for off in cache[sig]:
                    yield off, length, sdk_dir.name, lib, obj["name"], obj.get("labels", [])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version")
    parser.add_argument("module", nargs="?", default="main")
    parser.add_argument("--min-fixed", type=int, default=24, help="ignore signatures with fewer non-wildcard bytes")
    parser.add_argument("--symbols", action="store_true", help="print splat symbol_addrs lines")
    parser.add_argument("--range", nargs=2, metavar=("START", "END"), type=lambda v: int(v, 0),
                        help="only accept objects inside this address range (e.g. the SDK block)")
    args = parser.parse_args()

    image, vram = load_module(args.version, args.module)
    # (offset, length, lib, obj) -> SDK versions whose signature matched there
    found: dict[tuple, dict] = {}
    for off, length, sdk, lib, obj, labels in scan(image, args.min_fixed):
        key = (off, length, lib, obj)
        rec = found.setdefault(key, {"sdks": [], "labels": labels})
        rec["sdks"].append(sdk)

    if args.range:
        lo, hi = args.range[0] - vram, args.range[1] - vram
        found = {k: v for k, v in found.items() if lo <= k[0] and k[0] + k[1] <= hi}

    # Resolve overlaps: longer objects win, so short coincidental matches
    # inside a real object are dropped.
    objects: list[tuple] = []
    for key in sorted(found, key=lambda k: -k[1]):
        off, length = key[0], key[1]
        if all(off + length <= o or off >= o + n for o, n, _, _ in objects):
            objects.append(key)
    objects.sort()

    # An object whose signature matched in more than one place is ambiguous
    # (one copy is a lookalike); leave its functions unnamed.
    placements: dict[tuple, int] = defaultdict(int)
    for _, _, lib, obj in objects:
        placements[(lib, obj)] += 1
    ambiguous = {k for k, n in placements.items() if n > 1}

    # Static helpers share names across objects (memclr in each INTR*.OBJ);
    # keep the name plain where it is the object's entry (offset 0),
    # otherwise suffix the object name.
    labels = [(vram + off + label["offset"], label["name"], label["offset"], obj)
              for off, length, lib, obj in objects if (lib, obj) not in ambiguous
              for label in found[(off, length, lib, obj)]["labels"]
              if not LOCAL_LABEL.fullmatch(label["name"])]
    counts: dict[str, int] = defaultdict(int)
    for _, name, _, _ in labels:
        counts[name] += 1
    symbols: dict[int, str] = {}
    for addr, name, label_off, obj in labels:
        if counts[name] > 1 and label_off != 0:
            name = f"{name}_{obj.removesuffix('.OBJ').lower()}"
        symbols.setdefault(addr, name)

    if args.symbols:
        for addr, name in sorted(symbols.items()):
            print(f"{name} = 0x{addr:08X}; // type:func")
        return 0

    per_sdk_lib: dict[tuple, int] = defaultdict(int)
    covered = 0
    prev_end = 0
    print(f"{'address':10s} {'end':10s} {'library':14s} {'object':14s} SDK versions")
    for off, length, lib, obj in objects:
        sdks = found[(off, length, lib, obj)]["sdks"]
        for s in sdks:
            per_sdk_lib[(s, lib)] += length
        note = "  OVERLAP" if off < prev_end else ""
        prev_end = max(prev_end, off + length)
        covered += length
        print(f"{vram + off:#010x} {vram + off + length:#010x} {lib:14s} {obj:14s} "
              f"{','.join(sdks)}{note}")
    print(f"\n{len(objects)} objects, {covered} bytes, {len(symbols)} symbols")
    for lib, obj in sorted(ambiguous):
        print(f"  ambiguous (matched {placements[(lib, obj)]}x, left unnamed): {lib} {obj}")

    libs = sorted({lib for _, lib in per_sdk_lib})
    print("\nBytes matched per library and SDK version (best first):")
    for lib in libs:
        ranking = sorted(((n, s) for (s, l), n in per_sdk_lib.items() if l == lib), reverse=True)
        top = ranking[0][0]
        best = [s for n, s in ranking if n == top]
        print(f"  {lib:14s} {top:6d} bytes: {','.join(best)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
