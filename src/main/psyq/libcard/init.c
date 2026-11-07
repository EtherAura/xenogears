/*
 * PSY-Q 4.0 LIBCARD.LIB INIT.OBJ: InitCARD, StartCARD, StopCARD. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

void EnterCriticalSection(void);                    /* EnterCriticalSection (syscall 1) */
void ExitCriticalSection(void);                    /* ExitCriticalSection (syscall 2) */
void ChangeClearPAD(s32);                     /* B0:5B ChangeClearPad */
/* ---- LIBCARD INIT.OBJ ---- */

void InitCARD2(s32);   /* B0:4A InitCard */
void StartCARD2(void);  /* B0:4B StartCard */
void StopCARD2(void);  /* B0:4C StopCard */
void _patch_card(void);
void _patch_card2(void);
void _ExitCard(void);

void InitCARD(s32 pad_enable) {
    ChangeClearPAD(0);
    EnterCriticalSection();
    InitCARD2(pad_enable);
    _patch_card();
    _patch_card2();
    ExitCriticalSection();
}

void StartCARD(void) {
    EnterCriticalSection();
    StartCARD2();
    ChangeClearPAD(0);
    ExitCriticalSection();
}

void StopCARD(void) {
    StopCARD2();
    _patch_card2();
    _ExitCard();
}
