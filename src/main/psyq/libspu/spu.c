/*
 * PSY-Q 4.0 LIBSPU.LIB SPU.OBJ (US 0x8004C6DC-0x8004D270, JP 0x8004C584-
 * 0x8004D118): SPU register access, reset and manual/DMA transfer helpers.
 * Our own C (clean-room, from the disassembly).
 * DeliverEvent = DeliverEvent, printfStub = printf (stubbed; declared
 * returning s32: the call then sets v0, which keeps _spu_init's v0 = 0 at
 * the top of its shared tail as in retail).
 */
#include "common.h"
#include "psyq/libspu.h"

#define va_start(ap, last) (ap = (va_list)&(last) + sizeof(last))
#define va_arg(ap, type) (*(type*)((ap += sizeof(type)) - sizeof(type)))
typedef char* va_list;

/* SPU.OBJ .data (0x80058E08-0x80058E64) */
volatile SpuRegs* _spu_RXX = (volatile SpuRegs*)0x1F801C00; /* SPU registers */
volatile u32* spuDmaMadrReg = (volatile u32*)0x1F8010C0;         /* DMA4 MADR */
volatile u32* spuDmaBcrReg = (volatile u32*)0x1F8010C4;         /* DMA4 BCR */
volatile u32* spuDmaChcrReg = (volatile u32*)0x1F8010C8;         /* DMA4 CHCR */
volatile u32* spuDpcrReg = (volatile u32*)0x1F8010F0;         /* DPCR */
volatile u32* spuBusDelayReg = (volatile u32*)0x1F801014;         /* SPU bus delay */
u16 D_80058E20 = 0;
s32 D_80058E24 = 0;
s32 D_80058E28 = 0;
s32 D_80058E2C = 2;
s32 D_80058E30 = 3;
s32 D_80058E34 = 8;
s32 D_80058E38 = 7;
s32 D_80058E3C = 1;
volatile SpuCallbackFunc D_80058E40 = NULL;
volatile SpuCallbackFunc D_80058E44 = NULL;
u16 spuDummyVoiceBlock[8] = {0x0707, 0x0707, 0x0707, 0x0707, 0x0707, 0x0707, 0x0707, 0x0707};
s32 spuDmaReadMode = 0;
u32 spuDmaStartAddr = 0;
u32 spuDmaBlockCount = 0;
extern volatile u16 _spu_RQ[];

extern s32 printfStub(const char* fmt, ...); /* printf (stubbed) */
void DeliverEvent(u32 event, s32 spec);
void _spu_Fw1ts(void);
void _spu_FwriteByIO(void* addr, u32 size);
void _spu_FsetDelayR(void);
void _spu_FsetDelayW(void);
s32 _spu_t(s32 mode, ...);

