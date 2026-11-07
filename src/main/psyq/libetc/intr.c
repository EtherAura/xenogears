/*
 * PSY-Q 4.0 LIBETC.LIB INTR.OBJ: the interrupt dispatcher and callbacks. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

/* ---- LIBETC INTR.OBJ ---- */

typedef struct {
    /* 0x0000 */ u16 m0_started;
    /* 0x0002 */ u16 m2_inTrap;
    /* 0x0004 */ void (*m4_handlers[11])(void);
    /* 0x0030 */ u16 m30_enabled;
    /* 0x0032 */ u16 m32_savedMask;
    /* 0x0034 */ u32 m34_savedDpcr;
    /* 0x0038 */ s32 m38_jmpbuf[12];
    /* 0x0068 */ u32 m68_stack[1024];
} IntrCtl; /* 0x1068: interrupt dispatcher state (0x800578A4) */

typedef struct {
    /* 0x00 */ char *m0_rcsid;   /* -> "$Id: intr.c,v 1.76 ..." (.rodata 0x800193D4) */
    /* 0x04 */ void *(*m4_dma)(s32, void (*)(void));
    /* 0x08 */ void *(*m8_intr)(s32, void (*)(void));
    /* 0x0C */ IntrCtl *(*mC_reset)(void);
    /* 0x10 */ IntrCtl *(*m10_stop)(void);
    /* 0x14 */ void *(*m14_vsync)(s32, void (*)(void));
    /* 0x18 */ IntrCtl *(*m18_restart)(void);
} IntrEnv; /* callback dispatch table (intrEnvTable, .data) */

void EnterCriticalSection(void);          /* EnterCriticalSection (syscall 1) */
void ExitCriticalSection(void);          /* ExitCriticalSection (syscall 2) */
void ChangeClearPAD(s32);           /* B0:5B ChangeClearPad */
void ChangeClearRCnt(s32, s32);      /* C0:0A ChangeClearRCnt */
IntrCtl *startIntr(void);
void *setIntr(s32 irq, void (*func)(void));
IntrCtl *stopIntr(void);
IntrCtl *restartIntr(void);

/* INTR.OBJ .data (0x800578A4-0x80058940); the CVS id string is the first
 * thing in the object's .rodata (0x800193D4). */
IntrCtl intrDispatcherState = {0};
IntrEnv intrEnvTable = {
    "$Id: intr.c,v 1.76 1997/02/12 12:45:05 makoto Exp $",
    NULL, setIntr, startIntr, stopIntr, NULL, restartIntr,
};
IntrCtl *D_80058928 = &intrDispatcherState;                      /* unreferenced */
IntrEnv *pIntrEnv = &intrEnvTable;
volatile u16 *intrStatusReg = (volatile u16 *)0x1F801070; /* -> I_STAT */
volatile u16 *intrMaskReg = (volatile u16 *)0x1F801074; /* -> I_MASK */
volatile u32 *intrDpcrReg = (volatile u32 *)0x1F8010F0; /* -> DPCR */
s32 intrTrapTimeoutCount = 0;                                     /* trap timeout counter */

void printfStub(const char *fmt, ...);   /* printf (stubbed) */
void ReturnFromException(void);                   /* B0:17 ReturnFromException */
void ResetEntryInt(void);                   /* B0:18 ResetEntryInt */
void HookEntryInt(s32 *);                  /* B0:19 HookEntryInt */
void trapIntr(void);
s32 setjmp(s32 *buf);
void *startIntrVSync(void);
void *startIntrDMA(void);
void _96_remove(); /* A0:72 (CD remove); called with an unused argument, see startIntr */
void memclr_intr(s32 *p, s32 n);

void ResetCallback(void) {
    pIntrEnv->mC_reset();
}

void *InterruptCallback(s32 irq, void (*func)(void)) {
    return pIntrEnv->m8_intr(irq, func);
}

void *DMACallback(s32 ch, void (*func)(void)) {
    return pIntrEnv->m4_dma(ch, func);
}

void *VSyncCallback(void (*func)(void)) {
    return pIntrEnv->m14_vsync(4, func);
}

void *VSyncCallbacks(s32 idx, void (*func)(void)) {
    return pIntrEnv->m14_vsync(idx, func);
}

void StopCallback(void) {
    pIntrEnv->m10_stop();
}

void RestartCallback(void) {
    pIntrEnv->m18_restart();
}

u16 CheckCallback(void) {
    return intrDispatcherState.m2_inTrap;
}

u16 GetIntrMask(void) {
    return *intrMaskReg;
}

