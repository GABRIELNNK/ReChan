#pragma once
#include "ai/obstacle.h"

class Trails;

class SlipperyFloor : public Obstacle {
public:
    // PSX +116 (ptr): first trail particle system pointer
    Trails* trailA = nullptr;
    // PSX +120 (ptr): second trail particle system pointer
    Trails* trailB = nullptr;

    SlipperyFloor(const LVector* pos, u16 type);
    ~SlipperyFloor() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;

    void DoTrailEffect(Humanoid* hum);
};
