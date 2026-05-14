#pragma once
#include "snd/basesnd.h"

struct CKnockDownPublishedData {
    u8 pad0 = 0;
    u8 pad1 = 0;
    u16 kickSound = 0;
    u16 impactSound = 0;
    u8 fallPersistentId = 0;
    u8 pad7 = 0;
};

class CKnockDownSound : public CSound {
public:
    CKnockDownPublishedData pub = {};
    CGenericPersistentSound* persistentFallSound = nullptr;
    u16 multipleKickTimer = 0;
    u16 multipleImpactTimer = 0;

    CKnockDownSound();
    ~CKnockDownSound() override;

    s32 Initialize(const LVector* pos);
    s32 BeginFall();
    s32 EndFall();
    s32 Kick();
    s32 Impact();
    s32 HitHumanoid();
    s32 Think();
    s32 Load(const void* data);
};
