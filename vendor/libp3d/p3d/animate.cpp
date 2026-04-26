#include "p3d/animate.h"

void tFlipbook::SetFrame(s32 frame) {
    currentFrame = frame;
}

void tFlipbook::SetFrameReal(s32 frame) {
    SetFrame(frame >> 16);
}

void tFlipbook::UpdateReal() {
    Update();
}

void tFlipbook::SetAnimation(tAnimation* anim) {
    animation = anim;
}

s32 tFlipbook::AdvanceFrame() {
    if (cycleCallback) {
        cycleCallback(this);
    }
    Update();
    return 0;
}

s32 p3dFwdCycle(tFlipbook* flipbook) {
    if (!flipbook) {
        return 0;
    }

    const s32 currentFrame = static_cast<u16>(flipbook->GetFrame());
    s32 lastFrame = -1;

    if (flipbook->GetAnimation()) {
        lastFrame = flipbook->GetAnimation()->GetNumFrames() - 1;
    }

    if (currentFrame < lastFrame) {
        flipbook->SetFrame(currentFrame + 1);
    } else {
        flipbook->SetFrame(0);
    }

    return 0;
}