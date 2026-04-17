#include "gen/common.h"
#include "gen/animmat.h"
#include "p3d/skeleton.h"
#include "p3d/matrix.h"
#include "p3d/hash.h"

namespace {
    void BuildPsxIdentityMatrix(s32* matrixData) {
        std::memset(matrixData, 0, sizeof(s32) * 8);

        // PSX MATRIX rotation is Q12 fixed-point shorts at [0..8].
        s16* rot = reinterpret_cast<s16*>(matrixData);
        rot[0] = 0x1000;
        rot[4] = 0x1000;
        rot[8] = 0x1000;
    }

}

// PSX: _17AnimationMatrices (ANIMMAT.CPP:550, 0x80078880)
AnimationMatrices::AnimationMatrices() {
    MARKFUNCTION(0x80078880);

    humanoid = nullptr;
    copied = 0;
    extraMask = 0;

    current = &matricesA[0][0];
    previous = &matricesB[0][0];

    for (u32 i = 0; i < 10; i++) {
        BuildPsxIdentityMatrix(matricesA[i]);
        BuildPsxIdentityMatrix(matricesB[i]);
    }

    for (s32 i = 0; i < AM_NUM_SLOTS; i++) {
        boneJointIndex[i] = -1;
    }
    bonesCached = false;
}

// PSX: SetHumanoid__17AnimationMatricesP8Humanoid (ANIMMAT.CPP:577, 0x80078908)
void AnimationMatrices::SetHumanoid(void* owner) {
    MARKFUNCTION(0x80078908);
    humanoid = owner;
}

// PSX: GetHumanoid__17AnimationMatrices (ANIMMAT.CPP:587, 0x80078910)
void* AnimationMatrices::GetHumanoid() const {
    MARKFUNCTION(0x80078910);
    return humanoid;
}

// PSX: Copy__C17AnimationMatrices (ANIMMAT.CPP:597, 0x8007891C)
s32 AnimationMatrices::Copy() const {
    MARKFUNCTION(0x8007891C);
    return copied;
}

// PSX: Swap__17AnimationMatrices (ANIMMAT.CPP:831, 0x80078E80)
s32 AnimationMatrices::Swap() {
    MARKFUNCTION(0x80078E80);
    s32* oldCurrent = current;
    copied = 0;
    current = previous;
    previous = oldCurrent;
    return static_cast<s32>(reinterpret_cast<uintptr_t>(current) & 0xFFFFFFFFu);
}

// PSX: GetMatrix__C17AnimationMatricesUl (ANIMMAT.CPP:849, 0x80078E98)
s32* AnimationMatrices::GetMatrix(u32 joint) {
    MARKFUNCTION(0x80078E98);
    if (joint >= 10 || !current) {
        return nullptr;
    }
    return current + (joint * 8);
}

const s32* AnimationMatrices::GetMatrix(u32 joint) const {
    MARKFUNCTION(0x80078E98);
    if (joint >= 10 || !current) {
        return nullptr;
    }
    return current + (joint * 8);
}

s32* AnimationMatrices::GetMatrix(AnimationMatrices* am, u32 joint) {
    if (!am) {
        return nullptr;
    }
    return am->GetMatrix(joint);
}

const s32* AnimationMatrices::GetMatrix(const AnimationMatrices* am, u32 joint) {
    if (!am) {
        return nullptr;
    }
    return am->GetMatrix(joint);
}

// PSX: GetAttack__C17AnimationMatricesUlR10tagLVectorT2 (ANIMMAT.CPP:878, 0x80078EB8)
s32 AnimationMatrices::GetAttack(u32 joint, LVector& outPrev, LVector& outCur) const {
    MARKFUNCTION(0x80078EB8);
    if (joint >= 10) {
        return 0;
    }
    s32 offset = joint * 8;
    outPrev.x = previous[offset + 5];
    outPrev.y = previous[offset + 6];
    outPrev.z = previous[offset + 7];
    outCur.x = current[offset + 5];
    outCur.y = current[offset + 6];
    outCur.z = current[offset + 7];
    return 1;
}

// PSX bone name table: 10 tracked joints matching SetupModelCallbacks order.
// Slot 0: Head, 1: L Hand, 2: R Hand, 3: L Foot, 4: R Foot,
// 5: Pelvis, 6: L UpperArm, 7: R UpperArm, 8: L Thigh, 9: R Thigh
static const char* const AM_JointNames[AM_NUM_SLOTS] = {
    "Bip01 Head",
    "Bip01 L Hand",
    "Bip01 R Hand",
    "Bip01 L Foot",
    "Bip01 R Foot",
    "Bip01 Pelvis",
    "Bip01 L UpperArm",
    "Bip01 R UpperArm",
    "Bip01 L Thigh",
    "Bip01 R Thigh",
};

void AnimationMatrices::CacheBoneIndices(const STreeData* skeleton) {
    if (!skeleton || !skeleton->joints || skeleton->numJoints == 0) {
        return;
    }

    for (s32 slot = 0; slot < AM_NUM_SLOTS; slot++) {
        u32 nameHash = p3dHash(AM_JointNames[slot]);
        boneJointIndex[slot] = -2; // not found
        for (u32 j = 0; j < skeleton->numJoints; j++) {
            if (skeleton->joints[j].nameUID == nameHash) {
                boneJointIndex[slot] = (s32)j;
                break;
            }
        }
    }
    bonesCached = true;
}

void AnimationMatrices::UpdateWorldPositions(const STreeData* skeleton, const Mat4& worldMatrix) {
    if (!skeleton || !skeleton->joints || skeleton->numJoints == 0 || !current) {
        return;
    }

    if (!bonesCached) {
        CacheBoneIndices(skeleton);
    }

    // Compute model-local joint matrices (same as rendering path)
    Mat4* jointMatrices = new Mat4[skeleton->numJoints];
    skeleton->ComputeWorldMatrices(jointMatrices);

    for (s32 slot = 0; slot < AM_NUM_SLOTS; slot++) {
        s32 ji = boneJointIndex[slot];
        if (ji < 0) {
            continue;
        }

        // World-space = model world matrix * joint local matrix
        Mat4 worldJoint = worldMatrix * jointMatrices[ji];

        // Extract world-space translation into current buffer.
        // PSX layout: [0..4] = 3x3 rotation (Q12 shorts), [5..7] = translation (s32).
        // On PSX, CopyMatrix reads GTE C0-C7 directly.
        // Translation goes into offsets 5, 6, 7 of the 8-word slot.
        s32 offset = slot * 8;
        current[offset + 5] = (s32)worldJoint.GetTransX();
        current[offset + 6] = (s32)worldJoint.GetTransY();
        current[offset + 7] = (s32)worldJoint.GetTransZ();
    }

    copied = 1;

    delete[] jointMatrices;
}
