#pragma once

#include "core.h"
#include "gen/cclist.h"
#include "gen/effects.h"
#include "p3d/lvector.h"

struct OriginalGeo;

struct TrailInfo : public ccMinNode {
    s32 x1 = 0;
    s32 y1 = 0;
    s32 z1 = 0;
    s32 x2 = 0;
    s32 y2 = 0;
    s32 z2 = 0;

    s32 dx = 0;
    s32 dy = 0;
    s32 dz = 0;

    LVector velocity = {};

    u32 color = 0;

    s16 dr = 0;
    s16 dg = 0;
    s16 db = 0;

    s16 r = 0;
    s16 g = 0;
    s16 b = 0;

    u16 life = 0;
    u16 trailId = 0;

    TrailInfo();
    ~TrailInfo() override = default;

    void SetupDecrements(s32 steps);
    void SetVelocity(const LVector* vel);
    bool Update();
};

class Trails : public Effects {
public:
    Trails(s32 maxTrails);
    ~Trails() override;

    TrailInfo* Add(const LVector* start, const LVector* end, u32 color, s32 steps, const LVector* velocity);

    s32 PutBackEffect() override;
    void Flush();
    TrailInfo* FindDoneTrail(s32 threshold);

    s32 Update() override;
    void SetCurrentPos(LVector* pos);
    void Display(s32 blockNum) override;
    s32 Create() override;

private:
    void ChanZSortDisplayNonTexture(s32 count);
    void ChanZSortDisplayTexture(s32 count);

    s32 mode = 0;

    // PSX TRAIL.CPP +40/+44/+48/+52: textured trail geometry selectors.
    OriginalGeo* textureGeoFirst = nullptr;
    OriginalGeo* textureGeoSecond = nullptr;
    OriginalGeo* textureGeoMid = nullptr;
    OriginalGeo* textureGeoLast = nullptr;

    TrailInfo* trailInfoPool = nullptr;
    ccMinList freeList;
    ccMinList activeList;

    s32 activeInEffects = 0;
    LVector* currentPos = nullptr;

    u8* zSortScratch = nullptr;
    u8* zSortScratchEnd = nullptr;

    s32 poolCount = 0;
};
