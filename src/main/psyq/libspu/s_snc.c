/*
 * PSY-Q 4.0 LIBSPU.LIB S_SNC.OBJ: SpuSetNoiseClock. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;

s32 SpuSetNoiseClock(s32 n_clock) {
    s32 clock;

    if (n_clock < 0) {
        clock = 0;
    } else {
        clock = n_clock;
        if (clock >= 0x40) {
            clock = 0x3F;
        }
    }
    _spu_RXX->m1AA_control = (_spu_RXX->m1AA_control & 0xC0FF) | ((clock & 0x3F) << 8);
    return clock;
}
