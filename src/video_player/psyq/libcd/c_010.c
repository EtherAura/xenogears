/*
 * PSY-Q 4.0 LIBCD.LIB C_010.OBJ (not in the signature set; between C_009 and
 * C_011): stores three stream parameters. Our own C (clean-room, from the
 * disassembly).
 */
#include "common.h"

extern s32 stStartFrame;
extern s32 stEndFrame;
extern s32 stSeekingStart;

void StSetMask(s32 a, s32 b, s32 c) {
    stSeekingStart = a;
    stStartFrame = b;
    stEndFrame = c;
}
