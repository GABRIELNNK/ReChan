#pragma once
#include "ai/obstacle.h"

class CDestructibleSound;

void HandleObstacleDestructibleThingCollision(Obstacle* obstacle);

class DestructibleThing : public Obstacle {
public:
    u32 effectHash = 0;
    u32 effectParam = 0;
    s32 field124 = 0;
    s32 destroyFlag = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    u16 generateItemType = 0;
    u16 field142 = 0;
    char* itemModelName = nullptr;
    s32 itemParam1 = 0;
    s32 itemParam2 = 0;
    s32 itemParam3 = 0;
    s32 itemParam4 = 0;
    CDestructibleSound* sound = nullptr;

    DestructibleThing(const LVector* pos, u16 type);
    ~DestructibleThing() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void Draw() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    bool CareAboutAttack() const override;
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) override;
    s32 GetFloorMaterial() const override;

    virtual void Destroy();
    virtual void GenerateItem();
    virtual void MovePassengers();
    virtual void HandleObstacleCollision(Obstacle* other);
};
