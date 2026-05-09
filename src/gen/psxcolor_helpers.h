#pragma once

#include "core.h"

inline u32 PsxAbgr1555ToRgba8888(u16 color) {
    if (color == 0) {
        return 0;
    }

    const u32 r = (color & 0x1F) << 3;
    const u32 g = ((color >> 5) & 0x1F) << 3;
    const u32 b = ((color >> 10) & 0x1F) << 3;
    return (255u << 24) | (b << 16) | (g << 8) | r;
}