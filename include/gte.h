#ifndef GTE_H
#define GTE_H

/*
 * Inline GTE (geometry coprocessor, COP2) macros in the style of PSY-Q's
 * inline_c.h. Some use $12-$14 as scratch registers, as PSY-Q's do. Data registers: V0 $0/$1, V1 $2/$3, V2 $4/$5, RGBC $6,
 * OTZ $7, IR0 $8, IR1-3 $9-$11, SXY0-2 $12-$14, SZ0-3 $16-$19,
 * RGB0-2 $20-$22, MAC0-3 $24-$27. Commands are issued after two nops, as
 * PSY-Q's macros do; the command mnemonics come from include/gte_macros.inc
 * (pulled in by maspsx --macro-inc).
 *
 * This is the one sanctioned use of inline asm in matched code (see
 * docs/PLAN.md).
 */

#ifdef PLATFORM_PC
#include "gte_pc.h"
#else

#define gte_copy_sxy(dst, src) ((void)0)

/* Loads */
#define gte_ldv0(r0) __asm__ volatile("lwc2 $0, 0(%0); lwc2 $1, 4(%0)" : : "r"(r0))
#define gte_ldv1(r0) __asm__ volatile("lwc2 $2, 0(%0); lwc2 $3, 4(%0)" : : "r"(r0))
#define gte_ldv2(r0) __asm__ volatile("lwc2 $4, 0(%0); lwc2 $5, 4(%0)" : : "r"(r0))
#define gte_ldv3(r0, r1, r2)                                              \
    __asm__ volatile("lwc2 $0, 0(%0); lwc2 $1, 4(%0); lwc2 $2, 0(%1); "   \
                     "lwc2 $3, 4(%1); lwc2 $4, 0(%2); lwc2 $5, 4(%2)"      \
                     : : "r"(r0), "r"(r1), "r"(r2))
/* V0 and V1 from two SVECTORs (inline_c.h gte_ldv01). */
#define gte_ldv01(r0, r1)                                                 \
    __asm__ volatile("lwc2 $0, 0(%0); lwc2 $1, 4(%0); lwc2 $2, 0(%1); "   \
                     "lwc2 $3, 4(%1)" : : "r"(r0), "r"(r1))
/* Three consecutive SVECTORs from one base (inline_c.h gte_ldv3c). */
#define gte_ldv3c(r0)                                                     \
    __asm__ volatile("lwc2 $0, 0(%0); lwc2 $1, 4(%0); lwc2 $2, 8(%0); "   \
                     "lwc2 $3, 12(%0); lwc2 $4, 16(%0); lwc2 $5, 20(%0)"  \
                     : : "r"(r0))
#define gte_ldrgb(r0) __asm__ volatile("lwc2 $6, 0(%0)" : : "r"(r0))
#define gte_ldlvl(r0) __asm__ volatile("lwc2 $9, 0(%0); lwc2 $10, 4(%0); lwc2 $11, 8(%0)" : : "r"(r0))
#define gte_lddp(r0) __asm__ volatile("mtc2 %0, $8" : : "r"(r0))
#define gte_ldIR0(r0) gte_lddp(r0)
#define gte_ldsxy3(r0, r1, r2)                                            \
    __asm__ volatile("mtc2 %0, $12; mtc2 %2, $14; mtc2 %1, $13"           \
                     : : "r"(r0), "r"(r1), "r"(r2))

/* Commands */
#define gte_rtps() __asm__ volatile("nop; nop; rtps")
#define gte_rtpt() __asm__ volatile("nop; nop; rtpt")
#define gte_nclip() __asm__ volatile("nop; nop; nclip")
#define gte_avsz3() __asm__ volatile("nop; nop; avsz3")
#define gte_avsz4() __asm__ volatile("nop; nop; avsz4")
#define gte_gpf0() __asm__ volatile("nop; nop; gpf 0")
#define gte_gpf12() __asm__ volatile("nop; nop; gpf 1")
#define gte_gpl0() __asm__ volatile("nop; nop; gpl 0")
#define gte_gpl12() __asm__ volatile("nop; nop; gpl 1")
#define gte_ncds() __asm__ volatile("nop; nop; ncds")
#define gte_nccs() __asm__ volatile("nop; nop; nccs")
#define gte_ncs() __asm__ volatile("nop; nop; ncs")
#define gte_dpcs() __asm__ volatile("nop; nop; dpcs")
#define gte_intpl() __asm__ volatile("nop; nop; intpl")
#define gte_sqr0() __asm__ volatile("nop; nop; sqr 0")
#define gte_sqr12() __asm__ volatile("nop; nop; sqr 1")
/* MVMVA: rotate V0 by the rotation matrix only (RT * V0, no translation, sf=1). */
#define gte_rtv0() __asm__ volatile("nop; nop; mvmva 1, 0, 0, 3, 0")

