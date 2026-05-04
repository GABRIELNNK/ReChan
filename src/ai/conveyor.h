#pragma once
#include "ai/obstacle.h"

class Conveyor : public Obstacle {
public:
    s32 field116 = 0;
    s32 field120 = 0;
    s32 beltSpeed = 10;
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    s32 field140 = 0;
    s32 field144 = 0;

    Conveyor(const LVector* pos, u16 type);
    ~Conveyor() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
