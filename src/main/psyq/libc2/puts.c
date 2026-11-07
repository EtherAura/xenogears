/*
 * PSY-Q 4.0 LIBC2.LIB PUTS.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

s32 putchar(char c);

void puts(char* s) {
    char c;
    if (s == 0) {
        s = "<NULL>";
    }
    while ((c = *s++) != 0) {
        putchar(c);
    }
}
