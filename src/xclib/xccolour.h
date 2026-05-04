#pragma once
#include "core.h"

// PSX 5-to-8 bit expansion table (XCCOLOUR.H)
// Maps 5-bit channel (0-31) to 8-bit (0-255)
inline u8 xcCol5To8(u8 val5) {
    // PSX uses a lookup table; this is equivalent
    return (u8)((val5 * 255 + 15) / 31);
}

// xcColour1555
// Primary storage: full 8-bit r, g, b, a.
// PSX wire format (ABGR1555) is computed on demand via GetRaw().
//
// Packed return formats:
//   Get8()   → 0xAABBGGRR  u32  — xcFont::DrawText / ScreenDraw
//   GetRaw() → ABGR1555    u16  — PSX wire format / xclib
struct xcColour1555 {
    u8 r, g, b, a;

    // Default: opaque black
    xcColour1555() : r(0), g(0), b(0), a(255) {}

    // Construct from RGB — opaque
    xcColour1555(u8 r, u8 g, u8 b) : r(r), g(g), b(b), a(255) {}

    // Construct from RGBA
    xcColour1555(u8 r, u8 g, u8 b, u8 a) : r(r), g(g), b(b), a(a) {}

    // Construct from a raw PSX ABGR1555 u16 (e.g. xcColour1555{0x8000})
    xcColour1555(u16 psxRaw)
        : r(xcCol5To8((u8)(psxRaw & 0x1F)))
        , g(xcCol5To8((u8)((psxRaw >>  5) & 0x1F)))
        , b(xcCol5To8((u8)((psxRaw >> 10) & 0x1F)))
        , a((psxRaw & 0x8000) ? 255 : 0) {}

    // PSX: Set8__12xcColour1555UcUcUc (XCCOLOUR.H:197)
    void Set8(u8 r, u8 g, u8 b)           { this->r = r; this->g = g; this->b = b; this->a = 255; }
    void Set8(u8 r, u8 g, u8 b, u8 a)     { this->r = r; this->g = g; this->b = b; this->a = a;   }

    // ── Channel getters ────────────────────────────────────────────────────
    // PSX: GetRed8 / GetGreen8 / GetBlue8 / GetAlpha8 (XCCOLOUR.H:214–217)
    u8 GetRed8()   const { return r; }
    u8 GetGreen8() const { return g; }
    u8 GetBlue8()  const { return b; }
    u8 GetAlpha8() const { return a; }

    // Returns 0xAABBGGRR — used by xcFont::DrawText and ScreenDraw
    u32 Get8() const {
        return ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
    }

    // Returns PSX ABGR1555 u16 — computed from r/g/b/a
    u16 GetRaw() const {
        return (u16)(  ((r >> 3) & 0x1F)
                     | (((g >> 3) & 0x1F) << 5)
                     | (((b >> 3) & 0x1F) << 10)
                     | (a >= 128 ? 0x8000u : 0u));
    }
};

// Initializes the color cycling for menu text pulsing.
// PSX uses global state via $gp for start/end colors and deltas.
// On PC: simplified — starts with a base color.
void MenuColorStart(xcColour1555& col);

// Advances the color cycling by one step.
// PSX: calls CalcNextColor then checks if cycle wrapped.
void MenuColorNext(xcColour1555& col);
