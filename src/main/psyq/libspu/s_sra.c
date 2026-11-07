/*
 * PSY-Q 4.0 LIBSPU.LIB S_SRA.OBJ: _spu_setReverbAttr. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 * Writes each reverb register (0x1C0-0x1FE) whose mask bit is set (all if
 * mask == 0).
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;

void _spu_setReverbAttr(SpuReverbParam* attr) {
    u32 mask;
    s32 all;

    mask = attr->m0_mask;
    all = mask == 0;
    if (all || (mask & 0x1)) {
        _spu_RXX->m1C0_reverb[0] = attr->m4_regs[0];
    }
    if (all || (mask & 0x2)) {
        _spu_RXX->m1C0_reverb[1] = attr->m4_regs[1];
    }
    if (all || (mask & 0x4)) {
        _spu_RXX->m1C0_reverb[2] = attr->m4_regs[2];
    }
    if (all || (mask & 0x8)) {
        _spu_RXX->m1C0_reverb[3] = attr->m4_regs[3];
    }
    if (all || (mask & 0x10)) {
        _spu_RXX->m1C0_reverb[4] = attr->m4_regs[4];
    }
    if (all || (mask & 0x20)) {
        _spu_RXX->m1C0_reverb[5] = attr->m4_regs[5];
    }
    if (all || (mask & 0x40)) {
        _spu_RXX->m1C0_reverb[6] = attr->m4_regs[6];
    }
    if (all || (mask & 0x80)) {
        _spu_RXX->m1C0_reverb[7] = attr->m4_regs[7];
    }
    if (all || (mask & 0x100)) {
        _spu_RXX->m1C0_reverb[8] = attr->m4_regs[8];
    }
    if (all || (mask & 0x200)) {
        _spu_RXX->m1C0_reverb[9] = attr->m4_regs[9];
    }
    if (all || (mask & 0x400)) {
        _spu_RXX->m1C0_reverb[10] = attr->m4_regs[10];
    }
    if (all || (mask & 0x800)) {
        _spu_RXX->m1C0_reverb[11] = attr->m4_regs[11];
    }
    if (all || (mask & 0x1000)) {
        _spu_RXX->m1C0_reverb[12] = attr->m4_regs[12];
    }
    if (all || (mask & 0x2000)) {
        _spu_RXX->m1C0_reverb[13] = attr->m4_regs[13];
    }
    if (all || (mask & 0x4000)) {
        _spu_RXX->m1C0_reverb[14] = attr->m4_regs[14];
    }
    if (all || (mask & 0x8000)) {
        _spu_RXX->m1C0_reverb[15] = attr->m4_regs[15];
    }
    if (all || (mask & 0x10000)) {
        _spu_RXX->m1C0_reverb[16] = attr->m4_regs[16];
    }
    if (all || (mask & 0x20000)) {
        _spu_RXX->m1C0_reverb[17] = attr->m4_regs[17];
    }
    if (all || (mask & 0x40000)) {
        _spu_RXX->m1C0_reverb[18] = attr->m4_regs[18];
    }
    if (all || (mask & 0x80000)) {
        _spu_RXX->m1C0_reverb[19] = attr->m4_regs[19];
    }
    if (all || (mask & 0x100000)) {
        _spu_RXX->m1C0_reverb[20] = attr->m4_regs[20];
    }
    if (all || (mask & 0x200000)) {
        _spu_RXX->m1C0_reverb[21] = attr->m4_regs[21];
    }
    if (all || (mask & 0x400000)) {
        _spu_RXX->m1C0_reverb[22] = attr->m4_regs[22];
    }
    if (all || (mask & 0x800000)) {
        _spu_RXX->m1C0_reverb[23] = attr->m4_regs[23];
    }
    if (all || (mask & 0x1000000)) {
        _spu_RXX->m1C0_reverb[24] = attr->m4_regs[24];
    }
    if (all || (mask & 0x2000000)) {
        _spu_RXX->m1C0_reverb[25] = attr->m4_regs[25];
    }
    if (all || (mask & 0x4000000)) {
        _spu_RXX->m1C0_reverb[26] = attr->m4_regs[26];
    }
    if (all || (mask & 0x8000000)) {
        _spu_RXX->m1C0_reverb[27] = attr->m4_regs[27];
    }
    if (all || (mask & 0x10000000)) {
        _spu_RXX->m1C0_reverb[28] = attr->m4_regs[28];
    }
    if (all || (mask & 0x20000000)) {
        _spu_RXX->m1C0_reverb[29] = attr->m4_regs[29];
    }
    if (all || (mask & 0x40000000)) {
        _spu_RXX->m1C0_reverb[30] = attr->m4_regs[30];
    }
    if (all || (mask & 0x80000000)) {
        _spu_RXX->m1C0_reverb[31] = attr->m4_regs[31];
    }
}
