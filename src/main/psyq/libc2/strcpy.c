/*
 * PSY-Q 4.0 LIBC2.LIB STRCPY.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

char* strcpy(char* dst, const char* src) {
    char* r;
    if (dst == 0 || src == 0) {
        return 0;
    }
    r = dst;
    while ((*dst++ = *src++) != 0) {
    }
    return r;
}
