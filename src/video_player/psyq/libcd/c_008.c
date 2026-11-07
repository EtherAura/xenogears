/*
 * PSY-Q 4.0 LIBCD.LIB C_008.OBJ: init_ring_status. Our own C (clean-room,
 * from the disassembly).
 */
#include "common.h"

/* LIBCD streaming state (.bss, shared by the St* objects). */
extern u32* stRingBase; /* ring buffer (32-byte sector slots) */
extern u32 stRingSize;  /* ring size in sectors */

void init_ring_status(u32 start, u32 n) {
    s32 unused[1];
    u32 i;
    u32* p;

    for (i = 0; i < n; i++) {
        p = stRingBase + (i + start) * 8;
        *p = 0;
    }
}
