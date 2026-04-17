#pragma once
#include "ai/obstacle.h"

extern const tagCollisionBox INVALID_COLLISION_BOX;

bool ObstacleFillCollisionBox(tagCollisionBox& box, const DBRoot* root, u32 attribNum);
void ApplyDoorStandingZExtent(tagCollisionBox& box);

bool CorrectThingPositionObstacle(
    const LVector& basisA,
    const LVector& basisB,
    s32 rotA,
    s32 rotB,
    const tagCollisionBox& box,
    const LVector& pointA,
    const LVector& pointB,
    s32 radius,
    s32 yMinOffset,
    s32 yMaxOffset,
    LVector& outPos,
    LVector& outNormal,
    LVector& outPushedPos);

void SetCorrectThingPositionDebug(bool enabled);

inline s32 MulShift16(s32 a, s32 b);
inline s32 Div2TowardZero(s32 value);