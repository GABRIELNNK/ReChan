// colsect.cpp - CollisionSector functions
// Original: C:\CHAN\GAME\SRC\GEN\COLSECT.CPP
#include "gen/colsect.h"
#include "gen/colwall.h"
#include "gen/colfloor.h"

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
