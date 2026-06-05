#pragma once

#include "snd/basesnd.h"

class CGenericPersistentSound;

class CKickNRollSound : public CSound {
public:
    u16 pad16 = 0;
    u16 kickSound = 0;
    u8 rollPersistentId = 0;
    u8 pad21 = 0;
    u16 pad22 = 0;
    CGenericPersistentSound* rollPersistent = nullptr;
    u16 kickCooldown = 0;
    u16 pad30 = 0;

    CKickNRollSound();
    ~CKickNRollSound() override;

    s32 BeginRoll();
    s32 EndRoll();
    s32 Kick();
    s32 HitHumanoid();
    s32 Load(const void* data);
    s32 Initialize(const LVector* pos);
    s32 Think();
};
