#pragma once
#include "ai/obstacle.h"

class KickNRoll : public Obstacle {
public:
    // PSX +116 (s32): unknown
    s32 field116 = 0;
    // PSX +120 (s32): unknown
    s32 field120 = 0;
    // PSX +124 (s32): velocity X
    s32 velX = 0;
    // PSX +128 (s32): velocity Y (decremented by 9 per frame for gravity)
    s32 velY = 0;
    // PSX +132 (s32): velocity Z
    s32 velZ = 0;
    // PSX +136 (s32): unknown
    s32 field136 = 0;
    // PSX +140 (s32): unknown
    s32 field140 = 0;
    // PSX +144 (s32): unknown
    s32 field144 = 0;
    // PSX +148 (u16): roll timer (set to 8 on wall collision)
    u16 rollTimer = 0;
    // PSX +150 (u16): unknown
    u16 field150 = 0;
    // PSX +152 (ptr): CKickNRollSound instance (set to null in ctor)
    void* sound = nullptr;
    // PSX +156 (s32): breakable flag (if set, Destroy on collision)
    s32 breakable = 0;
    // PSX +160 (s32): unknown
    s32 field160 = 0;
    // PSX +164 (u32): effect hash for destruction
    u32 effectHash = 0;
    // PSX +168 (u32): effect param
    u32 effectParam = 0;

    KickNRoll(const LVector* pos, u16 type);
    ~KickNRoll() override;

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
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) override;

    virtual void Destroy();
    virtual void MovePassengers();
    virtual void HandleEnvironmentCollision(const LVector& normal);
};

class KnockDown : public Obstacle {
public:
    // PSX +116 (s32): unknown (init 0 in ctor)
    s32 field116 = 0;
    // PSX +120..+151: unknown (8 dwords)
    s32 field120 = 0;
    s32 field124 = 0;
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    s32 field140 = 0;
    s32 field144 = 0;
    s32 field148 = 0;
    // PSX +152..+167: saved collision box (INVALID initially)
    tagCollisionBox savedCollBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    // PSX +168 (s32): unknown (init 0 in ctor)
    s32 field168 = 0;
    // PSX +172 (s32): unknown (init 0 in ctor)
    s32 field172 = 0;
    // PSX +176..+183: unknown (2 dwords)
    s32 field176 = 0;
    s32 field180 = 0;
    // PSX +184 (s32): unknown (init 0 in ctor)
    s32 field184 = 0;
    // PSX +188..+191: unknown
    s32 field188 = 0;

    KnockDown(const LVector* pos, u16 type);
    ~KnockDown() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void Draw() override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Move() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) override;

    virtual void UpdateCollisionBox();
};

class Stack : public Obstacle {
public:
    // PSX +116..+123: unknown (2 dwords)
    s32 field116 = 0;
    s32 field120 = 0;
    // PSX +124 (s32): unknown (init 0 in ctor, a1[31])
    s32 field124 = 0;
    // PSX +128 (s32): unknown (init 0 in ctor, a1[32])
    s32 field128 = 0;
    // PSX +132 (s32): unknown (init 0 in ctor, a1[33])
    s32 field132 = 0;
    // PSX +136 (s32): unknown (init 0 in ctor, a1[34])
    s32 field136 = 0;
    // PSX +140 (s32): unknown (init 0 in ctor, a1[35])
    s32 field140 = 0;
    // PSX +144..+155: unknown (3 dwords)
    s32 field144 = 0;
    s32 field148 = 0;
    s32 field152 = 0;
    // PSX +156 (s32): unknown (init 0 in ctor, a1[39])
    s32 field156 = 0;
    // PSX +160..+235: unknown (19 dwords = 76 bytes)
    s32 field160 = 0;
    s32 field164 = 0;
    s32 field168 = 0;
    s32 field172 = 0;
    s32 field176 = 0;
    s32 field180 = 0;
    s32 field184 = 0;
    s32 field188 = 0;
    s32 field192 = 0;
    s32 field196 = 0;
    s32 field200 = 0;
    s32 field204 = 0;
    s32 field208 = 0;
    s32 field212 = 0;
    s32 field216 = 0;
    s32 field220 = 0;
    s32 field224 = 0;
    s32 field228 = 0;
    s32 field232 = 0;

    Stack(const LVector* pos, u16 type);
    ~Stack() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void Draw() override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) override;

    virtual void Wobble();
    virtual void Fall();
    virtual void FinishStack();
    virtual void UpdateCollisionBox();
    virtual void TriggerStackAnimation();
    virtual void SetupCallbacks();
    virtual void SetupJointPosition(s32 index, LVector pos);

    static s32 LoadDialog(u32 a, u32 b, u32 c);
};
