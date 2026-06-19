#pragma once

#include "core.h"

inline u32 PsxAbgr1555ToRgba8888(u16 color) {
    if (color == 0) {
        return 0;
    }

    const u32 r5 = color & 0x1F;
    const u32 g5 = (color >> 5) & 0x1F;
    const u32 b5 = (color >> 10) & 0x1F;
    const u32 r = (r5 << 3) | (r5 >> 2);
    const u32 g = (g5 << 3) | (g5 >> 2);
    const u32 b = (b5 << 3) | (b5 >> 2);
    return (255u << 24) | (b << 16) | (g << 8) | r;
}

// Converts a PSX 0..128-style color channel to modern 0..255 output.
inline u8 PsxColMod128To255(u8 value) {
    const u32 converted = ((u32)value * 255u + 64u) / 128u;
    return (converted > 255u) ? 255u : (u8)converted;
}