/* Stores */
#define gte_stsxy(r0) __asm__ volatile("swc2 $14, 0(%0)" : : "r"(r0) : "memory")
#define gte_stsxy3(r0, r1, r2)                                            \
    __asm__ volatile("swc2 $12, 0(%0); swc2 $13, 0(%1); swc2 $14, 0(%2)" \
                     : : "r"(r0), "r"(r1), "r"(r2) : "memory")
/* SXY0 and SXY1 (inline_c.h gte_stsxy01). */
#define gte_stsxy01(r0, r1)                                               \
    __asm__ volatile("swc2 $12, 0(%0); swc2 $13, 0(%1)"                   \
                     : : "r"(r0), "r"(r1) : "memory")
/* SXY0-2 into a POLY_FT4's x0/x1/x2 (inline_c.h gte_stsxy3_ft4). */
#define gte_stsxy3_ft4(r0)                                                \
    __asm__ volatile("swc2 $12, 8(%0); swc2 $13, 16(%0); swc2 $14, 24(%0)" \
                     : : "r"(r0) : "memory")
#define gte_stotz(r0) __asm__ volatile("swc2 $7, 0(%0)" : : "r"(r0) : "memory")
#define gte_stopz(r0) __asm__ volatile("swc2 $24, 0(%0)" : : "r"(r0) : "memory")
#define gte_stsz(r0) __asm__ volatile("swc2 $19, 0(%0)" : : "r"(r0) : "memory")
/* SZ1 / SZ2 alone. */
#define gte_stsz1(r0) __asm__ volatile("swc2 $17, 0(%0)" : : "r"(r0) : "memory")
#define gte_stsz2(r0) __asm__ volatile("swc2 $18, 0(%0)" : : "r"(r0) : "memory")
#define gte_strgb(r0) __asm__ volatile("swc2 $22, 0(%0)" : : "r"(r0) : "memory")
#define gte_stlvl(r0) __asm__ volatile("swc2 $9, 0(%0); swc2 $10, 4(%0); swc2 $11, 8(%0)" : : "r"(r0) : "memory")

/* Control-register matrix loads (5 words each). */
#define gte_SetRotMatrix(r0)                                                  \
    __asm__ volatile("lw $12, 0(%0); lw $13, 4(%0); ctc2 $12, $0; "           \
                     "ctc2 $13, $1; lw $12, 8(%0); lw $13, 12(%0); "           \
                     "lw $14, 16(%0); ctc2 $12, $2; ctc2 $13, $3; ctc2 $14, $4" \
                     : : "r"(r0) : "$12", "$13", "$14")
#define gte_SetLightMatrix(r0)                                                \
    __asm__ volatile("lw $12, 0(%0); lw $13, 4(%0); ctc2 $12, $8; "           \
                     "ctc2 $13, $9; lw $12, 8(%0); lw $13, 12(%0); "           \
                     "lw $14, 16(%0); ctc2 $12, $10; ctc2 $13, $11; ctc2 $14, $12" \
                     : : "r"(r0) : "$12", "$13", "$14")
#define gte_SetColorMatrix(r0)                                                \
    __asm__ volatile("lw $12, 0(%0); lw $13, 4(%0); ctc2 $12, $16; "          \
                     "ctc2 $13, $17; lw $12, 8(%0); lw $13, 12(%0); "          \
                     "lw $14, 16(%0); ctc2 $12, $18; ctc2 $13, $19; ctc2 $14, $20" \
                     : : "r"(r0) : "$12", "$13", "$14")
