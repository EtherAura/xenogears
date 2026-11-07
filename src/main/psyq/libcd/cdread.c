/*
 * PSY-Q 4.0 LIBCD.LIB CDREAD.OBJ: CdRead. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

typedef struct {
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
} CdlLOC;

typedef void (*CdlCB)(u8, u8*);

/* CdRead state (.data 0x80056848-0x80056880). One object: CdReadSync
 * reaches the members relative to one base register. */
typedef struct {
    /* 0x00 */ s32 m0_nsector;      /* sectors requested */
    /* 0x04 */ u32* m4_buf;         /* destination */
    /* 0x08 */ u32* m8_addr;        /* current destination */
    /* 0x0C */ s32 mC_mode;         /* CdlSetmode byte */
    /* 0x10 */ s32 m10_size;        /* sector size in words */
    /* 0x14 */ s32 m14_remain;      /* sectors left; -1 = error */
    /* 0x18 */ s32 m18_last;        /* VSync of the last interrupt */
    /* 0x1C */ s32 m1C_start;       /* VSync at CdRead */
    /* 0x20 */ s32 m20_pos;         /* expected sector (CdPosToInt) */
    /* 0x24 */ CdlCB m24_oldsync;
    /* 0x28 */ CdlCB m28_oldready;
    /* 0x2C */ void (*m2C_olddata)(void);
    /* 0x30 */ s32 m30_readmode;    /* CdReadMode: bit 0 = DMA via CdGetSector2 */
    /* 0x34 */ u8* m34_result;
} CdReadState; /* 0x38 */

/* CDREAD.OBJ .data (0x80056844-0x80056880) */
CdlCB cdReadCallbackFunc = NULL; /* CdReadCallback */
volatile CdReadState cdReadState = {0};
#define S cdReadState

extern s32 puts(const char* s);
extern s32 VSync(s32 mode);
s32 CdStatus(void);
s32 CdMode(void);
CdlLOC* CdLastPos(void);
s32 CdPosToInt(CdlLOC* p);
s32 CdControl(u8 com, u8* param, u8* result);
s32 CdControlF(u8 com, u8* param);
s32 CdControlB(u8 com, u8* param, u8* result);
void CdFlush(void);
s32 CdReady(s32 mode, u8* result);
CdlCB CdSyncCallback(CdlCB func);
CdlCB CdReadyCallback(CdlCB func);
void (*CdDataCallback(void (*func)(void)))(void);
s32 CdDataSync(s32 mode);
s32 CdGetSector(void* madr, s32 size);
s32 CdGetSector2(void* madr, s32 size);

void cb_read(u8 intr, u8* result);
void cb_data(void);
s32 cd_read_retry(s32 retry);

static inline void cd_read_end(void) {
    CdSyncCallback(S.m24_oldsync);
    CdReadyCallback(S.m28_oldready);
    if (S.m30_readmode & 1) {
        CdDataCallback(S.m2C_olddata);
    }
    CdControlF(9, 0);
}

void cb_read(u8 intr, u8* result) {
    CdlLOC pos[3];

    S.m34_result = result;
    if (intr == 1) {
        if (S.m14_remain > 0) {
            if (S.m10_size == 0x200) {
                if (S.m30_readmode & 1) {
                    CdDataCallback(0);
                    CdGetSector2(pos, 3);
                    CdDataSync(0);
                    CdDataCallback(cb_data);
                } else {
                    CdGetSector(pos, 3);
                }
                if (CdPosToInt(pos) != S.m20_pos) {
                    puts("CdRead: sector error\n");
                    S.m14_remain = -1;
                }
            }
            if (S.m30_readmode & 1) {
                CdGetSector2(S.m8_addr, S.m10_size);
            } else {
                CdGetSector(S.m8_addr, S.m10_size);
                S.m8_addr += S.m10_size;
                S.m14_remain--;
                S.m20_pos++;
            }
        }
    } else {
        S.m14_remain = -1;
    }
    S.m18_last = VSync(-1);
    if (S.m14_remain < 0) {
        cd_read_retry(1);
    }
    if (VSync(-1) > S.m1C_start + 1200) {
        S.m14_remain = -1;
    }
    if (S.m14_remain == 0 || VSync(-1) > S.m1C_start + 1200) {
        cd_read_end();
        if (cdReadCallbackFunc) {
            cdReadCallbackFunc(S.m14_remain == 0 ? 2 : 5, result);
        }
    }
}