s32 SetIntrMask(u16 mask) {
    u16 old = *intrMaskReg;
    *intrMaskReg = mask;
    return old;
}

IntrCtl *startIntr(void) {
    if (intrDispatcherState.m0_started != 0) {
        return NULL;
    }
    *intrStatusReg = *intrMaskReg = 0;
    *intrDpcrReg = 0x33333333;
    memclr_intr((s32 *)&intrDispatcherState, sizeof(IntrCtl) / 4);
    if (setjmp(intrDispatcherState.m38_jmpbuf)) {
        trapIntr();
    }
    intrDispatcherState.m38_jmpbuf[1] = (s32)&intrDispatcherState.m68_stack[1004];
    HookEntryInt(intrDispatcherState.m38_jmpbuf);
    intrDispatcherState.m0_started = 1;
    pIntrEnv->m14_vsync = startIntrVSync();
    pIntrEnv->m4_dma = startIntrDMA();
    /* retail has the environment pointer in a0 at this call */
    _96_remove(pIntrEnv);
    ExitCriticalSection();
    return &intrDispatcherState;
}

void trapIntr(void) {
    u16 mask;
    s32 i;

    if (intrDispatcherState.m0_started == 0) {
        printfStub("unexpected interrupt(%04x)\n", *intrStatusReg);
        ReturnFromException();
    }
    intrDispatcherState.m2_inTrap = 1;
    while ((mask = intrDispatcherState.m30_enabled & *intrStatusReg & *intrMaskReg) != 0) {
        for (i = 0; mask != 0 && i < 11; i++, mask >>= 1) {
            if (mask & 1) {
                *intrStatusReg = ~(1 << i);
                if (intrDispatcherState.m4_handlers[i] != NULL) {
                    intrDispatcherState.m4_handlers[i]();
                }
            }
        }
    }
    if (*intrStatusReg & *intrMaskReg) {
        if (intrTrapTimeoutCount++ > 0x800) {
            printfStub("intr timeout(%04x:%04x)\n", *intrStatusReg, *intrMaskReg);
            intrTrapTimeoutCount = 0;
            *intrStatusReg = 0;
        }
    } else {
        intrTrapTimeoutCount = 0;
    }
    intrDispatcherState.m2_inTrap = 0;
    ReturnFromException();
}

void *setIntr(s32 irq, void (*func)(void)) {
    void (*old)(void);
    s32 mask;

    old = intrDispatcherState.m4_handlers[irq];
    if (func != old && intrDispatcherState.m0_started != 0) {
        mask = SetIntrMask(0);
        if (func != NULL) {
            intrDispatcherState.m4_handlers[irq] = func;
            mask |= 1 << irq;
            intrDispatcherState.m30_enabled |= 1 << irq;
        } else {
            intrDispatcherState.m4_handlers[irq] = NULL;
            mask &= ~(1 << irq);
            intrDispatcherState.m30_enabled &= ~(1 << irq);
        }
        if (irq == 0) {
            ChangeClearPAD(func == NULL);
            ChangeClearRCnt(3, func == NULL);
        }
        if (irq == 4) {
            ChangeClearRCnt(0, func == NULL);
        }
        if (irq == 5) {
            ChangeClearRCnt(1, func == NULL);
        }
        if (irq == 6) {
            ChangeClearRCnt(2, func == NULL);
        }
        *intrMaskReg = mask;
    }
    return old;
}

IntrCtl *stopIntr(void) {
    if (intrDispatcherState.m0_started == 0) {
        return NULL;
    }
    EnterCriticalSection();
    intrDispatcherState.m32_savedMask = *intrMaskReg;
    intrDispatcherState.m34_savedDpcr = *intrDpcrReg;
    *intrStatusReg = *intrMaskReg = 0;
    *intrDpcrReg &= 0x77777777;
    ResetEntryInt();
    intrDispatcherState.m0_started = 0;
    return &intrDispatcherState;
}

IntrCtl *restartIntr(void) {
    if (intrDispatcherState.m0_started != 0) {
        return NULL;
    }
    HookEntryInt(intrDispatcherState.m38_jmpbuf);
    intrDispatcherState.m0_started = 1;
    *intrMaskReg = intrDispatcherState.m32_savedMask;
    *intrDpcrReg = intrDispatcherState.m34_savedDpcr;
    ExitCriticalSection();
    return &intrDispatcherState;
}

void memclr_intr(s32 *p, s32 n) {
    s32 i;
    s32 unused[1];

    for (i = n - 1; i != -1; i--) {
        *p++ = 0;
    }
}
