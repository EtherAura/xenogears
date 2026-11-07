/*
 * PSY-Q 4.0 LIBPRESS.LIB LIBPRESS.OBJ: DecDCT* and the MDEC register layer.
 * Our own C for the library routines (clean-room, from the disassembly).
 */
#include "common.h"

/* Quantisation/IDCT tables uploaded to the MDEC (.data 0x801D76E0-):
 * each table is a command word followed by its payload. */
extern u32 mdecIqCommand;       /* iq table command word */
extern u32 mdecIqY[16];   /* iq_y */
extern u32 mdecIqC[16];   /* iq_c */
extern u32 mdecDctCommand;       /* IDCT table command word */
extern u32 mdecDct[32];   /* dct */

/* Hardware register pointers (.data). */
extern volatile u32* mdecInMadr; /* DMA0 (MDEC in) MADR */
extern volatile u32* mdecInBcr; /* DMA0 BCR */
extern volatile u32* mdecInChcr; /* DMA0 CHCR */
extern volatile u32* mdecOutMadr; /* DMA1 (MDEC out) MADR */
extern volatile u32* mdecOutBcr; /* DMA1 BCR */
extern volatile u32* mdecOutChcr; /* DMA1 CHCR */
extern volatile u32* mdecCommandReg; /* MDEC0: command/parameter */
extern volatile u32* mdecControlReg; /* MDEC1: control/status */
extern volatile u32* mdecDpcr; /* DPCR */

typedef struct {
    u8 iq_y[64];
    u8 iq_c[64];
    s16 dct[64];
} DECDCTENV;

void ResetCallback(void);
void* DMACallback(s32 dma, void (*func)());
s32 printfStub(const char* fmt, ...); /* printf */

void MDEC_reset(s32 mode);
void MDEC_in(u32* addr, s32 size);
void MDEC_out(u32* addr, s32 size);
s32 MDEC_in_sync(void);
s32 MDEC_out_sync(void);
u32 MDEC_status(void);
s32 timeout(char* name);

void DecDCTReset(s32 mode) {
    if (mode == 0) {
        ResetCallback();
    }
    MDEC_reset(mode);
}

DECDCTENV* DecDCTGetEnv(DECDCTENV* env) {
    u32* src;
    u32* dst;
    s32 n;

    dst = (u32*)env->iq_y;
    src = mdecIqY;
    for (n = 15; n != -1; n--) {
        *dst++ = *src++;
    }
    dst = (u32*)env->iq_c;
    src = mdecIqC;
    for (n = 15; n != -1; n--) {
        *dst++ = *src++;
    }
    dst = (u32*)env->dct;
    src = mdecDct;
    for (n = 31; n != -1; n--) {
        *dst++ = *src++;
    }
    return env;
}

DECDCTENV* DecDCTPutEnv(DECDCTENV* env) {
    u32* src;
    u32* dst;
    s32 n;

    dst = mdecIqY;
    src = (u32*)env->iq_y;
    for (n = 15; n != -1; n--) {
        *dst++ = *src++;
    }
    dst = mdecIqC;
    src = (u32*)env->iq_c;
    for (n = 15; n != -1; n--) {
        *dst++ = *src++;
    }
    MDEC_in(&mdecIqCommand, 32);
    MDEC_in(&mdecDctCommand, 32);
    return env;
}

s32 DecDCTBufSize(u32* bs) {
    return *(u16*)bs;
}

void DecDCTin(u32* buf, s32 mode) {
    if (mode & 1) {
        *buf &= ~0x08000000;
    } else {
        *buf |= 0x08000000;
    }
    if (mode & 2) {
        *buf |= 0x02000000;
    } else {
        *buf &= ~0x02000000;
    }
    MDEC_in(buf, *(u16*)buf);
}

void DecDCTout(u32* buf, s32 size) {
    MDEC_out(buf, size);
}

s32 DecDCTinSync(s32 mode) {
    if (mode) {
        return (MDEC_status() >> 29) & 1;
    }
    return MDEC_in_sync();
}

s32 DecDCToutSync(s32 mode) {
    if (mode) {
        return (MDEC_status() >> 24) & 1;
    }
    return MDEC_out_sync();
}

void* DecDCTinCallback(void (*func)()) {
    return DMACallback(0, func);
}

void* DecDCToutCallback(void (*func)()) {
    return DMACallback(1, func);
}

void MDEC_reset(s32 mode) {
    switch (mode) {
    case 0:
        *mdecControlReg = 0x80000000;
        *mdecInChcr = 0;
        *mdecOutChcr = 0;
        *mdecControlReg = 0x60000000;
        MDEC_in(&mdecIqCommand, 32);
        MDEC_in(&mdecDctCommand, 32);
        break;
    case 1:
        *mdecControlReg = 0x80000000;
        *mdecInChcr = 0;
        *mdecOutChcr = 0;
        *mdecOutChcr;
        *mdecControlReg = 0x60000000;
        break;
    default:
        printfStub("MDEC_rest:bad option(%d)\n", mode);
        break;
    }
}

void MDEC_in(u32* addr, s32 size) {
    MDEC_in_sync();
    *mdecDpcr |= 0x88;
    *mdecInMadr = (u32)(addr + 1);
    *mdecInBcr = (((u32)size >> 5) << 16) | 0x20;
    *mdecCommandReg = *addr;
    *mdecInChcr = 0x1000201;
}

void MDEC_out(u32* addr, s32 size) {
    MDEC_out_sync();
    *mdecDpcr |= 0x88;
    *mdecOutChcr = 0;
    *mdecOutMadr = (u32)addr;
    *mdecOutBcr = (((u32)size >> 5) << 16) | 0x20;
    *mdecOutChcr = 0x1000200;
}

s32 MDEC_in_sync(void) {
    volatile s32 count = 0x100000;

    while (*mdecControlReg & 0x20000000) {
        if (--count == -1) {
            timeout("MDEC_in_sync");
            return -1;
        }
    }
    return 0;
}

s32 MDEC_out_sync(void) {
    volatile s32 count = 0x100000;

    while (*mdecOutChcr & 0x1000000) {
        if (--count == -1) {
            timeout("MDEC_out_sync");
            return -1;
        }
    }
    return 0;
}

u32 MDEC_status(void) {
    return *mdecControlReg;
}

static s32 mdec_print_status(void) {
    u32 status;

    status = *mdecControlReg;
    printfStub("\t DMA=(%d,%d), ADDR=(0x%08x->0x%08x)\n", (*mdecInChcr >> 24) & 1, (*mdecOutChcr >> 24) & 1,
                  *mdecInMadr, *mdecOutMadr);
    return printfStub("\t FIFO=(%d,%d),BUSY=%d,DREQ=(%d,%d),RGB24=%d,STP=%d\n", (~status >> 31) & 1,
                  (status >> 30) & 1, (status >> 29) & 1, (status >> 28) & 1, (status >> 27) & 1,
                  (status >> 25) & 1, (status >> 23) & 1);
}

s32 timeout(char* name) {
    printfStub("%s timeout:\n", name);
    mdec_print_status();
    MDEC_reset(1);
    return 0;
}

