/*
 * PSY-Q 4.0 LIBGPU.LIB SYS.OBJ: the GPU driver: ResetGraph, DrawSync, LoadImage, the command queue. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq.h"

/* libgpu driver jump table (gpuDriverFuncs, .data) */
typedef struct {
    /* 0x00 */ char* m0_version; /* -> "$Id: sys.c,v 1.129 ..." (.rodata 0x80019058) */
    /* 0x04 */ s32 (*m4_addque)(s32 (*func)(), void* param, s32 arg);
    /* 0x08 */ s32 (*m8_addque2)(s32 (*func)(), void* param, s32 size, s32 arg);
    /* 0x0C */ s32 (*mC_clr)();
    /* 0x10 */ void (*m10_ctl)(u32 cmd);
    /* 0x14 */ s32 (*m14_cwb)(u32* p, s32 n);
    /* 0x18 */ s32 (*m18_cwc)();
    /* 0x1C */ s32 (*m1C_drs)();
    /* 0x20 */ s32 (*m20_dws)();
    /* 0x24 */ s32 (*m24_exeque)(void);
    /* 0x28 */ s32 (*m28_getctl)(s32 n);
    /* 0x2C */ s32 (*m2C_otc)(u32* ot, s32 n);
    /* 0x30 */ s32 (*m30_param)(s32 n);
    /* 0x34 */ s32 (*m34_reset)(s32 mode);
    /* 0x38 */ u32 (*m38_status)(void);
    /* 0x3C */ s32 (*m3C_sync)(s32 mode);
} GpuDriver;

/* libgpu state (gpuEnvState, .data) */
typedef struct {
    /* 0x00 */ u8 m0_type;
    /* 0x01 */ u8 m1_queue;
    /* 0x02 */ u8 m2_debug;
    /* 0x03 */ u8 m3_reverse;
    /* 0x04 */ s16 m4_w;
    /* 0x06 */ s16 m6_h;
    /* 0x08 */ volatile s32 m8_drawSyncPending;
    /* 0x0C */ void (*mC_drawSyncCb)(void);
    /* 0x10 */ DRAWENV m10_draw;
    /* 0x6C */ volatile DISPENV m6C_disp;
} GpuState; /* 0x80 */

s32 printfStub(char* fmt, ...);
s32 _addque(s32 (*func)(), void* param, s32 arg);
s32 _addque2(s32 (*func)(), void* param, s32 size, s32 arg);
s32 _clr(RECT* rect, u32 color);
void _ctl(u32 cmd);
s32 _cwb(u32* p, s32 n);
void _cwc(u32* p);
s32 _drs(RECT* rect, u32* p);
s32 _dws(RECT* rect, u32* p);
s32 _exeque(void);
s32 _getctl(s32 n);
s32 _otc(u32* ot, s32 n);
s32 _param(s32 n);
s32 _reset(s32 mode);
u32 _status(void);
s32 _sync(s32 mode);

/* SYS.OBJ .data (0x80056888-0x800569F0), first part; the CVS id string is
 * the first thing in the object's .rodata (0x80019058). */
GpuDriver gpuDriverFuncs = {
    "$Id: sys.c,v 1.129 1996/12/25 03:36:20 noda Exp $",
    _addque, _addque2, _clr, _ctl, _cwb, (s32 (*)())_cwc, _drs, _dws,
    _exeque, _getctl, _otc, _param, _reset, _status, _sync,
};
GpuDriver* pGpuDriver = &gpuDriverFuncs;
s32 (*GPU_printf)(char*, ...) = printfStub; /* GPU_printf (prim uses it too) */
GpuState gpuEnvState = {0};
s32 gpuVramWidths[5] = {0x400, 0x400, 0x400, 0x400, 0x400}; /* VRAM width per GPU type */
s32 gpuVramHeights[5] = {0x200, 0x400, 0x400, 0x200, 0x400}; /* VRAM height per GPU type */

void GPU_cw(u32 arg);
void ResetCallback(void);
void DMACallback(s32 dma, void (*func)());
void memset_sys(u8* p, s32 c, s32 n);

s32 ResetGraph(s32 mode) {
    switch (mode & 7) {
    case 0:
    case 3:
        printfStub("ResetGraph:jtb=%08x,env=%08x\n", &gpuDriverFuncs, &gpuEnvState);
    case 5:
        memset_sys(&gpuEnvState, 0, sizeof(GpuState));
        ResetCallback();
        GPU_cw((u32)pGpuDriver & 0xFFFFFF);
        gpuEnvState.m0_type = _reset(mode);
        gpuEnvState.m1_queue = 1;
        gpuEnvState.m4_w = gpuVramWidths[gpuEnvState.m0_type];
        gpuEnvState.m6_h = gpuVramHeights[gpuEnvState.m0_type];
        memset_sys(&gpuEnvState.m10_draw, -1, sizeof(DRAWENV));
        memset_sys(&gpuEnvState.m6C_disp, -1, sizeof(DISPENV));
        return gpuEnvState.m0_type;
    }
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("ResetGraph(%d)...\n", mode);
    }
    return pGpuDriver->m34_reset(1);
}

