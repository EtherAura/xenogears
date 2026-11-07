/*
 * FA status (US 0x80071A64-0x80071EE8): gcc-2.7.2-psx -O2 -G0, aspsx 2.34
 * (also OK with the -G8 preset; setupRGBCalcSlot0_fadeToBlack differs under gcc-2.6.x):
 *   resetAllRGBFadersToBlack  OK  Noah: resetAllRGBFadersToBlack
 *   updateRGBFader  OK  updates one screen fader (Noah: none; suggest updateRGBFader)
 *   updateScreenEffectSlots  OK  updates both faders + renderRGBFaders (suggest updateScreenEffectSlots)
 *   setupRGBCalcMode  OK  Noah: setupRGBCalcMode
 *   setupRGBCalcSlot0_fadeToBlack  OK  Noah: setupRGBCalcSlot0_fadeToBlack
 *   setupRGBFaderSlot0_fadeIn  OK  Noah: setupRGBFaderSlot0_fadeIn
 * Retail's div here has no zero/overflow checks; the preset (no
 * --expand-div) reproduces that.
 */
#include "common.h"
#include "psyq.h"
#include "field/field.h"
#include "field/screen_effects.h"

extern s32 D_800ADC04;
extern s16 fieldScreenFadedOut;
extern s32 g_frameOddOrEven; /* Noah: g_frameOddOrEven? */

void resetRGBFaderToBlack(s32 index);
void renderRGBFaders(OTag* ot, s32 frame);

void resetAllRGBFadersToBlack(void) {
    resetRGBFaderToBlack(0);
    resetRGBFaderToBlack(1);
}

void updateRGBFader(s32 index) {
    if (fieldScreenEffects.m4C_screenEffects[index].m52_isActive != 0) {
        if (fieldScreenEffects.m4C_screenEffects[index].m54_duration <= 0) {
            fieldScreenEffects.m4C_screenEffects[index].m54_duration = 0;
            if (fieldScreenFadedOut != 1) {
                if (fieldScreenEffects.m4C_screenEffects[index].m38_current.m0_R == 0 && fieldScreenEffects.m4C_screenEffects[index].m38_current.m4_G == 0 &&
                    fieldScreenEffects.m4C_screenEffects[index].m38_current.m8_B == 0) {
                    fieldScreenEffects.m4C_screenEffects[index].m52_isActive = 0;
                }
            }
        } else {
            fieldScreenEffects.m4C_screenEffects[index].m38_current.m0_R += fieldScreenEffects.m4C_screenEffects[index].m44_step.m0_R;
            if ((fieldScreenEffects.m4C_screenEffects[index].m38_current.m0_R >> 8) >= 0x100) {
                fieldScreenEffects.m4C_screenEffects[index].m38_current.m0_R = 0xFF00;
            }
            if (fieldScreenEffects.m4C_screenEffects[index].m38_current.m0_R < 0) {
                fieldScreenEffects.m4C_screenEffects[index].m38_current.m0_R = 0;
            }
            fieldScreenEffects.m4C_screenEffects[index].m38_current.m4_G += fieldScreenEffects.m4C_screenEffects[index].m44_step.m4_G;
            if ((fieldScreenEffects.m4C_screenEffects[index].m38_current.m4_G >> 8) >= 0x100) {
                fieldScreenEffects.m4C_screenEffects[index].m38_current.m4_G = 0xFF00;
            }
            if (fieldScreenEffects.m4C_screenEffects[index].m38_current.m4_G < 0) {
                fieldScreenEffects.m4C_screenEffects[index].m38_current.m4_G = 0;
            }
            fieldScreenEffects.m4C_screenEffects[index].m38_current.m8_B += fieldScreenEffects.m4C_screenEffects[index].m44_step.m8_B;
            if ((fieldScreenEffects.m4C_screenEffects[index].m38_current.m8_B >> 8) >= 0x100) {
                fieldScreenEffects.m4C_screenEffects[index].m38_current.m8_B = 0xFF00;
            }
            if (fieldScreenEffects.m4C_screenEffects[index].m38_current.m8_B < 0) {
                fieldScreenEffects.m4C_screenEffects[index].m38_current.m8_B = 0;
            }
            fieldScreenEffects.m4C_screenEffects[index].m54_duration--;
        }
    }
}

