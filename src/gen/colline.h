// colline.h - Line struct for collision boundary lines
// Original: C:\CHAN\GAME\SRC\GEN\COLLINE.CPP
#pragma once

#include "common.h"

// Line - implicit 2D line equation: a*x + b*z + c = 0
// PSX: 12 bytes (3 x s32). Used as floor boundary lines and wall height lines.
struct Line {
    s32 a;  // +0: X coefficient
    s32 b;  // +4: Z coefficient
    s32 c;  // +8: constant offset

    // PSX: GetXOnLine__C4Linel (COLLINE.CPP:84) 0x800BFFD4
    s32 GetXOnLine(s32 z) const;

    // PSX: GetZOnLine__C4Linel (COLLINE.CPP:112) 0x800C003C
    s32 GetZOnLine(s32 x) const;
};

// PSX: Intersection__C4LineRC4LinelRlT3 (COLLINE.CPP:38) 0x800C0160
// Cramer's rule solve for two Line equations; outX,outZ = intersection point
bool Intersection(const Line& A, const Line& B, s32 unused, s32& outX, s32& outZ);

// PSX: Equal__C4LineRC4LineT1 (COLLINE.CPP:141) 0x800C00A4
// Compare two lines within tolerance of 8
bool Equal(const Line& a, const Line& b);
