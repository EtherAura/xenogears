/*
 * PSY-Q 4.0 LIBCD.LIB BIOS.OBJ: the CD-ROM controller driver (command
 * issue, interrupt handling, sector DMA). Our own C for the library
 * routine(s) (clean-room, from the disassembly).
 *
 * -O3 matters: CD_init inlines CD_initintr and CD_flush, CD_initvol inlines
 * CD_vol, and the static callback is inlined into CD_sync/CD_ready/CD_cw
 * and emitted at the end of the object, where retail has it. The object
 * owns .data 0x800564A8-0x800567B8; the order of definitions below (and
 * where the debug table sits between the functions) gives retail's .data
 * and .rodata layout.
 */
#include "common.h"
#include "psyq.h"

typedef volatile u8 vu8;

extern s32 printfStub(const char* fmt, ...); /* printf (stubbed) */
extern s32 puts(const char* s);
extern void* InterruptCallback(s32 irq, void (*func)(void));

static u8 cdSyncResult[8];          /* sync result */
static u8 cdReadyResult[8];          /* ready result */
static u8 cdDataEndResult[8];          /* data end result */

/* ---- LIBCD state (.data 0x800564A8-0x800567B8, in retail order) ---- */

CdlCB CD_cbsync = NULL;     /* CD_cbsync */
CdlCB CD_cbready = NULL;     /* CD_cbready */
s32 D_800564B0 = 0;
s32 CD_debug = 0;          /* CD_debug */
s32 CD_status = 0;          /* CD_status (SYS reads it as a byte) */
s32 CD_status1 = 0;          /* CD_status1 */
s32 CD_nopen = 0;          /* CD_nopen: shell open count */
CdlLOC CD_pos = {2, 0, 0, 0}; /* CD_pos */
u8 CD_mode = 0;           /* CD_mode */
u8 CD_com = 0;           /* CD_com */
s32 D_800564CC = 0;          /* read by the movie player */

/* Command and interrupt names; the compiler emits their strings after each
 * table, last-referenced first, which is retail's .rodata order. */
char* D_800564D0[32] = {
    "CdlSync", "CdlNop", "CdlSetloc", "CdlPlay",
    "CdlForward", "CdlBackward", "CdlReadN", "CdlStandby",
    "CdlStop", "CdlPause", "CdlReset", "CdlMute",
    "CdlDemute", "CdlSetfilter", "CdlSetmode", "?",
    "CdlGetlocL", "CdlGetlocP", "?", "CdlGetTN",
    "CdlGetTD", "CdlSeekL", "CdlSeekP", "?",
    "?", "?", "?", "CdlReadS",
    "?", "?", "?", "?",
};
char* D_80056550[8] = {
    "NoIntr", "DataReady", "Complete", "Acknowledge",
    "DataEnd", "DiskError", "?", "?",
};

/* Per-command tables: [0] Acknowledge ends the command, [1] the command
 * clears the ready interrupt, [2] Acknowledge carries the status, [3] number
 * of parameter bytes (CdlTest = 25 is settable). */
s32 cdCommandTraits[4][32] = {
    {
        0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
        0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 0,
        0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    }
};

vu8* cdReg0 = (vu8*)0x1F801800;           /* index/status */
vu8* cdReg1 = (vu8*)0x1F801801;
vu8* cdReg2 = (vu8*)0x1F801802;
vu8* cdReg3 = (vu8*)0x1F801803;
volatile s32* cdComDelayReg = (volatile s32*)0x1F801020; /* COM_DELAY */
volatile u16* cdSpuRegs = (volatile u16*)0x1F801C00; /* SPU */
volatile u8 cdLastIntr[3] = {0, 0, 0}; /* last intr: [0] sync, [1] ready, [2] data end */

static void callback(void);
void CD_flush(void);
s32 CD_cw(u8 com, u8* param, u8* result, s32 async);
s32 CD_sync(s32 mode, u8* result);

