/*
 * PSY-Q 4.0 LIBCD.LIB CDROM.OBJ (stream part): StSetRing. Our own C
 * (clean-room, from the disassembly).
 */
#include "common.h"

/* LIBCD streaming state (.bss, shared by the St* objects). */
extern u32* stRingBase; /* ring buffer (32-byte sector slots) */
extern u32 stRingSize;  /* ring size in sectors */

void StClearRing(void);

void StSetRing(u32* ring_addr, u32 ring_size) {
    stRingBase = ring_addr;
    stRingSize = ring_size;
    StClearRing();
}