s32 SetGraphReverse(s32 mode) {
    u8 prev;

    prev = gpuEnvState.m3_reverse;
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("SetGraphReverse(%d)...\n", mode);
    }
    gpuEnvState.m3_reverse = mode;
    pGpuDriver->m10_ctl(pGpuDriver->m28_getctl(8) | (gpuEnvState.m3_reverse ? 0x08000080 : 0x08000000));
    if (gpuEnvState.m0_type == 2) {
        pGpuDriver->m10_ctl(gpuEnvState.m3_reverse ? 0x20000501 : 0x20000504);
    }
    return prev;
}

s32 SetGraphDebug(s32 level) {
    u8 prev;

    prev = gpuEnvState.m2_debug;
    gpuEnvState.m2_debug = level;
    if (gpuEnvState.m2_debug) {
        GPU_printf("SetGraphDebug:level:%d,type:%d reverse:%d\n", gpuEnvState.m2_debug,
                   gpuEnvState.m0_type, gpuEnvState.m3_reverse);
    }
    return prev;
}

s32 SetGraphQueue(s32 mode) {
    u8 prev;

    prev = gpuEnvState.m1_queue;
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("SetGrapQue(%d)...\n", mode);
    }
    if (mode != gpuEnvState.m1_queue) {
        pGpuDriver->m34_reset(1);
        gpuEnvState.m1_queue = mode;
        DMACallback(2, 0);
    }
    return prev;
}

s32 GetGraphType(void) {
    return gpuEnvState.m0_type;
}

s32 GetGraphDebug(void) {
    return gpuEnvState.m2_debug;
}

void* DrawSyncCallback(void (*func)(void)) {
    void (*prev)(void);

    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("DrawSyncCallback(%08x)...\n", func);
    }
    prev = gpuEnvState.mC_drawSyncCb;
    gpuEnvState.mC_drawSyncCb = func;
    return prev;
}

void SetDispMask(s32 mask) {
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("SetDispMask(%d)...\n", mask);
    }
    if (mask == 0) {
        memset_sys(&gpuEnvState.m6C_disp, -1, sizeof(DISPENV));
    }
    pGpuDriver->m10_ctl(mask ? 0x03000000 : 0x03000001);
}

s32 DrawSync(s32 mode) {
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("DrawSync(%d)...\n", mode);
    }
    return pGpuDriver->m3C_sync(mode);
}

u32 moveImagePacket[5] = {0x04FFFFFF, 0x80000000}; /* MoveImage packet: tag, copy command, 3 words */
u32 clearOTagTerminator[5] = {0x04FFFFFF};             /* OT terminator (ClearOTag) */

void SetDrawEnv2(DR_ENV* dr, DRAWENV* env);

void checkRECT(char* name, RECT* rect) {
    switch (gpuEnvState.m2_debug) {
    case 1:
        if (rect->w > gpuEnvState.m4_w || rect->w + rect->x > gpuEnvState.m4_w ||
            rect->y > gpuEnvState.m6_h || rect->y + rect->h > gpuEnvState.m6_h || rect->w <= 0 ||
            rect->x < 0 || rect->y < 0 || rect->h <= 0) {
            GPU_printf("%s:bad RECT", name);
            GPU_printf("(%d,%d)-(%d,%d)\n", rect->x, rect->y, rect->w, rect->h);
        }
        break;
    case 2:
        GPU_printf("%s:", name);
        GPU_printf("(%d,%d)-(%d,%d)\n", rect->x, rect->y, rect->w, rect->h);
        break;
    }
}

/* Old-style definition: u_char parameters, callers pass ints (see psyq.h). */
void ClearImage(rect, r, g, b)
RECT* rect;
u8 r;
u8 g;
u8 b;
{
    checkRECT("ClearImage", rect);
    pGpuDriver->m8_addque2(pGpuDriver->mC_clr, rect, 8, (b << 16) | (g << 8) | r);
}

s32 ClearImage2(RECT* rect, u8 r, u8 g, u8 b) {
    checkRECT("ClearImage", rect);
    return pGpuDriver->m8_addque2(pGpuDriver->mC_clr, rect, 8, 0x80000000 | (b << 16) | (g << 8) | r);
}

void LoadImage(RECT* rect, void* p) {
    checkRECT("LoadImage", rect);
    pGpuDriver->m8_addque2(pGpuDriver->m20_dws, rect, 8, (s32)p);
}

