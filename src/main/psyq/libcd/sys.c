/*
 * PSY-Q 4.0 LIBCD.LIB SYS.OBJ: the Cd* API. Our own C for
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

extern void* DMACallback(s32 dma, void (*func)(void));

/* SYS.OBJ .data (0x80056420-0x800564A0): per command, 1 if its parameter
 * is a position that CdControl sends with CdlSetloc first (CdlPlay, CdlReadN,
 * CdlSeekL, CdlSeekP, CdlReadS). */
s32 cdCommandSendsSetloc[32] = {
    0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
};

/* LIBCD state owned by BIOS.OBJ */
extern CdlCB CD_cbsync;    /* CD_cbsync */
extern CdlCB CD_cbready;    /* CD_cbready */
extern s32 CD_debug;      /* CD_debug */
extern u8 CD_status;       /* CD_status */
extern CdlLOC CD_pos;   /* CD_pos */
extern u8 CD_mode;       /* CD_mode */
extern u8 CD_com;       /* CD_com */
extern char* D_800564D0[];  /* command names */
extern char* D_80056550[];  /* interrupt names */

s32 CD_sync(s32 mode, u8* result);
s32 CD_ready(s32 mode, u8* result);
s32 CD_cw(u8 com, u8* param, u8* result, s32 async);
s32 CD_vol(u8* vol);
void CD_flush(void);
s32 CD_initvol(void);
void CD_initintr(void);
s32 CD_init(void);
s32 CD_datasync(s32 mode);
s32 CD_getsector(void* madr, s32 size);
s32 CD_getsector2(void* madr, s32 size);

s32 CdStatus(void) {
    return CD_status;
}

s32 CdMode(void) {
    return CD_mode;
}

s32 CdLastCom(void) {
    return CD_com;
}

CdlLOC* CdLastPos(void) {
    return &CD_pos;
}

s32 CdReset(s32 mode) {
    if (mode == 2) {
        CD_initintr();
        return 1;
    }
    if (CD_init()) {
        return 0;
    }
    if (mode == 1) {
        if (CD_initvol()) {
            return 0;
        }
    }
    return 1;
}

void CdFlush(void) {
    CD_flush();
}

s32 CdSetDebug(s32 level) {
    s32 old = CD_debug;
    CD_debug = level;
    return old;
}

char* CdComstr(u8 com) {
    return com < 0x1C ? D_800564D0[com] : "none";
}

char* CdIntstr(u8 intr) {
    return intr < 7 ? D_80056550[intr] : "none";
}

s32 CdSync(s32 mode, u8* result) {
    return CD_sync(mode, result);
}

s32 CdReady(s32 mode, u8* result) {
    return CD_ready(mode, result);
}

CdlCB CdSyncCallback(CdlCB func) {
    CdlCB old = CD_cbsync;
    CD_cbsync = func;
    return old;
}

CdlCB CdReadyCallback(CdlCB func) {
    CdlCB old = CD_cbready;
    CD_cbready = func;
    return old;
}

/* Issue a command, up to four tries: 0 on success, -1 on failure. */
static inline s32 cd_retry(u8 com, u8* param, u8* result, s32 async) {
    CdlCB old = CD_cbsync;
    s32 cnt;

    for (cnt = 3; cnt != -1; cnt--) {
        CD_cbsync = 0;
        /* CdlNop; status bit 0x10 = shell open */
        if (com != 1 && (CD_status & 0x10)) {
            CD_cw(1, 0, 0, 0);
        }
        /* CdlSetloc first when the command takes a position */
        if (param == 0 || cdCommandSendsSetloc[com] == 0 || CD_cw(2, param, result, 0) == 0) {
            CD_cbsync = old;
            if (CD_cw(com, param, result, async) == 0) {
                return 0;
            }
        }
    }
    CD_cbsync = old;
    return -1;
}

s32 CdControl(u8 com, u8* param, u8* result) {
    return cd_retry(com, param, result, 0) + 1;
}

s32 CdControlF(u8 com, u8* param) {
    return cd_retry(com, param, 0, 1) + 1;
}

s32 CdControlB(u8 com, u8* param, u8* result) {
    if (cd_retry(com, param, result, 0) != 0) {
        return 0;
    }
    return CD_sync(0, result) == 2; /* CdlComplete */
}

s32 CdMix(u8* vol) {
    CD_vol(vol);
    return 1;
}

s32 CdGetSector(void* madr, s32 size) {
    return CD_getsector(madr, size) == 0;
}

s32 CdGetSector2(void* madr, s32 size) {
    return CD_getsector2(madr, size) == 0;
}

void (*CdDataCallback(void (*func)(void)))(void) {
    return DMACallback(3, func);
}

s32 CdDataSync(s32 mode) {
    return CD_datasync(mode);
}

#define itob(i) ((i) / 10 * 16 + (i) % 10)
#define btoi(b) ((b) / 16 * 10 + (b) % 16)

CdlLOC* CdIntToPos(s32 i, CdlLOC* p) {
    s32 min, sec, sector;

    i += 150;
    sec = i / 75;
    sector = i % 75;
    min = sec / 60;
    sec = sec % 60;
    p->sector = itob(sector);
    p->second = itob(sec);
    p->minute = itob(min);
    return p;
}

s32 CdPosToInt(CdlLOC* p) {
    return (btoi(p->minute) * 60 + btoi(p->second)) * 75 + btoi(p->sector) - 150;
}
