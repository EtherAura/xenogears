/*
 * PSY-Q 4.0 LIBPRESS.LIB BUILD.OBJ: DecDCTvlcBuild. Our own C (clean-room,
 * from the disassembly): expands the packed VLC table (literal runs,
 * back-references, 0xF0 = literal mode) and then undoes the delta coding.
 */
#include "common.h"

extern u8 vlcPackedTable[]; /* packed VLC table (.data) */

void DecDCTvlcBuild(u16* table) {
    u8* src;
    u8* dst;
    s32 dist;
    s32 n;
    s32 i;
    u8 c;

    dist = 0;
    src = vlcPackedTable;
    dst = (u8*)table;
    do {
        c = *src++;
        n = c;
        if (c < 0xF0) {
            if (dist != 0) {
                for (; n >= 0; n--) {
                    *dst = *(dst - dist);
                    dst++;
                }
            } else {
                for (; n >= 0; n--) {
                    *dst++ = *src++;
                }
            }
        } else if (n == 0xF0) {
            dist = 0;
        } else {
            dist = ((n << 8) | *src++) - 0xF0FF;
        }
    } while (dist != 0xF00);
    for (i = 4; i < 0x8800; i++) {
        table[i] ^= table[i - 4];
    }
}
