/*
 * PSY-Q 4.0 LIBETC.LIB VMODE.OBJ: SetVideoMode, GetVideoMode. Our own C for
 * the library routine(s) (clean-room, from the disassembly).
 */
#include "common.h"

/* ---- LIBETC VMODE.OBJ ---- */

s32 libetcVideoMode = 0; /* VMODE.OBJ .data: video mode (0 NTSC, 1 PAL) */

s32 SetVideoMode(s32 mode) {
    s32 old = libetcVideoMode;
    libetcVideoMode = mode;
    return old;
}

s32 GetVideoMode(void) {
    return libetcVideoMode;
}
