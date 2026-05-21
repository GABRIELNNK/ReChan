#include "common.h"
#include "gen/trail.h"

#include "gen/blockmgr.h"
#include "gen/colsect.h"

static ccMinList g_trailsPoolList;
static ccMinNode* g_trailsPoolInsertAfter = nullptr;

static constexpr s32 SPL_TRAIL_RATIO = 0x4E44;
static constexpr s32 TRAIL_ZSORT_SCRATCH_BYTES = 136;

TrailInfo::TrailInfo() {
    MARKFUNCTION(0x8007A624);
}

// PSX: SetupDecrements__9TrailInfoi (TRAIL.CPP:86)
void TrailInfo::SetupDecrements(s32 steps) {
    MARKFUNCTION(0x800791B8);

    ASSERT(steps != 0);

    const s32 midX = (x1 + x2) >> 1;
    const s32 midY = (y1 + y2) >> 1;
    const s32 midZ = (z1 + z2) >> 1;

    dx = (midX - x1) / steps;
    dy = (midY - y1) / steps;
    dz = (midZ - z1) / steps;

    r = static_cast<s16>((static_cast<u8>(color) << 8));
    g = static_cast<s16>((static_cast<u8>(color >> 8) << 8));
    b = static_cast<s16>((static_cast<u8>(color >> 16) << 8));

    dr = static_cast<s16>(r / steps);
    dg = static_cast<s16>(g / steps);
    db = static_cast<s16>(b / steps);

    life = static_cast<u16>(steps);
}

// PSX: SetVelocity__9TrailInfoP10tagLVector (TRAIL.CPP:127)
void TrailInfo::SetVelocity(const LVector* vel) {
    MARKFUNCTION(0x800793C4);

    if (vel) {
        velocity = *vel;
    }
    else {
        velocity = {};
    }
}

// PSX: Update__9TrailInfo (TRAIL.CPP:153)
bool TrailInfo::Update() {
    MARKFUNCTION(0x80079400);

    x1 += dx;
    y1 += dy;
    z1 += dz;

    x2 -= dx;
    y2 -= dy;
    z2 -= dz;

    TrailInfo* prevTrail = static_cast<TrailInfo*>(prev);
    if (prevTrail) {
        x1 += static_cast<s32>((static_cast<s64>(SPL_TRAIL_RATIO)
            * static_cast<s64>(prevTrail->x1 - x1)) >> 16);
        y1 += static_cast<s32>((static_cast<s64>(SPL_TRAIL_RATIO)
            * static_cast<s64>(prevTrail->y1 - y1)) >> 16);
        z1 += static_cast<s32>((static_cast<s64>(SPL_TRAIL_RATIO)
            * static_cast<s64>(prevTrail->z1 - z1)) >> 16);

        x2 += static_cast<s32>((static_cast<s64>(SPL_TRAIL_RATIO)
            * static_cast<s64>(prevTrail->x2 - x2)) >> 16);
        y2 += static_cast<s32>((static_cast<s64>(SPL_TRAIL_RATIO)
            * static_cast<s64>(prevTrail->y2 - y2)) >> 16);
        z2 += static_cast<s32>((static_cast<s64>(SPL_TRAIL_RATIO)
            * static_cast<s64>(prevTrail->z2 - z2)) >> 16);
    }

    r = static_cast<s16>(r - dr);
    g = static_cast<s16>(g - dg);
    b = static_cast<s16>(b - db);

    const u16 oldLife = life;
    life = static_cast<u16>(oldLife - 1);

    color = static_cast<u32>(static_cast<u8>(static_cast<u16>(r) >> 8))
        | static_cast<u32>(static_cast<u16>(g) & 0xFF00)
        | (static_cast<u32>(static_cast<u8>(static_cast<u16>(b) >> 8)) << 16);

    return oldLife > 0;
}

// PSX: _6Trailsi (TRAIL.CPP:220)
Trails::Trails(s32 maxTrails)
    : poolCount(maxTrails) {
    MARKFUNCTION(0x80079614);

    trailInfoPool = (poolCount > 0) ? new TrailInfo[poolCount] : nullptr;
    for (s32 i = 0; i < poolCount; i++) {
        trailInfoPool[i].trailId = static_cast<u16>(i);
        freeList.AddNode(freeList.tail, &trailInfoPool[i]);
    }

    mode = 0;
    activeInEffects = 0;
    currentPos = nullptr;

    if (poolCount > 0) {
        zSortScratch = new u8[TRAIL_ZSORT_SCRATCH_BYTES * poolCount];
        zSortScratchEnd = zSortScratch + (TRAIL_ZSORT_SCRATCH_BYTES * poolCount);
    }

    g_trailsPoolList.AddNode(g_trailsPoolInsertAfter, this);
}

// PSX: __6Trails (TRAIL.CPP:265)
Trails::~Trails() {
    MARKFUNCTION(0x80079778);

    if (!activeInEffects) {
        g_trailsPoolList.RemNode(this);
    }

    delete[] trailInfoPool;
    trailInfoPool = nullptr;

    delete[] zSortScratch;
    zSortScratch = nullptr;
    zSortScratchEnd = nullptr;

    mode = 0;
    activeInEffects = 0;
    currentPos = nullptr;

    // Keep list destructors from deleting pooled array elements.
    activeList.head = nullptr;
    activeList.tail = nullptr;
    freeList.head = nullptr;
    freeList.tail = nullptr;
}

