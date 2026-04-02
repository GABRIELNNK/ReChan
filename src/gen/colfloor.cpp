// colfloor.cpp - Floor collision functions
// Original: C:\CHAN\GAME\SRC\GEN\COLFLOOR.CPP
#include "gen/colfloor.h"
#include "p3d/p3dmath.h"

// PSX: GetFloorHeight__C5FloorRC10tagLVector (COLFLOOR.CPP:119) 0x800926BC
// Height = fixmul16(normalX, pos.x) + fixmul16(normalZ, pos.z) + heightC
s32 Floor::GetFloorHeight(const LVector& pos) const {
    MARKFUNCTION(0x800926BC);
    return fixmul16(normalX, pos.x) + fixmul16(normalZ, pos.z) + heightC;
}

// PSX: GetFloorNormal__C5FloorR10tagLVector (COLFLOOR.CPP:139) 0x8009272C
// Build normalized 3D normal from 2D height plane coefficients
void Floor::GetFloorNormal(LVector& out) const {
    MARKFUNCTION(0x8009272C);
    if (flags & 0x0001) {
        // Ceiling: normal points up
        out.x = -normalX;
        out.y = 0x00010000; // +1.0 in 16.16
        out.z = -normalZ;
    }
    else {
        // Floor: normal points down
        out.x = normalX;
        out.y = (s32)0xFFFF0000; // -1.0 in 16.16
        out.z = normalZ;
    }
    rmV3Normalize(&out, &out);
}

// PSX: CheckFloorBounds__C5FloorRC10tagLVectorl (COLFLOOR.CPP:167) 0x800927A0
// Test if position is inside all 4 boundary half-planes (within tolerance)
s32 Floor::CheckFloorBounds(const LVector& pos, s32 tolerance) const {
    MARKFUNCTION(0x800927A0);
    s32 negTol = -tolerance;

    // Test bound 0
    s32 dot = fixmul16(bound[0].a, pos.x) + fixmul16(bound[0].b, pos.z) + bound[0].c;
    if (dot < negTol) return 0;

    // Test bound 1
    dot = fixmul16(bound[1].a, pos.x) + fixmul16(bound[1].b, pos.z) + bound[1].c;
    if (dot < negTol) return 0;

    // Test bound 2
    dot = fixmul16(bound[2].a, pos.x) + fixmul16(bound[2].b, pos.z) + bound[2].c;
    if (dot < negTol) return 0;

    // Test bound 3
    dot = fixmul16(bound[3].a, pos.x) + fixmul16(bound[3].b, pos.z) + bound[3].c;
    return (dot >= negTol) ? 1 : 0;
}

// PSX: BoundNumber__C5Floor (COLFLOOR.CPP:340) 0x80093244
// Returns 3 if triangle (bound[2] == bound[3]), 4 if quad
s32 Floor::BoundNumber() const {
    MARKFUNCTION(0x80093244);
    return Equal(bound[2], bound[3]) ? 3 : 4;
}
