/*
 * PSY-Q 4.0 LIBCD.LIB C_009.OBJ: StGetNext. Our own C (clean-room, from the
 * disassembly).
 */
#include "common.h"

extern u32* stRingBase;
extern u32 stRingSize;
extern s32 stRingReadIndex;
extern s32 stEndFrame;

u32 StGetNext(u32** addr, u32** header) {
    volatile u16* slot;

    slot = (volatile u16*)(stRingBase + stRingReadIndex * 8);
    if (*slot == 1) {
        stRingReadIndex = 0;
        if (stEndFrame != 0) {
            *slot = 0;
        }
        slot = (volatile u16*)(stRingBase + stRingReadIndex * 8);
    }
    if (*slot != 2) {
        return 1;
    }
    *slot = 4;
    *addr = stRingBase + stRingSize * 8 + stRingReadIndex * 504;
    *header = (u32*)slot;
    return 0;
}
