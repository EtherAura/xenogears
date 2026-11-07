/*
 * PSY-Q 4.0 LIBSPU.LIB S_Q.OBJ: SpuQuit. Our own C for the library
 * routine(s) (clean-room, from the disassembly).
 * The callback globals must be volatile (otherwise a0=0 fills the jal slot).
 * EnterCriticalSection/ExitCriticalSection = Enter/ExitCriticalSection, CloseEvent =
 * CloseEvent, DisableEvent = DisableEvent (BIOS stubs).
 */
#include "common.h"
#include "psyq/libspu.h"

extern s32 D_8005899C;
extern s32 D_80058E04;
extern volatile SpuCallbackFunc D_80058E40;
extern volatile SpuCallbackFunc D_80058E44;
void _SpuDataCallback(SpuCallbackFunc func);
void EnterCriticalSection(void);
void ExitCriticalSection(void);
void CloseEvent(s32 event);
void DisableEvent(s32 event);

void SpuQuit(void) {
    if (D_80058E04 == 1) {
        D_80058E04 = 0;
        EnterCriticalSection();
        D_80058E40 = NULL;
        D_80058E44 = NULL;
        _SpuDataCallback(NULL);
        CloseEvent(D_8005899C);
        DisableEvent(D_8005899C);
        ExitCriticalSection();
    }
}
