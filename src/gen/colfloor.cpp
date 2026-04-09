#include "gen/colfloor.h"
#include "gen/colline.h"
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

// PSX: Get__C5FloorR10tagLVectorN31 (COLFLOOR.CPP:369) 0x80093278
// Reconstruct polygon vertices by intersecting adjacent boundary lines
bool Floor::Get(LVector& v0, LVector& v1, LVector& v2, LVector& v3) const {
    MARKFUNCTION(0x80093278);

    s32 numBounds = BoundNumber();
    s32 ok0, ok1, ok2, ok3;

    if (numBounds == 3) {
        ok0 = Intersection(bound[0], bound[1], 0, v0.x, v0.z);
        ok1 = Intersection(bound[1], bound[2], 0, v1.x, v1.z);
        ok2 = Intersection(bound[2], bound[0], 0, v3.x, v3.z);
        ok3 = ok2;
        v2.x = v3.x;
        v2.z = v3.z;
    }
    else {
        ok0 = Intersection(bound[0], bound[1], 0, v0.x, v0.z);
        ok1 = Intersection(bound[1], bound[2], 0, v1.x, v1.z);
        ok2 = Intersection(bound[2], bound[3], 0, v2.x, v2.z);
        ok3 = Intersection(bound[3], bound[0], 0, v3.x, v3.z);
    }

    if (ok0 == 0 || ok1 == 0 || ok2 == 0 || ok3 == 0) return false;

    v0.y = GetFloorHeight(v0);
    v1.y = GetFloorHeight(v1);
    v2.y = GetFloorHeight(v2);
    v3.y = GetFloorHeight(v3);

    return true;
}

// PSX: GetRailingCorrection__C5FloorR10tagLVectorRC10tagLVector (COLFLOOR.CPP:189) 0x8009296C
// Compute push-away correction from nearest boundary edge for railing floors
bool Floor::GetRailingCorrection(LVector& correction, const LVector& pos) const {
    MARKFUNCTION(0x8009296C);

    s32 hasRailing = (flags >> 2) & 1;
    if (!hasRailing) {
        correction.x = 0;
        correction.y = 0;
        correction.z = 0;
        return false;
    }

    // Find boundary line closest to position (most negative dot product)
    s32 dot0 = fixmul16(bound[0].a, pos.x) + fixmul16(bound[0].b, pos.z) + bound[0].c;
    s32 minDot = dot0;
    s32 minIdx = 0;

    for (int i = 1; i < 4; i++) {
        s32 dot = fixmul16(bound[i].a, pos.x) + fixmul16(bound[i].b, pos.z) + bound[i].c;
        if (dot < minDot) {
            minDot = dot;
            minIdx = i;
        }
    }

    // Push away from nearest edge: correction = -normal * (minDot + 2)
    s32 pushDist = minDot + 2;
    correction.x = -fixmul16(bound[minIdx].a, pushDist);
    correction.y = 0;
    correction.z = -fixmul16(bound[minIdx].b, pushDist);

    return hasRailing != 0;
}

// PSX: LedgePrototype__C5FloorRC10tagLVectorT1llR9_RMVECT16R10tagLVector (COLFLOOR.CPP:244) 0x80092B24
// Detect crossing a ledge edge between startPos and endPos
bool Floor::LedgePrototype(const LVector& startPos, const LVector& endPos,
                           s32 height, s32 maxFallHeight,
                           LVector& outNormal, LVector& outCorrectionPos) const {
    MARKFUNCTION(0x80092B24);

    if ((flags & 0x0001) == 0) {
        return false;
    }
    if (((flags >> 2) & 1) != 0) {
        return false;
    }

    s32 endDots[4] = {};
    for (s32 i = 0; i < 4; i++) {
        endDots[i] =
            fixmul16(bound[i].a, endPos.x) +
            fixmul16(bound[i].b, endPos.z) +
            bound[i].c;
    }

    if (endDots[0] < 0 || endDots[1] < 0 || endDots[2] < 0 || endDots[3] < 0) {
        return false;
    }

    s32 startDots[4] = {};
    for (s32 i = 0; i < 4; i++) {
        startDots[i] =
            fixmul16(bound[i].a, startPos.x) +
            fixmul16(bound[i].b, startPos.z) +
            bound[i].c;
    }

    s32 numBounds = BoundNumber();
    for (s32 i = 0; i < numBounds; i++) {
        s32 edgeDot = startDots[i];
        if (edgeDot >= 0) {
            continue;
        }

        outNormal.x = -bound[i].a;
        outNormal.y = 0;
        outNormal.z = -bound[i].b;

        outCorrectionPos.x = startPos.x + fixmul16(outNormal.x, edgeDot);
        outCorrectionPos.z = startPos.z + fixmul16(outNormal.z, edgeDot);
        outCorrectionPos.y = GetFloorHeight(outCorrectionPos);

        if (outCorrectionPos.y < height) {
            continue;
        }
        if (outCorrectionPos.y > maxFallHeight) {
            continue;
        }

        s32 prev = (i - 1 + numBounds) % numBounds;
        s32 next = (i + 1) % numBounds;

        s32 prevDot =
            fixmul16(bound[prev].a, outCorrectionPos.x) +
            fixmul16(bound[prev].b, outCorrectionPos.z) +
            bound[prev].c;
        if (prevDot < 0) {
            continue;
        }

        s32 nextDot =
            fixmul16(bound[next].a, outCorrectionPos.x) +
            fixmul16(bound[next].b, outCorrectionPos.z) +
            bound[next].c;
        if (nextDot < 0) {
            continue;
        }

        return true;
    }

    return false;
}