s32 _spu_init(s32 mode) {
    u32 i;
    s32 j;

    *spuDpcrReg |= 0xB0000;
    D_80058E24 = 0;
    D_80058E28 = 0;
    D_80058E20 = 0;
    _spu_RXX->m180_mainVolL = 0;
    _spu_RXX->m182_mainVolR = 0;
    _spu_RXX->m1AA_control = 0;
    _spu_Fw1ts();
    _spu_RXX->m180_mainVolL = 0;
    _spu_RXX->m182_mainVolR = 0;
    i = 0;
    while (_spu_RXX->m1AE_status & 0x7FF) {
        if (++i > 0xF00) {
            printfStub("SPU:T/O [%s]\n", "wait (reset)");
            break;
        }
    }
    D_80058E2C = 2;
    D_80058E30 = 3;
    D_80058E34 = 8;
    D_80058E38 = 7;
    _spu_RXX->m1AC_transferCtrl = 4;
    _spu_RXX->m184_revVolL = 0;
    _spu_RXX->m186_revVolR = 0;
    _spu_RXX->m18C_keyOff[0] = 0xFFFF;
    _spu_RXX->m18C_keyOff[1] = 0xFFFF;
    _spu_RXX->m198_reverbOn[0] = 0;
    _spu_RXX->m198_reverbOn[1] = 0;
    for (j = 0; j < 10; j++) {
        _spu_RQ[j] = 0;
    }
    if (mode == 0) {
        D_80058E20 = 0x200;
        _spu_RXX->m190_fm[0] = 0;
        _spu_RXX->m190_fm[1] = 0;
        _spu_RXX->m194_noise[0] = 0;
        _spu_RXX->m194_noise[1] = 0;
        _spu_RXX->m1B0_cdVolL = 0;
        _spu_RXX->m1B2_cdVolR = 0;
        _spu_RXX->m1B4_extVolL = 0;
        _spu_RXX->m1B6_extVolR = 0;
        _spu_FwriteByIO(spuDummyVoiceBlock, 0x10);
        for (j = 0; j < 24; j++) {
            _spu_RXX->m0_voice[j].m0_volL = 0;
            _spu_RXX->m0_voice[j].m2_volR = 0;
            _spu_RXX->m0_voice[j].m4_pitch = 0x3FFF;
            _spu_RXX->m0_voice[j].m6_addr = 0x200;
            _spu_RXX->m0_voice[j].m8_adsr1 = 0;
            _spu_RXX->m0_voice[j].mA_adsr2 = 0;
        }
        _spu_RXX->m188_keyOn[0] = 0xFFFF;
        _spu_RXX->m188_keyOn[1] = 0xFF;
        _spu_Fw1ts();
        _spu_Fw1ts();
        _spu_Fw1ts();
        _spu_Fw1ts();
        _spu_RXX->m18C_keyOff[0] = 0xFFFF;
        _spu_RXX->m18C_keyOff[1] = 0xFF;
        _spu_Fw1ts();
        _spu_Fw1ts();
        _spu_Fw1ts();
        _spu_Fw1ts();
    }
    D_80058E3C = 1;
    _spu_RXX->m1AA_control = 0xC000;
    D_80058E40 = NULL;
    D_80058E44 = NULL;
    return 0;
}

void _spu_FwriteByIO(void* addr, u32 size) {
    u16 status;
    u16 c;
    s32 n;
    s32 j;
    u32 i;
    u16* p;

    status = _spu_RXX->m1AE_status & 0x7FF;
    p = addr;
    _spu_RXX->m1A6_transferAddr = D_80058E20;
    _spu_Fw1ts();
    while (size != 0) {
        n = size > 0x40 ? 0x40 : size;
        for (j = 0; j < n; j += 2) {
            _spu_RXX->m1A8_transferData = *p++;
        }
        c = _spu_RXX->m1AA_control;
        c = (c & ~0x30) | 0x10;
        _spu_RXX->m1AA_control = c;
        _spu_Fw1ts();
        i = 0;
        while (_spu_RXX->m1AE_status & 0x400) {
            if (++i > 0xF00) {
                printfStub("SPU:T/O [%s]\n", "wait (wrdy H -> L)");
                break;
            }
        }
        _spu_Fw1ts();
        _spu_Fw1ts();
        size -= n;
    }
    c = _spu_RXX->m1AA_control;
    c &= ~0x30;
    _spu_RXX->m1AA_control = c;
    i = 0;
    while ((_spu_RXX->m1AE_status & 0x7FF) != status) {
        if (++i > 0xF00) {
            printfStub("SPU:T/O [%s]\n", "wait (dmaf clear/W)");
            break;
        }
    }
}

void _spu_FiDMA(void) {
    u32 i;

    if (spuDmaReadMode == 0) {
        _spu_Fw1ts();
    }
    _spu_RXX->m1AA_control &= ~0x30;
    i = 0;
    while (_spu_RXX->m1AA_control & 0x30) {
        if (++i > 0xF00) {
            break;
        }
    }
    if (D_80058E40 != NULL) {
        D_80058E40();
    } else {
        DeliverEvent(0xF0000009, 0x20);
    }
}

void _spu_Fr_(void* addr, u16 spu_addr, u32 size) {
    _spu_RXX->m1A6_transferAddr = spu_addr;
    _spu_Fw1ts();
    _spu_RXX->m1AA_control |= 0x30;
    _spu_Fw1ts();
    _spu_FsetDelayR();
    *spuDmaMadrReg = (u32)addr;
    *spuDmaBcrReg = (size << 16) | 0x10;
    spuDmaReadMode = 1;
    *spuDmaChcrReg = 0x01000200;
}