void cb_data(void) {
    S.m8_addr += S.m10_size;
    S.m14_remain--;
    S.m20_pos++;
    if (S.m14_remain == 0) {
        cd_read_end();
        if (cdReadCallbackFunc) {
            cdReadCallbackFunc(2, S.m34_result);
        }
    }
}

s32 cd_read_retry(s32 retry) {
    u8 param[4];

    CdSyncCallback(0);
    CdReadyCallback(0);
    if (S.m30_readmode & 1) {
        CdDataCallback(0);
    }
    if (CdStatus() & 0x10) {
        if ((VSync(-1) & 0x3F) == 0) {
            puts("CdRead: Shell open...\n");
        }
        CdControlF(1, 0);
        S.m1C_start = VSync(-1);
        S.m14_remain = -1;
        return S.m14_remain;
    }
    if (retry) {
        puts("CdRead: retry...\n");
        CdControl(9, 0, 0);
        if (CdControl(2, (u8*)CdLastPos(), 0) == 0) {
            S.m14_remain = -1;
            return S.m14_remain;
        }
    }
    CdFlush();
    param[0] = S.mC_mode;
    if (param[0] != CdMode() || retry) {
        if (CdControl(14, param, 0) == 0) {
            S.m14_remain = -1;
            return S.m14_remain;
        }
    }
    S.m20_pos = CdPosToInt(CdLastPos());
    CdReadyCallback(cb_read);
    if (S.m30_readmode & 1) {
        CdDataCallback(cb_data);
    }
    S.m8_addr = S.m4_buf;
    CdControlF(6, 0);
    S.m14_remain = S.m0_nsector;
    S.m18_last = VSync(-1);
    return S.m14_remain;
}

void CdReadBreak(void) {
    if (S.m30_readmode & 1) {
        CdDataSync(0);
    }
    S.m14_remain = 0;
    cd_read_end();
}

s32 CdRead(s32 sectors, u32* buf, s32 mode) {
    S.mC_mode = mode;
    switch (S.mC_mode & 0x30) {
    case 0:
        S.m10_size = 0x200;
        break;
    case 0x20:
        S.m10_size = 0x249;
        break;
    default:
        S.m10_size = 0x246;
        break;
    }
    S.mC_mode |= 0x20;
    S.m4_buf = buf;
    S.m0_nsector = sectors;
    S.m24_oldsync = CdSyncCallback(0);
    S.m28_oldready = CdReadyCallback(0);
    if (S.m30_readmode & 1) {
        S.m2C_olddata = CdDataCallback(0);
    }
    S.m1C_start = VSync(-1);
    if (CdStatus() & 0xE0) {
        CdControlB(9, 0, 0);
    }
    return cd_read_retry(0) > 0;
}

s32 CdReadSync(s32 mode, u8* result) {
    s32 ret;

    do {
        if (VSync(-1) > S.m1C_start + 1200) {
            ret = -1;
        } else if (S.m14_remain < 0 || VSync(-1) > S.m18_last + 60) {
            cd_read_retry(1);
            ret = S.m0_nsector;
        } else {
            ret = S.m14_remain;
        }
    } while (mode == 0 && ret > 0);
    CdReady(1, result);
    return ret;
}

CdlCB CdReadCallback(CdlCB func) {
    CdlCB old = cdReadCallbackFunc;
    cdReadCallbackFunc = func;
    return old;
}

s32 CdReadMode(s32 mode) {
    s32 old = S.m30_readmode;
    S.m30_readmode = mode;
    return old;
}
