#pragma once
#include "ai/obstacle.h"

class HorizontalPole : public Obstacle {
public:
    // PSX +116..+124: first line point
    LVector pointA = {};

    // PSX +128..+136: second line point
    LVector pointB = {};

    // PSX +140,+144,+148,+152: line projection parameters
    s32 field140 = 0;
    s32 field144 = 0;
    s32 field148 = 0;
    s32 field152 = 0;

    HorizontalPole(const LVector* pos, u16 type);
    ~HorizontalPole() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
