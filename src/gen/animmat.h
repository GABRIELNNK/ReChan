#pragma once
#include "core.h"

class Model;
struct STreeData;
struct STreeJoint;
struct Mat4;

// PSX bone slot names (10 tracked joints):
// 0: Bip01 Head, 1: Bip01 L Hand, 2: Bip01 R Hand,
// 3: Bip01 L Foot, 4: Bip01 R Foot, 5: Bip01 Pelvis,
// 6: Bip01 L UpperArm, 7: Bip01 R UpperArm,
// 8: Bip01 L Thigh, 9: Bip01 R Thigh
static constexpr s32 AM_NUM_SLOTS = 10;

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

    // PC: cached skeleton joint indices for the 10 tracked bones.
    // -1 = not yet resolved, -2 = bone not found in skeleton.
    s32 boneJointIndex[AM_NUM_SLOTS];
    bool bonesCached = false;
    bool captureEnabled = true;

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
    s32 GetWeaponAttack(u32 joint, const LVector& localOffset, LVector& outPrev, LVector& outCur) const;
    // Returns full prev/cur 8-word (rotation+translation) joint matrices, for render-time interpolation.
    s32 GetMatrixPair(u32 joint, s32 outPrev[8], s32 outCur[8]) const;
    void SetCaptureEnabled(s32 enable);
    s32 CaptureEnabled() const;

    void SetupCallbacks(Model* model);
    void SetupExtraCallbacks(Model* model);
    s32 SetExtraCallbacks(s32 matrixType, s32 enable);
    s32 CopyMatrix(u32 joint, const Mat4& jointMatrix);

    // PC: resolve bone name UIDs to skeleton joint indices (called once)
    void CacheBoneIndices(const STreeData* skeleton);
};
