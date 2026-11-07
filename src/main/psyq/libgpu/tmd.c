/*
 * PSY-Q 4.0 LIBGPU TMD.OBJ: TIM/TMD stream readers (OpenTIM, ReadTIM,
 * OpenTMD, ReadTMD) and the TMD packet unpacker. Our own C for the library
 * routines (clean-room, from the disassembly). GCC 2.7.2 (PSY-Q), -G0,
 * ASPSX 2.56.
 */
#include "common.h"
#include "psyq.h"

#define B(o) (((u8*)packet)[o])
#define H(o) (*(u16*)((u8*)packet + (o)))
#define RGB(n, o) prim->r##n = B(o); prim->g##n = B(o + 1); prim->b##n = B(o + 2)
#define TEX3() \
    prim->tpage = H(0xA); prim->clut = H(6); \
    prim->u0 = B(4); prim->v0 = B(5); prim->u1 = B(8); prim->v1 = B(9); \
    prim->u2 = B(0xC); prim->v2 = B(0xD)
#define TEX4() TEX3(); prim->u3 = B(0x10); prim->v3 = B(0x11)
#define DEBUG(s) if (GetGraphDebug() == 2) { printfStub(s); }

typedef struct {
    /* 0x00 */ u32 id;
    /* 0x04 */ u8 r0, g0, b0, p0;
    /* 0x08 */ u8 r1, g1, b1, p1;
    /* 0x0C */ u8 r2, g2, b2, p2;
    /* 0x10 */ u8 r3, g3, b3, p3;
    /* 0x14 */ u16 tpage, clut;
    /* 0x18 */ u8 u0, v0, u1, v1;
    /* 0x1C */ u8 u2, v2, u3, v3;
    /* 0x20 */ SVECTOR x0, x1, x2, x3;
    /* 0x40 */ SVECTOR n0, n1, n2, n3;
    /* 0x60 */ SVECTOR* v_ofs;
    /* 0x64 */ SVECTOR* n_ofs;
    /* 0x68 */ u16 vert0, vert1, vert2, vert3;
    /* 0x70 */ u16 norm0, norm1, norm2, norm3;
} TMD_PRIM; /* 0x78 */

typedef struct {
    /* 0x00 */ u32 vert_top;
    /* 0x04 */ u32 n_vert;
    /* 0x08 */ u32 normal_top;
    /* 0x0C */ u32 n_normal;
    /* 0x10 */ u32 primitive_top;
    /* 0x14 */ u32 n_primitive;
    /* 0x18 */ s32 scale;
} TMD_OBJ; /* 0x1C */

extern s32 GetGraphDebug(void);
extern void printfStub();
extern void* memset(void* dst, s32 c, s32 n);
s32 get_tim_addr(u32* tim, TIM_IMAGE* timimg);
s32 get_tmd_addr(u32* tmd, s32 obj_no, u32** prim, SVECTOR** vert, SVECTOR** norm);
s32 unpack_packet(u32* packet, TMD_PRIM* prim);
static u32* timStreamPtr;
static SVECTOR* tmdVertexTop;
static SVECTOR* tmdNormalTop;
static u32* tmdPrimPtr;
static s32 tmdPrimCount;
static s32 D_8005A390[3];  /* unreferenced: 0xC bytes before the COMMON area */

s32 OpenTIM(u32* addr) {
    timStreamPtr = addr;
    return 0;
}

TIM_IMAGE* ReadTIM(TIM_IMAGE* timimg) {
    s32 n;

    n = get_tim_addr(timStreamPtr, timimg);
    if (n == -1) {
        return 0;
    }
    timStreamPtr += n;
    return timimg;
}

s32 OpenTMD(u32* tmd, s32 obj_no) {
    return tmdPrimCount = get_tmd_addr(tmd, obj_no, &tmdPrimPtr, &tmdVertexTop, &tmdNormalTop);
}

