/*
 * PSY-Q 4.0 LIBGTE FOG_02.OBJ: SetFogNearFar. Our own C for the library
 * routine (clean-room, from the disassembly). GCC 2.7.2 (PSY-Q), ASPSX 2.56.
 */
#include "common.h"

void SetDQA(s32 iDqa);
void SetDQB(s32 iDqb);

/* FOG_02.OBJ: fog near/far distances -> GTE depth-cueing DQA/DQB. */
void SetFogNearFar(s32 a, s32 b, s32 h) {
    s32 dqa;
    s32 dqb;
    s32 d;

    d = b - a;
    if (d >= 100) {
        dqa = ((-a * b / d) << 8) / h;
        dqb = ((b << 12) / d) << 12;
        if (dqa < -0x8000) {
            dqa = -0x8000;
        }
        if (dqa > 0x7FFF) {
            dqa = 0x7FFF;
        }
        SetDQA(dqa);
        SetDQB(dqb);
    }
}
