/*
 * PSY-Q 4.0 LIBSPU.LIB S_SR.OBJ: SpuSetReverb. Our own C for the library
 * routine(s) (clean-room, from the disassembly).
 * The control value must be one local shared by all branches (a global
 * pseudo, so the register pointer gets v0 and the value v1).
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;
extern s32 D_800589A8;
extern s32 D_800589AC;
extern s32 D_800589B0;
s32 _SpuIsInAllocateArea_(u32 addr);

s32 SpuSetReverb(s32 on_off) {
    u16 c;

    switch (on_off) {
    case 0:
        D_800589A8 = 0;
        c = _spu_RXX->m1AA_control;
        c &= ~0x80;
        _spu_RXX->m1AA_control = c;
        break;
    case 1:
        if (D_800589AC != on_off && _SpuIsInAllocateArea_(D_800589B0)) {
            D_800589A8 = 0;
            c = _spu_RXX->m1AA_control;
            c &= ~0x80;
            _spu_RXX->m1AA_control = c;
        } else {
            D_800589A8 = on_off;
            c = _spu_RXX->m1AA_control;
            c |= 0x80;
            _spu_RXX->m1AA_control = c;
        }
        break;
    }
    return D_800589A8;
}
