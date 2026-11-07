/*
 * PSY-Q 4.0 LIBSPU.LIB S_M_INIT.OBJ: SpuInitMalloc. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern s32 D_80058E30;
/* S_M_INIT.OBJ .data (0x80058E64-0x80058E70) */
s32 D_80058E64 = 0;
s32 D_80058E68 = 0;
SpuMallocRec* D_80058E6C = NULL;

s32 SpuInitMalloc(s32 num, SpuMallocRec* top) {
    if (num > 0) {
        top->m0_addr = 0x40001010;
        D_80058E6C = top;
        D_80058E68 = 0;
        D_80058E64 = num;
        top->m4_size = (0x10000 << D_80058E30) - 0x1010;
        return num;
    }
    return 0;
}
