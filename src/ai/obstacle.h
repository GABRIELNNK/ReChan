#pragma once
#include "ai/thing.h"
#include "gen/colvol.h"

class Humanoid;
class Obstacle;
struct tSphere;

// Obstacle (116 bytes on PSX) - base class for interactive objects
// Inherits Thing (96 bytes)
// PSX vtable at 0x800CE7A0
class Obstacle : public Thing {
public:
    // PSX +96: collision bounding box (16 bytes: 6 s16s + extent + pad)
    tagCollisionBox collBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };

    // PSX +112: physical type byte
    u8 physicalType = 0;

    // PSX +113: lighting flag (1 = use hardware lights)
    u8 lightingFlag = 1;

    // PSX +114: shadow flag (1 = has shadow)
    u8 shadowFlag = 1;

    Obstacle(const LVector* pos, u16 type);
    ~Obstacle() override;

    void Think() override;
    void Draw() override;
    void Reset() override;
    void Move() override;
    void UpdatePosition() override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void AnalyzeMesh(DBRoot* root) override;

    virtual void FillSphere(tSphere& sphere) const;
    virtual void HandlePickupCollision(Thing* pickup);
    virtual void HandleHumanoidCollision(Humanoid* hum);
    virtual void Trigger();
    virtual void TriggerByName(Thing* source, const char* name, const char* param);
    virtual void ExplosiveTrigger(s32 damage, const char* name);
    virtual const LVector* GetDeltaVelocity() const;
    virtual bool CareAboutAttack() const;
    virtual void HandleAttack(Humanoid* attacker, s32 damageType, s32 damage);
    virtual s32 GetFloorMaterial() const;
    virtual s32 GetObstacleFloorHeight(const LVector& pos) const;

    s32 GetPhysical() const;
    void SetCollisionBox(const tagCollisionBox& box);

    static void HandleHumanoidObstacleCollision(Humanoid* hum);
};

// Global zero delta velocity (PSX: 0x800CE818)
extern const LVector ZERO_DELTA_VELOCITY;

// Obstacle animation table (PSX: g_animTable at 0x800E0E58, g_animCount at GP+0xF38)
// Populated by Obstacle::Load during petal loading, cleared by ClearPetalAnimList.
// Entries are MiscAnimNode* from AnimationManager. Access TransformAnim* via ->anim.
struct MiscAnimNode;
MiscAnimNode* Obstacle_GetAnimation(s32 animIndex);
void Obstacle_ClearPetalAnimList();
void Obstacle_AddAnimation(MiscAnimNode* node);

// PSX: Load__8ObstacleR10tReadChunkPPv (0x8007CAA4)
// Parses chunk 0x8A20 body and appends matching misc anim to obstacle table.
void Obstacle_LoadAnimChunk(const u8* body, u32 bodySize);
