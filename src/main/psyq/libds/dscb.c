/*
 * PSY-Q 4.0 LIBDS.LIB DSCB.OBJ: CD-DA/XA callbacks. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

extern void* DMACallback(s32 dma, void (*func)(void));

extern void (*GlobalCallback[2])(void); /* DS callbacks: [0] sync, [1] ready */

void (*DsSyncCallback(void (*func)(void)))(void) {
    void (**p)(void) = &GlobalCallback[0];
    void (*old)(void) = *p;
    *p = func;
    return old;
}

void (*DsReadyCallback(void (*func)(void)))(void) {
    void (**p)(void) = &GlobalCallback[1];
    void (*old)(void) = *p;
    *p = func;
    return old;
}

void DsDataCallback(void (*func)(void)) {
    DMACallback(3, func);
}
