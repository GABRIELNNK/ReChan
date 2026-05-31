// lvector.h - PSX integer vector types
// LVector = PSX tagLVector (s32 x,y,z) - 12 bytes, world-space positions
// SVector = PSX _RMVECT16 (s16 x,y,z,pad) - 8 bytes, direction/offset vectors
#pragma once

#include <cstdint>
#include <cmath>

using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using f32 = float;

struct Vec3;

// PSX 3D integer vector (tagLVector) - 16.16 fixed-point world units
struct LVector {
    s32 x, y, z = 0;

    LVector operator+(const LVector& r) const { return {x + r.x, y + r.y, z + r.z}; }
    LVector operator-(const LVector& r) const { return {x - r.x, y - r.y, z - r.z}; }
    LVector operator-() const { return {-x, -y, -z}; }

    LVector& operator+=(const LVector& r) { x += r.x; y += r.y; z += r.z; return *this; }
    LVector& operator-=(const LVector& r) { x -= r.x; y -= r.y; z -= r.z; return *this; }

    bool operator==(const LVector& r) const { return x == r.x && y == r.y && z == r.z; }
    bool operator!=(const LVector& r) const { return !(*this == r); }

    // 16.16 fixed-point multiply: (component * s) >> 16
    LVector FixMul(s32 s) const {
        return {
            (s32)(((s64)x * s) >> 16),
            (s32)(((s64)y * s) >> 16),
            (s32)(((s64)z * s) >> 16)
        };
    }

    // 16.16 fixed-point dot product: (a.x*b.x + a.y*b.y + a.z*b.z) >> 16
    s32 FixDot(const LVector& r) const {
        return (s32)((((s64)x * r.x) + ((s64)y * r.y) + ((s64)z * r.z)) >> 16);
    }

    // Integer dot product (no shift)
    s64 Dot(const LVector& r) const {
        return (s64)x * r.x + (s64)y * r.y + (s64)z * r.z;
    }

    // Integer cross product
    LVector Cross(const LVector& r) const {
        return {
            (s32)(((s64)y * r.z - (s64)z * r.y) >> 16),
            (s32)(((s64)z * r.x - (s64)x * r.z) >> 16),
            (s32)(((s64)x * r.y - (s64)y * r.x) >> 16)
        };
    }

    // Integer magnitude (sqrt of sum of squares)
    s32 Magnitude() const {
        return (s32)std::sqrt((f32)((s64)x * x + (s64)y * y + (s64)z * z));
    }

    // 2D magnitude in XZ plane
    s32 MagnitudeXZ() const {
        return (s32)std::sqrt((f32)((s64)x * x + (s64)z * z));
    }

    s64 MagnitudeSqr() const {
        return (s64)x * x + (s64)y * y + (s64)z * z;
    }

    void Set(s32 x_, s32 y_, s32 z_) { x = x_; y = y_; z = z_; }
    void Clear() { x = y = z = 0; }

    bool IsZero() const { return x == 0 && y == 0 && z == 0; }

    // Convert to float Vec3 (raw cast, no fixed-point conversion)
    inline Vec3 ToVec3() const;
    // Convert to float Vec3 with 16.16 -> float conversion
    inline Vec3 ToVec3Fix16() const;
};

// PSX short vector (_RMVECT16)
struct SVector {
    s16 x, y, z, pad;
};
