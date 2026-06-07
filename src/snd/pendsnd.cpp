#include "snd/pendsnd.h"

#include "p3d/lvector.h"

#include <string.h>

// PSX: Initialize__14CPendulumSoundPC10tagLVector (PNDLMSND.CPP:12, 0x800ACB84)
s32 CPendulumSound::Initialize(const LVector* pos) {
    MARKFUNCTION(0x800ACB84);
    return CSound::Initialize(const_cast<LVector*>(pos));
}

// PSX: Swing__14CPendulumSound (PNDLMSND.CPP:18, 0x800ACBA4)
s32 CPendulumSound::Swing() {
    MARKFUNCTION(0x800ACBA4);
    return PlayTransient(swingSfx, 8, 0);
}

// PSX: HitHumanoid__14CPendulumSound (PNDLMSND.CPP:23, 0x800ACBCC)
s32 CPendulumSound::HitHumanoid() {
    MARKFUNCTION(0x800ACBCC);
    return PlayTransient(hitHumanoidSfx, 8, 0);
}

// PSX: __14CPendulumSound (PNDLMSND.CPP:28, 0x800ACBF4)
CPendulumSound::CPendulumSound() {
    MARKFUNCTION(0x800ACBF4);
}

// PSX: _._14CPendulumSound (PNDLMSND.CPP:31, 0x800ACC28)
CPendulumSound::~CPendulumSound() {
    MARKFUNCTION(0x800ACC28);
}

// PSX: Load__14CPendulumSoundPCc (PNDLMSND.CPP:36, 0x800ACC7C)
s32 CPendulumSound::Load(const void* data) {
    MARKFUNCTION(0x800ACC7C);
    memcpy(&pad16, data, 6);
    return 0;
}
