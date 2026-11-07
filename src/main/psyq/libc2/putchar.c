/*
 * PSY-Q 4.0 LIBC2.LIB PUTCHAR.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

static s32 putcharColumn; /* output column */
extern u8 ctypeClassTable[]; /* CTYPE.OBJ's class table (EOF first, so +1) */
void func_80040554(s32 fd, void* buf, s32 n);

s32 putchar(char c) {
    switch (c) {
    case '\n':
        putchar('\r');
        putcharColumn = 0;
        break;
    case '\t':
        do {
            putchar(' ');
        } while (putcharColumn & 7);
        return;
    default:
        if (ctypeClassTable[c + 1] & 0x97) {
            putcharColumn++;
        }
        break;
    }
    func_80040554(1, &c, 1);
}
