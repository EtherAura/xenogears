/*
 * PSY-Q 4.0 LIBCD.LIB C_011.OBJ: StCdInterrupt, mem2mem, dma_execute. Our own
 * C (clean-room, from the disassembly).
 */
#include "common.h"

typedef struct {
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
} CdlLOC;

/* One 32-byte slot of the ring header area (as in c_004.c). */
typedef struct {
    /* 0x00 */ s16 m0_status;
    /* 0x02 */ u16 m2_type;
    /* 0x04 */ u16 m4_secCount;
    /* 0x06 */ u16 m6_nSectors;
    /* 0x08 */ u32 m8_frameCount;
    /* 0x0C */ u32 mC_frameSize;
    /* 0x10 */ u16 m10_width;
    /* 0x12 */ u16 m12_height;
    /* 0x14 */ u32 m14;
    /* 0x18 */ u32 m18;
    /* 0x1C */ CdlLOC m1C_loc;
} StSlot; /* 0x20 */


extern volatile u8* stCdIndexReg;  /* CD status register */
extern volatile u32* stDmaDpcr; /* DPCR */
extern volatile u32* stDmaDicr; /* DICR */
void printfStub(const char* fmt, ...); /* printf */

extern volatile u8* stCdDataReg;  /* CD data register */
extern volatile u8* stCdReg3;  /* CD register 3 */
extern volatile u32* stCdDelaySizeReg; /* CD delay/size register */
extern volatile u32* stComDelayReg; /* common delay register */
extern volatile u32* stMdecOutChcr; /* 0x1F801098: DMA1 (MDEC out) CHCR */
extern volatile u32* stCdDmaChcr; /* DMA3 CHCR */
extern s32 stInterruptResult;           /* last interrupt result code */
extern volatile u16* stCurrentSlot; /* current ring slot (read as halfwords) */
extern u32 stFrameNumber;
extern s16 stSectorIndex;
extern s32 stRgb24;
extern void (*stEndFrameCallback)(void);
extern s32 stDeferredInterrupt;
extern s32 D_801E89D8;
extern s32 stStartFrame;
extern s32 D_801E89E4;
extern s32 stFrameDmaPending;
extern s32 stEmulationSector;
extern s32 stRingWriteIndex;
extern s32 stRingFrameStart;
extern u8* stEmulationBuffer;
extern u32 stEndFrame;
extern s32 stSeekingStart;
extern u32* stSectorDest;
extern StSlot* stRingBase;
extern u32 stRingSize;
extern s32 D_8005A470;
s32 CdReady(s32 mode, u8* result);
void data_ready_callback(void);
void init_ring_status(u32 start, u32 n);
void mem2mem(u32* dst, u32* src, u32 n, s32 last);
void dma_execute(s32 ch, u32 madr, u32 blocks, u32 size, u32 chcr, u8 enable, s32 unused);

