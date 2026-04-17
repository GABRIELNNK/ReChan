#pragma once
#include "ai/obstacle.h"

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
