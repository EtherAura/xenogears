/*
 * PSY-Q 4.0 LIBC2.LIB MEMCPY.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

void* memcpy(u8* dst, const u8* src, s32 n) {
    u8* r;
    if (dst == 0) {
        return 0;
    }
    r = dst;
    for (; n > 0; n--) {
        *dst++ = *src++;
    }
    return r;
}
