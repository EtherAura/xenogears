/*
 * PSY-Q 4.0 LIBCD.LIB C_003.OBJ: StUnSetRing. Our own C (clean-room, from
 * the disassembly).
 */
#include "common.h"

extern s32 D_800564CC;
extern volatile u8* stUnSetRingCdIndexReg;
extern volatile u8* stUnSetRingCdReg3;
void EnterCriticalSection(void); /* EnterCriticalSection */
void ExitCriticalSection(void); /* ExitCriticalSection */
void* DsDataCallback(void (*func)());
void* DsReadyCallback(void (*func)());
void* CdDataCallback(void (*func)());
void* CdReadyCallback(void (*func)());

void StUnSetRing(void) {
    EnterCriticalSection();
    if (D_800564CC == 1) {
        DsDataCallback(0);
        DsReadyCallback(0);
    } else {
        CdDataCallback(0);
        CdReadyCallback(0);
    }
    *stUnSetRingCdIndexReg = 0;
    *stUnSetRingCdReg3 = 0;
    ExitCriticalSection();
}
