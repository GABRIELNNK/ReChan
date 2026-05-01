// p3dmath.h - PC float equivalents of PSX Pure3D math functions
// Original PSX: _RMVECT16 (3×s32), MATRIX (3×5 s16 Q12 fixed-point)
// PC: Vec3 (3×f32), Mat4 (4×4 f32) - same results, float precision
//
// PSX function names preserved for 1:1 mapping with decompiled code.
#pragma once

#include "p3d/vector.h"
#include "p3d/matrix.h"
#include "p3d/lvector.h"
#include <cmath>


// PSX math function equivalents (float versions)
// Names match decompiled PSX code for easy cross-reference.

// Pi
#define PI 3.14159265358979323846f
#define TWO_PI (2.0f * PI)

// PSX binary angle: 65536 = full circle (2pi)
#define ANGLE_TO_RAD (TWO_PI / 65536.0f)
#define RAD_TO_ANGLE (65536.0f / TWO_PI)

// PSX binary angle constants (0x0000-0xFFFF = 0-360 degrees)
#define PSX_ANGLE_0    0x0000
#define PSX_ANGLE_90   0x4000
#define PSX_ANGLE_180  0x8000
#define PSX_ANGLE_270  0xC000
#define PSX_ANGLE_360  0x10000
#define PSX_ANGLE_MASK 0xFFFF

// PSX 16.16 fixed-point
#define FIX16_SCALE 65536.0f
#define FIX16_INV (1.0f / 65536.0f)

// PSX 16.16 fixed-point named constants
#define FIX16_ONE   0x10000   // 1.0
#define FIX16_HALF  0x8000    // 0.5
#define FIX16_QUARTER 0x4000  // 0.25
#define FIX16_NEG_ONE ((s32)0xFFFF0000) // -1.0
#define FIX16_MAX   0x7FFFFFFF

// Sentinel values
#define INVALID_HANDLE 0xFFFF
#define INVALID_SLOT   0xFF

// Conversion macros
#define ANGLE2RAD(a) ((f32)(a) * ANGLE_TO_RAD)
#define RAD2ANGLE(r) ((s32)((r) * RAD_TO_ANGLE))
#define FIX16_TO_FLOAT(v) ((f32)(v) * FIX16_INV)
#define FLOAT_TO_FIX16(v) ((s32)((v) * FIX16_SCALE))

// p3dBuildRotMatrixZ / Y / X - single-axis rotation into Mat4
// PSX: builds into 3×5 s16 Q12 MATRIX. PC: builds into 4×4 float Mat4.
inline void p3dBuildRotMatrixZ(f32 angle, Mat4& m) {
    f32 c = std::cos(angle), s = std::sin(angle);
    m = Mat4(); // identity
    m.m[0] = c;  m.m[1] = s;
    m.m[4] = -s; m.m[5] = c;
}

inline void p3dBuildRotMatrixY(f32 angle, Mat4& m) {
    f32 c = std::cos(angle), s = std::sin(angle);
    m = Mat4();
    m.m[0] = c;  m.m[2] = -s;
    m.m[8] = s;  m.m[10] = c;
}

inline void p3dBuildRotMatrixX(f32 angle, Mat4& m) {
    f32 c = std::cos(angle), s = std::sin(angle);
    m = Mat4();
    m.m[5] = c;  m.m[6] = s;
    m.m[9] = -s; m.m[10] = c;
}


// p3dBuildRotMatrixZYX - PSX: 0x80093D58
// PSX params: (ax, ay, az) = (X angle, Y angle, Z angle)
// PSX computes: R = Ry(ay) * Rx(ax) * Rz(az)
// Verified against PSX decompile: m[1][2] = -sin(ax) (pitch extraction)
inline void p3dBuildRotMatrixZYX(f32 ax, f32 ay, f32 az, Mat4& m) {
    f32 cx = std::cos(ax), sx = std::sin(ax);
    f32 cy = std::cos(ay), sy = std::sin(ay);
    f32 cz = std::cos(az), sz = std::sin(az);

    m = Mat4();
    m.m[0]  = cy * cz + sy * sx * sz;
    m.m[1]  = cx * sz;
    m.m[2]  = cy * sx * sz - sy * cz;

    m.m[4]  = sy * sx * cz - cy * sz;
    m.m[5]  = cx * cz;
    m.m[6]  = sy * sz + cy * sx * cz;

    m.m[8]  = sy * cx;
    m.m[9]  = -sx;
    m.m[10] = cy * cx;
}

