#pragma once
#include "ai/obstacle.h"

class TriggerThing : public Obstacle {
public:
    u32* targetHashes = nullptr;
    Obstacle** cachedTargets = nullptr;
    s32 targetCount = 0;
    char* triggerName = nullptr;

    TriggerThing(const LVector* pos, u16 type);
    ~TriggerThing() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;

    bool HandleCollision(Thing* source);
};