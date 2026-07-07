#include "gen/colsect.h"
#include "pc/log.h"
#include "gen/blockmgr.h"
#include "gen/colwall.h"
#include "gen/colfloor.h"
#include "p3d/p3dmath.h"

// PSX: 12 collision sectors (demand-loaded). PC: 128 to cover all blocks at once.
CollisionSector g_collisionSectors[MAX_COLLISION_SECTORS];

// Scratch buffer for floor pointer collection (PSX: 0x800DFD58, capacity 64)
static Floor* g_floorPtrScratch[64];

// Scratch buffer for wall pointer collection used by ledge prototype checks
static Wall* g_wallPtrScratch[64];

// PSX: temporary world-wall query pointer array (0x800DFCB8).
static Wall* g_worldWallQueryScratch[64];

// PSX: __15CollisionSector (COLSECT.CPP:415) 0x80042278
CollisionSector::CollisionSector() {
    MARKFUNCTION(0x80042278);
    Zero();
}

// PSX: Zero__15CollisionSector (COLSECT.CPP:423) 0x80040C94
void CollisionSector::Zero() {
    MARKFUNCTION(0x80040C94);
    status = -1;
    boundsMin.x = 32767;
    boundsMin.y = 32767;
    boundsMin.z = 32767;
    boundsMax.x = -32767;
    boundsMax.y = -32767;
    boundsMax.z = -32767;
    wallCount = 0;
    walls = nullptr;
    floorCount = 0;
    floors = nullptr;
}

// PSX: Load__15CollisionSectorPUl (COLSECT.CPP:1601) 0x800422C0
// Data layout: [12]=minX, [16]=minY, [20]=minZ, [24]=maxX, [28]=maxY, [32]=maxZ,
//              [36]=numWalls, [40]=numFloors, [48]=wallData[], then floorData[]
void CollisionSector::Load(u32* data) {
    MARKFUNCTION(0x800422C0);
    boundsMin.x = (s32)data[3];   // data+12
    boundsMin.y = (s32)data[4];   // data+16
    boundsMin.z = (s32)data[5];   // data+20
    boundsMax.x = (s32)data[6];   // data+24
    boundsMax.y = (s32)data[7];   // data+28
    boundsMax.z = (s32)data[8];   // data+32
    wallCount = data[9];           // data+36
    floorCount = data[10];         // data+40
    walls = (Wall*)&data[12];      // data+48 (inline wall data)
    // Floor data follows walls: each wall is 56 bytes = 14 u32s
    floors = (Floor*)((u8*)walls + wallCount * 56);
}

// PSX: Unload__15CollisionSector (COLSECT.CPP:445) 0x800422A0
void CollisionSector::Unload() {
    MARKFUNCTION(0x800422A0);
    Zero();
}

// PSX: GetBlockNumber__15CollisionSectorRC10tagLVector (COLSECT.CPP:511) 0x80040D8C
s32 CollisionSector::GetBlockNumber(const LVector& pos) {
    MARKFUNCTION(0x80040D8C);

    s32 blockNum = -1;

    for (s32 sectorIdx = 0; sectorIdx < MAX_COLLISION_SECTORS; sectorIdx++) {
        const CollisionSector* sector = &g_collisionSectors[sectorIdx];
        if (sector->status == -1) {
            continue;
        }

        const u32 sectorBlockNum = static_cast<u32>(sector->status);
        if (g_blockManager && !g_blockManager->IsValidBlockNumber(sectorBlockNum)) {
            continue;
        }

        bool inside = false;
        if (pos.x >= sector->boundsMin.x && sector->boundsMax.x >= pos.x) {
            if (pos.y >= sector->boundsMin.y && sector->boundsMax.y >= pos.y) {
                if (pos.z >= sector->boundsMin.z && sector->boundsMax.z >= pos.z) {
                    inside = true;
                }
            }
        }

        if (inside) {
            blockNum = sector->status;
        }
    }

    return blockNum;
}