void updateScreenEffectSlots(OTag* ot, s32 oddOrEven) {
    if (D_800ADC04 == 2) {
        updateRGBFader(0);
        updateRGBFader(1);
    }
    renderRGBFaders(ot, g_frameOddOrEven);
}

void setupRGBCalcMode(s32 index, s32 steps, s32 r, s32 g, s32 b, s32 mode) {
    s32 stepR = R3000_DIV((r << 8) - fieldScreenEffects.m4C_screenEffects[index].m38_current.m0_R, steps);
    s32 stepG = R3000_DIV((g << 8) - fieldScreenEffects.m4C_screenEffects[index].m38_current.m4_G, steps);
    s32 stepB = R3000_DIV((b << 8) - fieldScreenEffects.m4C_screenEffects[index].m38_current.m8_B, steps);
    fieldScreenEffects.m4C_screenEffects[index].m54_duration = steps;
    fieldScreenEffects.m4C_screenEffects[index].m52_isActive = 1;
    fieldScreenEffects.m4C_screenEffects[index].m50_mode = mode;
    fieldScreenEffects.m4C_screenEffects[index].m44_step.m0_R = stepR;
    fieldScreenEffects.m4C_screenEffects[index].m44_step.m4_G = stepG;
    fieldScreenEffects.m4C_screenEffects[index].m44_step.m8_B = stepB;
}

void setupRGBCalcSlot0_fadeToBlack(s32 steps) {
    s32 step;
    if (fieldScreenFadedOut != 1) {
        fieldScreenFadedOut = 1;
        if (D_800ADC04 == 2) {
            step = R3000_DIV(0xFF00, steps);
            fieldScreenEffects.m4C_screenEffects[0].m38_current.m8_B = 0;
            fieldScreenEffects.m4C_screenEffects[0].m38_current.m4_G = 0;
            fieldScreenEffects.m4C_screenEffects[0].m38_current.m0_R = 0;
            fieldScreenEffects.m4C_screenEffects[0].m54_duration = steps;
            fieldScreenEffects.m4C_screenEffects[0].m52_isActive = 1;
            fieldScreenEffects.m4C_screenEffects[0].m50_mode = 2;
            fieldScreenEffects.m4C_screenEffects[0].m44_step.m8_B = step;
            fieldScreenEffects.m4C_screenEffects[0].m44_step.m4_G = step;
            fieldScreenEffects.m4C_screenEffects[0].m44_step.m0_R = step;
        }
    }
}

void setupRGBFaderSlot0_fadeIn(s32 steps) {
    s32 step;
    if (fieldScreenFadedOut != 0) {
        fieldScreenFadedOut = 0;
        if (D_800ADC04 == 2) {
            step = R3000_DIV(-0x10000, steps);
            fieldScreenEffects.m4C_screenEffects[0].m38_current.m8_B = 0xFF00;
            fieldScreenEffects.m4C_screenEffects[0].m38_current.m4_G = 0xFF00;
            fieldScreenEffects.m4C_screenEffects[0].m38_current.m0_R = 0xFF00;
            fieldScreenEffects.m4C_screenEffects[0].m52_isActive = 1;
            fieldScreenEffects.m4C_screenEffects[0].m54_duration = steps;
            fieldScreenEffects.m4C_screenEffects[0].m50_mode = 2;
            fieldScreenEffects.m4C_screenEffects[0].m44_step.m8_B = step;
            fieldScreenEffects.m4C_screenEffects[0].m44_step.m4_G = step;
            fieldScreenEffects.m4C_screenEffects[0].m44_step.m0_R = step;
        }
    }
}
