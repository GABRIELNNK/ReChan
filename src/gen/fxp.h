#pragma once
#include "core.h"
#include "p3d/lvector.h"

bool IsPointInFieldOf(const LVector& point, const LVector& origin, s32 orientation, s32 left, s32 right);
s32 ClipAngle(s32& angle);
bool IsAngleInFieldOf(s32 angle, s32 facing, s32 left, s32 right);