TMD_PRIM* ReadTMD(TMD_PRIM* tmdprim) {
    s32 n;

    n = unpack_packet(tmdPrimPtr, tmdprim);
    if (n < 0) {
        return 0;
    }
    tmdprim->v_ofs = tmdVertexTop;
    tmdprim->n_ofs = tmdNormalTop;
    tmdprim->n0.vx = tmdNormalTop[tmdprim->norm0].vx;
    tmdprim->n0.vy = tmdNormalTop[tmdprim->norm0].vy;
    tmdprim->n0.vz = tmdNormalTop[tmdprim->norm0].vz;
    tmdprim->n1.vx = tmdNormalTop[tmdprim->norm1].vx;
    tmdprim->n1.vy = tmdNormalTop[tmdprim->norm1].vy;
    tmdprim->n1.vz = tmdNormalTop[tmdprim->norm1].vz;
    tmdprim->n2.vx = tmdNormalTop[tmdprim->norm2].vx;
    tmdprim->n2.vy = tmdNormalTop[tmdprim->norm2].vy;
    tmdprim->n2.vz = tmdNormalTop[tmdprim->norm2].vz;
    tmdPrimPtr = (u32*)((u8*)tmdPrimPtr + n);
    tmdprim->n3.vx = tmdNormalTop[tmdprim->norm3].vx;
    tmdprim->n3.vy = tmdNormalTop[tmdprim->norm3].vy;
    tmdprim->n3.vz = tmdNormalTop[tmdprim->norm3].vz;
    tmdprim->x0.vx = tmdVertexTop[tmdprim->vert0].vx;
    tmdprim->x0.vy = tmdVertexTop[tmdprim->vert0].vy;
    tmdprim->x0.vz = tmdVertexTop[tmdprim->vert0].vz;
    tmdprim->x1.vx = tmdVertexTop[tmdprim->vert1].vx;
    tmdprim->x1.vy = tmdVertexTop[tmdprim->vert1].vy;
    tmdprim->x1.vz = tmdVertexTop[tmdprim->vert1].vz;
    tmdprim->x2.vx = tmdVertexTop[tmdprim->vert2].vx;
    tmdprim->x2.vy = tmdVertexTop[tmdprim->vert2].vy;
    tmdprim->x2.vz = tmdVertexTop[tmdprim->vert2].vz;
    tmdprim->x3.vx = tmdVertexTop[tmdprim->vert3].vx;
    tmdprim->x3.vy = tmdVertexTop[tmdprim->vert3].vy;
    tmdprim->x3.vz = tmdVertexTop[tmdprim->vert3].vz;
    return tmdprim;
}

s32 get_tim_addr(u32* tim, TIM_IMAGE* timimg) {
    s32 n;
    s32 m;

    if (*tim++ != 0x10) {
        return -1;
    }
    timimg->mode = *tim++;
    if (GetGraphDebug() == 2) {
        printfStub("id  =%08x\n", 0x10);
    }
    if (GetGraphDebug() == 2) {
        printfStub("mode=%08x\n", timimg->mode);
    }
    if (GetGraphDebug() == 2) {
        printfStub("timaddr=%08x\n", tim);
    }
    if (timimg->mode & 8) {
        n = *tim >> 2;
        timimg->crect = (RECT*)(tim + 1);
        timimg->caddr = tim + 3;
        tim += n;
    } else {
        n = 0;
        timimg->crect = 0;
        timimg->caddr = 0;
    }
    m = *tim >> 2;
    timimg->prect = (RECT*)(tim + 1);
    timimg->paddr = tim + 3;
    return n + 2 + m;
}

s32 get_tmd_addr(u32* tmd, s32 obj_no, u32** prim, SVECTOR** vert, SVECTOR** norm) {
    TMD_OBJ* obj;

    obj = (TMD_OBJ*)(tmd + 3);
    if (GetGraphDebug() == 2) {
        printfStub("analizing TMD...\n");
    }
    if (GetGraphDebug() == 2) {
        printfStub("\tid=%08X, flags=%d, nobj=%d, objid=%d\n", tmd[0], tmd[1], tmd[2], obj_no);
    }
    if (GetGraphDebug() == 2) {
        printfStub("\tvert=%08X, nvert=%d\n", obj[obj_no].vert_top, obj[obj_no].n_vert);
    }
    if (GetGraphDebug() == 2) {
        printfStub("\tnorm=%08X, nnorm=%d\n", obj[obj_no].normal_top, obj[obj_no].n_normal);
    }
    if (GetGraphDebug() == 2) {
        printfStub("\tprim=%08X, nprim=%d\n", obj[obj_no].primitive_top, obj[obj_no].n_primitive);
    }
    *vert = (SVECTOR*)((u8*)obj + obj[obj_no].vert_top);
    *norm = (SVECTOR*)((u8*)obj + obj[obj_no].normal_top);
    *prim = (u32*)((u8*)obj + obj[obj_no].primitive_top);
    return obj[obj_no].n_primitive;
}

