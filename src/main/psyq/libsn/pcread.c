/*
 * PSY-Q 4.0 LIBSN.LIB PCREAD.OBJ: PCread. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

/* ---- LIBSN READ.OBJ (PCread) ---- */

s32 _SN_read(s32, s32, s32, char *);    /* break 0x105 stub */

s32 PCread(s32 fd, char *buf, u32 len) {
    s32 total = 0;
    s32 chunk;
    s32 n;

    while (len != 0) {
        chunk = len;
        if (len > 0x8000) {
            chunk = 0x8000;
        }
        n = _SN_read(0, fd, chunk, buf);
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
