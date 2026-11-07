#!/usr/bin/env python3
"""Extract the code-bearing files listed in config/<version>/disc.yaml from
the user's disc image into disc/<version>/, verifying every file's sha1.

    python tools/extract.py            # all versions with a disc.yaml
    python tools/extract.py us         # one version
    python tools/extract.py us --image path/to/disc1.bin --check-image
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

import yaml

from xenofs import Disc, lzss_decompress

ROOT = Path(__file__).resolve().parent.parent


def output_name(entry: dict) -> str:
    return entry["name"] + (".exe" if "iso" in entry else ".bin")


def extract_version(version: str, image: Path | None, check_image: bool) -> bool:
    manifest = yaml.safe_load((ROOT / "config" / version / "disc.yaml").read_text())
    image = image or ROOT / manifest["image"]
    if not image.is_file():
        print(f"[{version}] disc image not found: {image}", file=sys.stderr)
        return False

    if check_image and manifest.get("image_sha256"):
        h = hashlib.sha256()
        with open(image, "rb") as fh:
            for chunk in iter(lambda: fh.read(1 << 24), b""):
                h.update(chunk)
        if h.hexdigest() != manifest["image_sha256"]:
            print(f"[{version}] image sha256 mismatch: {h.hexdigest()}", file=sys.stderr)
            return False

    out_dir = ROOT / "disc" / version
    out_dir.mkdir(parents=True, exist_ok=True)
    ok = True
    with Disc(image) as disc:
        for entry in manifest["files"]:
            if "iso" in entry:
                data = disc.iso_read(entry["iso"])
            else:
                data = disc.read_file(entry["dir"], entry["file"], whole_sectors=entry.get("lzss", False))
                if entry.get("lzss"):
                    data = lzss_decompress(data)
            digest = hashlib.sha1(data).hexdigest()
            want = entry.get("sha1")
            status = "ok" if digest == want else ("NEW " + digest if not want else "MISMATCH " + digest)
            if want and digest != want:
                ok = False
            (out_dir / output_name(entry)).write_bytes(data)
            print(f"[{version}] {entry['name']:15s} {len(data):7d} bytes  {status}")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("versions", nargs="*", help="versions to extract (default: all)")
    parser.add_argument("--image", type=Path, help="override the disc image path (single version only)")
    parser.add_argument("--check-image", action="store_true", help="also verify the whole image's sha256")
    args = parser.parse_args()

    versions = args.versions or sorted(p.parent.name for p in (ROOT / "config").glob("*/disc.yaml"))
    if args.image and len(versions) != 1:
        parser.error("--image needs exactly one version")
    results = [extract_version(v, args.image, args.check_image) for v in versions]
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
