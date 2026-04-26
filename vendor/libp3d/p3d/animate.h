#pragma once

#include "p3d/entity.h"

class tFlipbook;

using tFlipbookCycleCallback = s32 (*)(tFlipbook* flipbook);

class tAnimation : public tEntity {
public:
    tAnimation() = default;
    ~tAnimation() override = default;

    virtual s32 GetNumFrames() const = 0;
    virtual tFlipbook* MakePuppet() = 0;
};

class tPuppet : public tEntity {
public:
    tPuppet() = default;
    ~tPuppet() override = default;

protected:
    s32 fieldC = 0;
    s32 field10 = 0;
};

class tFlipbook : public tPuppet {
public:
    tFlipbook() = default;
    ~tFlipbook() override = default;

    virtual void Reset() = 0;
    virtual void Update() = 0;
    virtual void SetFrame(s32 frame);
    virtual void SetFrameReal(s32 frame);
    virtual void UpdateReal();
    virtual void SetAnimation(tAnimation* anim);

    s32 AdvanceFrame();
    s32 GetFrame() const { return currentFrame; }
    void SetCycleCallback(tFlipbookCycleCallback callback) { cycleCallback = callback; }
    tAnimation* GetAnimation() const { return animation; }

protected:
    s32 currentFrame = 0;
    tFlipbookCycleCallback cycleCallback = nullptr;
    tAnimation* animation = nullptr;
};

s32 p3dFwdCycle(tFlipbook* flipbook);