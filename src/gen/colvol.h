// colvol.h - Collision volume types for entity collision shapes
// Original: C:\CHAN\GAME\SRC\GEN\COLVOL.CPP
#pragma once

#include "core.h"

// tagCollisionBox - axis-aligned bounding box (16 bytes)
// PSX: stored at Thing+offset, used for broad-phase box tests
struct tagCollisionBox {
    s16 minX;     // +0
    s16 minY;     // +2
    s16 minZ;     // +4
    s16 maxX;     // +6
    s16 maxY;     // +8
    s16 maxZ;     // +10
    s16 extent;   // +12: max absolute extent (for broad-phase)
    s16 pad;      // +14
};

// tagCollisionCylinder - upright cylinder (12 bytes)
// PSX: stored at Thing+0x118, used for humanoid-vs-obstacle tests
struct tagCollisionCylinder {
    s32 radius;   // +0: horizontal radius
    s32 lowerY;   // +4: bottom Y offset
    s32 upperY;   // +8: top Y offset
};

// tagCollisionSphere - sphere (4 bytes)
// PSX: radius only, position comes from Thing::pos
struct tagCollisionSphere {
    s32 radius;   // +0
};
