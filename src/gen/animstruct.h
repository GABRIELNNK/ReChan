// animstruct.h - AnimStructure reversed from PSX MODEL.CPP:2335
// PSX source: C:\CHAN\GAME\SRC\GEN\MODEL.CPP
// Controls animation playback for models and camera.
// 104 bytes (26 DWORDs), inherits ccNode (24 bytes).
#pragma once

#include "core.h"
#include "p3d/p3dmath.h"
#include "p3d/flip.h"
#include "gen/cclist.h"

class Model;

// Loop type constants (PSX SetLoopType parameter)
enum AnimLoopType : s32 {
    ANIM_LOOP           = 0,
    ANIM_LOOP_REVERSE   = 1,
    ANIM_RUN_TO_LAST    = 2,
    ANIM_HOLD_FIRST     = 3,
    ANIM_HOLD_LAST      = 4,
    ANIM_BLEND          = 5,  // mode 5 = no frame advance in ExecuteHandler
    ANIM_DEC_FRAME      = 6,
    ANIM_BLEND2         = 7,
    ANIM_STOP           = 8,
};

// Humanoid callback info (PSX +96..+103)
struct AnimHumanoidCB {
    s16 offsetLo = 0;    // +96 low
    s16 offsetHi = 0;    // +96 high (HIWORD)
    void* funcPtr = nullptr; // +100
};

// AnimStructure (104 bytes on PSX)
// PSX layout:
//   [0-5]  ccNode (24 bytes)
//   [6]  +24: void* animation (tAnimation*)
//   [7]  +28: void* flip (tParamFlip*/tTreeFlip*/tSequenceFlip*)
//   [8]  +32: Model* model
//   [9]  +36: s32 mode (0=normal, 1=reverse, 2=runToLast, 3=camera)
//   [10] +40: (reserved)
//   [11] +44: s32 loopType
//   [12] +48: s32 speed (16.16, 0x10000 = 1.0)
//   [13] +52: void* blendPose (for mode 7 blend)
//   [14] +56: (reserved)
//   [15] +60: s32 currentFrame (16.16)
//   [16] +64: s32 startFrame
//   [17] +68: s32 endFrame
//   [18] +72: s32 prevFrame
//   [19] +76: s32 prevTick
//   [20] +80: s32 currentTick
//   [21] +84: s32 loopCount
//   [22] +88: s16 handlerOffset, s16 handlerIndex (+90)
//   [23] +92: s16 handlerTable (+92), s16 unused (+94)
//   [24] +96: s16 cbOffsetLo, s16 cbValid (+98)
//   [25] +100: void* cbFuncPtr
class AnimStructure : public ccNode {
public:
    // PSX: _13AnimStructurelP10tAnimationlP5ModelP13DrawableBasic (0x80070740)
    AnimStructure(s32 mode, void* animation, s32 loopType, Model* model = nullptr, void* drawableBasic = nullptr);

    // PSX: __13AnimStructure (0x80070AB8)
    ~AnimStructure() override;

    // PSX: ExecuteHandler__13AnimStructurei (0x80070E1C)
    void ExecuteHandler(s32 doFlip);

    // PSX: SetLoopType__13AnimStructureli (0x80070C20)
    void SetLoopType(s32 type, s32 resetCounts);

    // PSX: ResetCountsToAnim__13AnimStructure (0x80070D30)
    void ResetCountsToAnim();

    // PSX: ForceFrame__13AnimStructurel (0x80070DB8)
    void ForceFrame(s32 frame);

    // PSX: ReAttachTree__13AnimStructurell (0x80070B6C)
    void ReAttachTree(s32 type, s32 animEnum);

    // Accessors
    TransformFlip* GetFlip() const { return flip; }
    TransformAnim* GetAnimation() const { return animation; }
    s32 GetCurrentFrame() const { return currentFrame; }
    s32 GetLoopCount() const { return loopCount; }

    // +24
    TransformAnim* animation = nullptr;
    // +28
    TransformFlip* flip = nullptr;
    // +32
    Model* model = nullptr;
    // +36
    s32 mode = 0;
    // +40: current animation enum being played (set by ApplyAnimToModel)
    s32 animEnum = 0;
    // +44
    s32 loopTypeField = 0;
    // +48
    s32 speed = FIX16_ONE;
    // +52
    void* blendPose = nullptr;
    // +56
    s32 field56 = 0;
    // +60
    s32 currentFrame = 0;
    // +64
    s32 startFrame = 0;
    // +68
    s32 endFrame = 0;
    // +72
    s32 prevFrame = 0;
    // +76
    s32 prevTick = 0;
    // +80
    s32 currentTick = 0;
    // +84
    s32 loopCount = 0;
    // +88
    s16 handlerOffset = 0;
    s16 handlerIndex = 0;
    // +92
    s16 handlerTable = 0;
    s16 handlerPad = 0;
    // +96..+103
    AnimHumanoidCB humanoidCB = {};

    // PSX: these are called from Model virtual handler trampolines.
    // Must be accessible to Model-level handlers.
    void Loop();
    void LoopReverse();
    void HoldFirst();
    void HoldLast();
    void RunToLast();
    void IncFrame();
    void DecFrame();
    void RunToLastBlend();
    void ProcessHumanoidCB();
};
