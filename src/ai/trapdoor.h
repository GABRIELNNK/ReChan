#pragma once
#include "ai/obstacle.h"

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