s32 StoreImage(RECT* rect, u32* p) {
    checkRECT("StoreImage", rect);
    return pGpuDriver->m8_addque2(pGpuDriver->m1C_drs, rect, 8, (s32)p);
}

s32 MoveImage(RECT* rect, s32 x, s32 y) {
    checkRECT("MoveImage", rect);
    if (rect->w == 0 || rect->h == 0) {
        return -1;
    }
    moveImagePacket[2] = *(u32*)&rect->x;
    moveImagePacket[3] = (y << 16) | (x & 0xFFFF);
    moveImagePacket[4] = *(u32*)&rect->w;
    return pGpuDriver->m8_addque2(pGpuDriver->m18_cwc, moveImagePacket, 0x14, 0);
}

u32* ClearOTag(u32* ot, s32 n) {
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("ClearOTag(%08x,%d)...\n", ot, n);
    }
    while (--n) {
        setlen(ot, 0);
        setaddr(ot, ot + 1);
        ot++;
    }
    *ot = (u32)clearOTagTerminator & 0xFFFFFF;
    return ot;
}

u32* ClearOTagR(u32* ot, s32 n) {
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("ClearOTagR(%08x,%d)...\n", ot, n);
    }
    pGpuDriver->m2C_otc(ot, n);
    *ot = (u32)clearOTagTerminator & 0xFFFFFF;
    return ot;
}

void DrawPrim(void* p) {
    s32 len;

    len = ((P_TAG*)p)->len;
    pGpuDriver->m3C_sync(0);
    pGpuDriver->m14_cwb((u32*)p + 1, len);
}

void DrawOTag(u32* p) {
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("DrawOTag(%08x)...\n", p);
    }
    pGpuDriver->m8_addque2(pGpuDriver->m18_cwc, p, 0, 0);
}

DRAWENV* PutDrawEnv(DRAWENV* env) {
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("PutDrawEnv(%08x)...\n", env);
    }
    SetDrawEnv2(&env->dr_env, env);
    env->dr_env.tag |= 0xFFFFFF;
    pGpuDriver->m8_addque2(pGpuDriver->m18_cwc, &env->dr_env, 0x40, 0);
    gpuEnvState.m10_draw = *env;
    return env;
}

void DrawOTagEnv(u32* p, DRAWENV* env) {
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("DrawOTagEnv(%08x,&08x)...\n", p, env);
    }
    SetDrawEnv2(&env->dr_env, env);
    setaddr(&env->dr_env, p);
    pGpuDriver->m8_addque2(pGpuDriver->m18_cwc, &env->dr_env, 0x40, 0);
    gpuEnvState.m10_draw = *env;
}

DRAWENV* GetDrawEnv(DRAWENV* env) {
    memcpy(env, &gpuEnvState.m10_draw, sizeof(DRAWENV));
    return env;
}

s32 GetVideoMode(void);
s32 get_dx(DISPENV* env);

#define LIMIT(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))

DISPENV* PutDispEnv(DISPENV* env) {
    s32 hs;
    s32 he;
    s32 vs;
    s32 ve;
    u32 mode;
    s32 lace;

    mode = 0x08000000;
    if (gpuEnvState.m2_debug >= 2) {
        GPU_printf("PutDispEnv(%08x)...\n", env);
    }
    pGpuDriver->m10_ctl(0x05000000 | ((gpuEnvState.m0_type == 1 || gpuEnvState.m0_type == 2)
                                          ? ((env->disp.y & 0xFFF) << 12) | (get_dx(env) & 0xFFF)
                                          : ((env->disp.y & 0x3FF) << 10) | (env->disp.x & 0x3FF)));
    if (gpuEnvState.m6C_disp.screen.x != env->screen.x || gpuEnvState.m6C_disp.screen.y != env->screen.y ||
        gpuEnvState.m6C_disp.screen.w != env->screen.w || gpuEnvState.m6C_disp.screen.h != env->screen.h) {
        env->pad0 = GetVideoMode();
        hs = env->screen.x * 10 + 0x260;
        vs = env->screen.y + (env->pad0 ? 0x13 : 0x10);
        he = env->screen.w ? hs + env->screen.w * 10 : hs + 0xA00;
        ve = env->screen.h ? vs + env->screen.h : vs + 0xF0;
        hs = LIMIT(hs, 500, 3290);
        he = LIMIT(he, hs + 80, 3290);
        vs = LIMIT(vs, 16, env->pad0 ? 310 : 256);
        ve = LIMIT(ve, vs + 2, env->pad0 ? 312 : 258);
        pGpuDriver->m10_ctl(0x06000000 | ((he & 0xFFF) << 12) | (hs & 0xFFF));
        pGpuDriver->m10_ctl(0x07000000 | ((ve & 0x3FF) << 10) | (vs & 0x3FF));
    }
    if (*(u32*)&gpuEnvState.m6C_disp.isinter != *(u32*)&env->isinter ||
        gpuEnvState.m6C_disp.disp.x != env->disp.x || gpuEnvState.m6C_disp.disp.y != env->disp.y ||
        gpuEnvState.m6C_disp.disp.w != env->disp.w || gpuEnvState.m6C_disp.disp.h != env->disp.h) {
        env->pad0 = GetVideoMode();
        if (env->pad0 == 1) {
            mode |= 8;
        }
        if (env->isrgb24) {
            mode |= 0x10;
        }
        if (env->isinter) {
            mode |= 0x20;
        }
        if (gpuEnvState.m3_reverse) {
            mode |= 0x80;
        }
        if (env->disp.w > 280) {
            if (env->disp.w <= 352) {
                mode |= 1;
            } else if (env->disp.w <= 400) {
                mode |= 0x40;
            } else if (env->disp.w <= 560) {
                mode |= 2;
            } else {
                mode |= 3;
            }
        }
        lace = env->disp.h <= (env->pad0 ? 288 : 256);
        if (!lace) {
            mode |= 0x24;
        }
        pGpuDriver->m10_ctl(mode);
    }
    memcpy((void*)&gpuEnvState.m6C_disp, env, sizeof(DISPENV));
    return env;
}

