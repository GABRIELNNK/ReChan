// colwall.cpp - Wall collision functions
// Original: C:\CHAN\GAME\SRC\GEN\COLWALL.CPP
#include "gen/colwall.h"
#include "p3d/p3dmath.h"

// Collision globals (PSX: gp-relative)
s32 g_collisionMargin = 0;    // gp+2524
s32 g_collisionSmallTol = 0;  // gp+2528
s32 g_collisionLargeTol = 0;  // gp+2532

// PSX: CheckWallCollision__C4WallRC10tagLVectorT1llliRlR10tagLVectorT5 (COLWALL.CPP:194) 0x80091EAC
// Detect wall crossing between oldPos and newPos, compute intersection
s32 Wall::CheckWallCollision(const LVector& oldPos, const LVector& newPos,
    s32 radius, s32 height, s32 arg5, int checkHeight,
    s32& outFrac, LVector& outNormal, LVector& outHitPoint) const {
    MARKFUNCTION(0x80091EAC);

    // Signed distance of old position from wall plane
    s32 distOld = fixmul16(normalX, oldPos.x) + fixmul16(normalZ, oldPos.z) + distance;
    if (distOld < radius - g_collisionMargin) return 0;

    // Signed distance of new position from wall plane
    s32 distNew = fixmul16(normalX, newPos.x) + fixmul16(normalZ, newPos.z) + distance;
    if (distNew >= radius) return 0;

    // Interpolation fraction where distance == radius
    outFrac = rmDiv16i(distOld - radius, distOld - distNew);

    // Wall normal (XZ only, Y = 0)
    outNormal.x = normalX;
    outNormal.y = 0;
    outNormal.z = normalZ;

    // Intersection point: lerp(old, new, frac) pushed back by normal*radius
    outHitPoint.x = oldPos.x + fixmul16(outFrac, newPos.x - oldPos.x) - fixmul16(normalX, radius);
    outHitPoint.y = oldPos.y + fixmul16(outFrac, newPos.y - oldPos.y);
    outHitPoint.z = oldPos.z + fixmul16(outFrac, newPos.z - oldPos.z) - fixmul16(normalZ, radius);

    return CheckWallBounds(outHitPoint, radius, height, arg5, checkHeight);
}

// PSX: CheckWallBounds__C4WallRC10tagLVectorllli (COLWALL.CPP:312) 0x80092250
// Test if intersection point is within wall extents and height range
s32 Wall::CheckWallBounds(const LVector& pos, s32 radius, s32 height, s32 arg4, int checkHeight) const {
    MARKFUNCTION(0x80092250);

    s32 coord, wallMin, wallMax, normalComp;
    if ((flags & 0xFFFF0000) == 0xFFFF0000) {
        // X-aligned wall
        coord = pos.x;
        wallMin = xBound1;
        wallMax = xBound2;
        normalComp = normalZ;
    } else {
        // Z-aligned wall
        coord = pos.z;
        wallMin = zBound1;
        wallMax = zBound2;
        normalComp = normalX;
    }

    // Extend bounds by projected radius
    s32 nc = normalComp;
    if (nc < 0) nc = -nc;
    s32 ext = fixmul16(nc, radius);

    // Range check along wall axis
    if (coord + ext < wallMin) return 0;
    if (wallMax < coord - ext) return 0;

    // Height check using slope/intercept lines
    s32 topY = fixmul16(topSlope, coord) + topIntercept;
    s32 botY = fixmul16(bottomSlope, coord) + bottomIntercept;

    if (checkHeight != 0) {
        s32 diff = botY - topY;
        if (diff <= g_collisionSmallTol) return 0;
        if (pos.y + height < topY) return 0;
    }

    if (pos.y + arg4 < topY) return 0;
    return ((botY - g_collisionLargeTol) < (pos.y + height)) ? 0 : 1;
}
