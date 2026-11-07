/*
 * PSY-Q 4.0 LIBSPU.LIB S_DCB.OBJ: _SpuDataCallback. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

void DMACallback(s32 dma, void (*func)(void));

void _SpuDataCallback(void (*func)(void)) {
    DMACallback(4, func);
}
