#pragma once
#include "ai/obstacle.h"

class Pendulum : public Obstacle {
public:
    // PSX +116 (s32): unknown (init 0 in ctor, a1[29])
    s32 field116 = 0;
    // PSX +120 (s32): unknown (init 0 in ctor, a1[30])
    s32 field120 = 0;
    // PSX +124..+159: unknown (9 dwords)
    s32 field124 = 0;
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    s32 field140 = 0;
    s32 field144 = 0;
    s32 field148 = 0;
    s32 field152 = 0;
    s32 field156 = 0;
    // PSX +160 (s32): unknown (init 0 in ctor, a1[40])
    s32 field160 = 0;
    // PSX +164..+175: unknown (3 dwords)
    s32 field164 = 0;
    s32 field168 = 0;
    s32 field172 = 0;
    // PSX +176 (s32): unknown (init 0 in ctor, a1[44])
    s32 field176 = 0;

    Pendulum(const LVector* pos, u16 type);
    ~Pendulum() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void Draw() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
