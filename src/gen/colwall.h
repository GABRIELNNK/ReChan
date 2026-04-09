#pragma once
#include "core.h"
#include "p3d/lvector.h"

// Wall - collision wall segment (vertical or horizontal-aligned)
// PSX: 56 bytes. Plane equation: normalX*x + normalZ*z + distance = 0
// Height described by two linear functions (top/bottom edge).
// Bounded by two endpoints along the wall's primary axis.
struct Wall {
    s32 normalX;           // +0: wall plane normal X (16.16 fixed)
    s32 normalZ;           // +4: wall plane normal Z (16.16 fixed)
    s32 distance;          // +8: wall plane distance
    u32 flags;             // +12: upper 16 bits = orientation (0xFFFF = X-aligned)
    s32 field10;           // +16: unknown
    s32 field14;           // +20: unknown
    s32 topSlope;          // +24: top edge height slope
    s32 topIntercept;      // +28: top edge height intercept
    s32 bottomSlope;       // +32: bottom edge height slope
    s32 bottomIntercept;   // +36: bottom edge height intercept
    s32 xBound1;           // +40: X endpoint 1 (for X-aligned walls)
    s32 zBound1;           // +44: Z endpoint 1 (for Z-aligned walls)
    s32 xBound2;           // +48: X endpoint 2 (for X-aligned walls)
    s32 zBound2;           // +52: Z endpoint 2 (for Z-aligned walls)

    s32 CheckWallCollision(const LVector& oldPos, const LVector& newPos,
                           s32 radius, s32 height, s32 arg5, int checkHeight,
                           s32& outFrac, LVector& outNormal, LVector& outHitPoint) const;
    s32 CheckWallBounds(const LVector& pos, s32 radius, s32 height, s32 arg4, int checkHeight) const;
    s32 CheckWallIntersection(LVector& hitPos, const LVector& moveDir,
                              s32 radius, s32 height, s32 arg5, int checkHeight) const;
    void Get(LVector& v0, LVector& v1, LVector& v2, LVector& v3) const;
    bool IsCurb() const;
};

// Collision globals (PSX: gp-relative, set during init)
extern s32 g_collisionMargin;    // gp+2524: wall collision margin
extern s32 g_collisionSmallTol;  // gp+2528: height difference tolerance
extern s32 g_collisionLargeTol;  // gp+2532: ceiling clearance tolerance
