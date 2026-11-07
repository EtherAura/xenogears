#ifndef PC_PAD_H
#define PC_PAD_H
#ifdef PLATFORM_PC
int pcPadPromptSlot(int slot);
int pcPadDialogSlot(int slot);
unsigned char* pcPadDialogText(int slot);
int pcPadIsGlyph(unsigned type, unsigned id);
void pcPadPrintGlyph(unsigned short* out, int stride, int odd, int glyph);
#endif
#endif
