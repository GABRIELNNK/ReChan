// vector.h - Vec3 (PC float equivalent of PSX _RMVECT16)
// PSX layout: { s32 x, y, z } - 12 bytes, integer world units
// PC layout:  { f32 x, y, z } - 12 bytes, float world units
#pragma once

#include <cmath>

using f32 = float;
using s32 = int32_t;

struct LVector;

struct Vec3 {
    f32 x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& r) const { return {x + r.x, y + r.y, z + r.z}; }
    Vec3 operator-(const Vec3& r) const { return {x - r.x, y - r.y, z - r.z}; }
    Vec3 operator*(f32 s) const { return {x * s, y * s, z * s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& r) { x += r.x; y += r.y; z += r.z; return *this; }
    Vec3& operator-=(const Vec3& r) { x -= r.x; y -= r.y; z -= r.z; return *this; }
    Vec3& operator*=(f32 s) { x *= s; y *= s; z *= s; return *this; }

    f32 Dot(const Vec3& r) const { return x * r.x + y * r.y + z * r.z; }
    Vec3 Cross(const Vec3& r) const {
        return {y * r.z - z * r.y,
                z * r.x - x * r.z,
                x * r.y - y * r.x};
    }

    f32 Magnitude() const { return std::sqrt(x * x + y * y + z * z); }
    f32 MagnitudeSqr() const { return x * x + y * y + z * z; }

    // PSX: rmMag2 — 2D magnitude in XZ plane
    f32 MagnitudeXZ() const { return std::sqrt(x * x + z * z); }

    // Normalize in place, returns original magnitude
    f32 Normalize() {
        f32 m = Magnitude();
        if (m > 0.00001f) { f32 inv = 1.0f / m; x *= inv; y *= inv; z *= inv; }
        return m;
    }

    // Return normalized copy without modifying this
    Vec3 Normalized() const {
        f32 m = Magnitude();
        if (m > 0.00001f) { f32 inv = 1.0f / m; return {x * inv, y * inv, z * inv}; }
        return {0, 0, 0};
    }

    void Set(f32 x_, f32 y_, f32 z_) { x = x_; y = y_; z = z_; }
    void Clear() { x = y = z = 0; }

    // Linear interpolation: this + (b - this) * t
    Vec3 Lerp(const Vec3& b, f32 t) const {
        return {x + (b.x - x) * t,
                y + (b.y - y) * t,
                z + (b.z - z) * t};
    }

    // Convert to LVector (raw cast, no fixed-point conversion)
    inline LVector ToLVector() const;
    // Convert to LVector with float -> 16.16 conversion
    inline LVector ToLVectorFix16() const;
};

inline Vec3 operator*(f32 s, const Vec3& v) { return {s * v.x, s * v.y, s * v.z}; }