DISPENV* GetDispEnv(DISPENV* env) {
    memcpy(env, &gpuEnvState.m6C_disp, sizeof(DISPENV));
    return env;
}

u32 GetODE(void) {
    return pGpuDriver->m38_status() >> 31;
}

typedef struct {
    u32 tag;
    u32 code[2];
} DR_PAIR;

u32 get_mode(s32 dfe, s32 dtd, u16 tpage);
u32 get_cs(s16 x, s16 y);
u32 get_ce(s16 x, s16 y);
u32 get_ofs(s16 x, s16 y);
u32 get_tw(RECT* tw);

void SetTexWindow(DR_PAIR* p, RECT* tw) {
    setlen(p, 2);
    p->code[0] = get_tw(tw);
    p->code[1] = 0;
}

void SetDrawArea(DR_PAIR* p, RECT* r) {
    setlen(p, 2);
    p->code[0] = get_cs(r->x, r->y);
    p->code[1] = get_ce(r->x + r->w - 1, r->y + r->h - 1);
}

void SetDrawOffset(DR_PAIR* p, s16* ofs) {
    setlen(p, 2);
    p->code[0] = get_ofs(ofs[0], ofs[1]);
    p->code[1] = 0;
}

void SetPriority(DR_PAIR* p, s32 pbc, s32 pbw) {
    u32 code;

    setlen(p, 2);
    code = pbc ? 0xE6000002 : 0xE6000000;
    if (pbw) {
        code |= 1;
    }
    p->code[0] = code;
    p->code[1] = 0;
}

void SetDrawMode(DR_PAIR* p, s32 dfe, s32 dtd, s32 tpage, RECT* tw) {
    setlen(p, 2);
    p->code[0] = get_mode(dfe, dtd, tpage);
    p->code[1] = get_tw(tw);
}


void SetDrawEnv(DR_ENV* p, DRAWENV* env) {
    DR_ENV* dr;
    RECT r;
    s32 n;

    dr = p;

    dr->code[0] = get_cs(env->clip.x, env->clip.y);
    dr->code[1] = get_ce(env->clip.w + env->clip.x - 1, env->clip.y + env->clip.h - 1);
    dr->code[2] = get_ofs(env->ofs[0], env->ofs[1]);
    dr->code[3] = get_mode(env->dfe, env->dtd, env->tpage);
    dr->code[4] = get_tw(&env->tw);
    dr->code[5] = 0xE6000000;
    n = 7;
    if (env->isbg) {
        r.x = env->clip.x;
        r.y = env->clip.y;
        r.w = env->clip.w;
        r.h = env->clip.h;
        r.w = LIMIT(r.w, 0, gpuEnvState.m4_w - 1);
        r.h = LIMIT(r.h, 0, gpuEnvState.m6_h - 1);
        r.x -= env->ofs[0];
        r.y -= env->ofs[1];
        ((u32*)dr)[n++] = 0x60000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
        ((u32*)dr)[n++] = *(u32*)&r.x;
        ((u32*)dr)[n++] = *(u32*)&r.w;
        r.x += env->ofs[0];
        r.y += env->ofs[1];
    }
    setlen(dr, n - 1);
}

