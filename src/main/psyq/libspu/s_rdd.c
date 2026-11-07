/*
 * PSY-Q 4.0 LIBSPU.LIB S_RDD.OBJ: SpuReadDecodedData. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 * d_data is SpuDecodedData (cd_left/cd_right/voice1/voice3, 0x200 s16 each);
 * flag 5 = SPU_CDONLY, 6 = SPU_VOICEONLY.
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;
void _spu_Fr_(void* addr, u32 spu_addr, u32 size);

s32 SpuReadDecodedData(u8* d_data, s32 flag) {
    switch (flag) {
    case 5:
        _spu_Fr_(d_data, 0, 0x20);
        break;
    case 6:
        _spu_Fr_(d_data + 0x800, 0x100, 0x20);
        break;
    default:
        _spu_Fr_(d_data, 0, 0x40);
        break;
    }
    if (_spu_RXX->m1AE_status & 0x800) {
        return 1;
    }
    return 0;
}