// Overload taking PSX angle units (s32) - converts internally
inline void p3dBuildRotMatrixZYX(s32 ax, s32 ay, s32 az, Mat4& m) {
    p3dBuildRotMatrixZYX(
        ANGLE2RAD(ax & 0xFFFF),
        ANGLE2RAD(ay & 0xFFFF),
        ANGLE2RAD(az & 0xFFFF),
        m);
}

// p3dBuildRotMatrixXYZ - PSX: 0x800730F8
// PSX params: (ax, ay, az) = (X angle, Y angle, Z angle)
inline void p3dBuildRotMatrixXYZ(f32 ax, f32 ay, f32 az, Mat4& m) {
    f32 cx = std::cos(ax), sx = std::sin(ax);
    f32 cy = std::cos(ay), sy = std::sin(ay);
    f32 cz = std::cos(az), sz = std::sin(az);

    m = Mat4();
    m.m[0]  = cy * cz;
    m.m[1]  = cy * sz;
    m.m[2]  = -sy;

    m.m[4]  = sx * sy * cz - cx * sz;
    m.m[5]  = sx * sy * sz + cx * cz;
    m.m[6]  = sx * cy;

    m.m[8]  = cx * sy * cz + sx * sz;
    m.m[9]  = cx * sy * sz - sx * cz;
    m.m[10] = cx * cy;
}

// Overload taking PSX angle units (s32)
inline void p3dBuildRotMatrixXYZ(s32 ax, s32 ay, s32 az, Mat4& m) {
    p3dBuildRotMatrixXYZ(
        ANGLE2RAD(ax & 0xFFFF),
        ANGLE2RAD(ay & 0xFFFF),
        ANGLE2RAD(az & 0xFFFF),
        m);
}

// p3dBuildRotMatrixYZX - PSX: 0x800737A4
// PSX params: (ax, ay, az) = (X angle, Y angle, Z angle)
// PSX computes: R = Rx(ax) * Rz(az) * Ry(ay)
// Verified against PSX decompile scratchpad implementation
inline void p3dBuildRotMatrixYZX(f32 ax, f32 ay, f32 az, Mat4& m) {
    f32 cx = std::cos(ax), sx = std::sin(ax);
    f32 cy = std::cos(ay), sy = std::sin(ay);
    f32 cz = std::cos(az), sz = std::sin(az);

    m = Mat4();
    m.m[0]  = cz * cy;
    m.m[1]  = cx * sz * cy + sx * sy;
    m.m[2]  = sx * sz * cy - cx * sy;

    m.m[4]  = -sz;
    m.m[5]  = cx * cz;
    m.m[6]  = sx * cz;

    m.m[8]  = cz * sy;
    m.m[9]  = cx * sz * sy - sx * cy;
    m.m[10] = sx * sz * sy + cx * cy;
}

// Overload taking PSX angle units (s32) - converts internally
inline void p3dBuildRotMatrixYZX(s32 ax, s32 ay, s32 az, Mat4& m) {
    p3dBuildRotMatrixYZX(
        ANGLE2RAD(ax & 0xFFFF),
        ANGLE2RAD(ay & 0xFFFF),
        ANGLE2RAD(az & 0xFFFF),
        m);
}

// p3dVecTimesMatrix - transform vector by matrix (rotation + translation)
// PSX: 0x80094568 - result = vec * rotPart + translation
inline Vec3 p3dVecTimesMatrix(const Vec3& v, const Mat4& m) {
    return {
        v.x * m.m[0] + v.y * m.m[4] + v.z * m.m[8]  + m.m[12],
        v.x * m.m[1] + v.y * m.m[5] + v.z * m.m[9]  + m.m[13],
        v.x * m.m[2] + v.y * m.m[6] + v.z * m.m[10] + m.m[14]
    };
}


