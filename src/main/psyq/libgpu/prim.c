/*
 * PSY-Q 4.0 LIBGPU.LIB PRIM.OBJ: primitive setters and environment dumps. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq.h"

typedef struct {
    u32 tag;
    u32 code[3];
    u32 p[13];
} DR_LOAD;

extern s32 (*GPU_printf)(char*, ...);

u16 GetTPage(s32 tp, s32 abr, s32 x, s32 y) {
    return getTPage(tp, abr, x, y);
}

u16 GetClut(s32 x, s32 y) {
    return (y << 6) | ((x >> 4) & 0x3F);
}

void DumpTPage(u16 tpage) {
    GPU_printf("tpage: (%d,%d,%d,%d)\n", (tpage >> 7) & 3, (tpage >> 5) & 3, (tpage << 6) & 0x7C0,
               ((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200));
}

void DumpClut(u16 clut) {
    GPU_printf("clut: (%d,%d)\n", (clut & 0x3F) << 4, clut >> 6);
}

void* NextPrim(void* p) {
    return (void*)((*(u32*)p & 0xFFFFFF) | 0x80000000);
}

s32 IsEndPrim(void* p) {
    return (*(u32*)p & 0xFFFFFF) == 0xFFFFFF;
}

void AddPrim(void* ot, void* p) {
    addPrim(ot, p);
}

void AddPrims(void* ot, void* p0, void* p1) {
    setaddr(p1, getaddr(ot));
    setaddr(ot, p0);
}

void CatPrim(void* p0, void* p1) {
    *(u32*)p0 = (*(u32*)p0 & 0xFF000000) | ((u32)p1 & 0xFFFFFF);
}

void TermPrim(void* p) {
    *(u32*)p |= 0xFFFFFF;
}

void SetSemiTrans(void* p, s32 abe) {
    if (abe) {
        ((P_TAG*)p)->code |= 2;
    } else {
        ((P_TAG*)p)->code &= ~2;
    }
}

void SetShadeTex(void* p, s32 tge) {
    if (tge) {
        ((P_TAG*)p)->code |= 1;
    } else {
        ((P_TAG*)p)->code &= ~1;
    }
}

void SetPolyF3(void* p) { setlen(p, 4); setcode(p, 0x20); }
void SetPolyFT3(void* p) { setlen(p, 7); setcode(p, 0x24); }
void SetPolyG3(void* p) { setlen(p, 6); setcode(p, 0x30); }
void SetPolyGT3(void* p) { setlen(p, 9); setcode(p, 0x34); }
void SetPolyF4(void* p) { setlen(p, 5); setcode(p, 0x28); }
void SetPolyFT4(void* p) { setlen(p, 9); setcode(p, 0x2C); }
void SetPolyG4(void* p) { setlen(p, 8); setcode(p, 0x38); }
void SetPolyGT4(void* p) { setlen(p, 12); setcode(p, 0x3C); }
void SetSprt8(void* p) { setlen(p, 3); setcode(p, 0x74); }
void SetSprt16(void* p) { setlen(p, 3); setcode(p, 0x7C); }
void SetSprt(void* p) { setlen(p, 4); setcode(p, 0x64); }
void SetTile1(void* p) { setlen(p, 2); setcode(p, 0x68); }
void SetTile8(void* p) { setlen(p, 2); setcode(p, 0x70); }
void SetTile16(void* p) { setlen(p, 2); setcode(p, 0x78); }
void SetTile(void* p) { setlen(p, 3); setcode(p, 0x60); }
void SetLineF2(void* p) { setlen(p, 3); setcode(p, 0x40); }
void SetLineG2(void* p) { setlen(p, 4); setcode(p, 0x50); }

void SetLineF3(void* p) {
    setlen(p, 5);
    setcode(p, 0x48);
    ((u32*)p)[5] = 0x55555555;
}

void SetLineG3(void* p) {
    setlen(p, 7);
    setcode(p, 0x58);
    ((u32*)p)[7] = 0x55555555;
}

void SetLineF4(void* p) {
    setlen(p, 6);
    setcode(p, 0x4C);
    ((u32*)p)[6] = 0x55555555;
}

void SetLineG4(void* p) {
    setlen(p, 9);
    setcode(p, 0x5C);
    ((u32*)p)[9] = 0x55555555;
}

void SetDrawTPage(DR_TPAGE* p, s32 dfe, s32 dtd, s32 tpage) {
    setlen(p, 1);
    p->code[0] = 0xE1000000 | (dtd ? 0x200 : 0) | (dfe ? 0x400 : 0) | (tpage & 0x9FF);
}

void SetDrawMove(DR_MOVE* p, RECT* rect, s32 x, s32 y) {
    s32 len;

    len = 5;
    if (rect->w == 0 || rect->h == 0) {
        len = 0;
    }
    setlen(p, len);
    p->code[0] = 0x01000000;
    p->code[1] = 0x80000000;
    p->code[2] = *(u32*)&rect->x;
    p->code[3] = (y << 16) | (x & 0xFFFF);
    p->code[4] = *(u32*)&rect->w;
}

void SetDrawLoad(DR_LOAD* p, RECT* rect) {
    u32 size;
    s32 len;

    size = (rect->w * rect->h + 1) / 2;
    len = size + 4;
    if (size > 12) {
        len = 0;
    }
    setlen(p, len);
    p->code[0] = 0xA0000000;
    p->code[1] = *(u32*)&rect->x;
    p->code[2] = *(u32*)&rect->w;
    p->p[len - 4] = 0x01000000;
}

s32 MargePrim(void* p0, void* p1) {
    s32 len;

    len = ((P_TAG*)p0)->len + ((P_TAG*)p1)->len + 1;
    if (len > 16) {
        return -1;
    }
    setlen(p0, len);
    *(u32*)p1 = 0;
    return 0;
}

void DumpDrawEnv(DRAWENV* env) {
    GPU_printf("clip (%3d,%3d)-(%d,%d)\n", env->clip.x, env->clip.y, env->clip.w, env->clip.h);
    GPU_printf("ofs  (%3d,%3d)\n", env->ofs[0], env->ofs[1]);
    GPU_printf("tw   (%d,%d)-(%d,%d)\n", env->tw.x, env->tw.y, env->tw.w, env->tw.h);
    GPU_printf("dtd   %d\n", env->dtd);
    GPU_printf("dfe   %d\n", env->dfe);
    DumpTPage(env->tpage);
}

void DumpDispEnv(DISPENV* env) {
    GPU_printf("disp   (%3d,%3d)-(%d,%d)\n", env->disp.x, env->disp.y, env->disp.w, env->disp.h);
    GPU_printf("screen (%3d,%3d)-(%d,%d)\n", env->screen.x, env->screen.y, env->screen.w,
               env->screen.h);
    GPU_printf("isinter %d\n", env->isinter);
    GPU_printf("isrgb24 %d\n", env->isrgb24);
}
