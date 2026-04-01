// p3dmath.h — PC float equivalents of PSX Pure3D math functions
// Original PSX: _RMVECT16 (3×s32), MATRIX (3×5 s16 Q12 fixed-point)
// PC: Vec3 (3×f32), Mat4 (4×4 f32) — same results, float precision
//
// PSX function names preserved for 1:1 mapping with decompiled code.
#pragma once

#include "p3d/vector.h"
#include "p3d/matrix.h"
#include <cmath>


// PSX math function equivalents (float versions)
// Names match decompiled PSX code for easy cross-reference.


// PSX binary angle: 65536 = 2π. Convert to/from radians.
constexpr f32 P3D_ANGLE_TO_RAD = (2.0f * 3.14159265358979323846f) / 65536.0f;
constexpr f32 P3D_RAD_TO_ANGLE = 65536.0f / (2.0f * 3.14159265358979323846f);

// --------------------------------------------------------------------------
// p3dBuildRotMatrixZ / Y / X — single-axis rotation into Mat4
// PSX: builds into 3×5 s16 Q12 MATRIX. PC: builds into 4×4 float Mat4.
// --------------------------------------------------------------------------
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

// --------------------------------------------------------------------------
// p3dBuildRotMatrixZYX — compose Z*Y*X rotation (PSX Euler order)
// PSX: 0x80093D58 — composes three rotation matrices
// PC: builds directly from combined trig (avoids 3 matrix multiplies)
// --------------------------------------------------------------------------
inline void p3dBuildRotMatrixZYX(f32 az, f32 ay, f32 ax, Mat4& m) {
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

// Overload taking PSX angle units (s32) — converts internally
inline void p3dBuildRotMatrixZYX(s32 az, s32 ay, s32 ax, Mat4& m) {
    p3dBuildRotMatrixZYX(
        static_cast<f32>(az & 0xFFFF) * P3D_ANGLE_TO_RAD,
        static_cast<f32>(ay & 0xFFFF) * P3D_ANGLE_TO_RAD,
        static_cast<f32>(ax & 0xFFFF) * P3D_ANGLE_TO_RAD,
        m);
}

// --------------------------------------------------------------------------
// p3dVecTimesMatrix — transform vector by matrix (rotation + translation)
// PSX: 0x80094568 — result = vec * rotPart + translation
// --------------------------------------------------------------------------
inline Vec3 p3dVecTimesMatrix(const Vec3& v, const Mat4& m) {
    return {
        v.x * m.m[0] + v.y * m.m[4] + v.z * m.m[8]  + m.m[12],
        v.x * m.m[1] + v.y * m.m[5] + v.z * m.m[9]  + m.m[13],
        v.x * m.m[2] + v.y * m.m[6] + v.z * m.m[10] + m.m[14]
    };
}

// --------------------------------------------------------------------------
// p3dVecTimesRotMatrix — rotation only (no translation)
// PSX: 0x800945EC
// --------------------------------------------------------------------------
inline Vec3 p3dVecTimesRotMatrix(const Vec3& v, const Mat4& m) {
    return {
        v.x * m.m[0] + v.y * m.m[4] + v.z * m.m[8],
        v.x * m.m[1] + v.y * m.m[5] + v.z * m.m[9],
        v.x * m.m[2] + v.y * m.m[6] + v.z * m.m[10]
    };
}

// --------------------------------------------------------------------------
// p3dBuildTransMatrix — set translation part of matrix
// PSX: writes to MATRIX translation columns
// --------------------------------------------------------------------------
inline void p3dBuildTransMatrix(f32 x, f32 y, f32 z, Mat4& m) {
    m = Mat4();
    m.m[12] = x; m.m[13] = y; m.m[14] = z;
}

// --------------------------------------------------------------------------
// p3dFillHeadingMatrix — build orientation matrix from heading + up vectors
// PSX: 0x800947F0 — builds right/up/forward basis
// --------------------------------------------------------------------------
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

// --------------------------------------------------------------------------
// rmMag2 — 2D magnitude (PSX: 0x80113984)
// --------------------------------------------------------------------------
inline f32 rmMag2(f32 x, f32 y) {
    return std::sqrt(x * x + y * y);
}

// --------------------------------------------------------------------------
// rmMag3 — 3D magnitude (PSX: 0x80113B90)
// --------------------------------------------------------------------------
inline f32 rmMag3(f32 x, f32 y, f32 z) {
    return std::sqrt(x * x + y * y + z * z);
}

// --------------------------------------------------------------------------
// rmATan216 — two-argument arctangent returning PSX binary angle (0..65535)
// PSX: 0x80113CF0
// --------------------------------------------------------------------------
inline u16 rmATan216(f32 x, f32 y) {
    f32 rad = std::atan2(y, x);
    s32 angle = static_cast<s32>(rad * P3D_RAD_TO_ANGLE) & 0xFFFF;
    return static_cast<u16>(angle);
}

// --------------------------------------------------------------------------
// Convenience: build a LookAt view matrix (used by PC improved debug cam)
// Not a PSX function — wraps the existing LookAt from core.h for Vec3.
// --------------------------------------------------------------------------
inline Mat4 p3dLookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    return LookAt(eye.x, eye.y, eye.z, target.x, target.y, target.z, up.x, up.y, up.z);
}
