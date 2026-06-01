#pragma once
#include "ai/obstacle.h"

class Humanoid;

class DynamicObstacle : public Obstacle {
public:
    // PSX +116 (s32): linear velocity X
    s32 linVelX = 0;
    // PSX +120 (s32): linear velocity Y
    s32 linVelY = 0;
    // PSX +124 (s32): linear velocity Z
    s32 linVelZ = 0;
    // PSX +128 (s32): angular velocity X
    s32 angVelX = 0;
    // PSX +132 (s32): angular velocity Y
    s32 angVelY = 0;
    // PSX +136 (s32): angular velocity Z
    s32 angVelZ = 0;
    // PSX +140 (s32): accumulated force X
    s32 forceX = 0;
    // PSX +144 (s32): accumulated force Y
    s32 forceY = 0;
    // PSX +148 (s32): accumulated force Z
    s32 forceZ = 0;
    // PSX +152 (s32): accumulated moment X
    s32 momentX = 0;
    // PSX +156 (s32): accumulated moment Y
    s32 momentY = 0;
    // PSX +160 (s32): accumulated moment Z
    s32 momentZ = 0;
    // PSX +164 (s32): unknown
    s32 field164 = 0;
    // PSX +168 (s32): unknown
    s32 field168 = 0;
    // PSX +172 (ptr): last humanoid that interacted
    Humanoid* lastHumanoid = nullptr;
    // PSX +176 (s32): unknown
    s32 field176 = 0;
    // PSX +180 (u32): effect hash (from attrib 20, or default 106729104)
    u32 effectHash = 0;
    // PSX +184 (u32): effect param (0x80000000 or 0 depending on attrib 21)
    u32 effectParam = 0;
    // PSX +188 (s32): kick flag (set=1 when humanoid kicks; triggers Destroy on impact)
    s32 kickFlag = 0;

    DynamicObstacle(const LVector* pos, u16 type);
    ~DynamicObstacle() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Move() override;
    void Draw() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    bool CareAboutAttack() const override;
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) override;

    virtual void HandleObjectInterAction(Humanoid* hum);
    virtual void Destroy();
    virtual void HandleEnvironmentCollision(const LVector& prevPos);
    virtual void MovePassengers();

    void AddForce(s32 damage, const LVector* matrix);
    void AddMomentVector(const LVector& matrix, const LVector& contactPos);
    void Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos);
};

class Table : public DynamicObstacle {
public:
    Table(const LVector* pos, u16 type);
    ~Table() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos);
    void HandleHumanoidCollision(Humanoid* hum) override;
};

class Chair : public DynamicObstacle {
public:
    Chair(const LVector* pos, u16 type);
    ~Chair() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos);
    void HandleHumanoidCollision(Humanoid* hum) override;
};