void SetDrawEnv2(DR_ENV* p, DRAWENV* env) {
    DR_ENV* dr;
    RECT r;
    s32 n;

    dr = p;

    dr->code[0] = get_cs(env->clip.x, env->clip.y);
    dr->code[1] = get_ce(env->clip.w + env->clip.x - 1, env->clip.y + env->clip.h - 1);
    dr->code[2] = get_ofs(env->ofs[0], env->ofs[1]);
    dr->code[3] = get_mode(env->dfe, env->dtd, env->tpage);
    dr->code[4] = get_tw(&env->tw);
    dr->code[5] = 0xE6000000;
    n = 7;
    if (env->isbg) {
        r.x = env->clip.x;
        r.y = env->clip.y;
        r.w = env->clip.w;
        r.h = env->clip.h;
        r.w = LIMIT(r.w, 0, gpuEnvState.m4_w - 1);
        r.h = LIMIT(r.h, 0, gpuEnvState.m6_h - 1);
        if ((r.x & 0x3F) || (r.w & 0x3F)) {
            r.x -= env->ofs[0];
            r.y -= env->ofs[1];
            ((u32*)dr)[n++] = 0x60000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
            ((u32*)dr)[n++] = *(u32*)&r.x;
            ((u32*)dr)[n++] = *(u32*)&r.w;
            r.x += env->ofs[0];
            r.y += env->ofs[1];
        } else {
            ((u32*)dr)[n++] = 0x02000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
            ((u32*)dr)[n++] = *(u32*)&r.x;
            ((u32*)dr)[n++] = *(u32*)&r.w;
        }
    }
    setlen(dr, n - 1);
}

u32 get_mode(s32 dfe, s32 dtd, u16 tpage) {
    if (gpuEnvState.m0_type == 1 || gpuEnvState.m0_type == 2) {
        return 0xE1000000 | (dtd ? 0x800 : 0) | (dfe ? 0x1000 : 0) | (tpage & 0x27FF);
    } else {
        return 0xE1000000 | (dtd ? 0x200 : 0) | (dfe ? 0x400 : 0) | (tpage & 0x9FF);
    }
}

u32 get_cs(s16 x, s16 y) {
    x = LIMIT(x, 0, gpuEnvState.m4_w - 1);
    y = LIMIT(y, 0, gpuEnvState.m6_h - 1);
    if (gpuEnvState.m0_type == 1 || gpuEnvState.m0_type == 2) {
        return 0xE3000000 | ((y & 0xFFF) << 12) | (x & 0xFFF);
    } else {
        return 0xE3000000 | ((y & 0x3FF) << 10) | (x & 0x3FF);
    }
}

u32 get_ce(s16 x, s16 y) {
    x = LIMIT(x, 0, gpuEnvState.m4_w - 1);
    y = LIMIT(y, 0, gpuEnvState.m6_h - 1);
    if (gpuEnvState.m0_type == 1 || gpuEnvState.m0_type == 2) {
        return 0xE4000000 | ((y & 0xFFF) << 12) | (x & 0xFFF);
    } else {
        return 0xE4000000 | ((y & 0x3FF) << 10) | (x & 0x3FF);
    }
}

u32 get_ofs(s16 x, s16 y) {
    if (gpuEnvState.m0_type == 1 || gpuEnvState.m0_type == 2) {
        return 0xE5000000 | ((y & 0xFFF) << 12) | (x & 0xFFF);
    } else {
        return 0xE5000000 | ((y & 0x7FF) << 11) | (x & 0x7FF);
    }
}

u32 get_tw(RECT* tw) {
    u32 pat[4];

    if (tw) {
        pat[0] = (tw->x & 0xFF) >> 3;
        pat[2] = (-tw->w & 0xFF) >> 3;
        pat[1] = (tw->y & 0xFF) >> 3;
        pat[3] = (-tw->h & 0xFF) >> 3;
        return 0xE2000000 | (pat[1] << 15) | (pat[0] << 10) | (pat[3] << 5) | pat[2];
    }
    return 0;
}

s32 get_dx(DISPENV* env) {
    switch (gpuEnvState.m0_type) {
    case 1:
        if (gpuEnvState.m3_reverse) {
            return 0x400 - env->disp.x - env->disp.w;
        }
        break;
    case 2:
        return gpuEnvState.m3_reverse ? 0x400 - env->disp.x - env->disp.w / 2 : env->disp.x / 2;
    }
    return env->disp.x;
}

/* SYS.OBJ .data, second part (0x800569A0-0x800569F0): GPU / DMA register
 * pointers, then the queue state. */
volatile u32* gpuGp0Reg = (volatile u32*)0x1F801810; /* GP0 */
volatile u32* gpuGp1Reg = (volatile u32*)0x1F801814; /* GP1 */
volatile u32* gpuDma2MadrReg = (volatile u32*)0x1F8010A0; /* DMA2 MADR */
volatile u32* gpuDma2BcrReg = (volatile u32*)0x1F8010A4; /* DMA2 BCR */
volatile u32* gpuDma2ChcrReg = (volatile u32*)0x1F8010A8; /* DMA2 CHCR */
volatile u32* gpuDma6MadrReg = (volatile u32*)0x1F8010E0; /* DMA6 MADR */
volatile u32* gpuDma6BcrReg = (volatile u32*)0x1F8010E4; /* DMA6 BCR */
volatile u32* gpuDma6ChcrReg = (volatile u32*)0x1F8010E8; /* DMA6 CHCR */
volatile u32* gpuDpcrReg = (volatile u32*)0x1F8010F0; /* DPCR */

