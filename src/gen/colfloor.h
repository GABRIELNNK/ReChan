// colfloor.h - Floor struct for collision floor polygons
// Original: C:\CHAN\GAME\SRC\GEN\COLFLOOR.CPP
#pragma once

#include "core.h"
#include "gen/colline.h"
#include "p3d/lvector.h"

// Floor - collision floor polygon (triangle or quad)
// PSX: 64 bytes. Height plane: y = fixmul16(normalX, pos.x) + fixmul16(normalZ, pos.z) + heightC
// Bounded by 4 boundary lines (3rd == 4th for triangles).
struct Floor {
    s32 normalX;      // +0: height plane X coefficient (16.16 fixed)
    s32 normalZ;      // +4: height plane Z coefficient (16.16 fixed)
    s32 heightC;      // +8: height plane constant
    s16 field0C;      // +12: unknown
    u16 flags;        // +14: bit0=ceiling, bit16=ledge, bit18=railing
    Line bound[4];    // +16: boundary half-plane lines (12 bytes each)

    // PSX: GetFloorHeight__C5FloorRC10tagLVector (COLFLOOR.CPP:119) 0x800926BC
    s32 GetFloorHeight(const LVector& pos) const;

    // PSX: GetFloorNormal__C5FloorR10tagLVector (COLFLOOR.CPP:139) 0x8009272C
    void GetFloorNormal(LVector& out) const;

    // PSX: CheckFloorBounds__C5FloorRC10tagLVectorl (COLFLOOR.CPP:167) 0x800927A0
    s32 CheckFloorBounds(const LVector& pos, s32 tolerance) const;

    // PSX: BoundNumber__C5Floor (COLFLOOR.CPP:340) 0x80093244
    s32 BoundNumber() const;
};
