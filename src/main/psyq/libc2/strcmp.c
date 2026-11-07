/*
 * PSY-Q 4.0 LIBC2.LIB STRCMP.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

s32 strcmp(const char* s1, const char* s2) {
    if (s1 == 0 || s2 == 0) {
        if (s1 == s2) {
            return 0;
        }
        if (s1 == 0) {
            return -1;
        }
        return 1;
    }
    while (*s1 == *s2++) {
        if (*s1++ == 0) {
            return 0;
        }
    }
    return *(u8*)s1 - *(u8*)(s2 - 1);
}