// PSX: GetWorldFloorHeight__15CollisionSectorRC10tagLVectorl (COLSECT.CPP:1105) 0x800417B8
// Simple wrapper that discards ceiling/normal outputs
s32 CollisionSector::GetWorldFloorHeight(const LVector& pos, s32 radius) {
    MARKFUNCTION(0x800417B8);

    s32 floorHeight;
    s32 ceilingHeight;
    LVector floorNormal;
    LVector ceilingNormal;

    GetWorldFloorAndCeilingHeight(floorHeight, ceilingHeight,
                                  floorNormal, ceilingNormal, pos, radius);

    return floorHeight;
}

// PSX: (COLSECT.CPP:1142) 0x800417F0
// Query floor/ceiling height at a world position using all loaded collision sectors
void CollisionSector::GetWorldFloorAndCeilingHeight(
    s32& outFloorH, s32& outCeilingH,
    LVector& outFloorNormal, LVector& outCeilingNormal,
    const LVector& pos, s32 radius) {
    MARKFUNCTION(0x800417F0);

    // Build search box: pos ± radius
    LVector searchMin;
    memset(&searchMin, 0, sizeof(LVector));
    searchMin.x = pos.x - radius;
    searchMin.y = pos.y;
    searchMin.z = pos.z - radius;

    LVector searchMax;
    memset(&searchMax, 0, sizeof(LVector));
    searchMax.x = pos.x + radius;
    searchMax.y = pos.y;
    searchMax.z = pos.z + radius;

    // Collect overlapping floors into scratch array
    s32 count = FillWorldFloorArray(searchMin, searchMax, g_floorPtrScratch, 64);
    if (count > 64) count = 64;

    s32 localHasRailing;
    LVector localRailCorrection;
    GetArrayFloorAndCeilingHeight(g_floorPtrScratch, count,
                                  outFloorH, outCeilingH, outFloorNormal, outCeilingNormal,
                                  &localHasRailing, &localRailCorrection, &pos, radius);
}

// PSX: (COLSECT.CPP:1207) 0x80041980
// Collect Floor pointers overlapping with the search AABB from all sectors
s32 CollisionSector::FillWorldFloorArray(
    const LVector& searchMin, const LVector& searchMax,
    Floor** outArray, s32 maxCount) {
    MARKFUNCTION(0x80041980);

    s32 totalAdded = 0;

    for (s32 sectorIdx = 0; sectorIdx < MAX_COLLISION_SECTORS; sectorIdx++) {
        CollisionSector* sector = &g_collisionSectors[sectorIdx];
        if (sector->status == -1) continue;

        // AABB overlap test between search box and sector bounds
        if (searchMax.x < sector->boundsMin.x) continue;
        if (sector->boundsMax.x < searchMin.x) continue;
        if (searchMax.z < sector->boundsMin.z) continue;
        if (sector->boundsMax.z < searchMin.z) continue;

        // Iterate floors in this sector
        // PSX: floor entries are 80 bytes (64B Floor + 16B AABB) in the data stream
        // but our Floor struct is 64B; the AABB bounds are at offsets +64..+76 past each entry
        u8* floorBase = (u8*)sector->floors;
        for (u32 i = 0; i < sector->floorCount; i++) {
            Floor* floor = (Floor*)(floorBase + i * 80);
            // Per-floor AABB at offsets +64,+68,+72,+76 of the 80-byte entry
            s32* floorBounds = (s32*)(floorBase + i * 80 + 64);
            s32 fXMin = floorBounds[0]; // +64
            s32 fZMin = floorBounds[1]; // +68
            s32 fXMax = floorBounds[2]; // +72
            s32 fZMax = floorBounds[3]; // +76

            if (searchMax.x < fXMin) continue;
            if (fXMax < searchMin.x) continue;
            if (searchMax.z < fZMin) continue;
            if (fZMax < searchMin.z) continue;

            if (totalAdded < maxCount) {
                outArray[totalAdded] = floor;
            }
            totalAdded++;
        }
    }

    return totalAdded;
}

