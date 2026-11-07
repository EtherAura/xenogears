/*
 * PSY-Q 4.0 LIBC2.LIB MEMSET.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

void* memset(u8* p, s32 c, s32 n) {
    u8* r;
    if (p == 0) {
        return 0;
    }
    if (n <= 0) {
        return 0;
    }
    r = p;
    for (; n > 0; n--) {
        *p++ = c;
    }
    return r;
}
