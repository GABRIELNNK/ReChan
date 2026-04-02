// colsect.cpp - CollisionSector functions
// Original: C:\CHAN\GAME\SRC\GEN\COLSECT.CPP
#include "gen/colsect.h"
#include "gen/colwall.h"
#include "gen/colfloor.h"
#include "p3d/p3dmath.h"
#include <cstring>

// Global array of 12 collision sectors (PSX: gp+1156)
CollisionSector g_collisionSectors[12];

// Scratch buffer for floor pointer collection (PSX: 0x800DFD58, capacity 64)
static Floor* g_floorPtrScratch[64];

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

    s32 localIdx;
    GetArrayFloorAndCeilingHeight(g_floorPtrScratch, count,
        outFloorH, outCeilingH, outFloorNormal, outCeilingNormal,
        &localIdx, &pos, radius);
}

// PSX: (COLSECT.CPP:1207) 0x80041980
// Collect Floor pointers overlapping with the search AABB from all sectors
s32 CollisionSector::FillWorldFloorArray(
    const LVector& searchMin, const LVector& searchMax,
    Floor** outArray, s32 maxCount) {
    MARKFUNCTION(0x80041980);

    s32 totalAdded = 0;

    for (s32 sectorIdx = 0; sectorIdx < 12; sectorIdx++) {
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
    s32* outHasRailing, const LVector* pos, s32 radius) {
    MARKFUNCTION(0x80041ADC);

    s32 bestFloorH = 0x7FFFFFFF;
    s32 bestCeilH = 0x7FFFFFFF;

    // Pass 1: find best floor height below player
    for (s32 i = 0; i < count; i++) {
        Floor* floor = floorArray[i];
        s32 h = floor->GetFloorHeight(*pos);

        if (pos->y < h) continue; // floor is above us
        if (!floor->CheckFloorBounds(*pos, radius)) continue;

        if (floor->flags & 0x0001) {
            // Ceiling-type floor
            if (h < bestFloorH) {
                bestFloorH = h;
                floor->GetFloorNormal(outFloorNormal);
            }
        } else if ((floor->field0C >> 1) & 1) {
            // Secondary floor type
            if (h < bestCeilH) {
                bestCeilH = h;
            }
        }
    }

    if (bestCeilH < bestFloorH) {
        bestFloorH = bestCeilH;
    }

    // Pass 2: find ceiling above player from ledge floors
    s32 ceilH = (s32)0x80000001;
    for (s32 i = 0; i < count; i++) {
        Floor* floor = floorArray[i];

        bool isLedge = (floor->flags >> 0) & 1;  // bit 0 = ledge
        bool hasRailing = (floor->flags >> 2) & 1; // bit 2 = railing
        if (!(isLedge && !hasRailing)) continue;

        if (!floor->CheckFloorBounds(*pos, radius)) continue;

        s32 h = floor->GetFloorHeight(*pos);
        if (h >= bestFloorH) continue;
        if (ceilH >= h) continue;

        ceilH = h;
        floor->GetFloorNormal(outCeilingNormal);
    }

    // Pass 3: check railing floors for correction
    *outHasRailing = 0;
    for (s32 i = 0; i < count; i++) {
        Floor* floor = floorArray[i];

        bool isLedge = (floor->flags >> 0) & 1;
        bool hasRailing = (floor->flags >> 2) & 1;
        if (!(isLedge && hasRailing)) continue;

        if (!floor->CheckFloorBounds(*pos, radius)) continue;

        s32 h = floor->GetFloorHeight(*pos);
        if (h >= bestFloorH) continue;
        if (ceilH >= h) continue;

        *outHasRailing = 1;
        LVector correction;
        floor->GetRailingCorrection(correction, *pos);
    }

    // Clamp sentinel values
    outFloorH = (bestFloorH > 0x7FFFFFFE) ? 0x7FFFFFFF : bestFloorH;
    outCeilingH = (bestCeilH > 0x7FFFFFFE) ? 0x7FFFFFFF : bestCeilH;
}

// Wall collision result globals
WallCollisionInfo g_wallCollisionInfo;  // PSX: 0x800E00C8
Wall* g_colHitWall = nullptr;           // PSX: gp+1160

// PSX: FillWorldWallArray__15CollisionSectorRC10tagLVectorT1PPC4Walli (COLSECT.CPP:863) 0x800411F8
// Collect Wall pointers overlapping with the search AABB from all sectors
s32 CollisionSector::FillWorldWallArray(
    const LVector& searchMin, const LVector& searchMax,
    Wall** outArray, s32 maxCount) {
    MARKFUNCTION(0x800411F8);

    s32 totalAdded = 0;

    for (s32 sectorIdx = 0; sectorIdx < 12; sectorIdx++) {
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
        } else {
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
