#pragma once
#include "ai/obstacle.h"

class Blast : public Obstacle {
public:
    s32 field116 = 0;
    s32 field120 = 0;
    s32 field124 = 0;
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    s32 field140 = 0;
    s32 field144 = 0;
    s32 field148 = 0;
    s32 field152 = 0;
    s32 field156 = 0;
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

    Blast(const LVector* pos, u16 type);
    ~Blast() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Activate() override;
    void Deactivate() override;
    void Trigger() override;
    void Draw() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;

    virtual void CreateSound();
    virtual void UpdateSound();
    virtual void ReleaseSound();
};
