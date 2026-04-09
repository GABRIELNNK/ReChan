#include "gen/colwall.h"
#include "p3d/p3dmath.h"

// Collision globals (PSX: gp-relative, values from PSX .data section)
s32 g_collisionMargin = 16;    // gp+2524 (0x10)
s32 g_collisionSmallTol = 130; // gp+2528 (0x82) - thin wall threshold
s32 g_collisionLargeTol = 16;  // gp+2532 (0x10)

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
    }
    else {
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

    // Curb/thin-wall skip flag: only skip if wall is thin AND entity is above it
    s32 skipWall = 0;
    if (checkHeight != 0) {
        if (g_collisionSmallTol >= (botY - topY)) {
            skipWall = (pos.y + height >= topY) ? 1 : 0;
        }
    }

    if (coord + ext >= wallMin && wallMax >= coord - ext && !skipWall) {
        if (pos.y + arg4 >= topY) {
            return (botY - g_collisionLargeTol >= pos.y + height) ? 1 : 0;
        }
    }
    return 0;
}

// PSX: CheckWallIntersection__C4WallR10tagLVectorRC10tagLVectorllli (COLWALL.CPP:132) 0x80091C90
// Test if position is on the positive side of wall plane and within bounds
s32 Wall::CheckWallIntersection(LVector& hitPos, const LVector& moveDir,
                                s32 radius, s32 height, s32 arg5, int checkHeight) const {
    MARKFUNCTION(0x80091C90);

    // Signed distance from wall plane
    s32 dist = fixmul16(normalX, moveDir.x) + fixmul16(normalZ, moveDir.z) + distance;

    if (dist <= 0) return 0;
    if (dist >= radius) return 0;

    // Project position onto wall plane
    LVector hitVec;
    hitVec.x = moveDir.x - fixmul16(normalX, dist);
    hitVec.y = moveDir.y;
    hitVec.z = moveDir.z - fixmul16(normalZ, dist);

    // Check if projected point is within wall bounds
    s32 result = CheckWallBounds(hitVec, radius, height, arg5, checkHeight);
    if (result == 0) return 0;

    // Push position out along normal by (radius - dist + 2)
    s32 pushDist = radius - dist + 2;
    hitPos.x = moveDir.x + fixmul16(normalX, pushDist);
    hitPos.y = moveDir.y;
    hitPos.z = moveDir.z + fixmul16(normalZ, pushDist);

    return result;
}

// PSX: Get__C4WallR10tagLVectorN31 (COLWALL.CPP:348) 0x800923D8
// Reconstruct 4 corner vertices of the wall quad
void Wall::Get(LVector& v0, LVector& v1, LVector& v2, LVector& v3) const {
    MARKFUNCTION(0x800923D8);

    s32 bound1, bound2;

    if ((flags & 0xFFFF0000) == 0xFFFF0000) {
        // X-aligned wall — solve Z from plane: z = -(normalX*x + distance) / normalZ
        if (normalZ >= 0) {
            bound1 = xBound1;
            bound2 = xBound2;
        }
        else {
            bound1 = xBound2;
            bound2 = xBound1;
        }

        s32 z1 = (normalZ != 0) ? -rmDiv16i(fixmul16(normalX, bound1) + distance, normalZ) : 0;
        s32 z2 = (normalZ != 0) ? -rmDiv16i(fixmul16(normalX, bound2) + distance, normalZ) : 0;

        v0.x = bound1;
        v0.y = fixmul16(topSlope, bound1) + topIntercept;
        v0.z = z1;

        v1.x = bound2;
        v1.y = fixmul16(topSlope, bound2) + topIntercept;
        v1.z = z2;

        v2.x = bound2;
        v2.y = fixmul16(bottomSlope, bound2) + bottomIntercept;
        v2.z = z2;

        v3.x = bound1;
        v3.y = fixmul16(bottomSlope, bound1) + bottomIntercept;
        v3.z = z1;
    }
    else {
        // Z-aligned wall — solve X from plane: x = -(normalZ*z + distance) / normalX
        if (normalX >= 0) {
            bound1 = zBound1;
            bound2 = zBound2;
        }
        else {
            bound1 = zBound2;
            bound2 = zBound1;
        }

        s32 x1 = (normalX != 0) ? -rmDiv16i(fixmul16(normalZ, bound1) + distance, normalX) : 0;
        s32 x2 = (normalX != 0) ? -rmDiv16i(fixmul16(normalZ, bound2) + distance, normalX) : 0;

        v0.x = x1;
        v0.y = fixmul16(topSlope, bound1) + topIntercept;
        v0.z = bound1;

        v1.x = x2;
        v1.y = fixmul16(topSlope, bound2) + topIntercept;
        v1.z = bound2;

        v2.x = x2;
        v2.y = fixmul16(bottomSlope, bound2) + bottomIntercept;
        v2.z = bound2;

        v3.x = x1;
        v3.y = fixmul16(bottomSlope, bound1) + bottomIntercept;
        v3.z = bound1;
    }
}

// PSX: IsCurb__C4Wall (COLWALL.CPP:269) 0x80092144
// True if height difference at both endpoints is below g_collisionSmallTol
bool Wall::IsCurb() const {
    MARKFUNCTION(0x80092144);

    s32 bound1, bound2;

    if ((flags & 0xFFFF0000) == 0xFFFF0000) {
        bound1 = xBound1;
        bound2 = xBound2;
    }
    else {
        bound1 = zBound1;
        bound2 = zBound2;
    }

    s32 topH1 = fixmul16(bottomSlope, bound1) + bottomIntercept;
    s32 botH1 = fixmul16(topSlope, bound1) + topIntercept;
    s32 diff1 = topH1 - botH1;

    s32 topH2 = fixmul16(bottomSlope, bound2) + bottomIntercept;
    s32 botH2 = fixmul16(topSlope, bound2) + topIntercept;
    s32 diff2 = topH2 - botH2;

    if (diff1 < g_collisionSmallTol) {
        return (diff2 < g_collisionSmallTol);
    }
    return false;
}
