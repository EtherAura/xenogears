#!/usr/bin/env python3
"""Zero-pad a linked image to its retail size.

Several LZSS-compressed overlays declare a decompressed size that is not a
multiple of 4 (e.g. movie is 0x7453 bytes); the final 1-3 bytes are zeros
that no section covers, and spimdisasm drops sub-word tails. Only padding
is ever added: an image already larger than the target is an error.
"""

import os
import sys


def main() -> int:
    path, size = sys.argv[1], int(sys.argv[2], 0)
    current = os.path.getsize(path)
    if current > size:
        print(f"{path}: {current:#x} bytes, larger than the retail {size:#x}", file=sys.stderr)
        return 1
    with open(path, "ab") as fh:
        fh.write(bytes(size - current))
    return 0


if __name__ == "__main__":
    sys.exit(main())
