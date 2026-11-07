/*
 * PSY-Q 4.0 LIBAPI.LIB COUNTER.OBJ: root-counter access. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

/* ---- LIBAPI COUNTER.OBJ ---- */

typedef struct {
    /* 0x0 */ u16 m0_count;
    /* 0x2 */ u16 m2;
    /* 0x4 */ u16 m4_mode;
    /* 0x6 */ u16 m6;
    /* 0x8 */ u16 m8_target;
    /* 0xA */ u16 mA;
    /* 0xC */ u32 mC;
} RCntReg; /* one root counter (0x1F801100 + n * 0x10) */

/* COUNTER.OBJ .data (0x800563FC-0x80056414) */
volatile u32 *rcntIntrRegs = (volatile u32 *)0x1F801070;         /* -> I_STAT; [1] = I_MASK */
volatile RCntReg *rcntRegisters = (volatile RCntReg *)0x1F801100; /* -> root counters */
u32 rcntIntrMaskBits[] = {0x10, 0x20, 0x40, 0x01};                   /* interrupt mask bit per counter */

s32 SetRCnt(u32 spec, u16 target, s32 mode) {
    s32 s = spec & 0xFFFF;
    s32 m = 0x48;

    if (s >= 3) {
        return 0;
    }
    rcntRegisters[s].m4_mode = 0;
    rcntRegisters[s].m8_target = target;
    if (s == 0 || s == 1) {
        if (mode & 0x10) {
            m = 0x49;
        }
        if (!(mode & 1)) {
            m |= 0x100;
        }
    } else if (s == 2) {
        if (!(mode & 1)) {
            m = 0x248;
        }
    }
    if (mode & 0x1000) {
        m |= 0x10;
    }
    rcntRegisters[s].m4_mode = m;
    return 1;
}

s32 GetRCnt(u32 spec) {
    s32 s = spec & 0xFFFF;

    if (s < 3) {
        goto ok;
    }
    return 0;
ok:
    return rcntRegisters[s].m0_count;
}

s32 StartRCnt(u32 spec) {
    s32 s = spec & 0xFFFF;

    rcntIntrRegs[1] |= rcntIntrMaskBits[s];
    return s < 3;
}

s32 StopRCnt(u32 spec) {
    rcntIntrRegs[1] &= ~rcntIntrMaskBits[spec & 0xFFFF];
    return 1;
}

s32 ResetRCnt(u32 spec) {
    s32 s = spec & 0xFFFF;

    if (s < 3) {
        goto ok;
    }
    return 0;
ok:
    rcntRegisters[s].m0_count = 0;
    return 1;
}
