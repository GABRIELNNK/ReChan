// colsect.h - CollisionSector struct for per-block collision data
// Original: C:\CHAN\GAME\SRC\GEN\COLSECT.CPP
#pragma once

#include "common.h"
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

    // PSX: __15CollisionSector (COLSECT.CPP:415) 0x80042278
    CollisionSector();

    // PSX: Zero__15CollisionSector (COLSECT.CPP:423) 0x80040C94
    void Zero();

    // PSX: Load__15CollisionSectorPUl (COLSECT.CPP:1601) 0x800422C0
    void Load(u32* data);

    // PSX: Unload__15CollisionSector (COLSECT.CPP:445) 0x800422A0
    void Unload();

    // PSX: GetWorldFloorHeight__15CollisionSectorRC10tagLVectorl (COLSECT.CPP:1105) 0x800417B8
    s32 GetWorldFloorHeight(const LVector& pos, s32 radius);

    // PSX: (COLSECT.CPP:1142) 0x800417F0
    static void GetWorldFloorAndCeilingHeight(
        s32& outFloorH, s32& outCeilingH,
        LVector& outFloorNormal, LVector& outCeilingNormal,
        const LVector& pos, s32 radius);

    // PSX: (COLSECT.CPP:1207) 0x80041980
    static s32 FillWorldFloorArray(
        const LVector& searchMin, const LVector& searchMax,
        Floor** outArray, s32 maxCount);

    // PSX: (COLSECT.CPP:1271) 0x80041ADC
    static void GetArrayFloorAndCeilingHeight(
        Floor** floorArray, s32 count,
        s32& outFloorH, s32& outCeilingH,
        LVector& outFloorNormal, LVector& outCeilingNormal,
        s32* outHasRailing, const LVector* pos, s32 radius);

    // PSX: FillWorldWallArray__15CollisionSectorRC10tagLVectorT1PPC4Walli (COLSECT.CPP:863) 0x800411F8
    static s32 FillWorldWallArray(
        const LVector& searchMin, const LVector& searchMax,
        Wall** outArray, s32 maxCount);

    // PSX: CheckArrayWallCollision__15CollisionSectorPPC4WalliRC10tagLVectorT3llli (COLSECT.CPP:931) 0x80041384
    static s32 CheckArrayWallCollision(
        Wall** walls, s32 count,
        const LVector& oldPos, const LVector& newPos,
        s32 radius, s32 height, s32 arg5, int checkHeight);

    // PSX: CheckArrayWallIntersection__15CollisionSectorPPC4WalliR10tagLVectorRC10tagLVectorllli (COLSECT.CPP:1011) 0x800415C0
    static s32 CheckArrayWallIntersection(
        Wall** walls, s32 count,
        LVector& hitPos, const LVector& moveDir,
        s32 radius, s32 height, s32 arg5, int checkHeight);
};

// Wall collision result info (PSX: CS_CheckArrayWallCollision_Info at 0x800E00C8, 44 bytes)
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

// Global array of 12 collision sectors (PSX: gp+1156)
extern CollisionSector g_collisionSectors[12];
