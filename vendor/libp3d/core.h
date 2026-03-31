// core.h
#ifndef CORE_H
#define CORE_H

#include <cstdint>
#include <cstdio>
#include <cmath>

// Types
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using f32 = float;
using f64 = double;

#if defined(_WIN32) || defined(_WIN64)
    #ifndef RC_PLATFORM_WINDOWS
        #define RC_PLATFORM_WINDOWS
    #endif
#endif

// Debug
#ifdef RC_DEBUG
    #define RC_LOG(fmt, ...)   std::printf(fmt "\n", ##__VA_ARGS__)
    #define RC_ERR(fmt, ...)   std::fprintf(stderr, "[ERR] " fmt "\n", ##__VA_ARGS__)
    #define RC_WARN(fmt, ...)  std::fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#else
    #define RC_LOG(fmt, ...)   ((void)0)
    #define RC_ERR(fmt, ...)   ((void)0)
    #define RC_WARN(fmt, ...)  ((void)0)
#endif

// IDA address marker (no-op)
#define MARKFUNCTION(addr) ((void)0)

// PSX fixed-point (20.12)
struct Fixed
{
    s32 raw;

    static constexpr int FRAC_BITS = 12;
    static constexpr f32 SCALE = 1.0f / (1 << FRAC_BITS);

    f32 ToFloat() const { return raw * SCALE; }
    static Fixed FromFloat(f32 v) { return { static_cast<s32>(v * (1 << FRAC_BITS)) }; }
};

struct Mat4
{
    f32 m[16];

    Mat4()
    {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    const f32* Data() const { return m; }
};

inline Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near = -1.0f, f32 far = 1.0f)
{
    Mat4 o;
    o.m[0]  =  2.0f / (right - left);
    o.m[5]  =  2.0f / (top - bottom);
    o.m[10] = -2.0f / (far - near);
    o.m[12] = -(right + left) / (right - left);
    o.m[13] = -(top + bottom) / (top - bottom);
    o.m[14] = -(far + near) / (far - near);
    o.m[15] = 1.0f;
    return o;
}

inline Mat4 Perspective(f32 fovY, f32 aspect, f32 near, f32 far)
{
    f32 tanHalf = 1.0f / std::tan(fovY * 0.5f);
    Mat4 p;
    p.m[0]  = tanHalf / aspect;
    p.m[5]  = tanHalf;
    p.m[10] = -(far + near) / (far - near);
    p.m[11] = -1.0f;
    p.m[14] = -(2.0f * far * near) / (far - near);
    p.m[15] = 0.0f;
    return p;
}

inline Mat4 Mat4Multiply(const Mat4& a, const Mat4& b)
{
    Mat4 r;
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row) {
            r.m[c * 4 + row] = 0.0f;
            for (int k = 0; k < 4; ++k)
                r.m[c * 4 + row] += a.m[k * 4 + row] * b.m[c * 4 + k];
        }
    return r;
}

inline Mat4 LookAt(f32 ex, f32 ey, f32 ez, f32 cx, f32 cy, f32 cz, f32 ux, f32 uy, f32 uz)
{
    // forward = normalize(eye - center)
    f32 fx = ex - cx, fy = ey - cy, fz = ez - cz;
    f32 fl = 1.0f / std::sqrt(fx*fx + fy*fy + fz*fz);
    fx *= fl; fy *= fl; fz *= fl;
    // right = normalize(up x forward)
    f32 rx = uy*fz - uz*fy, ry = uz*fx - ux*fz, rz = ux*fy - uy*fx;
    f32 rl = 1.0f / std::sqrt(rx*rx + ry*ry + rz*rz);
    rx *= rl; ry *= rl; rz *= rl;
    // up = forward x right
    f32 upx = fy*rz - fz*ry, upy = fz*rx - fx*rz, upz = fx*ry - fy*rx;
    Mat4 m;
    m.m[0] = rx;  m.m[4] = ry;  m.m[8]  = rz;  m.m[12] = -(rx*ex + ry*ey + rz*ez);
    m.m[1] = upx; m.m[5] = upy; m.m[9]  = upz; m.m[13] = -(upx*ex + upy*ey + upz*ez);
    m.m[2] = fx;  m.m[6] = fy;  m.m[10] = fz;  m.m[14] = -(fx*ex + fy*ey + fz*ez);
    m.m[3] = 0;   m.m[7] = 0;   m.m[11] = 0;   m.m[15] = 1.0f;
    return m;
}

#endif // CORE_H
