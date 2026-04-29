#pragma once

#include "snd/basesnd.h"
#include "p3d/lvector.h"

class CWeaponSound : public CSound {
public:
    u16 field16 = 0;
    u16 hitHumanoidSfx = 0;
    u16 explodePrimarySfx = 0;
    u16 grabSfx = 0;
    u16 explodeSecondarySfx = 0;
    u16 missSfx = 0;

    CWeaponSound();
    ~CWeaponSound() override;

    s32 Initialize(const LVector* pos);
    s32 HitHumanoid();
    s32 Grab();
    s32 Explode();
    s32 Miss();
    s32 Load(const void* data);
};