// p3dVecTimesRotMatrix - rotation only (no translation)
// PSX: 0x800945EC
inline Vec3 p3dVecTimesRotMatrix(const Vec3& v, const Mat4& m) {
    return {
        v.x * m.m[0] + v.y * m.m[4] + v.z * m.m[8],
        v.x * m.m[1] + v.y * m.m[5] + v.z * m.m[9],
        v.x * m.m[2] + v.y * m.m[6] + v.z * m.m[10]
    };
}


// p3dBuildTransMatrix - build identity + translation
inline void p3dBuildTransMatrix(f32 x, f32 y, f32 z, Mat4& m) {
    m = Mat4();
    m.SetTranslation(x, y, z);
}


// p3dFillTransMatrix - fill translation into existing matrix (PSX: 0x800946D0)
// PSX: writes ONLY the translation part, leaves rotation untouched.
inline void p3dFillTransMatrix(const LVector& pos, Mat4& m) {
    m.SetTranslation((f32)pos.x, (f32)pos.y, (f32)pos.z);
}


// p3dFillHeadingMatrix - build orientation matrix from heading + up vectors
// PSX: 0x800947F0 - builds right/up/forward basis
inline void p3dFillHeadingMatrix(const Vec3& heading, const Vec3& up, Mat4& m) {
    Vec3 fwd = heading.Normalized();
    Vec3 right = up.Cross(fwd);
    right.Normalize();
    Vec3 realUp = fwd.Cross(right);

    m = Mat4();
    m.m[0] = right.x;  m.m[1] = right.y;  m.m[2] = right.z;
    m.m[4] = realUp.x;  m.m[5] = realUp.y;  m.m[6] = realUp.z;
    m.m[8] = fwd.x;     m.m[9] = fwd.y;     m.m[10] = fwd.z;
}

// fixmul16 - 16.16 fixed-point multiply: (a * b) >> 16
// PSX: implemented inline via MIPS mult + shift
inline s32 fixmul16(s32 a, s32 b) {
    return (s32)(((s64)a * (s64)b) >> 16);
}

// rmMag2 - 2D magnitude (PSX: 0x80113984)
inline f32 rmMag2(f32 x, f32 y) {
    return std::sqrt(x * x + y * y);
}

// rmMag3 - 3D magnitude (PSX: 0x80113B90)
inline f32 rmMag3(f32 x, f32 y, f32 z) {
    return std::sqrt(x * x + y * y + z * z);
}

// rmMag2ff - fast 2D magnitude approximation (PSX: radlib MAGFAST.C)
// Returns max(|a|,|b|) + min(|a|,|b|)/4
inline s32 rmMag2ff(s32 a, s32 b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    if (b < a) {
        return a + (b >> 2);
    }
    return b + (a >> 2);
}

// rmMag3ffu - unsigned fast 3D magnitude helper (PSX: radlib MAGFAST.C)
inline s32 rmMag3ffu(u32 a1, u32 a2, u32 a3) {
    if (a2 >= a1 || a3 >= a1) {
        if (a3 >= a2) {
            if (a2 < a1) {
                return a3 + (a1 >> 2) + (a1 >> 4) + (a1 >> 5) + (a2 >> 2) + (a2 >> 5);
            }
            return a3 + (a2 >> 2) + (a2 >> 4) + (a2 >> 5) + (a1 >> 2) + (a1 >> 5);
        }
        if (a3 < a1) {
            return a2 + (a1 >> 2) + (a1 >> 4) + (a1 >> 5) + (a3 >> 2) + (a3 >> 5);
        }
        return a2 + (a3 >> 2) + (a3 >> 4) + (a3 >> 5) + (a1 >> 2) + (a1 >> 5);
    }
    if (a3 < a2) {
        return a1 + (a2 >> 2) + (a2 >> 4) + (a2 >> 5) + (a3 >> 2) + (a3 >> 5);
    }
    return a1 + (a3 >> 2) + (a3 >> 4) + (a3 >> 5) + (a2 >> 2) + (a2 >> 5);
}

