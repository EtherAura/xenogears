/*
 * PSY-Q 4.0 LIBSPU.LIB S_GVEA.OBJ: SpuGetVoiceEnvelopeAttr. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;
extern u32 D_800589A0;

void SpuGetVoiceEnvelopeAttr(s32 vNum, s32* keyStat, s16* envx) {
    s16 env;

    env = ((volatile u16*)_spu_RXX)[(vNum << 3) | 6];
    *envx = env;
    if (D_800589A0 & (1 << vNum)) {
        if (env > 0) {
            *keyStat = 1;
        } else {
            *keyStat = 3;
        }
    } else {
        if (env > 0) {
            *keyStat = 2;
        } else {
            *keyStat = 0;
        }
    }
}
