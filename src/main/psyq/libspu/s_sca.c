/*
 * PSY-Q 4.0 LIBSPU.LIB S_SCA.OBJ: SpuSetCommonAttr. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 * .rodata: two 8-entry jump tables = retail jtbl_800194E4 and jtbl_80019504
 * (0x800194E4-0x80019524), checked with --rodata. Register allocation needs
 * the separate `case 0` bodies (identical to the defaults; jump2 cross-jumps
 * them) and `s32 mode`.
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;

void SpuSetCommonAttr(SpuCommonAttr* attr) {
    u32 mask;
    s32 all;
    u16 vl;
    u16 vr;
    s32 mode;
    u16 c;

    vl = 0;
    vr = 0;
    mask = attr->m0_mask;
    all = mask == 0;

    if (all || (mask & 1)) {
        if (all || (mask & 4)) {
            switch (attr->m8_mvolmode.m0_left) {
            case 0:
                vl = attr->m4_mvol.m0_left;
                mode = 0;
                break;
            case 1:
                mode = 0x8000;
                break;
            case 2:
                mode = 0x9000;
                break;
            case 3:
                mode = 0xA000;
                break;
            case 4:
                mode = 0xB000;
                break;
            case 5:
                mode = 0xC000;
                break;
            case 6:
                mode = 0xD000;
                break;
            case 7:
                mode = 0xE000;
                break;
            default:
                vl = attr->m4_mvol.m0_left;
                mode = 0;
                break;
            }
        } else {
            vl = attr->m4_mvol.m0_left;
            mode = 0;
        }
        if (mode != 0) {
            if (attr->m4_mvol.m0_left >= 0x80) {
                vl = 0x7F;
            } else if (attr->m4_mvol.m0_left < 0) {
                vl = 0;
            } else {
                vl = attr->m4_mvol.m0_left;
            }
        }
        _spu_RXX->m180_mainVolL = (vl & 0x7FFF) | mode;
    }

    if (all || (mask & 2)) {
        if (all || (mask & 8)) {
            switch (attr->m8_mvolmode.m2_right) {
            case 0:
                vr = attr->m4_mvol.m2_right;
                mode = 0;
                break;
            case 1:
                mode = 0x8000;
                break;
            case 2:
                mode = 0x9000;
                break;
            case 3:
                mode = 0xA000;
                break;
            case 4:
                mode = 0xB000;
                break;
            case 5:
                mode = 0xC000;
                break;
            case 6:
                mode = 0xD000;
                break;
            case 7:
                mode = 0xE000;
                break;
            default:
                vr = attr->m4_mvol.m2_right;
                mode = 0;
                break;
            }
        } else {
            vr = attr->m4_mvol.m2_right;
            mode = 0;
        }
        if (mode != 0) {
            if (attr->m4_mvol.m2_right >= 0x80) {
                vr = 0x7F;
            } else if (attr->m4_mvol.m2_right < 0) {
                vr = 0;
            } else {
                vr = attr->m4_mvol.m2_right;
            }
        }
        _spu_RXX->m182_mainVolR = (vr & 0x7FFF) | mode;
    }

    if (all || (mask & 0x40)) {
        _spu_RXX->m1B0_cdVolL = attr->m10_cd.m0_volume.m0_left;
    }
    if (all || (mask & 0x80)) {
        _spu_RXX->m1B2_cdVolR = attr->m10_cd.m0_volume.m2_right;
    }
    if (all || (mask & 0x400)) {
        _spu_RXX->m1B4_extVolL = attr->m1C_ext.m0_volume.m0_left;
    }
    if (all || (mask & 0x800)) {
        _spu_RXX->m1B6_extVolR = attr->m1C_ext.m0_volume.m2_right;
    }
    if (all || (mask & 0x100)) {
        if (attr->m10_cd.m4_reverb == 0) {
            c = _spu_RXX->m1AA_control;
            c &= ~4;
            _spu_RXX->m1AA_control = c;
        } else {
            c = _spu_RXX->m1AA_control;
            c |= 4;
            _spu_RXX->m1AA_control = c;
        }
    }
    if (all || (mask & 0x200)) {
        if (attr->m10_cd.m8_mix == 0) {
            c = _spu_RXX->m1AA_control;
            c &= ~1;
            _spu_RXX->m1AA_control = c;
        } else {
            c = _spu_RXX->m1AA_control;
            c |= 1;
            _spu_RXX->m1AA_control = c;
        }
    }
    if (all || (mask & 0x1000)) {
        if (attr->m1C_ext.m4_reverb == 0) {
            c = _spu_RXX->m1AA_control;
            c &= ~8;
            _spu_RXX->m1AA_control = c;
        } else {
            c = _spu_RXX->m1AA_control;
            c |= 8;
            _spu_RXX->m1AA_control = c;
        }
    }
    if (all || (mask & 0x2000)) {
        if (attr->m1C_ext.m8_mix == 0) {
            c = _spu_RXX->m1AA_control;
            c &= ~2;
            _spu_RXX->m1AA_control = c;
        } else {
            c = _spu_RXX->m1AA_control;
            c |= 2;
            _spu_RXX->m1AA_control = c;
        }
    }
}
