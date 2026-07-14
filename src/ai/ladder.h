#pragma once
#include "ai/obstacle.h"

class ActiveZone;

class Ladder : public Obstacle {
public:
    s32 deathCountdown = 3;
    s32 ladderFaceAngle = 0;
    s32 hatchEnabled = 1;
    s32 field80 = 0;
    s32 field84 = 0;
    s32 field88 = 0;
    s32 field8C = 0;
    s32 field90 = 0;
    s32 field94 = 0;
    s32 state = 0;
    s32 cutscenePending = 0;
    s32 hatchYTrigger = 0x7FFFFFFF;
    s32 hatchTriggerCRC = 0;
    Thing* hatchThing = nullptr;
    s32 teleportTargetCRC = 0;
    s32 hatchCloseCRC = 0;
    s32 deathCheckCRC = 0;
    ActiveZone* deathThing = nullptr;

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
    bool CheckForLedges(LVector& outNormal, LVector& outCorrectionPos);
};
