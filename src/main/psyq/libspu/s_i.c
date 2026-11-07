/*
 * PSY-Q 4.0 LIBSPU.LIB S_I.OBJ: SpuInit. Our own C for the library
 * routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

void _SpuInit(s32 mode);

void SpuInit(void) {
    _SpuInit(0);
}
