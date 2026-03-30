// core.h
#ifndef CORE_H
#define CORE_H

#include <cstdint>
#include <cstdio>

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

#endif // CORE_H
