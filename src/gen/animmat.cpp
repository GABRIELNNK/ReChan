#include "gen/common.h"
#include "gen/animmat.h"

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
