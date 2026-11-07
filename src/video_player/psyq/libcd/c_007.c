/*
 * PSY-Q 4.0 LIBCD.LIB C_007.OBJ: StFreeRing. Our own C (clean-room, from
 * the disassembly).
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

extern u32* stRingBase;
extern u32 stRingSize;
extern s32 stRingReadIndex;

u32 StFreeRing(u32* base) {
    s32 unused[1];
    s32 index;
    s32 i;
    s16 n;
    StSlot* slot;
    s16* p;

    index = (base - (stRingBase + stRingSize * 8)) / 504;
    slot = (StSlot*)(stRingBase + index * 8);
    n = slot->m6_nSectors;
    if (slot->m0_status != 4) {
        return 1;
    }
    for (i = 0; i < n; i++) {
        p = (s16*)(stRingBase + (i + index) * 8);
        *p = 0;
    }
    stRingReadIndex = i + index;
    return 0;
}