typedef struct {
    /* 0x0 */ s32 (*m0_func)();
    /* 0x4 */ void* m4_param;
    /* 0x8 */ s32 m8_arg;
} GpuCall;

typedef struct {
    /* 0x00 */ s32 (*m0_func)();
    /* 0x04 */ void* m4_param;
    /* 0x08 */ s32 m8_arg;
    /* 0x0C */ u32 mC_buf[21];
} GpuQueue; /* 0x60 */

volatile GpuCall gpuLastQueueCall = {0}; /* last executed command */
s32 D_800569D0 = 0;                /* unreferenced */
volatile s32 D_800569D4 = 0;       /* queue write index */
volatile s32 D_800569D8 = 0;       /* queue read index */
s32 gpuAddqueSavedMask = 0;
s32 gpuExequeSavedMask = 0;
s32 gpuResetSavedMask = 0;
s32 gpuAlarmDeadline = 0; /* timeout vsync */
s32 gpuAlarmCount = 0; /* timeout counter */
static u32 clearImagePacket[9];
static u32 clearImageEnvPacket[8];
static u8 gpuCtlShadow[0x100];
extern volatile GpuQueue _que[64];

s32 SetIntrMask(s32 mask);
s32 VSync(s32 mode);
void set_alarm(void);
s32 get_alarm(void);
s32 _version(s32 mode);

u32 _status(void) {
    return *gpuGp1Reg;
}

s32 _otc(u32* ot, s32 n) {
    *gpuDpcrReg |= 0x08000000;
    *gpuDma6ChcrReg = 0;
    *gpuDma6MadrReg = (u32)&ot[n - 1];
    *gpuDma6BcrReg = n;
    *gpuDma6ChcrReg = 0x11000002;
    set_alarm();
    while (*gpuDma6ChcrReg & 0x01000000) {
        if (get_alarm()) {
            return -1;
        }
    }
    return n;
}

s32 _clr(RECT* rect, u32 color) {
    rect->w = LIMIT(rect->w, 0, gpuEnvState.m4_w - 1);
    rect->h = LIMIT(rect->h, 0, gpuEnvState.m6_h - 1);
    if ((rect->x & 0x3F) || (rect->w & 0x3F)) {
        clearImagePacket[0] = ((u32)clearImageEnvPacket & 0xFFFFFF) | 0x08000000;
        clearImagePacket[1] = 0xE3000000;
        clearImagePacket[2] = 0xE4FFFFFF;
        clearImagePacket[3] = 0xE5000000;
        clearImagePacket[4] = 0xE6000000;
        clearImagePacket[5] = 0xE1000000 | (*gpuGp1Reg & 0x7FF) | ((color >> 31) << 10);
        clearImagePacket[6] = 0x60000000 | (color & 0xFFFFFF);
        clearImagePacket[7] = *(u32*)&rect->x;
        clearImagePacket[8] = *(u32*)&rect->w;
        clearImageEnvPacket[0] = 0x03FFFFFF;
        clearImageEnvPacket[1] = _param(3) | 0xE3000000;
        clearImageEnvPacket[2] = _param(4) | 0xE4000000;
        clearImageEnvPacket[3] = _param(5) | 0xE5000000;
    } else {
        clearImagePacket[0] = 0x05FFFFFF;
        clearImagePacket[1] = 0xE6000000;
        clearImagePacket[2] = 0xE1000000 | (*gpuGp1Reg & 0x7FF) | ((color >> 31) << 10);
        clearImagePacket[3] = 0x02000000 | (color & 0xFFFFFF);
        clearImagePacket[4] = *(u32*)&rect->x;
        clearImagePacket[5] = *(u32*)&rect->w;
    }
    _cwc(clearImagePacket);
    return 0;
}

