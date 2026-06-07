#pragma once
#include "ai/obstacle.h"

class CWorldEffectSound;
class ComEffect;

class Blast : public Obstacle {
public:
    // PSX +116 (s32): nonzero means the blast only starts from Trigger().
    s32 requireTrigger = 0;
    // PSX +120 (s16): frames spent extending from origin to full length.
    s16 extendFrames = 0;
    // PSX +122 (s16): frames spent retracting, used by framed effects.
    s16 retractFrames = 0;
    // PSX +124 (s16): inactive wait before an automatic blast may start.
    s16 cooldownFrames = 0;
    // PSX +126 (s16): frames held at full length.
    s16 holdFrames = 0;
    // PSX +128 (s16): timer for the current state.
    s16 stateTimer = 0;
    // PSX +130 (s16): initial timer advance subtracted from cooldownFrames.
    s16 initialTimerAdvance = 0;
    // PSX +132 (s32): current blast state.
    s32 blastState = 0;
    // PSX +136..+156 (s32): normalized push force ranges.
    s32 minForceX = 0;
    s32 minForceY = 0;
    s32 minForceZ = 0;
    s32 maxForceX = 0;
    s32 maxForceY = 0;
    s32 maxForceZ = 0;
    // PSX +160 (s32): collision half-width perpendicular to the blast axis.
    s32 halfWidth = 0;
    // PSX +164 (s32): collision length added per extend/retract frame.
    s32 lengthPerFrame = 0;
    // PSX +168 (s32): DB damage preset, mapped to collisionDamage.
    s32 damagePreset = 0;
    // PSX +172..+180 (s32): endpoint used to derive direction.
    s32 endPosX = 0;
    s32 endPosY = 0;
    s32 endPosZ = 0;
    // PSX +184 (s32): dominant direction axis, 0 X, 1 Y, 2 Z.
    s32 majorAxis = 0;
    // PSX +188 (s32): hashed DB effect resource name.
    s32 effectHash = 0;
    // PSX +192 (s32): nonzero when using a loaded framed FWEffect.
    s32 framedEffect = 0;
    // PSX +196 (ptr): loaded effect instance used for framed rendering/sound.
    ComEffect* effect = nullptr;
    // PSX +200 (s32): current effect animation frame.
    s32 effectFrame = 0;
    // PSX +204 (s32): last rendered effect animation frame.
    s32 lastEffectFrame = 0;
    // PSX +208 (s32): render flags copied from the FWEffect.
    s32 effectRenderFlags = 0;
    // PSX +212 (ptr): optional 8-entry frame pair table from attrs 24..31.
    s16* effectFrameTable = nullptr;
    // PC render cache: FWEffect scale must stay separate from Thing::orientation.
    LVector effectScale = { 0x10000, 0x10000, 0x10000 };
    // PSX +216 (s32): damage value passed through humanoid collision tags.
    s32 collisionDamage = 0;
    // PSX +220 (s32): frames between repeated humanoid damage hits.
    s32 hitCooldownFrames = 0;
    // PSX +224 (s32): repeated-hit countdown timer.
    s32 hitCooldownTimer = 0;
    // PSX +228 (ptr): persistent world effect sound.
    CWorldEffectSound* sound = nullptr;

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
