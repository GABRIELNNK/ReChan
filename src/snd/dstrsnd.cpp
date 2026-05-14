#include "snd/dstrsnd.h"

#include <cstring>

CDestructibleSound::CDestructibleSound() {
    MARKFUNCTION(0x800AC610);
    smashCooldown = 0;
    smashSfx = 0xFFFF;
}

CDestructibleSound::~CDestructibleSound() {
    MARKFUNCTION(0x800AC650);
}

s32 CDestructibleSound::Initialize(const void* pos) {
    MARKFUNCTION(0x800AC584);
    return CSound::Initialize(const_cast<void*>(pos));
}

s32 CDestructibleSound::Smash() {
    MARKFUNCTION(0x800AC5A4);
    if (smashCooldown != 0) {
        return 0;
    }

    PlayTransient(smashSfx, 8, 0);
    smashCooldown = 30;
    return 0;
}

s32 CDestructibleSound::Think() {
    MARKFUNCTION(0x800AC5F4);
    if (smashCooldown != 0) {
        smashCooldown = static_cast<u16>(smashCooldown - 1);
    }
    return 0;
}

s32 CDestructibleSound::Load(const void* data) {
    MARKFUNCTION(0x800AC6A4);
    std::memcpy(&field16, data, sizeof(u32) * 2);
    return 0;
}

s32 CDestructibleSound::GetMaterial(CSoundMaterial* outMaterial) {
    MARKFUNCTION(0x800AC6BC);
    *outMaterial = static_cast<CSoundMaterial>(material);
    return 0;
}
