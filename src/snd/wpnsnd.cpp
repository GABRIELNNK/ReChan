#include "snd/wpnsnd.h"

#include <cstring>

CWeaponSound::CWeaponSound() {
    MARKFUNCTION(0x800AD150);
}

CWeaponSound::~CWeaponSound() {
    MARKFUNCTION(0x800AD184);
}

s32 CWeaponSound::Initialize(const LVector* pos) {
    MARKFUNCTION(0x800AD074);
    return CSound::Initialize(const_cast<LVector*>(pos));
}

s32 CWeaponSound::HitHumanoid() {
    MARKFUNCTION(0x800AD094);
    return PlayTransient(hitHumanoidSfx, 8, 0);
}

s32 CWeaponSound::Grab() {
    MARKFUNCTION(0x800AD0BC);
    return PlayTransient(grabSfx, 8, 0);
}

s32 CWeaponSound::Explode() {
    MARKFUNCTION(0x800AD0E4);
    PlayTransient(explodePrimarySfx, 8, 0);
    return PlayTransient(explodeSecondarySfx, 8, 0);
}

s32 CWeaponSound::Miss() {
    MARKFUNCTION(0x800AD128);
    return PlayTransient(missSfx, 8, 0);
}

s32 CWeaponSound::Load(const void* data) {
    MARKFUNCTION(0x800AD1D8);
    if (!data) {
        return -1;
    }

    std::memcpy(&field16, data, sizeof(u32) * 3);
    return 0;
}