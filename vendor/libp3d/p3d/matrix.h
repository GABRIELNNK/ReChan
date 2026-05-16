// matrix.h - Mat4 (4x4 float matrix)
// PSX MATRIX: 3x5 s16 Q12 fixed-point. PC: 4x4 float column-major.
#pragma once

#include <cmath>

using f32 = float;

struct Vec3;

// 4x4 column-major float matrix (OpenGL convention)
// Column c, row r -> m[c*4 + r]
//   col 0 = right (X basis), col 1 = up (Y basis),
//   col 2 = forward (Z basis), col 3 = translation
struct Mat4 {
    f32 m[16];

    Mat4() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 Identity() { return Mat4(); }

    const f32* Data() const { return m; }

    // element access: (row, col) -> m[col*4 + row]
    f32  operator()(int row, int col) const { return m[col * 4 + row]; }
    f32& operator()(int row, int col)       { return m[col * 4 + row]; }

    // column access (basis vectors)
    f32 Right(int row)   const { return m[row]; }         // col 0
    f32 Up(int row)      const { return m[4 + row]; }     // col 1
    f32 Forward(int row) const { return m[8 + row]; }     // col 2

    // translation (col 3)
    f32  GetTransX() const { return m[12]; }
    f32  GetTransY() const { return m[13]; }
    f32  GetTransZ() const { return m[14]; }
    void SetTranslation(f32 x, f32 y, f32 z) { m[12] = x; m[13] = y; m[14] = z; }

    // scale the 3x3 rotation part uniformly
    void ScaleRotation(f32 s) {
        for (int i = 0; i < 12; i++) {
            m[i] *= s;
        }
    }

    // matrix multiply
    Mat4 operator*(const Mat4& b) const;
};

inline Mat4 Mat4::operator*(const Mat4& b) const {
    Mat4 r;
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row) {
            r.m[c * 4 + row] = 0.0f;
            for (int k = 0; k < 4; ++k)
                r.m[c * 4 + row] += m[k * 4 + row] * b.m[c * 4 + k];
        }
    return r;
}

// transform point (rotation + translation)
inline void Mat4TransformPoint(const Mat4& m, f32 ix, f32 iy, f32 iz,
                               f32& ox, f32& oy, f32& oz) {
    ox = m.m[0] * ix + m.m[4] * iy + m.m[8]  * iz + m.m[12];
    oy = m.m[1] * ix + m.m[5] * iy + m.m[9]  * iz + m.m[13];
    oz = m.m[2] * ix + m.m[6] * iy + m.m[10] * iz + m.m[14];
}

// transform vector (rotation only, no translation)
inline void Mat4TransformDir(const Mat4& m, f32 ix, f32 iy, f32 iz,
                             f32& ox, f32& oy, f32& oz) {
    ox = m.m[0] * ix + m.m[4] * iy + m.m[8]  * iz;
    oy = m.m[1] * ix + m.m[5] * iy + m.m[9]  * iz;
    oz = m.m[2] * ix + m.m[6] * iy + m.m[10] * iz;
}

inline Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near = -1.0f, f32 far = 1.0f) {
    Mat4 o;
    o.m[0] = 2.0f / (right - left);
    o.m[5] = 2.0f / (top - bottom);
    o.m[10] = -2.0f / (far - near);
    o.m[12] = -(right + left) / (right - left);
    o.m[13] = -(top + bottom) / (top - bottom);
    o.m[14] = -(far + near) / (far - near);
    o.m[15] = 1.0f;
    return o;
}

inline Mat4 Perspective(f32 fovY, f32 aspect, f32 near, f32 far) {
    f32 tanHalf = 1.0f / std::tan(fovY * 0.5f);
    Mat4 p;
    p.m[0] = tanHalf / aspect;
    p.m[5] = tanHalf;
    p.m[10] = -(far + near) / (far - near);
    p.m[11] = -1.0f;
    p.m[14] = -(2.0f * far * near) / (far - near);
    p.m[15] = 0.0f;
    return p;
}

inline Mat4 PerspectiveReversedZ(f32 fovY, f32 aspect, f32 near, f32 far) {
    f32 tanHalf = 1.0f / std::tan(fovY * 0.5f);
    Mat4 p;
    p.m[0] = tanHalf / aspect;
    p.m[5] = tanHalf;
    p.m[10] = near / (far - near);
    p.m[11] = -1.0f;
    p.m[14] = (far * near) / (far - near);
    p.m[15] = 0.0f;
    return p;
}

inline Mat4 PerspectiveReversedZTangents(f32 tanHalfX, f32 tanHalfY, f32 near, f32 far) {
    if (tanHalfX < 0.0001f) tanHalfX = 0.0001f;
    if (tanHalfY < 0.0001f) tanHalfY = 0.0001f;

    Mat4 p;
    p.m[0] = 1.0f / tanHalfX;
    p.m[5] = 1.0f / tanHalfY;
    p.m[10] = near / (far - near);
    p.m[11] = -1.0f;
    p.m[14] = (far * near) / (far - near);
    p.m[15] = 0.0f;
    return p;
}

inline Mat4 LookAt(f32 ex, f32 ey, f32 ez, f32 cx, f32 cy, f32 cz, f32 ux, f32 uy, f32 uz) {
    // forward = normalize(eye - center)
    f32 fx = ex - cx, fy = ey - cy, fz = ez - cz;
    f32 fl = 1.0f / std::sqrt(fx * fx + fy * fy + fz * fz);
    fx *= fl; fy *= fl; fz *= fl;
    // right = normalize(up x forward)
    f32 rx = uy * fz - uz * fy, ry = uz * fx - ux * fz, rz = ux * fy - uy * fx;
    f32 rl = 1.0f / std::sqrt(rx * rx + ry * ry + rz * rz);
    rx *= rl; ry *= rl; rz *= rl;
    // up = forward x right
    f32 upx = fy * rz - fz * ry, upy = fz * rx - fx * rz, upz = fx * ry - fy * rx;
    Mat4 m;
    m.m[0] = rx;  m.m[4] = ry;  m.m[8] = rz;  m.m[12] = -(rx * ex + ry * ey + rz * ez);
    m.m[1] = upx; m.m[5] = upy; m.m[9] = upz; m.m[13] = -(upx * ex + upy * ey + upz * ez);
    m.m[2] = fx;  m.m[6] = fy;  m.m[10] = fz;  m.m[14] = -(fx * ex + fy * ey + fz * ez);
    m.m[3] = 0;   m.m[7] = 0;   m.m[11] = 0;   m.m[15] = 1.0f;
    return m;
}
