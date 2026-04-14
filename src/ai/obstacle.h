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

class TrapDoor : public Obstacle {
public:
    // PSX +0x74..+0xBC (TrapDoor local state)
    s32 field74 = 0;
    s32 field78 = 0;
    LVector field7C = {};
    LVector field88 = {};
    tagCollisionBox field94 = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    s32 fieldA4 = 1;
    s32 fieldA8 = 0;
    s32 fieldAC = 0;
    s32 fieldB0 = 0;
    s32 fieldB4 = 0;
    s32 fieldB8 = 0;
    s32 fieldBC = 0;

    TrapDoor(const LVector* pos, u16 type);
    ~TrapDoor() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void TriggerByName(Thing* source, const char* name, const char* param) override;
    void Think() override;
    void UpdatePosition() override;
    void Draw() override;
    void Move() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    s32 GetFloorMaterial() const override;

    void SetupCollisionBox();
};

class Door : public Obstacle {
public:
    // PSX +116: death countdown (decremented when kill target dies)
    s32 deathCountdown = 3;
    // PSX +120: base Y rotation (from orientation.y in AnalyzeMesh)
    s32 baseRotY = 0;
    // PSX +124: current open amount (animated 0 to maxOpenDist)
    s32 currentOpen = 0;
    // PSX +128: rotation speed per frame (from DB attrib 6, default ~9 deg)
    s32 openSpeed = 1638;
    // PSX +132: max rotation when fully open (from DB attrib 9, default ~90 deg)
    s32 maxOpenDist = 16384;
    // PSX +136,+140,+144: draw rotation (used by Draw, animated by Move)
    LVector drawRot = {};
    // PSX +148: direction flag (0=subtract, nonzero=add currentOpen to baseRotY)
    s32 direction = 0;
    // PSX +152: door state (0=guarded, 1=cutscene, 2=opening, 3=open, 4=closing, 5=hub-closed)
    s32 doorState = 0;
    // PSX +156: cutscene triggered flag (1 = cutscene in progress)
    s32 cutsceneTriggered = 0;
    // PSX +160: saved closed collision box (16 bytes)
    tagCollisionBox closedBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    // PSX +176: secondary model hash (from DB attrib 11)
    s32 secondaryModelHash = 0;
    // PSX +180: tertiary model hash (from DB attrib 12)
    s32 tertiaryModelHash = 0;
    // PSX +184: target CRC for chain trigger (from DB attrib 10)
    u32 targetCRC = 0;
    // PSX +188: kill things CRC for DeathCheck (from DB attrib 8)
    s32 killThingsCRC = 0;
    // PSX +192: cached kill target pointer (ActiveZone*)
    Thing* killTarget = nullptr;

    Door(const LVector* pos, u16 type);
    ~Door() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void Draw() override;
    void Trigger() override;
    void Move() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;

    void Open();
    void TeleportPlayer();
    void DeathCheck();
};

class Ladder : public Obstacle {
public:
    s32 ladderFaceAngle = 0;
    s32 hatchEnabled = 1;
    s32 deathCountdown = 3;
    s32 state = 0;
    s32 cutscenePending = 0;
    s32 hatchTriggerCRC = 0;
    s32 hatchCloseCRC = 0;
    s32 teleportTargetCRC = 0;
    s32 deathCheckCRC = 0;
    Thing* hatchThing = nullptr;
    Thing* deathThing = nullptr;
    s32 hatchYTrigger = 0x7FFFFFFF;

    Ladder(const LVector* pos, u16 type);
    ~Ladder() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Move() override;
    void UpdatePosition() override;
    void Draw() override;
    void Trigger() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;

    void TeleportPlayer();
    void CloseHatch();
    s32 DeathCheck();
    void PutHumanoidOnLadder(Humanoid* hum);
};

class Teleporter : public Obstacle {
public:
    LVector targetPos = {};
    s32 targetAngle = 0;
    s32 killThings = 0;

    Teleporter(const LVector* pos, u16 type);
    ~Teleporter() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};

// Global zero delta velocity (PSX: 0x800CE818)
extern const LVector ZERO_DELTA_VELOCITY;