// PSX: (COLSECT.CPP:1271) 0x80041ADC
// Scan collected floors for best floor height, ceiling height, and railing corrections
void CollisionSector::GetArrayFloorAndCeilingHeight(
    Floor** floorArray, s32 count,
    s32& outFloorH, s32& outCeilingH,
    LVector& outFloorNormal, LVector& outCeilingNormal,
    s32* outHasRailing, LVector* outRailCorrection,
    const LVector* pos, s32 radius) {
    MARKFUNCTION(0x80041ADC);

    // PSX: v12 = lowest walkable ceiling above test pos
    //      v13 = lowest secondary ceiling above test pos
    s32 v12 = 0x7FFFFFFF;
    s32 v13 = 0x7FFFFFFF;

    // Pass 1: find lowest surfaces ABOVE the test position
    // PSX condition: a9[1] < h  (pos->y < floorHeight)
    for (s32 i = 0; i < count; i++) {
        Floor* floor = floorArray[i];
        s32 h = floor->GetFloorHeight(*pos);

        if (pos->y < h && floor->CheckFloorBounds(*pos, radius)) {
            if ((floor->flags & 1) && h < v12) {
                v12 = h;
            }
            else if (((floor->flags >> 1) & 1) && h < v13) {
                v13 = h;
            }
        }
    }

    if (v13 < v12) {
        v12 = v13;
    }

    // Pass 2: find highest walkable floor BELOW the ceiling (v12)
    s32 v17 = (s32)0x80000001;
    for (s32 i = 0; i < count; i++) {
        Floor* floor = floorArray[i];

        bool walkable = (floor->flags & 1) != 0;
        bool noRailing = ((floor->flags >> 2) & 1) == 0;
        if (!(walkable && noRailing)) continue;

        if (!floor->CheckFloorBounds(*pos, radius)) continue;

        s32 h = floor->GetFloorHeight(*pos);
        if (h < v12 && v17 < h) {
            v17 = h;
            floor->GetFloorNormal(outFloorNormal);
            outCeilingNormal.x = (s32)(u16)floor->field0C;
        }
    }

    // Pass 3: check railing floors for correction
    *outHasRailing = 0;
    for (s32 i = 0; i < count; i++) {
        Floor* floor = floorArray[i];

        bool walkable = (floor->flags & 1) != 0;
        bool hasRailing = ((floor->flags >> 2) & 1) != 0;
        if (!(walkable && hasRailing)) continue;

        if (!floor->CheckFloorBounds(*pos, radius)) continue;

        s32 h = floor->GetFloorHeight(*pos);
        if (h < v12 && v17 < h) {
            *outHasRailing = 1;
            floor->GetRailingCorrection(*outRailCorrection, *pos);
        }
    }

    // PSX output: a3 = v17 (floor to stand on), a4 = v13 (secondary ceiling)
    outFloorH = (v17 <= (s32)0x80000001) ? (s32)0x80000001 : v17;
    outCeilingH = (v13 == 0x7FFFFFFF) ? 0x7FFFFFFF : v13;
}

