#ifndef INCLUDE_ASM_H
#define INCLUDE_ASM_H

/*
 * INCLUDE_ASM pulls a not-yet-decompiled function's assembly into a C file,
 * in place, so the object keeps retail function order.
 *
 * GCC 2.7.2 with a non-zero -G moves functions after file-scope __asm__
 * blocks, so each include is wrapped in a dummy function that maspsx strips,
 * keeping only the lines tagged "maspsx-keep" (see maspsx's README).
 *
 * M2CTX/PERMUTER builds (m2c context, decomp-permuter) and the PC port
 * (PLATFORM_PC) see no assembly: the port supplies C for what is still
 * hand-written assembly.
 */

#if !defined(M2CTX) && !defined(PERMUTER) && !defined(PLATFORM_PC)

#define INCLUDE_ASM(FOLDER, NAME)                                   \
    void __maspsx_include_asm_hack_##NAME(void) {                   \
        __asm__(                                                    \
            ".text # maspsx-keep\n"                                 \
            "\t.align\t2 # maspsx-keep\n"                           \
            "\t.set noreorder # maspsx-keep\n"                      \
            "\t.set noat # maspsx-keep\n"                           \
            ".include \"" FOLDER "/" #NAME ".s\" # maspsx-keep\n"   \
            "\t.set reorder # maspsx-keep\n"                        \
            "\t.set at # maspsx-keep\n"                             \
        );                                                          \
    }

#define INCLUDE_RODATA(FOLDER, NAME)                                \
    void __maspsx_include_asm_hack_rodata_##NAME(void) {            \
        __asm__(                                                    \
            ".section .rodata # maspsx-keep\n"                      \
            ".include \"" FOLDER "/" #NAME ".s\" # maspsx-keep\n"   \
            ".section .text # maspsx-keep\n"                        \
        );                                                          \
    }

#else

#define INCLUDE_ASM(FOLDER, NAME)
#define INCLUDE_RODATA(FOLDER, NAME)

#endif

#endif /* INCLUDE_ASM_H */
