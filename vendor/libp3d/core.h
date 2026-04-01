// core.h
#pragma once

#include <cstdint>
#include <cstdio>
#include <cmath>

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
struct Fixed {
    s32 raw;

    static constexpr int FRAC_BITS = 12;
    static constexpr f32 SCALE = 1.0f / (1 << FRAC_BITS);

    f32 ToFloat() const { return raw * SCALE; }
    static Fixed FromFloat(f32 v) { return { static_cast<s32>(v * (1 << FRAC_BITS)) }; }
};

// Math types = split into separate headers
#include "p3d/vector.h"
#include "p3d/matrix.h"
