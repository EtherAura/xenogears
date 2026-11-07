/*
 * PSY-Q 4.0 LIBSPU.LIB S_INI.OBJ: _SpuInit, SpuStart. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 * The object owns .data 0x8005899C-0x80058E08: LIBSPU state that the other
 * LIBSPU objects use too (D_800589B4 = SpuReverbAttr, D_800589D0 = voice
 * center notes, spuZeroBuffer = the zero block SpuClearReverbWorkArea writes).
 */
#include "common.h"
#include "psyq/libspu.h"

/* S_INI.OBJ .data (0x8005899C-0x80058E08) */
s32 D_8005899C = 0; /* SPU DMA event (SpuStart) */
u32 D_800589A0 = 0;
s32 D_800589A4 = 0;
s32 D_800589A8 = 0;
s32 D_800589AC = 0;
s32 D_800589B0 = 0; /* reverb work area start */
SpuReverbAttr D_800589B4 = {0};
s32 D_800589C8 = 0;
s32 D_800589CC = 0;
u16 D_800589D0[24] = {
    0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000,
    0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000,
    0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000, 0xC000,
};
u8 spuZeroBuffer[0x400] = {0}; /* zeros (SpuClearReverbWorkArea) */
s32 D_80058E00 = 0;
s32 D_80058E04 = 0; /* SpuStart done */
extern s32 D_80058E24;
extern s32 D_80058E64;
extern s32 D_80058E68;
extern s32 D_80058E6C;
extern s32 D_80058E70[];
void ResetCallback(void);
void _spu_init(s32 mode);
void SpuStart(void);
void _spu_FsetRXX(s32 reg, u32 addr, s32 flag);
void _spu_FiDMA(void);
void _SpuDataCallback(void (*func)(void));
void EnterCriticalSection(void);
s32 OpenEvent(u32 desc, s32 spec, s32 mode, void* func);
void EnableEvent(s32 event);
void ExitCriticalSection(void);

void _SpuInit(s32 mode) {
    s32 i;

    ResetCallback();
    _spu_init(mode);
    if (mode == 0) {
        for (i = 0; i < 24; i++) {
            D_800589D0[i] = 0xC000;
        }
    }
    SpuStart();
    D_800589A8 = 0;
    D_800589AC = 0;
    D_800589B4.m4_mode = 0;
    D_800589B4.m8_depth.m0_left = 0;
    D_800589B4.m8_depth.m2_right = 0;
    D_800589B4.mC_delay = 0;
    D_800589B4.m10_feedback = 0;
    D_800589B0 = D_80058E70[0];
    _spu_FsetRXX(0xD1, D_800589B0, 0);
    D_80058E64 = 0;
    D_80058E68 = 0;
    D_80058E6C = 0;
    D_800589A4 = 0;
    D_80058E24 = 0;
    D_800589A0 = 0;
    D_800589CC = 0;
    D_800589C8 = 0;
    D_80058E00 = 0;
}

void SpuStart(void) {
    if (D_80058E04 == 0) {
        D_80058E04 = 1;
        EnterCriticalSection();
        _SpuDataCallback(_spu_FiDMA);
        D_8005899C = OpenEvent(0xF0000009, 0x20, 0x2000, 0);
        EnableEvent(D_8005899C);
        ExitCriticalSection();
    }
}
