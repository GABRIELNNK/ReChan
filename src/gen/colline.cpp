#include "gen/colline.h"
#include "p3d/p3dmath.h"

// PSX: Intersection__C4LineRC4LinelRlT3 (COLLINE.CPP:38) 0x800C0160
// Cramer's rule for intersection of two implicit 2D lines
// A.a*x + A.b*z + A.c = 0, B.a*x + B.b*z + B.c = 0
bool Intersection(const Line& A, const Line& B, s32 unused, s32& outX, s32& outZ) {
    MARKFUNCTION(0x800C0160);

    s32 absA = B.a;
    if (absA < 0) absA = -absA;
    s32 absB = B.b;
    if (absB < 0) absB = -absB;

    if (absB >= absA) {
        // |B.b| >= |B.a| — solve via B.b dominance
        if (B.b == 0) return false;

        // det1 = A.a * B.b - A.b * B.a
        s32 det1 = fixmul16(A.a, B.b) - fixmul16(A.b, B.a);
        if (det1 == 0) return false;

        // det2 = A.b * B.c - A.c * B.b
        s32 det2 = fixmul16(A.b, B.c) - fixmul16(A.c, B.b);
        if (det2 == 0 && det1 == 0) return false;

        // x = det2 / det1
        outX = rmDiv16i(det2, det1);

        // z from line B: z = -(B.a * x + B.c) / B.b
        s32 num = fixmul16(B.a, outX) + B.c;
        outZ = -rmDiv16i(num, B.b);
    }
    else {
        // |B.a| > |B.b| — solve via B.a dominance
        if (B.a == 0) return false;

        // det1 = A.b * B.a - A.a * B.b
        s32 det1 = fixmul16(A.b, B.a) - fixmul16(A.a, B.b);
        if (det1 == 0) return false;

        // det2 = A.a * B.c - A.c * B.a
        s32 det2 = fixmul16(A.a, B.c) - fixmul16(A.c, B.a);
        if (det2 == 0 && det1 == 0) return false;

        // z = det2 / det1
        outZ = rmDiv16i(det2, det1);

        // x from line B: x = -(B.b * z + B.c) / B.a
        s32 num = fixmul16(B.b, outZ) + B.c;
        outX = -rmDiv16i(num, B.a);
    }

    return true;
}

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
