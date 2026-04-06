// xccolour.h - xcColour1555 reversed from PSX XCCOLOUR.H
// PSX source: \CHAN\DEVSYS\PSX\XCLIB\INCLUDE\XCCOLOUR.H
// 16-bit ABGR1555 color used by xclib primitives.
#pragma once

#include "core.h"

// PSX 5-to-8 bit expansion table (XCCOLOUR.H)
// Maps 5-bit channel (0-31) to 8-bit (0-255)
inline u8 xcCol5To8(u8 val5) {
    // PSX uses a lookup table; this is equivalent
    return (u8)((val5 * 255 + 15) / 31);
}

// xcColour1555 [2 bytes] (PSX XCCOLOUR.H)
// Format: A(1) B(5) G(5) R(5) = standard PSX ABGR1555
struct xcColour1555 {
    u16 raw = 0x8000;

    // PSX: GetRed8__C12xcColour1555 (XCCOLOUR.H:214)
    u8 GetRed8() const { return xcCol5To8((u8)(raw & 0x1F)); }

    // PSX: GetGreen8__C12xcColour1555 (XCCOLOUR.H:215)
    u8 GetGreen8() const { return xcCol5To8((u8)((raw >> 5) & 0x1F)); }

    // PSX: GetBlue8__C12xcColour1555 (XCCOLOUR.H:216)
    u8 GetBlue8() const { return xcCol5To8((u8)((raw >> 10) & 0x1F)); }

    // PSX: GetAlpha8__C12xcColour1555 (XCCOLOUR.H:217)
    u8 GetAlpha8() const { return (raw & 0x8000) ? 255 : 0; }

    // PSX: Set8__12xcColour1555UcUcUc (XCCOLOUR.H:197)
    void Set8(u8 r, u8 g, u8 b) {
        raw = (u16)(((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10) | 0x8000);
    }
};

// PSX: MenuColorStart(xcColour1555&) (HDMENU.CPP:112, 0x8005CB4C)
// Initializes the color cycling for menu text pulsing.
// PSX uses global state via $gp for start/end colors and deltas.
// On PC: simplified — starts with a base color.
void MenuColorStart(xcColour1555& col);

// PSX: MenuColorNext(xcColour1555&) (HDMENU.CPP:143, 0x8005CD10)
// Advances the color cycling by one step.
// PSX: calls CalcNextColor then checks if cycle wrapped.
void MenuColorNext(xcColour1555& col);
