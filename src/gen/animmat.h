#pragma once
#include "core.h"

// AnimationMatrices - per-humanoid joint matrix buffers (660 bytes on PSX)
struct AnimationMatrices {
    // +0: Humanoid* owner
    void* humanoid = nullptr;
    // +4: copy-ready flag
    s32 copied = 0;
    // +8: extra callback mask
    s32 extraMask = 0;
    // +12: current matrix buffer (10 matrices)
    s32* current = nullptr;
    // +16: previous matrix buffer (10 matrices)
    s32* previous = nullptr;

    // +20..+339: first matrix set (10 x 32-byte matrices)
    s32 matricesA[10][8] = {};
    // +340..+659: second matrix set (10 x 32-byte matrices)
    s32 matricesB[10][8] = {};

    AnimationMatrices();
    void SetHumanoid(void* owner);
    void* GetHumanoid() const;
    s32 Copy() const;
    s32 Swap();

    s32* GetMatrix(u32 joint);
    const s32* GetMatrix(u32 joint) const;

    static s32* GetMatrix(AnimationMatrices* am, u32 joint);
    static const s32* GetMatrix(const AnimationMatrices* am, u32 joint);

    // Returns prev/cur frame bone translations for attack collision sweep
    s32 GetAttack(u32 joint, LVector& outPrev, LVector& outCur) const;
};
