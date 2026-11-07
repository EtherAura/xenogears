/*
 * PSY-Q 4.0 LIBC2.LIB RAND.OBJ. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq.h"

static s32 randSeed; /* RAND.OBJ data: the seed */

s32 rand(void) {
    randSeed = randSeed * 0x41C64E6D + 12345;
    return ((u32)randSeed >> 16) & 0x7FFF;
}

void srand(u32 seed) {
    randSeed = seed;
}
