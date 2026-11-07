/*
 * PSY-Q 4.0 LIBSPU.LIB S_STM.OBJ: SpuSetTransferMode. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern s32 D_800589A4;
extern s32 D_80058E24;

s32 SpuSetTransferMode(s32 mode) {
    s32 m;

    switch (mode) {
    case 0:
        m = 0;
        break;
    case 1:
        m = 1;
        break;
    default:
        m = 0;
        break;
    }
    D_800589A4 = mode;
    D_80058E24 = m;
    return m;
}
