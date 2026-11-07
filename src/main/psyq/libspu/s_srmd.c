/*
 * PSY-Q 4.0 LIBSPU.LIB S_SRMD.OBJ: SpuSetReverbModeDepth. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 * The depth globals must be fields of a struct (D_800589B4, SpuReverbAttr):
 * as plain s16 globals the scheduler moves their stores first.
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;
extern SpuReverbAttr D_800589B4;

void SpuSetReverbModeDepth(s16 left, s16 right) {
    _spu_RXX->m184_revVolL = left;
    _spu_RXX->m186_revVolR = right;
    D_800589B4.m8_depth.m0_left = left;
    D_800589B4.m8_depth.m2_right = right;
}