// PSX: LedgePrototype__15CollisionSectorRC10tagLVectorT1llR9_RMVECT16R10tagLVectorRll (COLSECT.CPP:1403)
bool CollisionSector::LedgePrototype(
    const LVector& startPos, const LVector& endPos,
    s32 minHeight, s32 maxHeight,
    LVector& outNormal, LVector& outCorrectionPos,
    u16& outMaterial, s32 clearance) {
    MARKFUNCTION(0x80041DC4);

    LVector searchMin = {};
    LVector searchMax = {};
    searchMin.x = (startPos.x < endPos.x) ? startPos.x : endPos.x;
    searchMin.y = (startPos.y < endPos.y) ? startPos.y : endPos.y;
    searchMin.z = (startPos.z < endPos.z) ? startPos.z : endPos.z;
    searchMax.x = (startPos.x > endPos.x) ? startPos.x : endPos.x;
    searchMax.y = (startPos.y > endPos.y) ? startPos.y : endPos.y;
    searchMax.z = (startPos.z > endPos.z) ? startPos.z : endPos.z;

    s32 wallCount = FillWorldWallArray(searchMin, searchMax, g_wallPtrScratch, 64);
    if (wallCount > 64) {
        wallCount = 64;
    }

    s32 floorCount = FillWorldFloorArray(searchMin, searchMax, g_floorPtrScratch, 64);
    if (floorCount > 64) {
        floorCount = 64;
    }

    for (s32 floorIdx = 0; floorIdx < floorCount; floorIdx++) {
        Floor* floor = g_floorPtrScratch[floorIdx];
        if (!floor->LedgePrototype(startPos, endPos, minHeight, maxHeight, outNormal, outCorrectionPos)) {
            continue;
        }

        outMaterial = (u16)floor->field0C;

        LVector floorNormal = {};
        floor->GetFloorNormal(floorNormal);
        if (floorNormal.y <= 46347) {
            s32 dot =
                fixmul16(outNormal.x, floorNormal.x) +
                fixmul16(outNormal.z, floorNormal.z);
            if (dot >= 0) {
                continue;
            }
        }

        LVector probePos = {};
        probePos.x = outCorrectionPos.x + (s16)((s64)outNormal.x >> 9);
        probePos.y = outCorrectionPos.y;
        probePos.z = outCorrectionPos.z + (s16)((s64)outNormal.z >> 9);

        s32 floorH = 0;
        s32 ceilingH = 0;
        LVector floorNorm = {};
        LVector ceilingNorm = {};
        s32 hasRailing = 0;
        LVector railCorrection = {};
        GetArrayFloorAndCeilingHeight(
            g_floorPtrScratch,
            floorCount,
            floorH,
            ceilingH,
            floorNorm,
            ceilingNorm,
            &hasRailing,
            &railCorrection,
            &probePos,
            0);

        if (outCorrectionPos.y < floorH + clearance) {
            continue;
        }

        s32 normalStepX = (s16)((s64)outNormal.x >> 10);
        s32 normalStepZ = (s16)((s64)outNormal.z >> 10);

        LVector wallStart = {};
        wallStart.x = outCorrectionPos.x + normalStepX;
        wallStart.y = outCorrectionPos.y + 64;
        wallStart.z = outCorrectionPos.z + normalStepZ;

        LVector wallEnd = {};
        wallEnd.x = outCorrectionPos.x - normalStepX;
        wallEnd.y = outCorrectionPos.y + 64;
        wallEnd.z = outCorrectionPos.z - normalStepZ;

        bool blocked = false;
        for (s32 wallIdx = 0; wallIdx < wallCount; wallIdx++) {
            s32 outFrac = 0;
            LVector outWallNormal = {};
            LVector outHitPoint = {};
            if (g_wallPtrScratch[wallIdx]->CheckWallCollision(
                wallStart,
                wallEnd,
                0,
                0,
                0,
                0,
                outFrac,
                outWallNormal,
                outHitPoint)) {
                blocked = true;
                break;
            }
        }

        if (blocked) {
            continue;
        }

        return true;
    }

    return false;
}

// Wall collision result globals
WallCollisionInfo g_wallCollisionInfo;  // PSX: 0x800E00C8
Wall* g_colHitWall = nullptr;           // PSX: gp+1160

