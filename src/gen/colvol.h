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

struct DBVolume;
struct OriginalGeo;

void FillCollisionBox(tagCollisionBox& box, const DBVolume& vol);
bool FillCollisionBox(tagCollisionBox& box, const OriginalGeo& geo);
bool FillCollisionBox(tagCollisionBox& box, const LVector& origin, const LVector* points, u32 pointCount);
void SetCollisionBoxExtent(tagCollisionBox& box);

// PSX: CheckStaticHorizontalBoxPointCollision (COLVOL.CPP:283, 0x800AA0D4)
// Tests if point posB (projected onto XZ) lies inside the oriented box centered at posA.
// Rotates the relative offset by -rotY before checking against box XZ bounds.
bool CheckStaticHorizontalBoxPointCollision(
    const LVector& posA, const tagCollisionBox& box, s32 rotY, const LVector& posB);

// PSX: CheckStaticBoxSphereCollision (COLVOL.CPP:461, 0x800AA22C)
// Tests if a sphere centered at spherePos intersects an oriented box centered at boxPos.
bool CheckStaticBoxSphereCollision(
    const LVector& boxPos,
    const tagCollisionBox& box,
    s32 rotY,
    const LVector& spherePos,
    const tagCollisionSphere& sphere);

bool CheckStaticCylinderSphereCollision(
    const LVector& cylPos,
    const tagCollisionCylinder& cyl,
    const LVector& spherePos,
    const tagCollisionSphere& sphere);
