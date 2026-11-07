"""splat segment type `linkfill`: bytes the linker left between two objects.

psylink pads the gap before an aligned section with whatever was in its
buffer, so retail sometimes has non-zero bytes there (e.g. the 3 bytes after
the shop's "BISLPS-00800"). They belong to no object, and SUBALIGN would
move an object that held them, so this type writes them into the linker
script in place, as BYTE() statements read from the retail image:

    - [0xD, linkfill]
"""

from pathlib import Path
from typing import List

from splat.segtypes.common.segment import CommonSegment
from splat.segtypes.linker_entry import LinkerEntry, LinkerWriter
from splat.util import options


class LinkerEntryFill(LinkerEntry):
    def __init__(self, segment: "PSXSegLinkfill", data: bytes):
        super().__init__(segment, [], Path(), "linkfill", "linkfill", False)
        self.object_path = None
        self.data = data

    def emit_entry(self, linker_writer: LinkerWriter):
        linker_writer._writeln(" ".join(f"BYTE(0x{b:02X});" for b in self.data))


class PSXSegLinkfill(CommonSegment):
    def get_linker_section_order(self) -> str:
        return ""

    def get_linker_section_linksection(self) -> str:
        return ""

    def get_linker_entries(self) -> List[LinkerEntry]:
        assert isinstance(self.rom_start, int) and isinstance(self.rom_end, int)
        with open(options.opts.target_path, "rb") as f:
            f.seek(self.rom_start)
            data = f.read(self.rom_end - self.rom_start)
        return [LinkerEntryFill(self, data)]
