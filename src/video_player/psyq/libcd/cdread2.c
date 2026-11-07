/*
 * PSY-Q 4.0 LIBCD.LIB CDREAD2.OBJ: CdRead2, StCdInterrupt2. Our own C
 * (clean-room, from the disassembly).
 */
#include "common.h"

extern s32 D_8005A470;
s32 CdControl(u8 com, u8* param, u8* result);
void* CdDataCallback(void (*func)());
void* CdReadyCallback(void (*func)());
void data_ready_callback(void);
void StCdInterrupt(void);
void StCdInterrupt2(void);

s32 CdRead2(s32 mode) {
    u8 param[4];

    param[0] = mode;
    CdControl(0xE, param, 0);
    if (mode & 0x100) {
        if (mode & 0x20) {
            D_8005A470 = 0;
        } else {
            D_8005A470 = 1;
        }
        CdDataCallback(data_ready_callback);
        CdReadyCallback(StCdInterrupt2);
    }
    return CdControl(0x1B, 0, 0);
}

void StCdInterrupt2(void) {
    StCdInterrupt();
}