typedef struct {
    /* 0x0 */ s32 m0_time;   /* VSync deadline */
    /* 0x4 */ s32 m4_count;  /* poll counter */
    /* 0x8 */ char* m8_name; /* function name */
} CdAlarm;
static CdAlarm cdAlarm;
extern s32 VSync(s32 mode);
extern s32 CheckCallback(void);

static inline void cd_set_alarm(char* name) {
    cdAlarm.m0_time = VSync(-1) + 960;
    cdAlarm.m4_count = 0;
    cdAlarm.m8_name = name;
}

static inline s32 cd_get_alarm(void) {
    if (cdAlarm.m0_time < VSync(-1) || cdAlarm.m4_count++ > 0x3C0000) {
        puts("CD timeout: ");
        printfStub("%s:(%s) Sync=%s, Ready=%s\n", cdAlarm.m8_name, D_800564D0[CD_com],
                      D_80056550[cdLastIntr[0]], D_80056550[cdLastIntr[1]]);
        CD_flush();
        return -1;
    }
    return 0;
}

static inline void cd_memcpy(u8* dst, volatile u8* src, s32 n) {
    s32 i;

    if (dst) {
        for (i = n - 1; i != -1; i--) {
            *dst++ = *src++;
        }
    }
}

s32 getintr(void) {
    volatile u8 intr;
    volatile u8 result[8];
    s32 i, j;
    s32 err = 0;

    *cdReg0 = 1;
    intr = *cdReg3 & 7;
    if (intr == 0) {
        return 0;
    }
    while (intr != (*cdReg3 & 7)) {
        intr = *cdReg3 & 7;
    }
    for (i = 0; i < 8; i++) {
        if (!(*cdReg0 & 0x20)) {
            break;
        }
        result[i] = *cdReg1;
    }
    for (j = i; j < 8; j++) {
        result[j] = 0;
    }
    *cdReg0 = 1;
    *cdReg3 = 7;
    *cdReg2 = 7;
    if (intr != 3 || cdCommandTraits[2][CD_com] != 0) {
        if (!(CD_status & 0x10) && (result[0] & 0x10)) {
            CD_nopen++;
        }
        CD_status = result[0];
        CD_status1 = result[1];
        err = CD_status & 0x1D;
    }
    if (intr == 5 && CD_debug > 0) {
        printfStub("DiskError: ");
        if (CD_debug > 0) {
            printfStub("com=%s,code=(%02x:%02x)\n", D_800564D0[CD_com], CD_status, CD_status1);
        }
    }
    switch (intr) {
    case 3:
        if (err) {
            cdLastIntr[0] = 5;
            cd_memcpy(cdSyncResult, result, 8);
            return 2;
        }
        if (cdCommandTraits[0][CD_com]) {
            cdLastIntr[0] = 3;
            cd_memcpy(cdSyncResult, result, 8);
            return 1;
        }
        cdLastIntr[0] = 2;
        cd_memcpy(cdSyncResult, result, 8);
        return 2;
    case 2:
        cdLastIntr[0] = err ? 5 : 2;
        cd_memcpy(cdSyncResult, result, 8);
        return 2;
    case 1:
        if (err && i == 1) {
            err = 0;
        }
        cdLastIntr[1] = err ? 5 : 1;
        cd_memcpy(cdReadyResult, result, 8);
        *cdReg0 = 0;
        *cdReg3 = 0;
        return 4;
    case 4:
        cdLastIntr[1] = cdLastIntr[2] = 4;
        cd_memcpy(cdDataEndResult, result, 8);
        cd_memcpy(cdReadyResult, result, 8);
        return 4;
    case 5:
        cdLastIntr[0] = cdLastIntr[1] = 5;
        cd_memcpy(cdSyncResult, result, 8);
        cd_memcpy(cdReadyResult, result, 8);
        return 6;
    }
    puts("CDROM: unknown intr");
    printfStub("(%d)\n", intr);
    return 0;
}