s32 _dws(RECT* rect, u32* p) {
    s32 size;
    s32 n;
    s32 blk;
    s32 rd;

    set_alarm();
    rd = 0;
    rect->w = LIMIT(rect->w, 0, gpuEnvState.m4_w);
    rect->h = LIMIT(rect->h, 0, gpuEnvState.m6_h);
    size = (rect->w * rect->h + 1) / 2;
    if (size <= 0) {
        return -1;
    }
    n = size % 16;
    blk = size / 16;
    while (!(*gpuGp1Reg & 0x04000000)) {
        if (get_alarm()) {
            return -1;
        }
    }
    *gpuGp1Reg = 0x04000000;
    *gpuGp0Reg = 0x01000000;
    *gpuGp0Reg = rd ? 0xB0000000 : 0xA0000000;
    *gpuGp0Reg = *(u32*)&rect->x;
    *gpuGp0Reg = *(u32*)&rect->w;
    for (n--; n != -1; n--) {
        *gpuGp0Reg = *p++;
    }
    if (blk) {
        *gpuGp1Reg = 0x04000002;
        *gpuDma2MadrReg = (u32)p;
        *gpuDma2BcrReg = (blk << 16) | 0x10;
        *gpuDma2ChcrReg = 0x01000201;
    }
    return 0;
}

s32 _drs(RECT* rect, u32* p) {
    s32 size;
    s32 n;
    s32 blk;

    set_alarm();
    rect->w = LIMIT(rect->w, 0, gpuEnvState.m4_w);
    rect->h = LIMIT(rect->h, 0, gpuEnvState.m6_h);
    size = (rect->w * rect->h + 1) / 2;
    if (size <= 0) {
        return -1;
    }
    n = size % 16;
    blk = size / 16;
    while (!(*gpuGp1Reg & 0x04000000)) {
        if (get_alarm()) {
            return -1;
        }
    }
    *gpuGp1Reg = 0x04000000;
    *gpuGp0Reg = 0x01000000;
    *gpuGp0Reg = 0xC0000000;
    *gpuGp0Reg = *(u32*)&rect->x;
    *gpuGp0Reg = *(u32*)&rect->w;
    while (!(*gpuGp1Reg & 0x08000000)) {
        if (get_alarm()) {
            return -1;
        }
    }
    for (n--; n != -1; n--) {
        *p++ = *gpuGp0Reg;
    }
    if (blk) {
        *gpuGp1Reg = 0x04000003;
        *gpuDma2MadrReg = (u32)p;
        *gpuDma2BcrReg = (blk << 16) | 0x10;
        *gpuDma2ChcrReg = 0x01000200;
    }
    return 0;
}

void _ctl(u32 cmd) {
    *gpuGp1Reg = cmd;
    gpuCtlShadow[cmd >> 24] = cmd;
}

s32 _getctl(s32 n) {
    return gpuCtlShadow[n];
}

s32 _cwb(u32* p, s32 n) {
    s32 i;

    *gpuGp1Reg = 0x04000000;
    for (i = n - 1; i != -1; i--) {
        *gpuGp0Reg = *p++;
    }
    return 0;
}

void _cwc(u32* p) {
    *gpuGp1Reg = 0x04000002;
    *gpuDma2MadrReg = (u32)p;
    *gpuDma2BcrReg = 0;
    *gpuDma2ChcrReg = 0x01000401;
}

s32 _param(s32 n) {
    *gpuGp1Reg = n | 0x10000000;
    return *gpuGp0Reg & 0xFFFFFF;
}

s32 _addque(s32 (*func)(), void* param, s32 arg) {
    return _addque2(func, param, 0, arg);
}

s32 _addque2(s32 (*func)(), void* param, s32 size, s32 arg) {
    s32 i;

    set_alarm();
    while (((D_800569D4 + 1) & 0x3F) == D_800569D8) {
        if (get_alarm()) {
            return -1;
        }
        _exeque();
    }
    gpuAddqueSavedMask = SetIntrMask(0);
    gpuEnvState.m8_drawSyncPending = 1;
    if (gpuEnvState.m1_queue == 0 ||
        (D_800569D4 == D_800569D8 && !(*gpuDma2ChcrReg & 0x01000000) && gpuEnvState.mC_drawSyncCb == 0)) {
        while (!(*gpuGp1Reg & 0x04000000)) {
        }
        func(param, arg);
        gpuLastQueueCall.m0_func = func;
        gpuLastQueueCall.m4_param = param;
        gpuLastQueueCall.m8_arg = arg;
        SetIntrMask(gpuAddqueSavedMask);
        return 0;
    }
    DMACallback(2, (void (*)())_exeque);
    if (size) {
        for (i = 0; i < size / 4; i++) {
            _que[D_800569D4].mC_buf[i] = ((u32*)param)[i];
        }
        _que[D_800569D4].m4_param = _que[D_800569D4].mC_buf;
    } else {
        _que[D_800569D4].m4_param = param;
    }
    _que[D_800569D4].m8_arg = arg;
    _que[D_800569D4].m0_func = func;
    D_800569D4 = (D_800569D4 + 1) & 0x3F;
    SetIntrMask(gpuAddqueSavedMask);
    _exeque();
    return (D_800569D4 - D_800569D8) & 0x3F;
}

