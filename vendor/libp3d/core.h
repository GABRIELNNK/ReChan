// core.h - shared types and platform detection
#pragma once

#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

// Types
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using f32 = float;
using f64 = double;

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#endif

// PSX fixed-point (20.12)
struct Fixed {
    s32 raw;

    static constexpr int FRAC_BITS = 12;
    static constexpr f32 SCALE = 1.0f / (1 << FRAC_BITS);

    f32 ToFloat() const { return raw * SCALE; }
    static Fixed FromFloat(f32 v) { return { static_cast<s32>(v * (1 << FRAC_BITS)) }; }
};

// IDA address marker (no-op on PC)
#define MARKFUNCTION(addr) ((void)0)

#ifndef ASSERT
#define ASSERT(expr) assert(expr)
#endif

// Math types = split into separate headers
#include "p3d/vector.h"
#include "p3d/matrix.h"
#include "p3d/lvector.h"

inline Vec3 LVector::ToVec3() const {
    return Vec3((f32)x, (f32)y, (f32)z);
}
inline Vec3 LVector::ToVec3Fix16() const {
    return Vec3((f32)x / 65536.0f, (f32)y / 65536.0f, (f32)z / 65536.0f);
}
inline LVector Vec3::ToLVector() const {
    return {(s32)x, (s32)y, (s32)z};
}
inline LVector Vec3::ToLVectorFix16() const {
    return {(s32)(x * 65536.0f), (s32)(y * 65536.0f), (s32)(z * 65536.0f)};
}
