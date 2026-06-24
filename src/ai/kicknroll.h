#pragma once
#include "ai/obstacle.h"

class AnimStructure;
class CKickNRollSound;
class CKnockDownSound;
class CGenericPersistentSound;

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
    CKickNRollSound* sound = nullptr;
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
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) override;
    bool CareAboutAttack() const override;

    virtual void Destroy();
    virtual void MovePassengers();
    virtual bool HandleEnvironmentCollision(LVector& prevPos);
};

class KnockDown : public Obstacle {
public:
    // PSX +116: 0 upright, 1 falling, 2 finished.
    s32 field116 = 0;
    // PSX +120/+124: current and maximum angular velocity.
    s32 field120 = 0;
    s32 field124 = 0;
    // PSX +128..+136: initial orientation.
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    // PSX +140..+148: falling orientation.
    s32 field140 = 0;
    s32 field144 = 0;
    s32 field148 = 0;
    // PSX +152..+167: saved collision box (INVALID initially)
    tagCollisionBox savedCollBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    // PSX +168: optional trigger target hash.
    s32 field168 = 0;
    // PSX +172: impact effect hash.
    s32 field172 = 0;
    // PSX +176: damage; +180: remain alive after impact.
    s32 field176 = 0;
    s32 field180 = 0;
    // PSX +184 (ptr): CKnockDownSound* (init nullptr in ctor)
    CKnockDownSound* field184 = nullptr;
    // PSX +188: fall direction (0 +X, 1 -Z, 2 -X, 3 +Z rotation).
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
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) override;
    bool CareAboutAttack() const override;

    virtual void UpdateCollisionBox();
};

class Stack : public Obstacle {
public:
    // PSX +116 (s32): damage sent to humanoid on hit.
    s32 damage = 0;
    // PSX +120 (s32): obstacle animation table index.
    s32 modelIndex = 0;
    // PSX +124 (u32): effect hash used when stack finishes.
    u32 effectHash = 0;
    // PSX +128 (s32): dialog runtime handle.
    s32 dialogHandle = 0;
    // PSX +132 (s32): dialog id copied to player on success.
    s32 dialogID = 0;
    // PSX +136 (s32): state.
    s32 state = 0;
    // PSX +140 (ptr): active anim structure.
    AnimStructure* anim = nullptr;
    // PSX +144 (ptr): cached obstacle anim node.
    MiscAnimNode* animBasic = nullptr;
    // PSX +148 (s32): remaining wobble count.
    s32 timesToWobble = 0;
    // PSX +152 (s32): current frame counter.
    s32 currentFrame = 0;
#if HIGH_FPS_PLAY_PRESENTATION
    // Render-only: frame value at the start of the last logic tick, for Draw()-time interpolation.
    s32 renderPrevFrame = 0;
#endif
    // PSX +156 (ptr): knockdown sound object.
    CKnockDownSound* knockDownSound = nullptr;
    // PSX +160 (char[50]): generated pushable model name.
    char pushableName[50] = {};
    // Joint positions captured by stack joint callback.
    LVector jointPositions[2] = {};

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
    void HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) override;
    bool CareAboutAttack() const override;

    virtual void Wobble();
    virtual void Fall();
    virtual void FinishStack();
    virtual void UpdateCollisionBox();
    virtual void TriggerStackAnimation();
    virtual void SetupCallbacks();
    virtual void SetupJointPosition(s32 index, LVector pos);
    s32 LoadDialog(u32 dialogId, u32 priority);
};
