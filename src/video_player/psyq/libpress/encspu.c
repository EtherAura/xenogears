/*
 * PSY-Q 4.0 LIBPRESS.LIB ENCSPU.OBJ: SPU ADPCM encoder (_EncSPU_encode,
 * _EncSPU, EncSPU). Our own C (clean-room, from the disassembly).
 */
#include "common.h"

typedef struct {
    /* 0x00 */ s16* m0_src;
    /* 0x04 */ s16* m4_dest;
    /* 0x08 */ s16* m8_work;
    /* 0x0C */ s32 mC_size;
    /* 0x10 */ s32 m10_loopStart;
    /* 0x14 */ u8 m14_loop;
    /* 0x15 */ u8 m15_byteSwap;
    /* 0x16 */ u8 m16_proceed;
    /* 0x17 */ u8 m17_quality;
} ENCSPUENCODEPARAM;

int abs(int);

/* One 28-sample block: clip the input to +-0x7800, scale it to Q10 in work[],
 * pick the shift for the block's peak and round each sample to its 4-bit
 * nibble (left in the top of a 16-bit word for _EncSPU to pack).
 * The 16-bit saturation of x never fires (x is already a short) but keeps
 * retail's sll 16 / sra 16 / sll 10: while combine runs, its branch splits the
 * sign extension from the scale (x has three definitions), so the two cannot
 * merge into sra 6; combine then proves (s16)x == x, the branch becomes a
 * jump and the dead clamp is deleted after register allocation. PSY-Q 3.30's
 * encoder has the same step. */
void _EncSPU_encode(u16* src, s16* out, s16* shift, s32 swap, s32* work) {
    s32 max;
    s32 i;
    s32* w;
    s32 s;
    u32 t;
    s32 a;
    s32 m;
    s32 mask;
    s32 x;
    s32 y;

    max = 0;
    w = work;
    for (i = 0; i < 28; i++) {
        t = *src;
        if (swap) {
            t = (t >> 8) | ((t & 0xFF) << 8);
        }
        s = t;
        if ((s16)t > 0x77FF) {
            s = 0x77FF;
        }
        if ((s16)t < -0x7800) {
            s = -0x7800;
        }
        x = (s16)s;
        if ((s16)x != x) {
            if (x < 0) {
                x = -0x8000;
            } else {
                x = 0x7FFF;
            }
        }
        *w = x << 10;
        a = abs(*w);
        if (max < a) {
            max = a;
        }
        w++;
        src++;
    }
    if (max < 0) {
        m = -((-max) >> 10);
    } else {
        m = max >> 10;
    }
    if (m > 0x7FFF) {
        m = 0x7FFF;
    }
    if (m < -0x8000) {
        m = -0x8000;
    }
    mask = 0x4000;
    for (*shift = 0; *shift < 12; (*shift)++) {
        if ((m + (mask >> 3)) & mask) {
            break;
        }
        mask >>= 1;
    }
    for (i = 0; i < 28; i++) {
        y = work[i] << *shift;
        if (y < 0) {
            x = -((-y) >> 10);
        } else {
            x = y >> 10;
        }
        y = (x + 0x800) & 0xFFFFF000;
        if (y > 0x7FFF) {
            y = 0x7FFF;
        }
        if (y < -0x8000) {
            y = -0x8000;
        }
        *out++ = y;
    }
}

extern s16 encSpuLastShift; /* last block's shift (-1: none) for proceed modes 2/4 */

s32 _EncSPU(s16* src, s16* dest, s32 size, s32 loopStart, s32 loop, s32 byteSwap, s32 proceed, s16* work) {
    s16 outbuf[28];
    s32 workbuf[28];
    s16 shift;
    s16* out;
    s32* wk;
    s16* ptr;
    s32 n;
    s32 i;
    s32 j;
    s32 end;
    s16 flags;

    n = 0;
    end = 0;
    out = outbuf;
    wk = workbuf;
    shift = 0;
    if (work) {
        out = work;
        wk = (s32*)(work + 28);
    }
    switch (proceed) {
    case 0:
    case 1:
        for (i = 7; i >= 0; i--) {
            dest[n++] = 0;
        }
        i = 0;
        break;
    case 2:
    case 4:
        if (encSpuLastShift < 0) {
            return -1;
        }
        shift = encSpuLastShift;
        i = 0;
        break;
    default:
        return -1;
    }
    ptr = src;
    do {
        _EncSPU_encode((u16*)ptr, out, &shift, byteSwap, wk);
        flags = 0x200;
        if (loop && loopStart >= 0 && i >= loopStart && i < loopStart + 28) {
            flags = 0x600;
        }
        if (proceed == 0 || proceed == 4) {
            if (i >= size - 28) {
                end = 1;
                if (loop) {
                    flags |= 0x100;
                }
            }
        }
        dest[n++] = flags | shift;
        for (j = 0; j < 28; j += 4) {
            dest[n++] = (out[j + 3] & 0xF000) | ((out[j + 2] >> 4) & 0xF00) | ((out[j + 1] >> 8) & 0xF0) |
                        ((u16)out[j] >> 12);
        }
        i += 28;
        ptr += 28;
    } while (i < size && !end);
    if (proceed & 3) {
        encSpuLastShift = shift;
    } else if (loop) {
        encSpuLastShift = -1;
    } else {
        dest[n++] = 0x700;
        for (i = 1; i < 8; i++) {
            dest[n++] = 0;
        }
        encSpuLastShift = -1;
    }
    return n * 2;
}

s32 EncSPU(ENCSPUENCODEPARAM* p) {
    return _EncSPU(p->m0_src, p->m4_dest, p->mC_size >> 1, p->m10_loopStart >> 1, p->m14_loop, p->m15_byteSwap,
                   p->m16_proceed, p->m8_work);
}
