/*
 * PSY-Q 4.0 LIBCD.LIB C_004.OBJ: data_ready_callback, StGetBackloc. Our own
 * C (clean-room, from the disassembly).
 */
#include "common.h"

typedef struct {
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
} CdlLOC;

/* One 32-byte slot of the ring header area (LIBCD StHEADER layout); the
 * library keeps the slot state in m0 (1: reading, 2: ready, 4: in use). */
typedef struct {
    /* 0x00 */ s16 m0_status;
    /* 0x02 */ u16 m2_type;
    /* 0x04 */ u16 m4_secCount;
    /* 0x06 */ s16 m6_nSectors;
    /* 0x08 */ u32 m8_frameCount;
    /* 0x0C */ u32 mC_frameSize;
    /* 0x10 */ u16 m10_width;
    /* 0x12 */ u16 m12_height;
    /* 0x14 */ u32 m14;
    /* 0x18 */ u32 m18;
    /* 0x1C */ CdlLOC m1C_loc;
} StSlot; /* 0x20 */

extern StSlot* stRingBase;
extern CdlLOC stLastLoc;
extern volatile u32 stLastFrameNumber;
extern void (*stFrameReadyCallback)(void);
extern s32 stFrameDmaPending;
extern s32 stRingWriteIndex;
extern volatile s32 stRingFrameStart;
extern s32 D_8005A470;
CdlLOC* CdIntToPos(s32 i, CdlLOC* p);
s32 CdPosToInt(CdlLOC* p);

void data_ready_callback(void) {
    StSlot* slot;

    slot = &stRingBase[stRingFrameStart];
    slot->m0_status = 2;
    stLastLoc = slot->m1C_loc;
    stLastFrameNumber = slot->m8_frameCount;
    stRingFrameStart = stRingWriteIndex;
    if (stFrameReadyCallback != 0) {
        stFrameReadyCallback();
    }
    stFrameDmaPending = 0;
}

s32 StGetBackloc(CdlLOC* loc) {
    if (D_8005A470 != 0) {
        return -1;
    }
    CdIntToPos(CdPosToInt(&stLastLoc) + 1, loc);
    return stLastFrameNumber;
}