s32 unpack_packet(u32* packet, TMD_PRIM* prim) {
    memset(prim, 0, sizeof(TMD_PRIM));
    prim->id = *packet;
    switch (prim->id & 0xFDFFFFFF) {
    case 0x20000304:
    case 0x20020304:
        DEBUG("F3L ");
        RGB(0, 4); RGB(1, 4); RGB(2, 4);
        prim->vert0 = H(0xA); prim->vert1 = H(0xC); prim->vert2 = H(0xE);
        prim->norm0 = H(8); prim->norm1 = H(8); prim->norm2 = H(8);
        return 0x10;
    case 0x30000406:
    case 0x30020406:
        DEBUG("G3L ");
        RGB(0, 4); RGB(1, 4); RGB(2, 4);
        prim->vert0 = H(0xA); prim->vert1 = H(0xE); prim->vert2 = H(0x12);
        prim->norm0 = H(8); prim->norm1 = H(0xC); prim->norm2 = H(0x10);
        return 0x14;
    case 0x24000507:
    case 0x24020507:
        DEBUG("FT3L ");
        TEX3();
        prim->vert0 = H(0x12); prim->vert1 = H(0x14); prim->vert2 = H(0x16);
        prim->norm0 = H(0x10); prim->norm1 = H(0x10); prim->norm2 = H(0x10);
        return 0x18;
    case 0x34000609:
    case 0x34020609:
        DEBUG("GT3L ");
        TEX3();
        prim->vert0 = H(0x12); prim->vert1 = H(0x16); prim->vert2 = H(0x1A);
        prim->norm0 = H(0x10); prim->norm1 = H(0x14); prim->norm2 = H(0x18);
        return 0x1C;
    case 0x21010304:
    case 0x21030304:
        DEBUG("F3 ");
        RGB(0, 4); RGB(1, 4); RGB(2, 4);
        prim->vert0 = H(8); prim->vert1 = H(0xA); prim->vert2 = H(0xC);
        return 0x10;
    case 0x31010506:
    case 0x31030506:
        DEBUG("G3 ");
        RGB(0, 4); RGB(1, 8); RGB(2, 0xC);
        prim->vert0 = H(0x10); prim->vert1 = H(0x12); prim->vert2 = H(0x14);
        return 0x18;
    case 0x25010607:
    case 0x25030607:
        DEBUG("FT3 ");
        TEX3();
        RGB(0, 0x10); RGB(1, 0x10); RGB(2, 0x10);
        prim->vert0 = H(0x14); prim->vert1 = H(0x16); prim->vert2 = H(0x18);
        return 0x1C;
    case 0x35010809:
    case 0x35030809:
        DEBUG("GT3 ");
        TEX3();
        prim->vert0 = H(0x1C); prim->vert1 = H(0x1E); prim->vert2 = H(0x20);
        RGB(0, 0x10); RGB(1, 0x14); RGB(2, 0x18);
        return 0x24;
    case 0x28000405:
    case 0x28020405:
        DEBUG("F4L ");
        RGB(0, 4); RGB(1, 4); RGB(2, 4); RGB(3, 4);
        prim->vert0 = H(0xA); prim->vert1 = H(0xC); prim->vert2 = H(0xE); prim->vert3 = H(0x10);
        prim->norm0 = H(8); prim->norm1 = H(8); prim->norm2 = H(8); prim->norm3 = H(8);
        return 0x14;
    case 0x38000508:
    case 0x38020508:
        DEBUG("G4L ");
        RGB(0, 4); RGB(1, 4); RGB(2, 4); RGB(3, 4);
        prim->vert0 = H(0xA); prim->vert1 = H(0xE); prim->vert2 = H(0x12); prim->vert3 = H(0x16);
        prim->norm0 = H(8); prim->norm1 = H(0xC); prim->norm2 = H(0x10); prim->norm3 = H(0x14);
        return 0x18;
    case 0x2C000709:
    case 0x2C020709:
        DEBUG("FT4L ");
        TEX4();
        prim->vert0 = H(0x16); prim->vert1 = H(0x18); prim->vert2 = H(0x1A); prim->vert3 = H(0x1C);
        prim->norm0 = H(0x14); prim->norm1 = H(0x14); prim->norm2 = H(0x14); prim->norm3 = H(0x14);
        return 0x20;
    case 0x3C00080C:
    case 0x3C02080C:
        DEBUG("GT4L ");
        TEX4();
        prim->vert0 = H(0x16); prim->vert1 = H(0x1A); prim->vert2 = H(0x1E); prim->vert3 = H(0x22);
        prim->norm0 = H(0x14); prim->norm1 = H(0x18); prim->norm2 = H(0x1C); prim->norm3 = H(0x20);
        return 0x24;
    case 0x29010305:
    case 0x29030305:
        DEBUG("F4 ");
        RGB(0, 4); RGB(1, 4); RGB(2, 4); RGB(3, 4);
        prim->vert0 = H(8); prim->vert1 = H(0xA); prim->vert2 = H(0xC); prim->vert3 = H(0xE);
        return 0x10;
    case 0x39010608:
    case 0x39030608:
        DEBUG("G4 ");
        RGB(0, 4); RGB(1, 8); RGB(2, 0xC); RGB(3, 0x10);
        prim->vert0 = H(0x14); prim->vert1 = H(0x16); prim->vert2 = H(0x18); prim->vert3 = H(0x1A);
        return 0x1C;
    case 0x2D010709:
    case 0x2D030709:
        DEBUG("FT4 ");
        TEX4();
        RGB(0, 0x14); RGB(1, 0x14); RGB(2, 0x14); RGB(3, 0x14);
        prim->vert0 = H(0x18); prim->vert1 = H(0x1A); prim->vert2 = H(0x1C); prim->vert3 = H(0x1E);
        return 0x20;
    case 0x3D010A0C:
    case 0x3D030A0C:
        DEBUG("GT4 ");
        TEX4();
        prim->vert0 = H(0x24); prim->vert1 = H(0x26); prim->vert2 = H(0x28); prim->vert3 = H(0x2A);
        RGB(0, 0x14); RGB(1, 0x18); RGB(2, 0x1C); RGB(3, 0x20);
        return 0x2C;
    default:
        printfStub("unsupported type (%08x)\n", prim->id & 0xFDFFFFFF);
        return -1;
    }
}
