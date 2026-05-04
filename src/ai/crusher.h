#pragma once
#include "ai/obstacle.h"

class Crusher : public Obstacle {
public:
    // PSX +116 (s32): state/phase (init 0 in ctor)
    s32 field116 = 0;
    // PSX +120 (s32): unknown (init 0 in ctor)
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
    // PSX +160 (s32): unknown (init 0 in ctor)
    s32 field160 = 0;
    // PSX +164 (s32): unknown
    s32 field164 = 0;
    // PSX +168 (s32): direction flag (init 1 in ctor moving down)
    s32 field168 = 1;
    // PSX +172 (s32): unknown (init 0 in ctor)
    s32 field172 = 0;

    Crusher(const LVector* pos, u16 type);
    ~Crusher() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Move() override;
    void UpdatePosition() override;
    void Draw() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