#define gte_SetBackColor(r0, r1, r2)                                          \
    __asm__ volatile("sll $12, %0, 4; sll $13, %1, 4; sll $14, %2, 4; "        \
                     "ctc2 $12, $13; ctc2 $13, $14; ctc2 $14, $15"             \
                     : : "r"(r0), "r"(r1), "r"(r2) : "$12", "$13", "$14")

/* Matrix column <-> IR1-3. */
#define gte_ldclmv(r0)                                                        \
    __asm__ volatile("lhu $12, 0(%0); lhu $13, 6(%0); lhu $14, 12(%0); "       \
                     "mtc2 $12, $9; mtc2 $13, $10; mtc2 $14, $11"              \
                     : : "r"(r0) : "$12", "$13", "$14")
#define gte_stclmv(r0)                                                        \
    __asm__ volatile("mfc2 $12, $9; mfc2 $13, $10; mfc2 $14, $11; "            \
                     "sh $12, 0(%0); sh $13, 6(%0); sh $14, 12(%0)"            \
                     : : "r"(r0) : "$12", "$13", "$14", "memory")
/* SVECTOR <-> IR1-3 (inline_c.h gte_stsv / gte_ldsv). */
#define gte_stsv(r0)                                                          \
    __asm__ volatile("mfc2 $12, $9; mfc2 $13, $10; mfc2 $14, $11; "            \
                     "sh $12, 0(%0); sh $13, 2(%0); sh $14, 4(%0)"             \
                     : : "r"(r0) : "$12", "$13", "$14", "memory")
#define gte_ldsv(r0)                                                          \
    __asm__ volatile("lhu $12, 0(%0); lhu $13, 2(%0); lhu $14, 4(%0); "        \
                     "mtc2 $12, $9; mtc2 $13, $10; mtc2 $14, $11"              \
                     : : "r"(r0) : "$12", "$13", "$14")
#define gte_rtir() __asm__ volatile("nop; nop; mvmva 1, 0, 3, 3, 0")

/* inline_c.h gte_MulMatrix0: r3 = r1 * r2 (column by column). */
#define gte_MulMatrix0(r1, r2, r3)                                            \
    {                                                                         \
        gte_SetRotMatrix(r1);                                                 \
        gte_ldclmv(r2);                                                       \
        gte_rtir();                                                           \
        gte_stclmv(r3);                                                       \
        gte_ldclmv((char*)(r2) + 2);                                          \
        gte_rtir();                                                           \
        gte_stclmv((char*)(r3) + 2);                                          \
        gte_ldclmv((char*)(r2) + 4);                                          \
        gte_rtir();                                                           \
        gte_stclmv((char*)(r3) + 4);                                          \
    }

/* More data-register stores / loads. */
#define gte_stsxy0(r0) __asm__ volatile("swc2 $12, 0(%0)" : : "r"(r0) : "memory")
#define gte_stsxy1(r0) __asm__ volatile("swc2 $13, 0(%0)" : : "r"(r0) : "memory")
#define gte_stsxy2(r0) __asm__ volatile("swc2 $14, 0(%0)" : : "r"(r0) : "memory")
#define gte_stsz3(r0, r1, r2)                                                 \
    __asm__ volatile("swc2 $17, 0(%0); swc2 $18, 0(%1); swc2 $19, 0(%2)"      \
                     : : "r"(r0), "r"(r1), "r"(r2) : "memory")
#define gte_ldsz4(r0, r1, r2, r3)                                             \
    __asm__ volatile("mtc2 %0, $16; mtc2 %1, $17; mtc2 %2, $18; mtc2 %3, $19" \
                     : : "r"(r0), "r"(r1), "r"(r2), "r"(r3))
#define gte_stflg(r0)                                                         \
    __asm__ volatile("cfc2 $12, $31; nop; sw $12, 0(%0)"                      \
                     : : "r"(r0) : "$12", "memory")

/* Translation vector (control regs TRX-TRZ) and long-vector transform. */
#define gte_SetTransMatrix(r0)                                                \
    __asm__ volatile("lw $12, 20(%0); lw $13, 24(%0); ctc2 $12, $5; "         \
                     "lw $14, 28(%0); ctc2 $13, $6; ctc2 $14, $7"              \
                     : : "r"(r0) : "$12", "$13", "$14")
