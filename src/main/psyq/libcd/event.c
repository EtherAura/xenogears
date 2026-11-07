/*
 * PSY-Q 4.0 LIBCD.LIB EVENT.OBJ: CdInit and the event callbacks. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

typedef void (*CdlCB)(u8, u8*);

extern s32 printfStub(const char* fmt, ...);      /* printf (stubbed) */
extern void DeliverEvent(u32 ev, u32 spec);         /* DeliverEvent */

s32 CdReset(s32 mode);
CdlCB CdSyncCallback(CdlCB func);
CdlCB CdReadyCallback(CdlCB func);
CdlCB CdReadCallback(CdlCB func);
s32 CdReadMode(s32 mode);
void def_cbsync(u8 intr, u8* result);
void def_cbready(u8 intr, u8* result);
void def_cbread(u8 intr, u8* result);

s32 CdInit(void) {
    s32 i = 4;

retry:
    if (CdReset(1) != 1) {
        if (--i != -1) {
            goto retry;
        }
        printfStub("CdInit: Init failed\n");
        return 0;
    }
    CdSyncCallback(def_cbsync);
    CdReadyCallback(def_cbready);
    CdReadCallback(def_cbread);
    CdReadMode(0);
    return 1;
}

/* HwCdRom = 0xF0000003, EvSpCOMP = 0x20, EvSpDR = 0x40 */
void def_cbsync(u8 intr, u8* result) {
    DeliverEvent(0xF0000003, 0x20);
}

void def_cbready(u8 intr, u8* result) {
    DeliverEvent(0xF0000003, 0x40);
}

void def_cbread(u8 intr, u8* result) {
    DeliverEvent(0xF0000003, 0x40);
}