// rmMag3ff - fast 3D magnitude approximation (PSX: radlib MAGFAST.C)
inline s32 rmMag3ff(s32 a1, s32 a2, s32 a3) {
    if (a1 < 0) a1 = -a1;
    if (a2 < 0) a2 = -a2;
    if (a3 < 0) a3 = -a3;
    return rmMag3ffu((u32)a1, (u32)a2, (u32)a3);
}

// rmRangedRandom - PRNG returning value in [0, range)
// PSX: 0x80078404, Source: C:\chan\devsys\psx\radlib\SOURCE\MATH\RANDOM\RANDOM0.C:48
inline u32 rmRangedRandom(u32 range) {
    static u32 seed = 0x12345678;
    if (range == 0) {
        return 0;
    }
    seed ^= 0x1D872B41;
    seed ^= (seed >> 5);
    seed ^= (seed << 27);
    return seed % range;
}

// rmDiv16i - 16.16 fixed-point division (PSX: 0x8007D8B4)
// Returns (a << 16) / b as a 16.16 fixed-point result.
// Source: C:\chan\devsys\psx\radlib\SOURCE\MATH\MULTDIV\DIVIDE.C:30
inline s32 rmDiv16i(s32 a, s32 b) {
    if (b == 0) return (a >= 0) ? 0x7FFFFFFF : -0x7FFFFFFF;
    return (s32)(((s64)a << 16) / b);
}

// rmV3Normalize - normalize integer vector to 16.16 fixed-point unit vector
// PSX: 0x8009DA64 - calls rmMag3, rmInverse16, then scales components.
// If magnitude is zero, returns {0x10000, 0, 0}.
// Source: C:\chan\devsys\psx\radlib\SOURCE\MATH\VECTOR\VECT3D.CPP:22
inline void rmV3Normalize(LVector* out, const LVector* in) {
    s32 mag = (s32)rmMag3((f32)in->x, (f32)in->y, (f32)in->z);
    if (mag == 0) {
        out->x = 0x10000; out->y = 0; out->z = 0;
        return;
    }
    out->x = (s32)(((s64)in->x << 16) / mag);
    out->y = (s32)(((s64)in->y << 16) / mag);
    out->z = (s32)(((s64)in->z << 16) / mag);
}

// rmV3Dot - integer 3D dot product
inline s32 rmV3Dot(const LVector* a, const LVector* b) {
    return (s32)((s64)a->x * (s64)b->x
        + (s64)a->y * (s64)b->y
        + (s64)a->z * (s64)b->z);
}

// rmV3Scale - scale vector by 16.16 fixed-point scalar
// PSX: 0x8009346C
// Source: C:\chan\devsys\psx\radlib\SOURCE\MATH\VECTOR\VECT3D.CPP
inline void rmV3Scale(LVector* out, const LVector* in, s32 scale) {
    out->x = (s32)(((s64)in->x * (s64)scale) >> 16);
    out->y = (s32)(((s64)in->y * (s64)scale) >> 16);
    out->z = (s32)(((s64)in->z * (s64)scale) >> 16);
}

// rmSin16 - 16.16 fixed-point sine from PSX binary angle (0..65535 = full circle)
// PSX: 0x80078364 (lookup table based)
inline s32 rmSin16(s32 angle) {
    f32 rad = ANGLE2RAD(angle & 0xFFFF);
    return FLOAT_TO_FIX16(std::sin(rad));
}

// rmCos16 - 16.16 fixed-point cosine from PSX binary angle
inline s32 rmCos16(s32 angle) {
    return rmSin16(angle + 0x4000);
}


// rmATan216 - two-argument arctangent returning PSX binary angle (0..65535)
// PSX: 0x80113CF0
inline u16 rmATan216(f32 x, f32 y) {
    f32 rad = std::atan2(y, x);
    s32 angle = RAD2ANGLE(rad) & 0xFFFF;
    return static_cast<u16>(angle);
}


// Convenience: build a LookAt view matrix (used by PC improved debug cam)
// Not a PSX function - wraps the existing LookAt from core.h for Vec3.
inline Mat4 p3dLookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    return LookAt(eye.x, eye.y, eye.z, target.x, target.y, target.z, up.x, up.y, up.z);
}
