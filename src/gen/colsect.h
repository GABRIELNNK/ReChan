#pragma once
#include "core.h"
#include "p3d/lvector.h"

struct Wall;
struct Floor;

// CollisionSector - collision data for one level block
// PSX: 44 bytes. Contains walls and floors loaded from BLK data.
// Pointers reference data within the BLK file buffer (not separately allocated).
struct CollisionSector {
    s32 status;           // +0: -1 = empty/unloaded
    LVector boundsMin;    // +4,+8,+12: AABB minimum
    LVector boundsMax;    // +16,+20,+24: AABB maximum
    u32 wallCount;        // +28: number of walls
    Wall* walls;          // +32: wall array (points into BLK data)
    u32 floorCount;       // +36: number of floors
    Floor* floors;        // +40: floor array (points into BLK data)

    CollisionSector();
    void Zero();
    void Load(u32* data);
    void Unload();
    s32 GetWorldFloorHeight(const LVector& pos, s32 radius);

    static void GetWorldFloorAndCeilingHeight(
        s32& outFloorH, s32& outCeilingH,
        LVector& outFloorNormal, LVector& outCeilingNormal,
        const LVector& pos, s32 radius);
    static s32 GetBlockNumber(const LVector& pos);
    static s32 FillWorldFloorArray(
        const LVector& searchMin, const LVector& searchMax,
        Floor** outArray, s32 maxCount);
    static void GetArrayFloorAndCeilingHeight(
        Floor** floorArray, s32 count,
        s32& outFloorH, s32& outCeilingH,
        LVector& outFloorNormal, LVector& outCeilingNormal,
        s32* outHasRailing, LVector* outRailCorrection,
        const LVector* pos, s32 radius);
    static bool LedgePrototype(
        const LVector& startPos, const LVector& endPos,
        s32 minHeight, s32 maxHeight,
        LVector& outNormal, LVector& outCorrectionPos,
        u16& outMaterial, s32 clearance);
    static s32 FillWorldWallArray(
        const LVector& searchMin, const LVector& searchMax,
        Wall** outArray, s32 maxCount);
    static s32 CheckWorldWallCollision(
        const LVector& oldPos, const LVector& newPos,
        s32 radius, s32 height, s32 arg5,
        s32& outRatio, LVector& outNormal, LVector& outHitPoint, s32& outWallHorizontal);
    static s32 CheckArrayWallCollision(
        Wall** walls, s32 count,
        const LVector& oldPos, const LVector& newPos,
        s32 radius, s32 height, s32 arg5, int checkHeight);
    static s32 CheckArrayWallIntersection(
        Wall** walls, s32 count,
        LVector& hitPos, const LVector& moveDir,
        s32 radius, s32 height, s32 arg5, int checkHeight);
};

// Wall collision result info
struct WallCollisionInfo {
    s32 collisionRatio;         // +0: best (smallest) fraction (16.16)
    LVector wallNormal;         // +4: wall normal at hit point
    LVector hitPoint;           // +16: world-space hit point
    s32 wallHorizontal;         // +28: CheckWallBounds result
    s32 wallVerticalMin;        // +32: top edge height at hit point
    s32 wallVerticalMax;        // +36: bottom edge height at hit point
    s32 wallMaterial;           // +40: wall flags lower 16 bits
};

extern WallCollisionInfo g_wallCollisionInfo;  // PSX: 0x800E00C8
extern Wall* g_colHitWall;                     // PSX: gp+1160

static constexpr s32 MAX_COLLISION_SECTORS = 128;
extern CollisionSector g_collisionSectors[MAX_COLLISION_SECTORS];
