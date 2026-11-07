/*
 * PSY-Q 4.0 LIBSN.LIB PCWRITE.OBJ: PCwrite. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

s32 _SN_write(s32, s32, s32, char *);   /* break 0x106 stub */

/* ---- LIBSN READ.OBJ #2 (PCwrite) ---- */

s32 PCwrite(s32 fd, char *buf, u32 len) {
    s32 total = 0;
    s32 chunk;
    s32 n;

    while (len != 0) {
        chunk = len;
        if (len > 0x8000) {
            chunk = 0x8000;
        }
        n = _SN_write(0, fd, chunk, buf);
        if (n == -1) {
            return -1;
        }
        total += n;
        buf += n;
        len -= n;
        if (n < chunk) {
            return total;
        }
    }
    return total;
}
