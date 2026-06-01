#pragma once
#include "ai/obstacle.h"

class Explosive : public Obstacle {
public:
    // PSX +116 (s32): explosion trigger state (>= 2 triggers explosion)
    s32 state = 0;
    // PSX +120 (s32): unknown
    s32 field120 = 0;
    // PSX +124 (ptr): unknown field
    void* field124 = nullptr;
    // PSX +128..+139 (padding/unknown, 3 dwords)
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    // PSX +140: saved collision box (tagCollisionBox, 16 bytes)
    tagCollisionBox savedCollBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    // PSX +156..+167: additional unknown fields (3 dwords)
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

    virtual void CheckObstacleCollisions();
    virtual void ExplodeThing();
    virtual void MovePassengers();
    virtual void Trigger();
    virtual void HandleObstacleCollision(Obstacle* other);
};
