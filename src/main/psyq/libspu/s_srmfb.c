/*
 * PSY-Q 4.0 LIBSPU.LIB S_SRMFB.OBJ: SpuSetReverbModeFeedback. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 * Retail loads the reverb mode twice (range check, then table index): the
 * byte copy is a static helper that -O3 inlines, and CSE does not carry the
 * first load across the inlined call (written out in place, it does).
 */
#include "common.h"
#include "psyq/libspu.h"

extern SpuReverbAttr D_800589B4;
extern SpuReverbParam spuReverbPresets[];
void _spu_setReverbAttr(SpuReverbParam* attr);

static void copyBytes(void* dst, void* src, u32 n) {
    u8* d = dst;
    u8* s = src;

    while (n--) {
        *d++ = *s++;
    }
}

void SpuSetReverbModeFeedback(s32 feedback) {
    SpuReverbParam attr;

    switch (D_800589B4.m4_mode) {
    case 7:
    case 8:
        copyBytes(&attr, &spuReverbPresets[D_800589B4.m4_mode], sizeof(SpuReverbParam));
        attr.m0_mask = 0x80;
        D_800589B4.m10_feedback = feedback;
        attr.m4_regs[7] = (feedback * 0x8100) / 127;
        _spu_setReverbAttr(&attr);
        break;
    }
}
