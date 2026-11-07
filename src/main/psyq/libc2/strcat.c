/*
 * PSY-Q 4.0 LIBC2.LIB STRCAT.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

s32 strlen(const char* s);

char* strcat(char* dst, const char* src) {
    char* r;
    if (dst == 0 || src == 0) {
        return 0;
    }
    if (dst + strlen(dst) != src + strlen(src)) {
        r = dst;
        while (*dst++ != 0) {
        }
        dst--;
        while ((*dst++ = *src++) != 0) {
        }
        return r;
    }
    return 0;
}
