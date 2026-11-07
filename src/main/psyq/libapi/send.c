/*
 * PSY-Q 4.0 LIBAPI.LIB SEND.OBJ: SendPAD. Our own C for
 * the library routine(s) (clean-room, from the disassembly). Its signature
 * is byte-identical to LIBGS GS_106.OBJ (GsSetProjection), which it was
 * first taken for; it calls _SendPAD and sits among the pad routines.
 */
#include "common.h"

/* ---- LIBAPI SEND.OBJ ---- */

void _SendPAD(void);

void SendPAD(void) {
    _SendPAD();
}
