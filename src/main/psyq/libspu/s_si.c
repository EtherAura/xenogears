/*
 * PSY-Q 4.0 LIBSPU.LIB S_SI.OBJ: SpuSetIRQ. Our own C for the library
 * routine(s) (clean-room, from the disassembly).
 * .rodata (3 strings) = retail 0x800194B4-0x800194E4 (checked with
 * --rodata). printfStub is printf (defined in kernel.c).
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuRegs* _spu_RXX;
extern void printfStub();

s32 SpuSetIRQ(s32 on_off) {
    u32 i;

    if (on_off == 0 || on_off == 3) {
        _spu_RXX->m1AA_control &= ~0x40;
        i = 0;
        while (_spu_RXX->m1AA_control & 0x40) {
            if (++i > 0xF00) {
                printfStub("SPU:T/O [%s]\n", "wait (IRQ/ON)");
                return -1;
            }
        }
    }
    if (on_off == 1 || on_off == 3) {
        _spu_RXX->m1AA_control |= 0x40;
        i = 0;
        while (!(_spu_RXX->m1AA_control & 0x40)) {
            if (++i > 0xF00) {
                printfStub("SPU:T/O [%s]\n", "wait (IRQ/OFF)");
                return -1;
            }
        }
    }
    return on_off;
}
