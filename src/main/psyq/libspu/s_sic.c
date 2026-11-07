/*
 * PSY-Q 4.0 LIBSPU.LIB S_SIC.OBJ: SpuSetIRQCallback. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuCallbackFunc D_80058E44;
void _SpuCallback(SpuCallbackFunc func);

SpuCallbackFunc SpuSetIRQCallback(SpuCallbackFunc func) {
    SpuCallbackFunc old = D_80058E44;

    if (func != old) {
        D_80058E44 = func;
        _SpuCallback(func);
    }
    return old;
}
