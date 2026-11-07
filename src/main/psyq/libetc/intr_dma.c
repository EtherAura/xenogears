/*
 * PSY-Q 4.0 LIBETC.LIB INTR_DMA.OBJ: the DMA interrupt. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

/* ---- LIBETC INTR_DMA.OBJ ---- */

typedef struct {
    /* 0x0 */ u32 m0_madr;
    /* 0x4 */ u32 m4_bcr;
    /* 0x8 */ u32 m8_chcr;
    /* 0xC */ u32 mC;
} DmaChannel;

/* declarations from other objects */
void printfStub(const char *fmt, ...);   /* printf (stubbed) */
void *setIntrDMA(s32 ch, void (*func)(void));
void *InterruptCallback(s32 irq, void (*func)(void));
/* INTR_DMA.OBJ .data (0x80058968-0x80058990) */
volatile u32 *dmaDicrReg = (volatile u32 *)0x1F8010F4;               /* -> DICR */
void (*dmaCallbacks[8])(void) = {NULL};                                 /* DMA completion callbacks */
volatile DmaChannel *dmaChannelRegs = (volatile DmaChannel *)0x1F801080; /* -> DMA channel registers */
void trapIntrDMA(void);
void memclr_intr_dma(s32 *p, s32 n);

void *startIntrDMA(void) {
    memclr_intr_dma((s32 *)dmaCallbacks, 8);
    *dmaDicrReg = 0;
    InterruptCallback(3, trapIntrDMA);
    return setIntrDMA;
}

void trapIntrDMA(void) {
    u32 mask;
    s32 i;

    while ((mask = (*dmaDicrReg >> 24) & 0x7F) != 0) {
        for (i = 0; mask != 0 && i < 7; i++, mask >>= 1) {
            if (mask & 1) {
                *dmaDicrReg &= (1 << (i + 24)) | 0xFFFFFF;
                if (dmaCallbacks[i] != NULL) {
                    dmaCallbacks[i]();
                }
            }
        }
    }
    if ((*dmaDicrReg & 0xFF000000) == 0x80000000 || (*dmaDicrReg & 0x8000)) {
        printfStub("DMA bus error: code=%08x\n", *dmaDicrReg);
        for (i = 0; i < 7; i++) {
            printfStub("MADR[%d]=%08x\n", i, dmaChannelRegs[i].m0_madr);
        }
    }
}

void *setIntrDMA(s32 ch, void (*func)(void)) {
    void (*old)(void) = dmaCallbacks[ch];

    if (func != old) {
        if (func != NULL) {
            dmaCallbacks[ch] = func;
            *dmaDicrReg = (*dmaDicrReg & 0xFFFFFF) | 0x800000 | (1 << (ch + 16));
        } else {
            dmaCallbacks[ch] = NULL;
            *dmaDicrReg = ((*dmaDicrReg & 0xFFFFFF) | 0x800000) & ~(1 << (ch + 16));
        }
    }
    return old;
}

void memclr_intr_dma(s32 *p, s32 n) {
    s32 i;
    s32 unused[1];

    for (i = n - 1; i != -1; i--) {
        *p++ = 0;
    }
}
