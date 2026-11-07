/*
 * PSY-Q 4.0 LIBC2.LIB BZERO.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

void* bzero(u8* p, s32 n) {
    u8* r;
    if (p == 0) {
        return 0;
    }
    if (n <= 0) {
        return 0;
    }
    r = p;
    for (; n > 0; n--) {
        *p++ = 0;
    }
    return r;
}
