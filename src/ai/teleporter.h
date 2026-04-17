#pragma once
#include "ai/obstacle.h"

class Teleporter : public Obstacle {
public:
    LVector targetPos = {};
    s32 targetAngle = 0;
    s32 killThings = 0;

    Teleporter(const LVector* pos, u16 type);
    ~Teleporter() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
