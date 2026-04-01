// psx_types.h — PSX hardware type equivalents
#pragma once

#include "core.h"
#include <cstring>

// PSX short vector (GTE format)
struct SVECTOR {
    s16 vx, vy, vz, pad;

    void ToFloat3(f32& ox, f32& oy, f32& oz) const {
        ox = static_cast<f32>(vx);
        oy = static_cast<f32>(vy);
        oz = static_cast<f32>(vz);
    }
};
static_assert(sizeof(SVECTOR) == 8);

// PSX long vector
struct VECTOR {
    s32 vx, vy, vz, pad;
};
static_assert(sizeof(VECTOR) == 16);

// PSX colour vector
struct CVECTOR {
    u8 r, g, b, cd;
};
static_assert(sizeof(CVECTOR) == 4);

// PSX 16-bit rect
struct RECT16 {
    s16 x, y, w, h;
};
static_assert(sizeof(RECT16) == 8);
