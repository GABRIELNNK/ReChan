// animstruct.cpp - AnimStructure reversed from PSX MODEL.CPP:2335
// PSX source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP
#include "gen/animstruct.h"
#include "gen/model.h"
#include "gen/time.h"
#include "gen/charmgr.h"
#include "p3d/p3dmath.h"

// PSX: _13AnimStructurelP10tAnimationlP5ModelP13DrawableBasic (0x80070740)
// mode: 0=normal, 1=reverse, 2=runToLast, 3=camera
// animation: TransformAnim* (parsed from raw binary)
// loopType: initial loop type
// model: Model* (nullptr for camera anims)
// drawableBasic: DrawableBasic* (nullptr for camera anims)
AnimStructure::AnimStructure(s32 m, void* anim, s32 lt, Model* mdl, void* drawableBasic) {
    MARKFUNCTION(0x80070740);

    animation = (TransformAnim*)anim;
    mode = m;
    model = mdl;
    flip = nullptr;
    blendPose = nullptr;
    speed = FIX16_ONE;
    loopCount = 0;

    // PSX: mode 0 creates tTransformFlip2 + tTreeFlip, attaches to tree + animation
    if (mode == 0 && animation && mdl) {
        // Get skeleton from model's drawable
        STreeData* skeleton = nullptr;
        if (mdl->drawable && mdl->drawable->original) {
            skeleton = mdl->drawable->original->skeleton;
        }
        if (skeleton) {
            flip = new TransformFlip();
            flip->Attach(skeleton, animation);

            // Set endFrame from animation's numFrames
            if (animation->numFrames > 0) {
                endFrame = (animation->numFrames - 1) << 16;
            }

            flip->Reset();
        }
    }

    ResetCountsToAnim();
    SetLoopType(lt, 1);
}

// PSX: __13AnimStructure (0x80070AB8)
AnimStructure::~AnimStructure() {
    MARKFUNCTION(0x80070AB8);

    delete flip;
    flip = nullptr;
    animation = nullptr;
    blendPose = nullptr;
}

// PSX: ResetCountsToAnim__13AnimStructure (0x80070D30)
void AnimStructure::ResetCountsToAnim() {
    MARKFUNCTION(0x80070D30);

    if (!animation) {
        return;
    }

    // PSX: endFrame = (GetNumFrames(animation) - 1) << 16
    if (animation->numFrames > 0) {
        endFrame = (animation->numFrames - 1) << 16;
    }
    startFrame = 0;
    currentFrame = 0;
    prevFrame = 0;
    loopCount = 0;

    s32 tick = g_time ? (s32)g_time->frameCounter : 0;
    currentTick = tick;
    prevTick = tick - 1;
}

// PSX: ForceFrame__13AnimStructurel (0x80070DB8)
void AnimStructure::ForceFrame(s32 frame) {
    MARKFUNCTION(0x80070DB8);
    currentFrame = frame << 16;
    if (flip) {
        flip->SetFrame(frame);
        flip->UpdateJoints();
    }
}

// PSX: SetLoopType__13AnimStructureli (0x80070C20)
void AnimStructure::SetLoopType(s32 type, s32 resetCounts) {
    MARKFUNCTION(0x80070C20);

    switch (type) {
        case 0: // Loop
            handlerOffset = 0;
            handlerIndex = 7;
            handlerTable = 8;
            break;
        case 1: // LoopReverse
            handlerOffset = 0;
            handlerIndex = 8;
            handlerTable = 8;
            break;
        case 2: // RunToLast
            handlerOffset = 0;
            handlerIndex = 11;
            handlerTable = 8;
            break;
        case 3: // HoldFirst
            handlerOffset = 0;
            handlerIndex = 9;
            handlerTable = 8;
            break;
        case 4: // HoldLast
            handlerOffset = 0;
            handlerIndex = 10;
            handlerTable = 8;
            break;
        case 5: // Blend
            handlerOffset = 0;
            handlerIndex = 14;
            handlerTable = 8;
            break;
        case 6: // DecFrame
            handlerOffset = 0;
            handlerIndex = 15;
            handlerTable = 8;
            break;
        case 7: // Blend2
            handlerOffset = 0;
            handlerIndex = 17;
            handlerTable = 8;
            resetCounts = 0;
            break;
        case 8: // Stop
            handlerOffset = 0;
            handlerIndex = 0;
            handlerTable = 0;
            handlerPad = 0;
            break;
        default:
            break;
    }

    loopTypeField = type;
    if (resetCounts) {
        ResetCountsToAnim();
    }
}

