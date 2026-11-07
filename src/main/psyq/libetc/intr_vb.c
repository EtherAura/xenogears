/*
 * PSY-Q 4.0 LIBETC.LIB INTR_VB.OBJ: the VSync interrupt. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

void *InterruptCallback(s32 irq, void (*func)(void));
/* ---- LIBETC INTR_VB.OBJ ---- */

/* INTR_VB.OBJ .data (0x80058940-0x80058968) */
void (*vsyncCallbacks[8])(void) = {NULL};                   /* vsync callbacks */
volatile s32 vblankCounter = 0;                            /* vblank counter (VSync reads it) */
volatile s32 *rcnt1ModeReg = (volatile s32 *)0x1F801114; /* -> root counter 1 mode */
void trapIntrVSync(void);
void memclr_intr_vb(s32 *p, s32 n);
void *setIntrVSync(s32 idx, void (*func)(void));

void *startIntrVSync(void) {
    *rcnt1ModeReg = 0x107;
    vblankCounter = 0;
    memclr_intr_vb((s32 *)vsyncCallbacks, 8);
    InterruptCallback(0, trapIntrVSync);
    return setIntrVSync;
}

void trapIntrVSync(void) {
    s32 i;

    ++vblankCounter;
    for (i = 0; i < 8; i++) {
        if (vsyncCallbacks[i] != NULL) {
            vsyncCallbacks[i]();
        }
    }
}

void *setIntrVSync(s32 idx, void (*func)(void)) {
    void (*old)(void) = vsyncCallbacks[idx];

    if (func != old) {
        vsyncCallbacks[idx] = func;
    }
    return old;
}

void memclr_intr_vb(s32 *p, s32 n) {
    s32 i;
    s32 unused[1];

    for (i = n - 1; i != -1; i--) {
        *p++ = 0;
    }
}
