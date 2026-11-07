/*
 * PSY-Q 4.0 LIBSPU.LIB S_M_UTIL.OBJ: _SpuIsInAllocateArea,
 * _SpuIsInAllocateArea_. Our own C for the library routine(s) (clean-room,
 * from the disassembly).
 * The loop must index the table (strength reduction creates the walking
 * pointer after the hoisted mask constants); a pointer walk puts the copy
 * first.
 */
#include "common.h"
#include "psyq/libspu.h"

extern s32 D_80058E30;
extern SpuMallocRec* D_80058E6C;

s32 _SpuIsInAllocateArea(u32 addr) {
    s32 i;
    u32 start;

    if (D_80058E6C == NULL) {
        return 0;
    }
    i = 0;
    while (1) {
        if (!(D_80058E6C[i].m0_addr & 0x80000000)) {
            if (D_80058E6C[i].m0_addr & 0x40000000) {
                break;
            }
            start = D_80058E6C[i].m0_addr & 0x0FFFFFFF;
            if (start >= addr) {
                return 1;
            }
            if (addr < start + D_80058E6C[i].m4_size) {
                return 1;
            }
        }
        i++;
    }
    return 0;
}

s32 _SpuIsInAllocateArea_(u32 addr) {
    s32 i;
    u32 start;

    addr <<= D_80058E30;
    if (D_80058E6C == NULL) {
        return 0;
    }
    i = 0;
    while (1) {
        if (!(D_80058E6C[i].m0_addr & 0x80000000)) {
            if (D_80058E6C[i].m0_addr & 0x40000000) {
                break;
            }
            start = D_80058E6C[i].m0_addr & 0x0FFFFFFF;
            if (start >= addr) {
                return 1;
            }
            if (addr < start + D_80058E6C[i].m4_size) {
                return 1;
            }
        }
        i++;
    }
    return 0;
}
