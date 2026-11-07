#ifndef STACK_H
#define STACK_H

/*
 * Inline-assembly macros for two register tricks the retail code performs
 * inline, which C cannot express:
 *
 * - STACK_SWITCH(top) / STACK_RESTORE(): run on a scratch stack. The old
 *   $sp is saved at *top and $sp becomes top - 4; STACK_RESTORE() pops it
 *   back. Used around big uploads (e.g. a 0x2000-byte heap block as stack).
 *   Retail: move $t0,top; sw $sp,0($t0); addiu $t0,$t0,-4; move $sp,$t0
 *   ... addiu $sp,$sp,4; lw $sp,0($sp).
 *
 * - GET_RA(dst): store the caller's return address into *dst, through $t7
 *   (the heap manager records who allocated each block).
 *   Retail: move $t7,dst; sw $ra,0($t7).
 *
 * Together with include/gte.h these are the only sanctioned inline asm in
 * matched code (user decision; see docs/PLAN.md).
 */

#ifdef PLATFORM_PC
/*
 * The PC runs these calls on its own stack (the scratch stacks only kept
 * the PS1's small stack from overflowing), and records the return address
 * through the compiler. GET_RA's destination is a u32: the heap and the error
 * reporter keep it for debugging only, so a truncated host address is fine.
 */
#define STACK_SWITCH(top) ((void)(top))
#define STACK_RESTORE() ((void)0)
#define GET_RA(dst) (*(dst) = (u32)(uptr)__builtin_return_address(0))
#else

#define STACK_SWITCH(top)                                                   \
    __asm__ volatile("move $8, %0\n\t"                                      \
                     "sw $29, 0($8)\n\t"                                    \
                     "addiu $8, $8, -4\n\t"                                 \
                     "move $29, $8"                                         \
                     : : "r"(top) : "$8", "memory")

#define STACK_RESTORE()                                                     \
    __asm__ volatile("addiu $29, $29, 4\n\t"                                \
                     "lw $29, 0($29)"                                       \
                     : : : "memory")

#define GET_RA(dst)                                                         \
    __asm__ volatile("move $15, %0\n\t"                                     \
                     "sw $31, 0($15)"                                       \
                     : : "r"(dst) : "$15", "memory")

#endif /* PLATFORM_PC */

#endif /* STACK_H */