s32 _spu_t(s32 mode, ...) {
    va_list ap;
    u32 i;
    u16 c;
    u32 addr;
    u32 chcr;

    va_start(ap, mode);
    switch (mode) {
    case 2:
        addr = va_arg(ap, u32);
        _spu_RXX->m1A6_transferAddr = D_80058E20 = addr >> D_80058E30;
        break;
    case 1:
        spuDmaReadMode = 0;
        i = 0;
        while (_spu_RXX->m1A6_transferAddr != D_80058E20) {
            if (++i > 0xF00) {
                return -2;
            }
        }
        c = _spu_RXX->m1AA_control;
        c = (c & ~0x30) | 0x20;
        _spu_RXX->m1AA_control = c;
        break;
    case 0:
        spuDmaReadMode = 1;
        i = 0;
        while (_spu_RXX->m1A6_transferAddr != D_80058E20) {
            if (++i > 0xF00) {
                return -2;
            }
        }
        c = _spu_RXX->m1AA_control;
        c |= 0x30;
        _spu_RXX->m1AA_control = c;
        break;
    case 3:
        if (spuDmaReadMode == 1) {
            c = 0x30;
        } else {
            c = 0x20;
        }
        i = 0;
        while ((_spu_RXX->m1AA_control & 0x30) != c) {
            if (++i > 0xF00) {
                return -2;
            }
        }
        if (spuDmaReadMode == 1) {
            _spu_FsetDelayR();
        } else {
            _spu_FsetDelayW();
        }
        addr = va_arg(ap, u32);
        spuDmaStartAddr = addr;
        addr = va_arg(ap, u32);
        spuDmaBlockCount = (addr >> 6) + ((addr & 0x3F) != 0);
        *spuDmaMadrReg = spuDmaStartAddr;
        *spuDmaBcrReg = (spuDmaBlockCount << 16) | 0x10;
        if (spuDmaReadMode == 1) {
            chcr = 0x01000200;
        } else {
            chcr = 0x01000201;
        }
        *spuDmaChcrReg = chcr;
        break;
    }
    return 0;
}

u32 _spu_Fw(u8* addr, u32 size) {
    if (D_80058E24 == 0) {
        _spu_t(2, D_80058E20 << D_80058E30);
        _spu_t(1);
        _spu_t(3, addr, size);
    } else {
        _spu_FwriteByIO((u16*)addr, size);
    }
    return size;
}

u32 _spu_Fr(u8* addr, u32 size) {
    _spu_t(2, D_80058E20 << D_80058E30);
    _spu_t(0);
    _spu_t(3, addr, size);
    return size;
}

void _spu_FsetRXX(s32 l, u32 addr, s32 flag) {
    if (flag == 0) {
        ((volatile u16*)_spu_RXX)[l] = addr;
    } else {
        ((volatile u16*)_spu_RXX)[l] = addr >> D_80058E30;
    }
}

u32 _spu_FsetRXXa(s32 l, u32 addr) {
    u32 v;

    if (D_80058E2C != 0 && addr % D_80058E34 != 0) {
        addr += D_80058E34;
        addr &= ~D_80058E38;
    }
    v = addr >> D_80058E30;
    switch (l) {
    case -1:
        return (u16)v;
    case -2:
        return addr;
    default:
        ((volatile u16*)_spu_RXX)[l] = v;
        return addr;
    }
}

u32 _spu_FgetRXXa(s32 l, s32 flag) {
    u16 v = ((volatile u16*)_spu_RXX)[l];

    if (flag == -1) {
        return v;
    }
    return v << D_80058E30;
}

void _spu_FsetPCR(s32 flag) {
    *spuDpcrReg &= 0xFFF8FFFF;
    if (flag) {
        *spuDpcrReg |= 0x30000;
    } else {
        *spuDpcrReg |= 0x50000;
    }
}

void _spu_FsetDelayW(void) {
    *spuBusDelayReg = (*spuBusDelayReg & 0xF0FFFFFF) | 0x20000000;
}

void _spu_FsetDelayR(void) {
    *spuBusDelayReg = (*spuBusDelayReg & 0xF0FFFFFF) | 0x22000000;
}

void _spu_Fw1ts(void) {
    volatile s32 i;
    volatile s32 v;

    v = 13;
    for (i = 0; i < 60; i++) {
        v = v * 13;
    }
}
