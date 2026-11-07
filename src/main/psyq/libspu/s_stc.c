/*
 * PSY-Q 4.0 LIBSPU.LIB S_STC.OBJ: SpuSetTransferCallback. Our own C for the
 * library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"
#include "psyq/libspu.h"

extern volatile SpuCallbackFunc D_80058E40;

void SpuSetTransferCallback(SpuCallbackFunc func) {
    if (func != D_80058E40) {
        D_80058E40 = func;
    }
}
