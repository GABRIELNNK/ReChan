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

    s32 GetFloorHeight(const LVector& pos) const;
    void GetFloorNormal(LVector& out) const;
    s32 CheckFloorBounds(const LVector& pos, s32 tolerance) const;
    s32 BoundNumber() const;
    bool Get(LVector& v0, LVector& v1, LVector& v2, LVector& v3) const;
    bool GetRailingCorrection(LVector& correction, const LVector& pos) const;
    bool LedgePrototype(const LVector& startPos, const LVector& endPos,
                        s32 height, s32 maxFallHeight,
                        LVector& outNormal, LVector& outCorrectionPos) const;
};
