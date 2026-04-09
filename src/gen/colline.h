#pragma once
#include "core.h"

// Line - implicit 2D line equation: a*x + b*z + c = 0
// PSX: 12 bytes (3 x s32). Used as floor boundary lines and wall height lines.
struct Line {
    s32 a;  // +0: X coefficient
    s32 b;  // +4: Z coefficient
    s32 c;  // +8: constant offset

    s32 GetXOnLine(s32 z) const;
    s32 GetZOnLine(s32 x) const;
};

// Cramer's rule solve for two Line equations; outX,outZ = intersection point
bool Intersection(const Line& A, const Line& B, s32 unused, s32& outX, s32& outZ);

// Compare two lines within tolerance of 8
bool Equal(const Line& a, const Line& b);
