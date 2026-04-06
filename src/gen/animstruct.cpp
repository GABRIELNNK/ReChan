// animstruct.cpp - AnimStructure reversed from PSX MODEL.CPP:2335
// PSX source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP
#include "gen/animstruct.h"
#include "gen/time.h"
#include "gen/charmgr.h"
#include "p3d/p3dmath.h"

// PSX: _13AnimStructurelP10tAnimationlP5ModelP13DrawableBasic (0x80070740)
// mode: 0=normal, 1=reverse, 2=runToLast, 3=camera
// animation: tAnimation* (opaque on PC for now)
// loopType: initial loop type (typically 4=hold_last for camera)
// model: Model* (nullptr for camera anims)
// drawableBasic: DrawableBasic* (nullptr for camera anims)
AnimStructure::AnimStructure(s32 m, void* anim, s32 lt, Model* mdl, void* drawableBasic) {
    MARKFUNCTION(0x80070740);

    animation = anim;
    mode = m;
    model = mdl;
    flip = nullptr;
    blendPose = nullptr;
    speed = FIX16_ONE;
    loopCount = 0;

    // PSX: mode 3 (camera) creates tParamFlip or tSequenceFlip as flip handler
    // For mode 0 (normal humanoid), creates tTransformFlip2 + tTreeFlip
    // For mode 1/2, gets flip from animation vtable+20
    // We don't implement the flip creation since tParamFlip/tTreeFlip etc
    // are Pure3D classes not yet reversed on PC

    ResetCountsToAnim();
    SetLoopType(lt, 1);
}

// PSX: __13AnimStructure (0x80070AB8)
AnimStructure::~AnimStructure() {
    MARKFUNCTION(0x80070AB8);

    // PSX: deletes flip via vtable+8 destructor
    // PSX: if blendPose, deletes its child then deletes blendPose
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

    // PSX: endFrame = (vtable+16(animation) - 1) << 16
    // vtable+16 = GetNumFrames. We don't have this on PC yet.
    // For camera anims, the frame count comes from the loaded animation data.
    // Use a safe default for now.
    startFrame = 0;
    currentFrame = 0;
    prevFrame = 0;
    loopCount = 0;

    // PSX: prevTick = currentTick = MEMORY[0x1C]
    s32 tick = g_time ? (s32)g_time->frameCounter : 0;
    currentTick = tick;
    prevTick = tick - 1;
}

// PSX: ForceFrame__13AnimStructurel (0x80070DB8)
void AnimStructure::ForceFrame(s32 frame) {
    MARKFUNCTION(0x80070DB8);
    currentFrame = frame << 16;
    // PSX: calls flip vtable+24 (SetFrame) then vtable+20 (Update)
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

    // Run the boundary handler (loop, hold, etc)
    if (handlerIndex != 0) {
        if (model == nullptr) {
            // No model - use built-in handlers
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
        // PSX: with model, dispatches through model's vtable
        // Not needed for camera mode
    }

    // PSX: if doFlip && flip exists, calls flip->SetFrame then flip->Update
    // This drives the t2PointCamFlip which updates camera position/target

    // Store state
    prevFrame = currentFrame;
    prevTick = currentTick;
}

// PSX: Loop__13AnimStructure (0x8007119C)
void AnimStructure::Loop() {
    if (endFrame > 0 && currentFrame > endFrame) {
        loopCount++;
        if (endFrame + FIX16_ONE != 0) {
            currentFrame = currentFrame % (endFrame + FIX16_ONE);
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
        // Not past start yet
    } else {
        currentFrame = startFrame;
        loopCount++;
        ProcessHumanoidCB();
    }
}

// PSX: HoldLast__13AnimStructure (0x80071278)
void AnimStructure::HoldLast() {
    if (endFrame > 0 && currentFrame > endFrame) {
        currentFrame = endFrame;
        loopCount++;
        ProcessHumanoidCB();
    }
}

// PSX: RunToLast__13AnimStructure (0x800712BC)
void AnimStructure::RunToLast() {
    if (endFrame > 0 && currentFrame > endFrame) {
        currentFrame = endFrame;
        loopCount++;
        ProcessHumanoidCB();
    }
}

// PSX: IncFrame__13AnimStructure
void AnimStructure::IncFrame() {
    currentFrame += FIX16_ONE;
}

// PSX: DecFrame__13AnimStructure
void AnimStructure::DecFrame() {
    currentFrame -= FIX16_ONE;
    if (currentFrame < 0) {
        currentFrame = 0;
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
    // PSX: checks humanoidCB.valid (+98), dispatches callback
    // Not critical for camera operation
}

// PSX: ReAttachTree__13AnimStructurell (0x80070B6C)
void AnimStructure::ReAttachTree(s32 type, s32 animEnum) {
    MARKFUNCTION(0x80070B6C);
    if (!g_characterManager) {
        return;
    }
    void* anim = g_characterManager->GetAnimation((u32)type, animEnum);
    if (!anim) {
        return;
    }
    // PSX: checks GetAnimationType == 65539 (tTreeAnim), reattaches flip
    // Not critical for camera
}
