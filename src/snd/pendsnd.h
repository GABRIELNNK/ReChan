#pragma once

#include "snd/basesnd.h"

struct LVector;

class CPendulumSound : public CSound {
public:
    u16 pad16 = 0;
    u16 swingSfx = 0;
    u16 hitHumanoidSfx = 0;
    u16 pad22 = 0;

    CPendulumSound();
    ~CPendulumSound() override;

    s32 Initialize(const LVector* pos);
    s32 Swing();
    s32 HitHumanoid();
    s32 Load(const void* data);
};
