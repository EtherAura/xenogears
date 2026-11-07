/*
 * PSY-Q 4.0 LIBC2.LIB MEMCHR.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

void* memchr(const u8* s, s32 c, s32 n) {
    if (s == 0 || n <= 0) {
        return 0;
    }
    while (--n >= 0) {
        if (*s++ == (u8)c) {
            return (void*)(s - 1);
        }
    }
    return 0;
}
