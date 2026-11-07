/*
 * PSY-Q 4.0 LIBETC.LIB VSYNC.OBJ: VSync. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq.h"

void ChangeClearPAD(s32);                     /* B0:5B ChangeClearPad */
/* ---- LIBETC VSYNC.OBJ ---- */

/* VSYNC.OBJ .data (0x8005783C-0x800578A2) */
volatile s32 *vsyncGpuStatReg = (volatile s32 *)0x1F801814; /* -> GPUSTAT */
volatile s32 *vsyncHblankCountReg = (volatile s32 *)0x1F801110; /* -> root counter 1 count (hblanks) */
s32 vsyncLastHcount = 0;                                     /* hblank count at the last VSync */
volatile s32 vsyncLastVcount = 0;                            /* vblank count at the last VSync */
char strLibraryCopyright[] = "Library Programs (c) 1993-1997 Sony Computer Entertainment Inc., All Rights Reserved."; /* unreferenced */
extern volatile s32 vblankCounter;    /* vblank counter (INTR_VB) */
void puts(const char *s);
void ChangeClearRCnt(s32, s32);      /* C0:0A ChangeClearRCnt */
void v_wait(s32 target, s32 timeout);

s32 VSync(s32 mode) {
    s32 stat;
    s32 hcnt;

    stat = *vsyncGpuStatReg;
    hcnt = (*vsyncHblankCountReg - vsyncLastHcount) & 0xFFFF;
    if (mode < 0) {
        return vblankCounter;
    }
    if (mode == 1) {
        return hcnt;
    }
    v_wait((mode > 0) ? vsyncLastVcount + (mode - 1) : vsyncLastVcount, (mode > 0) ? mode - 1 : 0);
    stat = *vsyncGpuStatReg;
    v_wait(vblankCounter + 1, 1);
    if (stat & 0x400000) {
        while (!((stat ^ *vsyncGpuStatReg) & 0x80000000)) {
        }
    }
    vsyncLastVcount = vblankCounter;
    vsyncLastHcount = *vsyncHblankCountReg;
    return hcnt;
}

void v_wait(s32 target, s32 timeout) {
    volatile s32 cnt;

    cnt = timeout << 15;
    while (vblankCounter < target) {
        if (--cnt == -1) {
            puts("VSync: timeout\n");
            ChangeClearPAD(0);
            ChangeClearRCnt(3, 0);
            return;
        }
    }
}
