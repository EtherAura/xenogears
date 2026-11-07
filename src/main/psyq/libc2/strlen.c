/*
 * PSY-Q 4.0 LIBC2.LIB STRLEN.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

s32 strlen(const char* s) {
    s32 n = 0;
    if (s == 0) {
        return 0;
    }
    while (*s++ != 0) {
        n++;
    }
    return n;
}
