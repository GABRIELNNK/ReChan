#pragma once

#include "snd/basesnd.h"
#include "snd/hmndsnd.h"

// CDestructibleSound (28 bytes on PSX)
// PSX source: C:\CHAN\GAME\SRC\SND\DSTRSND.CPP
class CDestructibleSound : public CSound {
public:
    u16 field16 = 0;
    u16 smashSfx = 0xFFFF;
    s32 material = 0;
    u16 smashCooldown = 0;
    u16 pad26 = 0;

    CDestructibleSound();
    ~CDestructibleSound() override;

    s32 Initialize(const void* pos);
    s32 Smash();
    s32 Think();
    s32 Load(const void* data);
    s32 GetMaterial(CSoundMaterial* outMaterial);
};