// PSX: CheckWorldWallCollision__15CollisionSectorRC10tagLVectorT1lllRlR9_RMVECT16R10tagLVectorRl
// (COLSECT.CPP:657) 0x80041038
s32 CollisionSector::CheckWorldWallCollision(
    const LVector& oldPos, const LVector& newPos,
    s32 radius, s32 height, s32 arg5,
    s32& outRatio, LVector& outNormal, LVector& outHitPoint, s32& outWallHorizontal) {
    MARKFUNCTION(0x80041038);

    LVector searchMin = {};
    LVector searchMax = {};
    searchMin.x = (oldPos.x < newPos.x) ? oldPos.x : newPos.x;
    searchMin.y = (oldPos.y < newPos.y) ? oldPos.y : newPos.y;
    searchMin.z = (oldPos.z < newPos.z) ? oldPos.z : newPos.z;
    searchMax.x = (oldPos.x > newPos.x) ? oldPos.x : newPos.x;
    searchMax.y = (oldPos.y > newPos.y) ? oldPos.y : newPos.y;
    searchMax.z = (oldPos.z > newPos.z) ? oldPos.z : newPos.z;

    searchMin.x -= radius;
    searchMin.z -= radius;
    searchMax.x += radius;
    searchMax.z += radius;
    searchMin.y += height;
    searchMax.y += arg5;

    s32 wallCount = FillWorldWallArray(searchMin, searchMax, g_worldWallQueryScratch, 64);
    if (wallCount > 64) {
        wallCount = 64;
    }

    const s32 hit = CheckArrayWallCollision(
        g_worldWallQueryScratch,
        wallCount,
        oldPos,
        newPos,
        radius,
        height,
        arg5,
        0);

    outRatio = g_wallCollisionInfo.collisionRatio;
    outNormal = g_wallCollisionInfo.wallNormal;
    outHitPoint = g_wallCollisionInfo.hitPoint;
    outWallHorizontal = g_wallCollisionInfo.wallHorizontal;
    return hit;
}

// PSX: FillWorldWallArray__15CollisionSectorRC10tagLVectorT1PPC4Walli (COLSECT.CPP:863) 0x800411F8
// Collect Wall pointers overlapping with the search AABB from all sectors
s32 CollisionSector::FillWorldWallArray(
    const LVector& searchMin, const LVector& searchMax,
    Wall** outArray, s32 maxCount) {
    MARKFUNCTION(0x800411F8);

    s32 totalAdded = 0;

    for (s32 sectorIdx = 0; sectorIdx < MAX_COLLISION_SECTORS; sectorIdx++) {
        CollisionSector* sector = &g_collisionSectors[sectorIdx];
        if (sector->status == -1) continue;

        // 6-axis AABB overlap test (sector bounds vs search box)
        if (searchMax.x < sector->boundsMin.x) continue;
        if (sector->boundsMax.x < searchMin.x) continue;
        if (searchMax.y < sector->boundsMin.y) continue;
        if (sector->boundsMax.y < searchMin.y) continue;
        if (searchMax.z < sector->boundsMin.z) continue;
        if (sector->boundsMax.z < searchMin.z) continue;

        // Iterate walls in this sector (stride = 56 bytes = sizeof(Wall))
        for (u32 i = 0; i < sector->wallCount; i++) {
            Wall* wall = &sector->walls[i];

            // Per-wall AABB check using bound fields
            if (!(searchMin.x < wall->xBound2)) continue;
            if (!(searchMin.z < wall->zBound2)) continue;
            if (!(wall->xBound1 < searchMax.x)) continue;
            if (!(wall->zBound1 < searchMax.z)) continue;

            if (totalAdded < maxCount) {
                outArray[totalAdded] = wall;
            }
            totalAdded++;
        }
    }

    return totalAdded;
}

