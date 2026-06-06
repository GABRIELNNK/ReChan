#pragma once
#include "ai/obstacle.h"

class CWorldEffectSound;
class ComEffect;

class Blast : public Obstacle {
public:
    // Attribute 22: non-zero waits for an external trigger instead of auto-firing.
    s32 field116 = 0;
    // Attribute 10: expansion/opening duration.
    s32 field120 = 0;
    // Attribute 9: startup delay before auto-fire is allowed.
    s32 field124 = 0;
    // Runtime timer for delay/open/active/close phases.
    s32 field128 = 0;
    // Runtime phase: 0 idle, 1 expanding, 2 active, 3 retracting.
    s32 field132 = 0;
    // PSX +136/+140/+144: minimum blast force vector.
    s32 field136 = 0;
    s32 field140 = 0;
    s32 field144 = 0;
    // PSX +148/+152/+156: maximum blast force vector.
    s32 field148 = 0;
    s32 field152 = 0;
    s32 field156 = 0;
    // Attribute 14: collision half-width around the blast ray.
    s32 field160 = 0;
    // Length advanced per expansion frame.
    s32 field164 = 0;
    // Attribute 21: blast/fire type, also selects damage amount.
    s32 field168 = 0;
    // Endpoint derived from position plus the direction vector.
    s32 field172 = 0;
    s32 field176 = 0;
    s32 field180 = 0;
    // Dominant direction axis: 0 X, 1 Y, 2 Z.
    s32 field184 = 0;
    // Attribute 20: FW effect hash.
    s32 field188 = 0;
    s32 field192 = 0;
    // PSX +196: linked ComEffect used by framed blasts.
    ComEffect* field196 = nullptr;
    // Current visual frame.
    s32 field200 = 0;
    s32 field204 = 0;
    s32 field208 = 0;
    s32 field212 = 0;
    // Damage value chosen from the blast/fire type.
    s32 field216 = 0;
    s32 field220 = 0;
    s32 field224 = 0;
    // PSX +228: CWorldEffectSound* (ambient blast sound)
    CWorldEffectSound* field228 = nullptr;

    // Temp unpacked fields for PSX halfword values/decoded frame table.
    s32 field122 = 0;
    s32 field126 = 0;
    s32 field130 = 0;
    s32 blastDirX = 0;
    s32 blastDirY = 0;
    s32 blastDirZ = 0;
    s16 frameTable[8] = {};
    bool field192Enabled = false;

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
