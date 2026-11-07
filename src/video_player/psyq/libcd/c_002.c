/*
 * PSY-Q 4.0 LIBCD.LIB C_002.OBJ: StClearRing. Our own C (clean-room, from
 * the disassembly).
 */
#include "common.h"

/* LIBCD streaming state (.bss, shared by the St* objects). */
extern u32* stRingBase; /* ring buffer (32-byte sector slots) */
extern u32 stRingSize;  /* ring size in sectors */
extern s32 stFrameNumber;
extern s16 stSectorIndex;
extern s32 stDeferredInterrupt;
extern s32 stFrameDmaPending;
extern s32 stRingWriteIndex;
extern s32 stRingFrameStart;
extern s32 stRingReadIndex;

void init_ring_status(u32 start, u32 n);

void StClearRing(void) {
    stRingReadIndex = 0;
    stRingFrameStart = 0;
    stRingWriteIndex = 0;
    stFrameDmaPending = 0;
    init_ring_status(0, stRingSize);
    stDeferredInterrupt = 0;
    stSectorIndex = 0;
    stFrameNumber = 0;
}