// PSX: Add__6TrailsP10tagLVectorT1UliT1 (TRAIL.CPP:297)
TrailInfo* Trails::Add(const LVector* start, const LVector* end, u32 inColor, s32 steps, const LVector* vel) {
    MARKFUNCTION(0x80079894);

    if (!start || !end) {
        return nullptr;
    }

    const s32 block = CollisionSector::GetBlockNumber(*start);
    if (block != -1) {
        blockNum = block;
    }

    LVector startPos = *start;
    LVector endPos = *end;

    if (vel) {
        startPos -= *vel;
        endPos -= *vel;
    }

    if (!activeInEffects) {
        activeInEffects = 1;
        g_trailsPoolList.RemNode(this);
        Effects_AddEffect(this, 0);
    }

    TrailInfo* trail = nullptr;

    if (freeList.head) {
        trail = static_cast<TrailInfo*>(freeList.RemHead());
    }
    else {
        trail = FindDoneTrail(1);
        if (!trail) {
            return nullptr;
        }
        activeList.RemNode(trail);
    }

    trail->x1 = startPos.x;
    trail->y1 = startPos.y;
    trail->z1 = startPos.z;

    trail->x2 = endPos.x;
    trail->y2 = endPos.y;
    trail->z2 = endPos.z;

    trail->color = inColor;

    trail->x1 <<= 8;
    trail->y1 <<= 8;
    trail->z1 <<= 8;

    trail->x2 <<= 8;
    trail->y2 <<= 8;
    trail->z2 <<= 8;

    trail->SetupDecrements(steps);
    trail->SetVelocity(vel);

    mode = 2;

    activeList.AddNode(nullptr, trail);
    return trail;
}

// PSX: PutBackEffect__6Trails (TRAIL.CPP:479)
s32 Trails::PutBackEffect() {
    MARKFUNCTION(0x80079AB4);

    g_trailsPoolList.AddNode(g_trailsPoolInsertAfter, this);
    activeInEffects = 0;
    Flush();
    return 0;
}

// PSX: Flush__6Trails (TRAIL.CPP:499)
void Trails::Flush() {
    MARKFUNCTION(0x80079AF4);

    mode = 0;

    while (activeList.head) {
        ccMinNode* node = activeList.RemHead();
        freeList.AddNode(freeList.tail, node);
    }
}

// PSX: FindDoneTrail__6Trailsi (TRAIL.CPP:517)
TrailInfo* Trails::FindDoneTrail(s32 threshold) {
    MARKFUNCTION(0x80079B54);

    TrailInfo* trail = static_cast<TrailInfo*>(activeList.tail);
    while (trail && threshold < static_cast<s16>(trail->life)) {
        trail = static_cast<TrailInfo*>(trail->prev);
    }

    return trail;
}

// PSX: Update__6Trails (TRAIL.CPP:548)
s32 Trails::Update() {
    MARKFUNCTION(0x80079B88);

    if (mode != 0) {
        TrailInfo* trail = static_cast<TrailInfo*>(activeList.head);

        if (trail) {
            while (trail) {
                TrailInfo* next = static_cast<TrailInfo*>(trail->next);

                if (!trail->Update()) {
                    activeList.RemNode(trail);
                    freeList.AddNode(freeList.tail, trail);
                }

                trail = next;
            }
        }
        else {
            Effects_RemoveEffect(this);
            g_trailsPoolList.AddNode(g_trailsPoolInsertAfter, this);
            activeInEffects = 0;
            currentPos = nullptr;
        }
    }

    return mode;
}

// PSX: SetCurrentPos__6TrailsP10tagLVector (TRAIL.CPP:586)
void Trails::SetCurrentPos(LVector* pos) {
    MARKFUNCTION(0x80079C44);
    currentPos = pos;
}

// PSX: Display__6Trailsi (TRAIL.CPP:599)
void Trails::Display(s32 inBlockNum) {
    MARKFUNCTION(0x80079C4C);

    if (mode == 0 || !g_blockManager) {
        return;
    }

    Block* block = g_blockManager->GetBlock(static_cast<u32>(inBlockNum));
    if (!block || block->blockNum != static_cast<u16>(inBlockNum)) {
        return;
    }

    const s32 numTrails = activeList.GetNumElements();
    if (mode == 2 && numTrails >= 2) {
        ChanZSortDisplayNonTexture(numTrails);
    }
    else if (mode == 3 && numTrails >= 3) {
        ChanZSortDisplayTexture(numTrails);
    }
}

// PSX: ChanZSortDisplayNonTexture__6Trailsi (TRAIL.CPP:629)
void Trails::ChanZSortDisplayNonTexture(s32 /*count*/) {
    MARKFUNCTION(0x80079D1C);
}

// PSX: ChanZSortDisplayTexture__6Trailsi (TRAIL.CPP:826)
void Trails::ChanZSortDisplayTexture(s32 /*count*/) {
    MARKFUNCTION(0x8007A134);
}

// PSX: Create__6Trails (TRAIL.HPP:103)
s32 Trails::Create() {
    MARKFUNCTION(0x8007A61C);
    return 1;
}
