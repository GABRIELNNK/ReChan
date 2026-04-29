#pragma once

#include "ai/thing.h"

class CWeaponSound;
struct DBRoot;

class Pickup : public DynamicThing {
public:
    Thing* attachedOwner = nullptr;          // +200
    Thing* ignoreCollisionOwner = nullptr;   // +204
    s32 attachJoint = 0;                     // +208
    u32 fightingSystemRoot = 0;              // +212
    s32 idleAnim = 0;                        // +216
    const s32* moveStruct = nullptr;         // +220
    u32 highPickupMove = 0;                  // +224
    u32 lowPickupMove = 0;                   // +228
    u32 currentPickupMove = 0;               // +232
    u32 throwMove = 0;                       // +236
    LVector collisionPoints[4] = {};         // +240
    s32 collisionPointCount = 0;             // +288
    LVector attachOffset = {};               // +292
    s32 damage = 20;                         // +304
    s32 field308 = 0;                        // +308
    s32 deactivateFlag = 0;                  // +312
    s32 weaponField = 0;                     // +316
    CWeaponSound* weaponSound = nullptr;     // +320
    u32 pickupFlags = 0;                     // +324
    u16 field328 = 0;                        // +328
    u16 pad330 = 0;
    u32 field332 = 0;                        // +332

    Pickup(const LVector* initialPos, u16 type);
    ~Pickup() override;

    void Reset() override;
    void CreateModel(const char* name) override;
    void AnalyzeMesh(DBRoot* root) override;
    void Think() override;
    void UpdatePosition() override;
    void Move() override;
    void HandleCollision(Thing* other, s32 damage, ...) override;

    void DamageExtra();
    s32 SetupPickup(Thing* owner, u32 joint);
    s32 Release(Thing* owner, ccList* list, const SVector* forceDir, s32 forceMag);
    s32 PlayEffect();
    bool PickupDeactivate() const;
    s32 GetCollisionYMin() const;
    CWeaponSound* GetWeaponSoundPtr();
    const CWeaponSound* GetWeaponSoundPtr() const;
    s32 SetPickupMove(s32 compareY);
    u16 GetPickupMove() const;
    s8 GetPickupMoveGrabFrame() const;
    u16 GetThrowMove() const;
    s8 GetThrowMoveThrowFrame() const;
};