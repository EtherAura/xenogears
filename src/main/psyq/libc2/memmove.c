/*
 * PSY-Q 4.0 LIBC2.LIB MEMMOVE.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

void* memmove(u8* dst, const u8* src, s32 n) {
    if (dst >= src) {
        while (n-- > 0) {
            dst[n] = src[n];
        }
    } else {
        while (n-- > 0) {
            *dst++ = *src++;
        }
    }
    return dst;
}
