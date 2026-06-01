#include "snd/pushsnd.h"
#include "snd/basesnd.h"
#include "gen/common.h"
#include <cstring>

CPushableSound::CPushableSound() : CSound() {
    MARKFUNCTION(0x800AC77C);
}

CPushableSound::~CPushableSound() {
    MARKFUNCTION(0x800AC7C8);
    EndPush();
}

s32 CPushableSound::Load(const void* data) {
    MARKFUNCTION(0x800AC824);
    std::memcpy(&pad16, data, sizeof(u32) * 3);
    return 0;
}

s32 CPushableSound::Initialize(const LVector* pos) {
    MARKFUNCTION(0x800AC844);
    return CSound::Initialize((void*)pos);
}

void CPushableSound::Think() {
    MARKFUNCTION(0x800AC864);
    if (kickCooldown != 0) {
        kickCooldown--;
    }
}

void CPushableSound::BeginPush() {
    MARKFUNCTION(0x800AC6D0);
    BeginPersistent(loopSoundId, &pushPersistent);
}

void CPushableSound::EndPush() {
    MARKFUNCTION(0x800AC6F4);
    EndPersistent(&pushPersistent);
}

void CPushableSound::Kick() {
    MARKFUNCTION(0x800AC714);
    if (kickCooldown != 0) {
        return;
    }
    PlayTransient(sfxID, 8, 0);
    kickCooldown = 5;
}

void CPushableSound::HitHumanoid() {
    MARKFUNCTION(0x800AC75C);
    Kick();
}

void CPushableSound::GetMaterial(s32* outMaterial) const {
    MARKFUNCTION(0x800AC880);
    *outMaterial = (s32)material;
}
