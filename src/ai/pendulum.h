#pragma once
#include "ai/obstacle.h"

class CPendulumSound;

class Pendulum : public Obstacle {
public:
    // PSX +116/+120: 16.16 angular velocities for Z/X swing axes.
    s32 swingVelocityZ = 0;
    s32 swingVelocityX = 0;
    // PSX +124: spring/gravity scale, attr 6.
    s32 swingScale = 0;
    // PSX +128: pendulum length from pivot to mesh bottom, 16.16.
    s32 pendulumLength = 0;
    // PSX +132/+134/+136: collision half extents around the heavy end.
    s16 bulbHalfX = 0;
    s16 bulbHalfY = 0;
    s16 bulbHalfZ = 0;
    s16 field138 = 0;
    // PSX +140/+142: default box X/Z half extents.
    s16 armHalfX = 0;
    s16 armHalfZ = 0;
    // PSX +144/+148/+152: collision damage tags/flags, attrs 10-12.
    s32 impactRegionMode = 0;
    s32 damageAmount = 0;
    s32 strongHitMode = 0;
    // PSX +156: impulse scale, attr 13.
    s32 impulseScale = 0;
    // PSX +160: nonzero if attrs 16/17/18 are present.
    s32 hasImpulseChain = 0;
    // PSX +164/+168/+172: chained impulse scales, attrs 16-18.
    s32 impulseChainA = 0;
    s32 impulseChainB = 0;
    s32 impulseChainC = 0;
    // PSX +176: CPendulumSound*.
    CPendulumSound* pendulumSound = nullptr;

    Pendulum(const LVector* pos, u16 type);
    ~Pendulum() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void Draw() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
