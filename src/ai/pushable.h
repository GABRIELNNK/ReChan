#pragma once
#include "ai/obstacle.h"

class Pushable : public Obstacle {
public:
    // PSX +116..+171: unknown fields (14 dwords)
    s32 field116 = 0;
    s32 field120 = 0;
    s32 field124 = 0;
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    s32 field140 = 0;
    s32 field144 = 0;
    s32 field148 = 0;
    s32 field152 = 0;
    s32 field156 = 0;
    s32 field160 = 0;
    s32 field164 = 0;
    s32* field168 = nullptr;

    Pushable(const LVector* pos, u16 type);
    ~Pushable() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Move() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    bool CareAboutAttack() const override;
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) override;
    s32 GetFloorMaterial() const override;

    virtual void MovePassengers();
    virtual bool HandleEnvironmentCollision(const LVector& prevPos);
};
