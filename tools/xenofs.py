"""Read Xenogears files straight from a raw (2352-byte sector) disc image.

The disc carries two file systems:
  * a minimal ISO9660 root holding SYSTEM.CNF and the boot executable, and
  * the game's own flat file table: 16 sectors from LBA 0x18 of 7-byte entries
    (u24 start LBA, s32 size), plus a directory index at LBA 0x28 of u16
    one-based base entries. File N of directory D is table entry
    base[D] - 1 + N - 1.

Most large program overlays are LZSS-compressed (see lzss_decompress).
"""

from __future__ import annotations

import mmap
import struct
from pathlib import Path

RAW_SECTOR = 2352
USER_OFFSET = 24  # sync(12) + header(4) + XA subheader(8), Mode 2 Form 1
USER_SIZE = 2048

TABLE_LBA = 0x18
TABLE_SECTORS = 16
DIR_INDEX_LBA = 0x28
ENTRY_SIZE = 7


class Disc:
    def __init__(self, path: str | Path):
        self.path = Path(path)
        self._fh = open(self.path, "rb")
        self._mm = mmap.mmap(self._fh.fileno(), 0, access=mmap.ACCESS_READ)
        if len(self._mm) % RAW_SECTOR:
            raise ValueError(f"{path}: not a raw 2352-byte-sector image")
        self._table: list[tuple[int, int]] | None = None
        self._dir_index: list[int] | None = None

    def close(self) -> None:
        self._mm.close()
        self._fh.close()

    def __enter__(self) -> "Disc":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def sector(self, lba: int) -> bytes:
        off = lba * RAW_SECTOR + USER_OFFSET
        return self._mm[off:off + USER_SIZE]

    def read(self, lba: int, size: int) -> bytes:
        count = (size + USER_SIZE - 1) // USER_SIZE
        return b"".join(self.sector(lba + i) for i in range(count))[:size]

    # --- ISO9660 root directory ------------------------------------------

    def iso_find(self, name: str) -> tuple[int, int]:
        pvd = self.sector(16)
        if pvd[1:6] != b"CD001":
            raise ValueError("no ISO9660 primary volume descriptor")
        root_lba, root_size = struct.unpack_from("<I", pvd, 158)[0], struct.unpack_from("<I", pvd, 166)[0]
        data = self.read(root_lba, root_size)
        pos = 0
        while pos < len(data):
            length = data[pos]
            if length == 0:  # records never straddle sectors
                pos = (pos // USER_SIZE + 1) * USER_SIZE
                continue
            name_len = data[pos + 32]
            rec_name = data[pos + 33:pos + 33 + name_len].decode("ascii", "replace")
            if rec_name.split(";")[0].upper() == name.upper():
                lba = struct.unpack_from("<I", data, pos + 2)[0]
                size = struct.unpack_from("<I", data, pos + 10)[0]
                return lba, size
            pos += length
        raise FileNotFoundError(name)

    def iso_read(self, name: str) -> bytes:
        return self.read(*self.iso_find(name))

    # --- Xenogears file table --------------------------------------------

    @property
    def table(self) -> list[tuple[int, int]]:
        if self._table is None:
            raw = b"".join(self.sector(TABLE_LBA + i) for i in range(TABLE_SECTORS))
            self._table = []
            for off in range(0, len(raw) - ENTRY_SIZE + 1, ENTRY_SIZE):
                lba = raw[off] | raw[off + 1] << 8 | raw[off + 2] << 16
                size = struct.unpack_from("<i", raw, off + 3)[0]
                self._table.append((lba, size))
        return self._table

    @property
    def dir_index(self) -> list[int]:
        if self._dir_index is None:
            raw = self.sector(DIR_INDEX_LBA)
            self._dir_index = list(struct.unpack_from(f"<{len(raw) // 2}H", raw))
        return self._dir_index

    def entry_index(self, directory: int, file: int) -> int:
        """Zero-based global table index of file `file` in `directory`."""
        return self.dir_index[directory] - 1 + file - 1

    def entry(self, directory: int, file: int) -> tuple[int, int]:
        return self.table[self.entry_index(directory, file)]

    def read_file(self, directory: int, file: int, whole_sectors: bool = False) -> bytes:
        """Read a table file. whole_sectors keeps the sector padding, as the
        game's loader does; some LZSS streams run a few bytes into it."""
        lba, size = self.entry(directory, file)
        if size <= 0:
            raise ValueError(f"dir {directory:#x} file {file:#x} is not a regular file (size {size})")
        if whole_sectors:
            size = (size + USER_SIZE - 1) // USER_SIZE * USER_SIZE
        return self.read(lba, size)


def lzss_decompress(data: bytes) -> bytes:
    """Xenogears LZSS: u32 output size, then groups of 8 items led by a flag
    byte (LSB first). Flag 0 = literal byte; flag 1 = u16 back-reference with
    a 12-bit distance (low 12 bits) and a 4-bit length + 3 (high 4 bits)."""
    out_size = struct.unpack_from("<I", data, 0)[0]
    out = bytearray()
    pos = 4
    end = len(data)
    while len(out) < out_size and pos < end:
        flags = data[pos]
        pos += 1
        for _ in range(8):
            if len(out) >= out_size or pos >= end:
                break
            if flags & 1:
                ref = data[pos] | data[pos + 1] << 8
                pos += 2
                dist = ref & 0xFFF
                length = (ref >> 12) + 3
                start = len(out) - dist
                for i in range(length):  # may overlap the bytes being written
                    out.append(out[start + i])
            else:
                out.append(data[pos])
                pos += 1
            flags >>= 1
    if len(out) != out_size:
        raise ValueError(f"LZSS stream ended early: {len(out)} of {out_size} bytes")
    return bytes(out)
