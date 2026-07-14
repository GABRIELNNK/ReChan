#pragma once

#include "core.h"
#include "p3d/lvector.h"
#include "p3d/p3dmath.h"
#include <cmath>
#include <cstdlib>

inline s32 PsxRandom0() {
    return std::rand();
}

inline s32 PsxMulShift16Signed(s32 a, s32 b) {
    return (s32)(((s64)a * (s64)b) >> 16);
}

inline s32 PsxShiftLeft16Wrap(s32 value) {
    return (s32)((u32)value << 16);
}

// Integer floor sqrt used by PSX fixed-point magnitude paths.
inline u32 PsxSqrtFloorU32(u32 value) {
    u32 result = 0;
    u32 bit = 1u << 30;

    while (bit > value) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        }
        else {
            result >>= 1;
        }
        bit >>= 2;
    }

    return result;
}

// PSX rmMag3 integer path (radlib MAG3.CPP).
inline u32 PsxRmMag3(s32 x, s32 y, s32 z) {
    u8 shift = 0;
    if (x < 0) {
        x = -x;
    }
    if (y < 0) {
        y = -y;
    }
    if (z < 0) {
        z = -z;
    }

    while ((x + y + z) > 46339) {
        x >>= 2;
        y >>= 2;
        z >>= 2;
        shift += 2;
    }

    const u32 lengthSquared = (u32)(x * x + y * y + z * z);
    return PsxSqrtFloorU32(lengthSquared) << shift;
}

// PSX rmDiv16i path (radlib DIVIDE.C:30).
inline s32 PsxRmDiv16i(s32 numerator, s32 denominator) {
    s32 shift = 16;
    bool negative = false;

    u32 denominatorAbs = (u32)denominator;
    if (denominator < 0) {
        denominatorAbs = (u32)(0u - denominatorAbs);
        negative = true;
    }

    u32 numeratorAbs = (u32)numerator;
    if (numerator < 0) {
        numeratorAbs = (u32)(0u - numeratorAbs);
        negative = !negative;
    }

    if (numeratorAbs <= 0x1FFFFFFFu) {
        while (shift != 0) {
            numeratorAbs *= 4u;
            shift -= 2;
            if (numeratorAbs > 0x1FFFFFFFu) {
                break;
            }
        }
    }

    const u32 scaledDenominator = denominatorAbs >> shift;
    if (scaledDenominator != 0) {
        const s32 quotient = (s32)(numeratorAbs / scaledDenominator);
        return negative ? -quotient : quotient;
    }

    if (numeratorAbs == 0) {
        return 0;
    }

    return negative ? (s32)0x80000001u : 0x7FFFFFFF;
}

template <typename T>
inline T PsxMin(T leftValue, T rightValue) {
    return (leftValue < rightValue) ? leftValue : rightValue;
}

template <typename T>
inline T PsxMax(T leftValue, T rightValue) {
    return (leftValue > rightValue) ? leftValue : rightValue;
}

template <typename T>
inline T PsxClamp(T value, T minValue, T maxValue) {
    return PsxMin(PsxMax(value, minValue), maxValue);
}

inline u32 PsxCeilPositiveF32ToU32(f32 value) {
    if (value <= 0.0f) {
        return 0;
    }

    return static_cast<u32>(std::ceil(value));
}

inline s32 PsxAbsS32(s32 value) {
    return value < 0 ? -value : value;
}

inline s32 PsxAbsDiffS32(s32 a, s32 b) {
    const s32 d = a - b;
    return (d < 0) ? -d : d;
}

inline s32 PsxClipAngle360(s32 angle) {
    while (angle > 0xFFFF) {
        angle -= 0xFFFF;
    }
    while (angle < 0) {
        angle += 0xFFFF;
    }
    return angle;
}

inline s32 PsxAngleDelta16(s32 current, s32 previous) {
    return (s16)((u16)current - (u16)previous);
}

inline s32 PsxLerpAngle16(s32 previous, s32 current, f32 alpha) {
    const s32 step = PsxAngleDelta16(current, previous);
    return (u16)(previous + (s32)((f32)step * alpha));
}

inline s32 PsxTruncToS32(f32 value) {
    return static_cast<s32>(std::trunc(value));
}

inline f32 PsxTanHalfFromFov(s32 fov) {
    return FIX16_TO_FLOAT(fov);
}

inline f32 PsxScaleHorizontalTanForAspect(f32 tanHalfX, f32 aspectRatio, f32 baseAspectRatio) {
    if (baseAspectRatio <= 0.0f || aspectRatio <= 0.0f) {
        return tanHalfX;
    }

    return tanHalfX * (aspectRatio / baseAspectRatio);
}

inline f32 PsxProjectionDistanceFromTan(s32 screenCenter, f32 tanHalf) {
    return (tanHalf > 0.0f) ? (static_cast<f32>(screenCenter) / tanHalf) : 0.0f;
}

inline s32 PsxProjectScreenCoord(s32 screenCenter, s32 viewValue, f32 projectionDistance, s32 viewDepth) {
    const s32 denominator = (viewDepth == 0) ? 1 : viewDepth;
    return screenCenter + PsxTruncToS32((static_cast<f32>(viewValue) * projectionDistance) / static_cast<f32>(denominator));
}

inline void PsxTransformPointToPort(const Mat4& viewMatrix, s32 inX, s32 inY, s32 inZ,
                                    s32* outX, s32* outY, s32* outZ) {
    f32 transformedX = 0.0f;
    f32 transformedY = 0.0f;
    f32 transformedZ = 0.0f;
    Mat4TransformPoint(viewMatrix,
                       static_cast<f32>(inX),
                       static_cast<f32>(inY),
                       static_cast<f32>(inZ),
                       transformedX,
                       transformedY,
                       transformedZ);

    *outX = PsxTruncToS32(transformedX);
    *outY = PsxTruncToS32(-transformedY);
    *outZ = PsxTruncToS32(-transformedZ);
}

