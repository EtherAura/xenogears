/*
 * PSY-Q 4.0 LIBC2.LIB CTYPE.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

/* character classes */
#define _U 0x01 /* upper case */
#define _L 0x02 /* lower case */
#define _N 0x04 /* digit */
#define _S 0x08 /* white space */
#define _P 0x10 /* punctuation (and ' ') */
#define _C 0x20 /* control */
#define _X 0x40 /* hex letter */

/* CTYPE.OBJ .data (0x800567C0-0x80056841): the class of EOF (-1), then of
 * 0x00-0x7F, so users index it from +1 (putchar too). */
u8 ctypeClassTable[0x81] = {
    0,
    _C, _C, _C, _C, _C, _C, _C, _C, _C, _S, _S, _S, _S, _S, _C, _C,  /* 0x00-0x0F */
    _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C,  /* 0x10-0x1F */
    _S | _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, /* 0x20-0x2F */
    _N, _N, _N, _N, _N, _N, _N, _N, _N, _N, _P, _P, _P, _P, _P, _P,  /* 0x30-0x3F */
    _P, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U, _U, _U, _U, _U, _U, _U, _U, _U, /* 0x40-0x4F */
    _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _P, _P, _P, _P, _P,  /* 0x50-0x5F */
    _P, _L | _X, _L | _X, _L | _X, _L | _X, _L | _X, _L | _X, _L, _L, _L, _L, _L, _L, _L, _L, _L, /* 0x60-0x6F */
    _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _P, _P, _P, _P, _C,  /* 0x70-0x7F */
};

s32 toupper(s32 c) {
    char r = c;
    if (ctypeClassTable[r + 1] & _L) {
        r = c - 0x20;
    }
    return r;
}

s32 tolower(s32 c) {
    char r = c;
    if (ctypeClassTable[r + 1] & _U) {
        r = c + 0x20;
    }
    return r;
}