void StCdInterrupt(void) {
    volatile u16 unused;
    volatile u16 r0;
    volatile u16 r1;
    CdlLOC loc;
    u8 result[8];
    u8* p;
    u32 i;
    u32 chcr;

    if (stFrameDmaPending == 1) {
        return;
    }
    if (stRgb24 != 0 && (*stMdecOutChcr & 0x1000000)) {
        stDeferredInterrupt = 1;
        if (stEmulationBuffer != 0) {
            stEmulationSector++;
        }
        stInterruptResult = 1;
        return;
    }
    if (CdReady(1, result) == 5) {
        return;
    }
    r0 = result[0];
    r1 = result[1];
    if (r0 & 4) {
        stInterruptResult = 3;
        return;
    }
    stCurrentSlot = (u16*)&stRingBase[stRingWriteIndex];
    if (stCurrentSlot[0] != 0) {
        if (stEmulationBuffer != 0) {
            stEmulationSector++;
        }
        stInterruptResult = 4;
        return;
    }
    *stCdIndexReg = 0;
    *stCdReg3 = 0;
    *stCdIndexReg = 0;
    *stCdReg3 = 0x80;
    *stCdDelaySizeReg = 0x20943;
    *stComDelayReg = 0x1323;
    if (D_8005A470 == 0) {
        p = (u8*)&loc;
        do {
            *p++ = *stCdDataReg;
        } while (p < (u8*)&loc + 4);
        for (i = 0; i < 8; i++) {
            *stCdDataReg;
        }
    }
    chcr = 0x11000000;
    if (stEmulationBuffer != 0) {
        mem2mem((u32*)stCurrentSlot, (u32*)(stEmulationBuffer + (stEmulationSector << 11)), 8, 0);
    } else {
        dma_execute(3, (u32)stCurrentSlot, 0, 8, chcr, 0, 0);
    }
    while (*stCdDmaChcr & 0x1000000) {
    }
    *(CdlLOC*)(stCurrentSlot + 14) = loc;
    *stCdDelaySizeReg = 0x20843;
    *stComDelayReg = 0x1325;
    if (stSeekingStart == 1 && stStartFrame != 0) {
        if (stStartFrame != stCurrentSlot[4]) {
            stCurrentSlot[0] = 0;
            if (stEmulationBuffer != 0) {
                stEmulationSector++;
            }
            return;
        }
        stSeekingStart = 0;
    }
    if ((stCurrentSlot[0] & 0xFFFF) != 0x160 || ((stCurrentSlot[1] >> 10) & 0x1F) != D_801E89E4) {
        if (stEmulationBuffer != 0) {
            stEmulationSector = 0;
        } else {
            stCurrentSlot[0];
        }
        stInterruptResult = 5;
        stCurrentSlot[0] = 0;
        return;
    }
    if (stSectorIndex != stCurrentSlot[2] || (stFrameNumber != 0 && stFrameNumber != stCurrentSlot[4])) {
        stFrameNumber = 0;
        stSectorIndex = 0;
        init_ring_status(stRingFrameStart, stRingWriteIndex - stRingFrameStart);
        stRingWriteIndex = stRingFrameStart;
        stCurrentSlot[0] = 0;
        if (stEmulationBuffer != 0) {
            stEmulationSector++;
        }
        stInterruptResult = 6;
        return;
    }
    if (stCurrentSlot[2] == 0) {
        stSectorIndex = 0;
        stFrameNumber = stCurrentSlot[4] & 0xFFFF;
        if (stEndFrame != 0 && stFrameNumber >= stEndFrame) {
            stFrameNumber = 0;
            stSectorIndex = 0;
            init_ring_status(stRingFrameStart, stRingWriteIndex - stRingFrameStart);
            stRingWriteIndex = stRingFrameStart;
            stCurrentSlot[0] = 0;
            stSeekingStart = 1;
            if (stEndFrameCallback != 0) {
                stEndFrameCallback();
            }
            if (stEmulationBuffer != 0) {
                stEmulationSector++;
            }
            stInterruptResult = 7;
            return;
        }
        if (stRingSize - stRingWriteIndex - 1 < stCurrentSlot[3]) {
            if (stEndFrame == 0) {
                stCurrentSlot[0] = 1;
                stSeekingStart = 1;
                if (stEndFrameCallback != 0) {
                    stEndFrameCallback();
                }
                if (stEmulationBuffer != 0) {
                    stEmulationSector++;
                }
                stInterruptResult = 8;
                return;
            }
            if (stRingBase->m0_status != 0) {
                stCurrentSlot[0] = 0;
                if (stEmulationBuffer != 0) {
                    stEmulationSector++;
                }
                stInterruptResult = 9;
                return;
            }
            stCurrentSlot[0] = 1;
            {
                u32* dst = (u32*)stRingBase;
                u32* src = (u32*)stCurrentSlot;
                stRingWriteIndex = 0;
                for (i = 0; i < 8; i++) {
                    *dst++ = *src++;
                }
            }
            stCurrentSlot = (u16*)stRingBase;
        }
        stRingFrameStart = stRingWriteIndex;
    }
    stInterruptResult = 10;
    stSectorIndex++;
    stSectorDest = (u32*)&stRingBase[stRingSize] + stRingWriteIndex * 0x1F8;
    chcr = 0x11000000;
    if (stRgb24 != 0) {
        *stCdDelaySizeReg = 0x20943;
        *stComDelayReg = 0x1323;
    } else {
        *stCdDelaySizeReg = 0x21020843;
        chcr = 0x11400100;
    }
    if (stCurrentSlot[3] - 1 == stCurrentSlot[2]) {
        stFrameDmaPending = 1;
        if (stEmulationBuffer != 0) {
            mem2mem(stSectorDest, (u32*)(stEmulationBuffer + (stEmulationSector << 11) + 0x20), 0x1F8, 1);
            stEmulationSector++;
        } else {
            dma_execute(3, (u32)stSectorDest, 0, 0x1F8, chcr, 1, 0);
        }
        stSectorIndex = 0;
        stFrameNumber = 0;
        D_801E89E4 = D_801E89D8;
    } else if (stEmulationBuffer != 0) {
        mem2mem(stSectorDest, (u32*)(stEmulationBuffer + (stEmulationSector << 11) + 0x20), 0x1F8, 0);
        stEmulationSector++;
    } else {
        dma_execute(3, (u32)stSectorDest, 0, 0x1F8, chcr, 0, 0);
    }
    *stComDelayReg = 0x1325;
    stCurrentSlot[0] = 3;
    stRingWriteIndex++;
    if (stEmulationBuffer != 0 && stFrameDmaPending != 0) {
        data_ready_callback();
    }
}

void mem2mem(u32* dst, u32* src, u32 n, s32 last) {
    s32 unused[1];
    u32 i;

    for (i = 0; i < n; i++) {
        *dst++ = *src++;
    }
}

void dma_execute(s32 ch, u32 madr, u32 blocks, u32 size, u32 chcr, u8 enable, s32 unused) {
    volatile u32 dummy;
    volatile u32* p;
    s32 i;
    u8* d;
    u8 x;
    u32 t;
    s32 b;

    for (i = 0; *(volatile u32*)(0x1F801088 + (ch << 4)) & 0x1000000; i++) {
        if (i == 0x10000) {
            printfStub("DMA STATUS ERROR %x\n", *(volatile u32*)(0x1F801088 + (ch << 4)));
            break;
        }
    }
    if (enable == 1) {
        d = (u8*)stDmaDicr + 2;
        x = *d;
        x |= 1 << ch;
        *d = x;
        dummy = *stDmaDicr;
    } else {
        d = (u8*)stDmaDicr + 2;
        x = *d;
        x &= ~(1 << ch);
        *d = x;
        dummy = *stDmaDicr;
    }
    t = ch << 2;
    t += 3;
    b = 1;
    b <<= t;
    t = *stDmaDpcr;
    t |= b;
    *stDmaDpcr = t;
    p = (volatile u32*)(0x1F801080 + (ch << 4));
    *p++ = madr;
    *p++ = (blocks << 16) | size;
    while (!(*stCdIndexReg & 0x40)) {
    }
    *p = chcr;
    dummy = *p;
}