inline s32 PsxVecLengthSquaredQuarter(s32 xValue, s32 yValue, s32 zValue) {
    const s64 xSquared = static_cast<s64>(xValue) * static_cast<s64>(xValue);
    const s64 ySquared = static_cast<s64>(yValue) * static_cast<s64>(yValue);
    const s64 zSquared = static_cast<s64>(zValue) * static_cast<s64>(zValue);
    return static_cast<s32>((xSquared >> 2) + (ySquared >> 2) + (zSquared >> 2));
}

inline s32 PsxAsin16FromFix16Clamped(s32 value) {
    f32 clamped = FIX16_TO_FLOAT(value);
    if (clamped < -1.0f) {
        clamped = -1.0f;
    }
    else if (clamped > 1.0f) {
        clamped = 1.0f;
    }

    return RAD2ANGLE(std::asin(clamped));
}

inline s32 PsxAcos16FromFix16Clamped(s32 value) {
    f32 clamped = FIX16_TO_FLOAT(value);
    if (clamped < -1.0f) {
        clamped = -1.0f;
    }
    else if (clamped > 1.0f) {
        clamped = 1.0f;
    }

    return RAD2ANGLE(std::acos(clamped));
}

inline void PsxV3Cross16(LVector* out, const LVector* lhs, const LVector* rhs) {
    out->x = (s32)((((s64)lhs->y * rhs->z) >> 16) - (((s64)lhs->z * rhs->y) >> 16));
    out->y = (s32)((((s64)lhs->z * rhs->x) >> 16) - (((s64)lhs->x * rhs->z) >> 16));
    out->z = (s32)((((s64)lhs->x * rhs->y) >> 16) - (((s64)lhs->y * rhs->x) >> 16));
}

inline void PsxInverseOrthMatrix(const Mat4& matrix, Mat4& inverse) {
    inverse = Mat4();

    inverse.m[0] = matrix.m[0];
    inverse.m[1] = matrix.m[4];
    inverse.m[2] = matrix.m[8];
    inverse.m[4] = matrix.m[1];
    inverse.m[5] = matrix.m[5];
    inverse.m[6] = matrix.m[9];
    inverse.m[8] = matrix.m[2];
    inverse.m[9] = matrix.m[6];
    inverse.m[10] = matrix.m[10];

    const f32 tx = matrix.GetTransX();
    const f32 ty = matrix.GetTransY();
    const f32 tz = matrix.GetTransZ();
    inverse.m[12] = -(tx * inverse.m[0] + ty * inverse.m[4] + tz * inverse.m[8]);
    inverse.m[13] = -(tx * inverse.m[1] + ty * inverse.m[5] + tz * inverse.m[9]);
    inverse.m[14] = -(tx * inverse.m[2] + ty * inverse.m[6] + tz * inverse.m[10]);
}

inline void PsxVecTimesMatrixTrunc(const LVector& in, const Mat4& matrix, LVector& out) {
    out.x = PsxTruncToS32((f32)in.x * matrix.m[0] + (f32)in.y * matrix.m[4] + (f32)in.z * matrix.m[8] + matrix.m[12]);
    out.y = PsxTruncToS32((f32)in.x * matrix.m[1] + (f32)in.y * matrix.m[5] + (f32)in.z * matrix.m[9] + matrix.m[13]);
    out.z = PsxTruncToS32((f32)in.x * matrix.m[2] + (f32)in.y * matrix.m[6] + (f32)in.z * matrix.m[10] + matrix.m[14]);
}

inline void PsxCartesianToSpherical(const LVector& cartesian, LVector& spherical) {
    const s32 mag = (s32)rmMag3((f32)cartesian.x, (f32)cartesian.y, (f32)cartesian.z);
    if (mag == 0) {
        spherical = { 0, 0, 0 };
        return;
    }

    s32 xDiv = rmDiv16i(cartesian.x, mag);
    if (xDiv > FIX16_ONE) {
        xDiv = FIX16_ONE;
    }
    else if (xDiv < -FIX16_ONE) {
        xDiv = -FIX16_ONE;
    }

    const s32 polar = PsxAcos16FromFix16Clamped(xDiv);
    const s32 polarSin = rmSin16(polar);

    s32 yDiv = 0;
    const s32 denom = (s32)(((s64)mag * polarSin) >> 16);
    if (denom != 0) {
        yDiv = rmDiv16i(cartesian.y, denom);
    }
    if ((u32)(yDiv + FIX16_ONE) > 0x20000u) {
        yDiv = (yDiv >= 0) ? 0xFFFF : -0xFFFF;
    }

    s32 azimuth = PsxAsin16FromFix16Clamped(yDiv);
    if (cartesian.z < 0) {
        azimuth = 0x8000 - azimuth;
    }

    spherical.x = mag;
    spherical.y = azimuth;
    spherical.z = polar;
}

inline void PsxSphericalToCartesian(const LVector& spherical, LVector& cartesian) {
    const s32 radiusSin = (s32)(((s64)spherical.x * rmSin16(spherical.z)) >> 16);
    LVector converted = {};
    converted.x = (s32)(((s64)radiusSin * rmSin16(spherical.y + 0x4000)) >> 16);
    converted.y = (s32)(((s64)radiusSin * rmSin16(spherical.y)) >> 16);
    converted.z = (s32)(((s64)spherical.x * rmSin16(spherical.z + 0x4000)) >> 16);

    cartesian.x = converted.z;
    cartesian.y = converted.y;
    cartesian.z = converted.x;
}