s32 _exeque(void) {
    if (*gpuDma2ChcrReg & 0x01000000) {
        return 1;
    }
    gpuExequeSavedMask = SetIntrMask(0);
    if (D_800569D4 != D_800569D8 && !(*gpuDma2ChcrReg & 0x01000000)) {
        do {
            if (((D_800569D8 + 1) & 0x3F) == D_800569D4 && gpuEnvState.mC_drawSyncCb == 0) {
                DMACallback(2, 0);
            }
            while (!(*gpuGp1Reg & 0x04000000)) {
            }
            _que[D_800569D8].m0_func(_que[D_800569D8].m4_param, _que[D_800569D8].m8_arg);
            gpuLastQueueCall.m0_func = _que[D_800569D8].m0_func;
            gpuLastQueueCall.m4_param = _que[D_800569D8].m4_param;
            gpuLastQueueCall.m8_arg = _que[D_800569D8].m8_arg;
            D_800569D8 = (D_800569D8 + 1) & 0x3F;
        } while (D_800569D4 != D_800569D8 && !(*gpuDma2ChcrReg & 0x01000000));
    }
    SetIntrMask(gpuExequeSavedMask);
    if (D_800569D4 == D_800569D8 && !(*gpuDma2ChcrReg & 0x01000000) && gpuEnvState.m8_drawSyncPending &&
        gpuEnvState.mC_drawSyncCb) {
        gpuEnvState.m8_drawSyncPending = 0;
        gpuEnvState.mC_drawSyncCb();
    }
    return (D_800569D4 - D_800569D8) & 0x3F;
}

s32 _reset(s32 mode) {
    gpuResetSavedMask = SetIntrMask(0);
    D_800569D4 = D_800569D8 = 0;
    switch (mode & 7) {
    case 0:
    case 5:
        *gpuDma2ChcrReg = 0x401;
        *gpuDpcrReg |= 0x800;
        *gpuGp1Reg = 0;
        memset_sys(gpuCtlShadow, 0, sizeof(gpuCtlShadow));
        memset_sys(_que, 0, sizeof(_que));
        break;
    case 1:
    case 3:
        *gpuDma2ChcrReg = 0x401;
        *gpuDpcrReg |= 0x800;
        *gpuGp1Reg = 0x02000000;
        *gpuGp1Reg = 0x01000000;
        break;
    }
    SetIntrMask(gpuResetSavedMask);
    if (mode & 7) {
        return 0;
    }
    return _version(mode);
}

s32 _sync(s32 mode) {
    s32 n;

    if (mode == 0) {
        set_alarm();
        while (D_800569D4 != D_800569D8) {
            _exeque();
            if (get_alarm()) {
                return -1;
            }
        }
        while ((*gpuDma2ChcrReg & 0x01000000) || !(*gpuGp1Reg & 0x04000000)) {
            if (get_alarm()) {
                return -1;
            }
        }
        return 0;
    }
    n = (D_800569D4 - D_800569D8) & 0x3F;
    if (n) {
        _exeque();
    }
    if ((*gpuDma2ChcrReg & 0x01000000) || !(*gpuGp1Reg & 0x04000000)) {
        if (n == 0) {
            return 1;
        }
        return n;
    }
    return n;
}

void set_alarm(void) {
    gpuAlarmDeadline = VSync(-1) + 240;
    gpuAlarmCount = 0;
}

s32 get_alarm(void) {
    if (gpuAlarmDeadline < VSync(-1) || gpuAlarmCount++ > 0xF0000) {
        _status();
        printfStub("GPU timeout:que=%d,stat=%08x,chcr=%08x,madr=%08x,", (D_800569D4 - D_800569D8) & 0x3F,
                      *gpuGp1Reg, *gpuDma2ChcrReg, *gpuDma2MadrReg);
        printfStub("func=(%08x)(%08x,%08x)\n", gpuLastQueueCall.m0_func, gpuLastQueueCall.m4_param, gpuLastQueueCall.m8_arg);
        _reset(1);
        return -1;
    }
    return 0;
}

s32 _version(s32 mode) {
    *gpuGp1Reg = 0x10000007;
    if ((*gpuGp0Reg & 0xFFFFFF) != 2) {
        *gpuGp0Reg = (*gpuGp1Reg & 0x3FFF) | 0xE1001000;
        *gpuGp0Reg;
        if (!(*gpuGp1Reg & 0x1000)) {
            return 0;
        }
        if (!(mode & 8)) {
            return 1;
        }
        *gpuGp1Reg = 0x20000504;
        return 2;
    }
    if (!(mode & 8)) {
        return 3;
    }
    *gpuGp1Reg = 0x09000001;
    return 4;
}

void memset_sys(u8* p, s32 c, s32 n) {
    s32 i;

    for (i = n - 1; i != -1; i--) {
        *p++ = c;
    }
}
