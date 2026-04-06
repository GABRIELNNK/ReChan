// animmat.h - AnimationMatrices reversed from PSX ANIMMAT.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\ANIMMAT.CPP
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

    // PSX: _17AnimationMatrices (ANIMMAT.CPP:550)
    AnimationMatrices();

    // PSX: SetHumanoid__17AnimationMatricesP8Humanoid (ANIMMAT.CPP:577)
    void SetHumanoid(void* owner);

    // PSX: GetHumanoid__17AnimationMatrices (ANIMMAT.CPP:587)
    void* GetHumanoid() const;

    // PSX: Copy__C17AnimationMatrices (ANIMMAT.CPP:597)
    s32 Copy() const;

    // PSX: Swap__17AnimationMatrices (ANIMMAT.CPP:831)
    s32 Swap();

    // PSX: GetMatrix__C17AnimationMatricesUl (ANIMMAT.CPP:849)
    s32* GetMatrix(u32 joint);
    const s32* GetMatrix(u32 joint) const;

    static s32* GetMatrix(AnimationMatrices* am, u32 joint);
    static const s32* GetMatrix(const AnimationMatrices* am, u32 joint);
};
