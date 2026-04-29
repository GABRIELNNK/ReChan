#include "gen/fxp.h"
#include "p3d/p3dmath.h"

// PSX: IsPointInFieldOf__FRC10tagLVectorT0lll (FXP.CPP:147)
bool IsPointInFieldOf(const LVector& point, const LVector& origin, s32 orientation, s32 left, s32 right) {
    MARKFUNCTION(0x8009DD1C);

    const s32 angle = rmATan216((f32)(point.x - origin.x), (f32)(point.z - origin.z));
    return IsAngleInFieldOf(angle, 0x4000 - orientation, left, right);
}

// PSX: ClipAngle__FRl (FXP.CPP:164)
s32 ClipAngle(s32& angle) {
    MARKFUNCTION(0x8009DD88);

    while (angle > 0xFFFF) {
        angle -= 0xFFFF;
    }

    while (angle < 0) {
        angle += 0xFFFF;
    }

    return angle;
}

// PSX: IsAngleInFieldOf__Fllll (FXP.CPP:184)
bool IsAngleInFieldOf(s32 angle, s32 facing, s32 left, s32 right) {
    MARKFUNCTION(0x8009DDE8);

    ClipAngle(angle);
    ClipAngle(facing);

    const s32 minAngle = facing - right;
    const s32 maxAngle = facing + left;

    // PSX tests the direct interval and +/-0xFFFF wrapped variants.
    const bool lowerWrap = (angle <= (maxAngle - 0xFFFF)) && (angle >= (minAngle - 0xFFFF));
    const bool direct = (angle <= maxAngle) && (angle >= minAngle);
    const bool upperWrap = (angle <= (maxAngle + 0xFFFF)) && (angle >= (minAngle + 0xFFFF));

    return lowerWrap || direct || upperWrap;
}
