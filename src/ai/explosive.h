#pragma once
#include "ai/obstacle.h"

class Explosive : public Obstacle {
public:
    // PSX +116 (s32): explosion state
    s32 state = 0;
    // PSX +120 (s32): initial countdown value
    s32 field120 = 0;
    // PSX +124 (s32): countdown divisor / growth limit
    s32 field124 = 0;
    // PSX +128..+139: state payload values
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    // PSX +140: saved collision box (tagCollisionBox, 16 bytes)
    tagCollisionBox savedCollBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    // PSX +156..+167: runtime countdown/effect state
    s32 field156 = 0;
    s32 field160 = 0;
    s32 field164 = 0;

    Explosive(const LVector* pos, u16 type);
    ~Explosive() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Draw() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) override;
    bool CareAboutAttack() const override;
    void TriggerByName(Thing* source, const char* name, const char* param) override;
    void ExplosiveTrigger(s32 damage, const char* name) override;

    virtual void CheckObstacleCollisions();
    virtual void AdjustCollisionBox();
    virtual void ExplodeThing(Thing* target);
    virtual void MovePassengers();
    virtual void HandleObstacleCollision(Obstacle* other);
};