#define gte_ldlv0(r0)                                                         \
    __asm__ volatile("lhu $13, 4(%0); lhu $12, 0(%0); sll $13, $13, 16; "     \
                     "or $12, $12, $13; mtc2 $12, $0; lwc2 $1, 8(%0)"          \
                     : : "r"(r0) : "$12", "$13")
#define gte_rt() __asm__ volatile("nop; nop; mvmva 1, 0, 0, 0, 0")
#define gte_stlvnl(r0)                                                        \
    __asm__ volatile("swc2 $25, 0(%0); swc2 $26, 4(%0); swc2 $27, 8(%0)"      \
                     : : "r"(r0) : "memory")

/* inline_c.h gte_CompMatrix: r3 = r1 * r2 with translation. */
#define gte_CompMatrix(r1, r2, r3)                                            \
    {                                                                         \
        gte_MulMatrix0(r1, r2, r3);                                           \
        gte_SetTransMatrix(r1);                                               \
        gte_ldlv0((char*)(r2) + 20);                                          \
        gte_rt();                                                             \
        gte_stlvnl((char*)(r3) + 20);                                         \
    }

/* Outer product (inline_c.h gte_OuterProduct12). */
#define gte_ldopv1(r0)                                                        \
    __asm__ volatile("lw $12, 0(%0); lw $13, 4(%0); ctc2 $12, $0; "           \
                     "lw $14, 8(%0); ctc2 $13, $2; ctc2 $14, $4"               \
                     : : "r"(r0) : "$12", "$13", "$14")
#define gte_ldopv2(r0)                                                        \
    __asm__ volatile("lwc2 $11, 8(%0); lwc2 $9, 0(%0); lwc2 $10, 4(%0)"      \
                     : : "r"(r0))
#define gte_op12() __asm__ volatile("nop; nop; op 1")
#define gte_op0() __asm__ volatile("nop; nop; op 0")
#define gte_OuterProduct0(r1, r2, r3)                                         \
    {                                                                         \
        gte_ldopv1(r1);                                                       \
        gte_ldopv2(r2);                                                       \
        gte_op0();                                                            \
        gte_stlvnl(r3);                                                       \
    }
#define gte_OuterProduct12(r1, r2, r3)                                        \
    {                                                                         \
        gte_ldopv1(r1);                                                       \
        gte_ldopv2(r2);                                                       \
        gte_op12();                                                           \
        gte_stlvnl(r3);                                                       \
    }

/* RotTransPers inline (inline_c.h gte_RotTransPers). */
#define gte_stdp(r0) __asm__ volatile("swc2 $8, 0(%0)" : : "r"(r0) : "memory")
#define gte_stszotz(r0)                                                       \
    __asm__ volatile("mfc2 $12, $19; nop; sra $12, $12, 2; sw $12, 0(%0)"     \
                     : : "r"(r0) : "$12", "memory")
/* SZ0-SZ3 to four consecutive words (inline_c.h gte_stsz4c). */
#define gte_stsz4c(r0)                                                        \
    __asm__ volatile("swc2 $16, 0(%0); swc2 $17, 4(%0); swc2 $18, 8(%0); "     \
                     "swc2 $19, 12(%0)"                                        \
                     : : "r"(r0) : "memory")
/* SXY0-SXY2 / SXY2 into C variables. */
#define gte_stsxy3_r(r0, r1, r2)                                              \
    __asm__ volatile("mfc2 %0, $12; mfc2 %1, $13; mfc2 %2, $14; nop"          \
                     : "=r"(r0), "=r"(r1), "=r"(r2))
#define gte_stsxy2_r(r0) __asm__ volatile("mfc2 %0, $14; nop" : "=r"(r0))
#define gte_RotTransPers(r1, r2, r3, r4, r5)                                  \
    {                                                                         \
        gte_ldv0(r1);                                                         \
        gte_rtps();                                                           \
        gte_stsxy(r2);                                                        \
        gte_stdp(r3);                                                         \
        gte_stflg(r4);                                                        \
        gte_stszotz(r5);                                                      \
    }

#endif /* PLATFORM_PC */

#endif /* GTE_H */