// PSX: ExecuteHandler__13AnimStructurei (0x80070E1C)
void AnimStructure::ExecuteHandler(s32 doFlip) {
    MARKFUNCTION(0x80070E1C);

    // PSX: currentTick = MEMORY[0x1C]
    s32 tick = g_time ? (s32)g_time->frameCounter : 0;
    currentTick = tick;

    // Compute frame delta based on speed
    s32 elapsed;
    if (speed == FIX16_ONE) {
        elapsed = (tick - prevTick) << 16;
    } else {
        elapsed = (s32)((((s64)(tick - prevTick)) << 16) * (s64)speed >> 16);
    }

    // Advance frame based on loop type
    if (loopTypeField == 1) {
        // Reverse: subtract
        currentFrame = currentFrame - elapsed;
    } else if (loopTypeField != 5) {
        // Normal/other: add
        currentFrame = currentFrame + elapsed;
    }
    // mode 5 (blend) = no frame advance

    // Run the boundary handler (loop, hold, etc).
    // PSX dispatches via model-side tables when model != null. That table
    // is not fully reversed on PC yet, so use the same built-in handlers.
    if (handlerIndex != 0) {
        switch (loopTypeField) {
            case 0: Loop(); break;
            case 1: LoopReverse(); break;
            case 2: RunToLast(); break;
            case 3: HoldFirst(); break;
            case 4: HoldLast(); break;
            case 6: DecFrame(); break;
            default: break;
        }
    }

    // PSX: if doFlip, updates flipbook state.
    if (doFlip && flip) {
        if (currentFrame < 0) {
            currentFrame = 0;
        }
        flip->SetFrameReal(currentFrame);
        flip->UpdateJoints();
    }

    // Store state
    prevFrame = currentFrame;
    prevTick = currentTick;
}

// PSX: Loop__13AnimStructure (0x8007119C)
void AnimStructure::Loop() {
    if (currentFrame > endFrame) {
        loopCount++;
        s32 denom = endFrame + FIX16_ONE;
        if (denom != 0) {
            currentFrame = currentFrame % denom;
        }
    }
}

// PSX: LoopReverse__13AnimStructure (0x80071200)
void AnimStructure::LoopReverse() {
    if (currentFrame < 0) {
        currentFrame = endFrame - FIX16_ONE;
        loopCount++;
    }
}

// PSX: HoldFirst__13AnimStructure (0x80071234)
void AnimStructure::HoldFirst() {
    if (startFrame < currentFrame) {
        currentFrame = startFrame;
        loopCount++;
        ProcessHumanoidCB();
    }
}

// PSX: HoldLast__13AnimStructure (0x80071278)
void AnimStructure::HoldLast() {
    if (currentFrame > endFrame) {
        currentFrame = endFrame;
        loopCount++;
        ProcessHumanoidCB();
    }
}

// PSX: RunToLast__13AnimStructure (0x800712BC)
void AnimStructure::RunToLast() {
    if (currentFrame > endFrame) {
        currentFrame = endFrame;
        loopCount++;
        ProcessHumanoidCB();
    }
}

// PSX: IncFrame__13AnimStructure
void AnimStructure::IncFrame() {
    if (endFrame >= currentFrame) {
        currentFrame = prevFrame + FIX16_ONE;
    } else {
        s32 denom = endFrame + FIX16_ONE;
        loopCount++;
        if (denom != 0) {
            currentFrame = currentFrame % denom;
        }
        ProcessHumanoidCB();
    }

    if (flip) {
        flip->SetFrameReal(currentFrame);
        flip->UpdateJoints();
    }
}

// PSX: DecFrame__13AnimStructure
void AnimStructure::DecFrame() {
    currentFrame = prevFrame - FIX16_ONE;
    if (currentFrame < 0) {
        currentFrame = endFrame;
        ProcessHumanoidCB();
    }
}

// PSX: RunToLastBlend__13AnimStructure
void AnimStructure::RunToLastBlend() {
    if (endFrame > 0 && currentFrame > endFrame) {
        currentFrame = endFrame;
        loopCount++;
    }
}

// PSX: ProcessHumanoidCB__13AnimStructure (0x80071108)
void AnimStructure::ProcessHumanoidCB() {
    // PSX dispatches callback and clears cb fields. Clear fields here to
    // preserve one-shot semantics until full callback dispatch is reversed.
    if (humanoidCB.offsetHi != 0) {
        humanoidCB = {};
    }
}

// PSX: ReAttachTree__13AnimStructurell (0x80070B6C)
void AnimStructure::ReAttachTree(s32 type, s32 animEnum) {
    MARKFUNCTION(0x80070B6C);
    if (!g_characterManager) {
        return;
    }
    TransformAnim* newAnim = (TransformAnim*)g_characterManager->GetAnimation((u32)type, animEnum);
    if (!newAnim) {
        return;
    }
    // PSX: reattaches flip to new animation
    animation = newAnim;
    if (flip) {
        flip->anim = newAnim;
        flip->dirty = 1;
    }
    ResetCountsToAnim();
}
