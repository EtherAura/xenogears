/*
 * PSY-Q 4.0 LIBSPU.LIB S_R.OBJ: SpuRead. Our own C for the library
 * routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern s32 D_80058E3C;
extern volatile SpuCallbackFunc D_80058E40;
void _spu_Fr(void* addr, u32 size);

u32 SpuRead(u8* addr, u32 size) {
    if (size > 0x7EFF0) {
        size = 0x7EFF0;
    }
    _spu_Fr(addr, size);
    if (D_80058E40 == NULL) {
        D_80058E3C = 0;
    }
    return size;
}
