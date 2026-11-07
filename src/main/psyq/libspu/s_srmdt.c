/*
 * PSY-Q 4.0 LIBSPU.LIB S_SRMDT.OBJ: SpuSetReverbModeDelayTime. Our own C for
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

void SpuSetReverbModeDelayTime(s32 delay) {
    SpuReverbParam attr;
    s32 dt;
    s32 dt2;

    switch (D_800589B4.m4_mode) {
    case 7:
    case 8:
        copyBytes(&attr, &spuReverbPresets[D_800589B4.m4_mode], sizeof(SpuReverbParam));
        attr.m0_mask = 0x0C011C00;
        D_800589B4.mC_delay = delay;
        dt = (delay << 13) / 127;
        dt2 = (delay << 12) / 127;
        attr.m4_regs[10] = dt - attr.m4_regs[0];
        attr.m4_regs[11] = dt2 - attr.m4_regs[1];
        attr.m4_regs[12] = attr.m4_regs[13] + dt2;
        attr.m4_regs[16] = attr.m4_regs[17] + dt2;
        attr.m4_regs[26] = attr.m4_regs[28] + dt2;
        attr.m4_regs[27] = attr.m4_regs[29] + dt2;
        _spu_setReverbAttr(&attr);
        break;
    }
}
