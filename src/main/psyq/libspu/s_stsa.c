/*
 * PSY-Q 4.0 LIBSPU.LIB S_STSA.OBJ: SpuSetTransferStartAddr. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern u16 D_80058E20;
extern s32 D_80058E30;
u32 _spu_FsetRXXa(s32 reg, u32 addr);

u32 SpuSetTransferStartAddr(u32 addr) {
    if (addr < 0x1010 || addr > 0x7FFF8) {
        return 0;
    }
    D_80058E20 = _spu_FsetRXXa(-1, addr);
    return D_80058E20 << D_80058E30;
}