static void callback(void) {
    u8 mode = *cdReg0 & 3;
    s32 intr;

    while ((intr = getintr()) != 0) {
        if (intr & 4) {
            if (CD_cbready) {
                CD_cbready(cdLastIntr[1], cdReadyResult);
            }
        }
        if (intr & 2) {
            if (CD_cbsync) {
                CD_cbsync(cdLastIntr[0], cdSyncResult);
            }
        }
    }
    *cdReg0 = mode;
}

s32 CD_sync(s32 mode, u8* result) {
    s32 intr;

    cd_set_alarm("CD_sync");
    for (;;) {
        if (cd_get_alarm()) {
            return -1;
        }
        if (CheckCallback()) {
            callback();
        }
        intr = cdLastIntr[0];
        if (intr == 2 || intr == 5) {
            cdLastIntr[0] = 2;
            cd_memcpy(result, cdSyncResult, 8);
            return intr;
        }
        if (mode) {
            return 0;
        }
    }
}

s32 CD_ready(s32 mode, u8* result) {
    s32 intr;

    cd_set_alarm("CD_ready");
    for (;;) {
        if (cd_get_alarm()) {
            return -1;
        }
        if (CheckCallback()) {
            callback();
        }
        if ((intr = cdLastIntr[2]) != 0) {
            cdLastIntr[2] = 0;
            cd_memcpy(result, cdDataEndResult, 8);
            return intr;
        }
        if ((intr = cdLastIntr[1]) != 0) {
            cdLastIntr[1] = 0;
            cd_memcpy(result, cdReadyResult, 8);
            return intr;
        }
        if (mode) {
            return 0;
        }
    }
}

s32 CD_cw(u8 com, u8* param, u8* result, s32 async) {
    s32 i;

    if (CD_debug >= 2) {
        printfStub("%s...\n", D_800564D0[com]);
    }
    if (cdCommandTraits[3][com] && param == 0) {
        if (CD_debug > 0) {
            printfStub("%s: no param\n", D_800564D0[com]);
        }
        return -2;
    }
    CD_sync(0, 0);
    if (com == 2) {
        for (i = 0; i < 4; i++) {
            ((u8*)&CD_pos)[i] = param[i];
        }
    }
    if (com == 14) {
        CD_mode = param[0];
    }
    cdLastIntr[0] = 0;
    if (cdCommandTraits[1][com]) {
        cdLastIntr[1] = 0;
    }
    *cdReg0 = 0;
    for (i = 0; i < cdCommandTraits[3][com]; i++) {
        *cdReg2 = param[i];
    }
    CD_com = com;
    *cdReg1 = com;
    if (async) {
        return 0;
    }
    cd_set_alarm("CD_cw");
    while (cdLastIntr[0] == 0) {
        if (cd_get_alarm()) {
            return -1;
        }
        if (CheckCallback()) {
            callback();
        }
    }
    cd_memcpy(result, cdSyncResult, 8);
    return cdLastIntr[0] == 5 ? -1 : 0;
}

s32 CD_vol(u8* vol) {
    *cdReg0 = 2;
    *cdReg2 = vol[0];
    *cdReg3 = vol[1];
    *cdReg0 = 3;
    *cdReg1 = vol[2];
    *cdReg2 = vol[3];
    *cdReg3 = 0x20;
    return 0;
}

void CD_flush(void) {
    *cdReg0 = 1;
    while (*cdReg3 & 7) {
        *cdReg0 = 1;
        *cdReg3 = 7;
        *cdReg2 = 7;
    }
    cdLastIntr[1] = cdLastIntr[2] = 0;
    cdLastIntr[0] = 2;
    *cdReg0 = 0;
    *cdReg3 = 0;
    *cdComDelayReg = 0x1325;
}

