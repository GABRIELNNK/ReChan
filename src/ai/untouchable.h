#pragma once
#include "ai/obstacle.h"

class Untouchable : public Obstacle {
public:
    // PSX +116 (ptr): ParticleSystemMgr* (allocated in ctor, released in dtor, a1[29])
    void* particleMgr = nullptr;
    // PSX +120..+131: sound-related (3 dwords)
    s32 field120 = 0;
    s32 field124 = 0;
    s32 field128 = 0;
    // PSX +132 (s32): last hit counter (init 0)
    s32 lastHitCounter = 0;
    // PSX +136 (s32): damage type (default 2, from attrib 6)
    s32 damageType = 2;
    // PSX +140 (s32): damage value (default 3, from attrib 7)
    s32 damageValue = 3;
    // PSX +144 (s32): countdown timer (init = damageValue in Reset)
    s32 countdownTimer = 0;
    // PSX +148 (s32): unknown (init 0 in ctor, a1[38] is +152 but)
    s32 field148 = 0;
    // PSX +152 (ptr): sound pointer (init 0 in ctor, a1[38])
    void* soundPtr = nullptr;

    Untouchable(const LVector* pos, u16 type);
    ~Untouchable() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
};
