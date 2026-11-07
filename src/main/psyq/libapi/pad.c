/*
 * PSY-Q 4.0 LIBAPI.LIB PAD.OBJ: BIOS pad setup. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

/* ---- LIBAPI PAD.OBJ ---- */

typedef struct {
    /* 0x0 */ s32 m0_next;
    /* 0x4 */ s32 (*m4_func2)(void);
    /* 0x8 */ s32 (*m8_func1)(void);
    /* 0xC */ s32 mC;
} IntRP; /* BIOS SysEnqIntRP entry */

/* PAD.OBJ .data (0x80056414-0x80056420) */
s32 padInitFlag = 0;                                     /* pad initialised flag */
static IntRP padIntrHandlerEntry;                                /* pad interrupt handler entry */
volatile s16 *sio0Regs = (volatile s16 *)0x1F801040; /* -> SIO0 / pad port; [5] = JOY_CTRL */
volatile u32 *padIntrRegs = (volatile u32 *)0x1F801070; /* -> I_STAT; [1] = I_MASK */
void EnterCriticalSection(void);                    /* EnterCriticalSection (syscall 1) */
void ExitCriticalSection(void);                    /* ExitCriticalSection (syscall 2) */
void ChangeClearPAD(s32);                     /* B0:5B ChangeClearPad */
void InitPAD2(s32, s32, s32, s32);      /* B0:12 InitPad */
void StartPAD2(void);                    /* B0:13 StartPad */
void StopPAD2(void);                    /* B0:14 StopPad */
void PAD_init2(s32, s32, s32, s32);      /* B0:15 InitPad (outdated) */
void SysEnqIntRP(s32, IntRP *);            /* C0:02 SysEnqIntRP */
void SysDeqIntRP(s32, IntRP *);            /* C0:03 SysDeqIntRP */
void _remove_ChgclrPAD(void);
void _patch_pad(void);
void _send_pad(void);
void EnablePAD(void);
void DesablePAD(void);
s32 SetPatchPad(void);
s32 RemovePatchPad(void);
s32 _Pad1(void);
s32 _IsVSync(void);

void SetInitPadFlag(s32 flag) {
    padInitFlag = flag;
}

s32 ReadInitPadFlag(void) {
    return padInitFlag;
}

void PAD_init(s32 a0, s32 a1, s32 a2, s32 a3) {
    _remove_ChgclrPAD();
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    SetPatchPad();
    PAD_init2(a0, a1, a2, a3);
    _send_pad();
    padInitFlag = 1;
}

void InitPAD(s32 a0, s32 a1, s32 a2, s32 a3) {
    _remove_ChgclrPAD();
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    SetPatchPad();
    InitPAD2(a0, a1, a2, a3);
    _send_pad();
    padInitFlag = 1;
}

void StartPAD(void) {
    StartPAD2();
    ChangeClearPAD(0);
    EnablePAD();
}

void StopPAD(void) {
    DesablePAD();
    StopPAD2();
    RemovePatchPad();
    padInitFlag = 0;
}

s32 SetPatchPad(void) {
    s32* p;
    EnterCriticalSection();
    padIntrHandlerEntry.m4_func2 = _Pad1;
    padIntrHandlerEntry.m8_func1 = _IsVSync;
    /* One local for both zero stores: reassigning p drops its equivalence
     * with &padIntrHandlerEntry, so the call argument is derived from the m4 address
     * (s0 = v1 - 4) and the m0 store keeps its direct address, as in retail. */
    p = &padIntrHandlerEntry.m0_next;
    *p = 0;
    p = &padIntrHandlerEntry.mC;
    *p = 0;
    SysDeqIntRP(1, &padIntrHandlerEntry);
    SysEnqIntRP(1, &padIntrHandlerEntry);
    ExitCriticalSection();
    return 1;
}

s32 RemovePatchPad(void) {
    EnterCriticalSection();
    SysDeqIntRP(1, &padIntrHandlerEntry);
    ExitCriticalSection();
    return 1;
}

s32 _Pad1(void) {
    volatile s32 i;
    s32 unused[2];

    sio0Regs[5] = 0;
    i = 10;
    while (--i != -1) {
    }
    return 0;
}

s32 _IsVSync(void) {
    if (!(padIntrRegs[1] & 1) || !(padIntrRegs[0] & 1)) {
        return 0;
    }
    return 1;
}