s32 CD_initvol(void) {
    u8 vol[4];

    if (cdSpuRegs[0x1B8 / 2] == 0 && cdSpuRegs[0x1BA / 2] == 0) {
        cdSpuRegs[0x180 / 2] = 0x3FFF;
        cdSpuRegs[0x182 / 2] = 0x3FFF;
    }
    cdSpuRegs[0x1B0 / 2] = 0x3FFF;
    cdSpuRegs[0x1B2 / 2] = 0x3FFF;
    cdSpuRegs[0x1AA / 2] = 0xC001;
    vol[0] = vol[2] = 0x80;
    vol[1] = vol[3] = 0;
    CD_vol(vol);
    return 0;
}

void CD_initintr(void) {
    CD_cbready = 0;
    CD_cbsync = 0;
    CD_status1 = 0;
    CD_status = 0;
    ResetCallback();
    InterruptCallback(2, callback);
}

/* Debug info: addresses of the driver state, then the RCS id. */
void* cdDebugInfo[] = {
    (void*)cdLastIntr, cdSyncResult, &CD_com, &CD_status, &CD_pos,
    "$Id: bios.c,v 1.86 1997/03/28 07:42:42 makoto Exp $",
};

volatile s32* cdBusConfigReg = (volatile s32*)0x1F801018; /* CD bus config */
volatile s32* cdDpcrReg = (volatile s32*)0x1F8010F0; /* DPCR */
volatile s32* cdDmaMadrReg = (volatile s32*)0x1F8010B0; /* DMA3 MADR */
volatile s32* cdDmaBcrReg = (volatile s32*)0x1F8010B4; /* DMA3 BCR */
volatile s32* cdDmaChcrReg = (volatile s32*)0x1F8010B8; /* DMA3 CHCR */

s32 CD_init(void) {
    puts("CD_init:");
    printfStub("addr=%08x\n", cdDebugInfo);
    CD_com = 0;
    CD_mode = 0;
    CD_initintr();
    CD_flush();
    CD_cw(1, 0, 0, 0); /* CdlNop */
    if (CD_status & 0x10) {
        CD_cw(1, 0, 0, 0);
    }
    if (CD_cw(10, 0, 0, 0)) { /* CdlInit */
        return -1;
    }
    if (CD_cw(12, 0, 0, 0)) { /* CdlDemute */
        return -1;
    }
    if (CD_sync(0, 0) != 2) {
        return -1;
    }
    return 0;
}

s32 CD_datasync(s32 mode) {
    cd_set_alarm("CD_datasync");
    while (cd_get_alarm() == 0) {
        if (!(*cdDmaChcrReg & 0x1000000)) { /* DMA3 idle */
            return 0;
        }
        if (mode) {
            return 1;
        }
    }
    return -1;
}

s32 CD_getsector(void* madr, s32 size) {
    *cdReg0 = 0;
    *cdReg3 = 0x80;
    *cdBusConfigReg = 0x20943;
    *cdComDelayReg = 0x1323;
    *cdDpcrReg |= 0x8000;
    *cdDmaMadrReg = (s32)madr;
    *cdDmaBcrReg = size | 0x10000;
    do {
    } while (!(*cdReg0 & 0x40));
    *cdDmaChcrReg = 0x11000000;
    while (*cdDmaChcrReg & 0x1000000) {
    }
    *cdComDelayReg = 0x1325;
    return 0;
}

s32 CD_getsector2(void* madr, s32 size) {
    volatile s32 chcr;

    *cdReg0 = 0;
    *cdReg3 = 0x80;
    *cdBusConfigReg = 0x21020843;
    *cdComDelayReg = 0x1325;
    *cdDpcrReg |= 0x8000;
    *cdDmaMadrReg = (s32)madr;
    *cdDmaBcrReg = size | 0x10000;
    while (!(*cdReg0 & 0x40)) {
    }
    *cdDmaChcrReg = 0x11400100;
    chcr = *cdDmaChcrReg;
    return 0;
}

void CD_set_test_parmnum(s32 n) {
    cdCommandTraits[3][25] = n;
}
