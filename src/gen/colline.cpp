// colline.cpp - Line collision functions
// Original: C:\CHAN\GAME\SRC\GEN\COLLINE.CPP
#include "gen/colline.h"
#include "p3d/p3dmath.h"

// PSX: GetXOnLine__C4Linel (COLLINE.CPP:84) 0x800BFFD4
// Solve for x: a*x + b*z + c = 0 → x = -(b*z + c) / a
s32 Line::GetXOnLine(s32 z) const {
    MARKFUNCTION(0x800BFFD4);
    if (a == 0) return 0;
    s32 num = fixmul16(b, z) + c;
    return -rmDiv16i(num, a);
}

// PSX: GetZOnLine__C4Linel (COLLINE.CPP:112) 0x800C003C
// Solve for z: a*x + b*z + c = 0 → z = -(a*x + c) / b
s32 Line::GetZOnLine(s32 x) const {
    MARKFUNCTION(0x800C003C);
    if (b == 0) return 0;
    s32 num = fixmul16(a, x) + c;
    return -rmDiv16i(num, b);
}

// PSX: Equal__C4LineRC4LineT1 (COLLINE.CPP:141) 0x800C00A4
// Compare two lines within tolerance of 8
bool Equal(const Line& a, const Line& b) {
    MARKFUNCTION(0x800C00A4);
    s32 da = a.a - b.a;
    if (da < 0) da = -da;
    if (da >= 8) return false;

    s32 db = a.b - b.b;
    if (db < 0) db = -db;
    if (db >= 8) return false;

    s32 dc = a.c - b.c;
    if (dc < 0) dc = -dc;
    if (dc >= 8) return false;

    return true;
}
