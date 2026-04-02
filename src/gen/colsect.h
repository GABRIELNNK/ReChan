// colsect.h - CollisionSector struct for per-block collision data
// Original: C:\CHAN\GAME\SRC\GEN\COLSECT.CPP
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

    // PSX: __15CollisionSector (COLSECT.CPP:415) 0x80042278
    CollisionSector();

    // PSX: Zero__15CollisionSector (COLSECT.CPP:423) 0x80040C94
    void Zero();

    // PSX: Load__15CollisionSectorPUl (COLSECT.CPP:1601) 0x800422C0
    void Load(u32* data);

    // PSX: Unload__15CollisionSector (COLSECT.CPP:445) 0x800422A0
    void Unload();
};
