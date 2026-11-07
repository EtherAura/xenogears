/*
 * PSY-Q 4.0 LIBCD.LIB C_005.OBJ: StSetStream. Our own C (clean-room, from
 * the disassembly).
 */
#include "common.h"

extern s32 stFrameNumber;
extern s16 stSectorIndex;
extern s32 stRgb24;
extern void (*stFrameReadyCallback)(void);
extern void (*stEndFrameCallback)(void);
extern s32 D_801E89D8;
extern s32 D_801E89E4;
extern s32 stEmulationBuffer;
void StSetMask(s32 a, s32 b, s32 c);

void StSetStream(u32 mode, u32 start_frame, u32 end_frame, void (*func1)(void), void (*func2)(void)) {
    StSetMask(1, start_frame, end_frame);
    stEmulationBuffer = 0;
    stFrameReadyCallback = func1;
    stRgb24 = mode & 1;
    D_801E89E4 = 0;
    D_801E89D8 = 0;
    stSectorIndex = 0;
    stFrameNumber = 0;
    stEndFrameCallback = func2;
}
