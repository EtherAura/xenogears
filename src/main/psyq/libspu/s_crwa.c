/*
 * PSY-Q 4.0 LIBSPU.LIB S_CRWA.OBJ: SpuClearReverbWorkArea. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 * WaitEvent = WaitEvent (BIOS B0:0Ah stub, 0x8004E564-0x8004E574,
 * assembly). The saved transfer callback is a volatile local (retail keeps
 * it at 0x10($sp)).
 */
#include "common.h"
#include "psyq/libspu.h"

extern s32 D_8005899C;
extern u8 spuZeroBuffer[];
extern s32 D_80058E24;
extern s32 D_80058E30;
extern volatile SpuCallbackFunc D_80058E40;
extern s32 D_80058E70[];
s32 _SpuIsInAllocateArea_(u32 addr);
s32 _spu_t(s32 mode, ...);
void WaitEvent(s32 event);

s32 SpuClearReverbWorkArea(s32 mode) {
    u32 size;
    u32 addr;
    u32 n;
    s32 last;
    s32 transMode;
    s32 restore;
    volatile SpuCallbackFunc cb;

    cb = NULL;
    restore = 0;
    if ((u32)mode >= 10 || _SpuIsInAllocateArea_(D_80058E70[mode])) {
        return -1;
    }
    if (mode == 0) {
        size = 0x10 << D_80058E30;
        addr = 0xFFF0 << D_80058E30;
    } else {
        size = (0x10000 - D_80058E70[mode]) << D_80058E30;
        addr = D_80058E70[mode] << D_80058E30;
    }
    transMode = D_80058E24;
    if (transMode == 1) {
        D_80058E24 = 0;
        restore = 1;
    }
    last = 1;
    if (D_80058E40 != NULL) {
        cb = D_80058E40;
        D_80058E40 = NULL;
    }
    do {
        if (size > 0x400) {
            n = 0x400;
        } else {
            n = size;
            last = 0;
        }
        _spu_t(2, addr);
        _spu_t(1);
        _spu_t(3, spuZeroBuffer, n);
        WaitEvent(D_8005899C);
        size -= 0x400;
        addr += 0x400;
    } while (last);
    if (restore) {
        D_80058E24 = transMode;
    }
    if (cb != NULL) {
        D_80058E40 = cb;
    }
    return 0;
}