// PSX: CheckArrayWallCollision__15CollisionSectorPPC4WalliRC10tagLVectorT3llli (COLSECT.CPP:931) 0x80041384
// Find best (smallest fraction) wall collision from the wall array
s32 CollisionSector::CheckArrayWallCollision(
    Wall** walls, s32 count,
    const LVector& oldPos, const LVector& newPos,
    s32 radius, s32 height, s32 arg5, int checkHeight) {
    MARKFUNCTION(0x80041384);

    s32 found = 0;
    g_colHitWall = nullptr;
    g_wallCollisionInfo.collisionRatio = 0x00020000; // 2.0 in 16.16 (sentinel)

    for (s32 i = 0; i < count; i++) {
        Wall* wall = walls[i];

        // Skip curb walls when checking height
        if (checkHeight != 0) {
            if (wall->IsCurb()) continue;
        }

        s32 outFrac;
        LVector outNormal;
        LVector outHitPoint;

        s32 hit = wall->CheckWallCollision(
            oldPos, newPos,
            radius, height, arg5, checkHeight,
            outFrac, outNormal, outHitPoint);

        if (!hit) continue;
        if (!(outFrac < g_wallCollisionInfo.collisionRatio)) continue;

        // New best collision
        g_colHitWall = wall;
        g_wallCollisionInfo.collisionRatio = outFrac;
        g_wallCollisionInfo.wallNormal = outNormal;
        g_wallCollisionInfo.hitPoint = outHitPoint;

        // Determine coordinate for slope evaluation
        s32 coord;
        if ((wall->flags & 0xFFFF0000) == 0xFFFF0000) {
            coord = outHitPoint.x;
        }
        else {
            coord = outHitPoint.z;
        }

        g_wallCollisionInfo.wallHorizontal = wall->CheckWallBounds(
            outHitPoint, radius, height, arg5, checkHeight);
        g_wallCollisionInfo.wallVerticalMin =
            fixmul16(wall->topSlope, coord) + wall->topIntercept;
        g_wallCollisionInfo.wallVerticalMax =
            fixmul16(wall->bottomSlope, coord) + wall->bottomIntercept;
        g_wallCollisionInfo.wallMaterial = (wall->flags & 0xFFFF);

        found = 1;
    }

    return found;
}

// Helper: fixed-point dot product of two LVectors
static s32 LVectorDot(const LVector* a, const LVector* b) {
    return fixmul16(a->x, b->x) + fixmul16(a->y, b->y) + fixmul16(a->z, b->z);
}

// PSX: CheckArrayWallIntersection__15CollisionSectorPPC4WalliR10tagLVectorRC10tagLVectorllli (COLSECT.CPP:1011) 0x800415C0
// Iteratively push position out of all intersecting walls
s32 CollisionSector::CheckArrayWallIntersection(
    Wall** walls, s32 count,
    LVector& hitPos, const LVector& moveDir,
    s32 radius, s32 height, s32 arg5, int checkHeight) {
    MARKFUNCTION(0x800415C0);

    s32 foundAny = 0;
    s32 iterCount = 0;
    LVector currentPos = moveDir;

    do {
        s32 hitCount = 0;
        s32 anyNegDot = 0;
        LVector hitInfoArray[4]; // {normalX, 0, normalZ} entries for dot product checks

        for (s32 wallIdx = 0; wallIdx < count; wallIdx++) {
            Wall* wall = walls[wallIdx];

            if (checkHeight != 0) {
                if (wall->IsCurb()) continue;
            }

            LVector hitResult;
            s32 hit = wall->CheckWallIntersection(
                hitResult, currentPos,
                radius, height, arg5, checkHeight);

            if (!hit) continue;

            currentPos = hitResult;
            foundAny = 1;

            if (hitCount >= 4) continue;

            // Record wall normal for dot product checks
            hitInfoArray[hitCount].x = wall->normalX;
            hitInfoArray[hitCount].y = 0;
            hitInfoArray[hitCount].z = wall->normalZ;

            // Check for opposing normals
            for (s32 j = 0; j < hitCount; j++) {
                s32 dot = LVectorDot(&hitInfoArray[hitCount], &hitInfoArray[j]);
                if (dot < 0) {
                    anyNegDot = 1;
                }
            }

            hitCount++;
        }

        if (anyNegDot == 0) break;
        iterCount++;
    } while (iterCount < 16);

    if (foundAny) {
        hitPos = currentPos;
    }

    return foundAny;
}
