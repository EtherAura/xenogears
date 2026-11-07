/*
 * PSY-Q 4.0 LIBSPU.LIB S_CB.OBJ: _SpuCallback. Our own C for the library
 * routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

void InterruptCallback(s32 irq, void (*func)(void));

void _SpuCallback(void (*func)(void)) {
    InterruptCallback(9, func);
}
