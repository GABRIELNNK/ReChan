#include "ai/pendulum.h"

#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "gen/common.h"
#include "gen/control.h"
#include "gen/database.h"
#include "gen/psxmath_helpers.h"
#include "snd/pendsnd.h"
#include "snd/sndfact.h"

static s32 PendulumAttrToSpringScale(s32 value) {
    return rmDiv16i(value << 16, 0x000A0000);
}

static s32 PendulumAngularStep(s32 velocity) {
    const s32 high = static_cast<s16>(velocity >> 16);
    return (high << 16) / 360;
}

static s32 PendulumLengthSinComponent(s32 length, s32 angle) {
    return static_cast<s32>((static_cast<s64>(length) * rmSin16(angle)) >> 32);
}

Pendulum::Pendulum(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80024DF0);
    pendulumSound = nullptr;
    swingVelocityZ = 0;
    swingVelocityX = 0;
    hasImpulseChain = 0;
}

Pendulum::~Pendulum() {
    MARKFUNCTION(0x80024E38);
    delete pendulumSound;
    pendulumSound = nullptr;
}

void Pendulum::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80024EA0);

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    ObstacleFillCollisionBox(localBox, root, 5);
    pendulumLength = static_cast<s32>(localBox.minY) << 16;

    Obstacle::AnalyzeMesh(root);

    const DBAttrib* a6 = root->FindAttrib(6);
    swingScale = a6 ? PendulumAttrToSpringScale(static_cast<s32>(a6->value)) : 0x10000;

    const DBAttrib* a7 = root->FindAttrib(7);
    bulbHalfX = a7 ? static_cast<s16>(Div2TowardZero(static_cast<s32>(a7->value)))
                   : static_cast<s16>(Div2TowardZero(static_cast<s32>(localBox.maxX) - static_cast<s32>(localBox.minX)));

    const DBAttrib* a8 = root->FindAttrib(8);
    bulbHalfY = a8 ? static_cast<s16>(a8->value)
                   : static_cast<s16>(static_cast<s32>(localBox.maxY) - static_cast<s32>(localBox.minY));

    const DBAttrib* a9 = root->FindAttrib(9);
    bulbHalfZ = a9 ? static_cast<s16>(Div2TowardZero(static_cast<s32>(a9->value)))
                   : static_cast<s16>(Div2TowardZero(static_cast<s32>(localBox.maxZ) - static_cast<s32>(localBox.minZ)));

    armHalfX = static_cast<s16>(Div2TowardZero(static_cast<s32>(localBox.maxX) - static_cast<s32>(localBox.minX)));
    armHalfZ = static_cast<s16>(Div2TowardZero(static_cast<s32>(localBox.maxZ) - static_cast<s32>(localBox.minZ)));

    const DBAttrib* a10 = root->FindAttrib(10);
    impactRegionMode = a10 ? static_cast<s32>(a10->value) : 0;

    const DBAttrib* a11 = root->FindAttrib(11);
    damageAmount = a11 ? static_cast<s32>(a11->value) : 0;

    const DBAttrib* a12 = root->FindAttrib(12);
    strongHitMode = a12 ? static_cast<s32>(a12->value) : 0;

    const DBAttrib* a13 = root->FindAttrib(13);
    impulseScale = a13 ? static_cast<s32>(a13->value) : 10;

    const DBAttrib* a16 = root->FindAttrib(16);
    impulseChainA = a16 ? static_cast<s32>(a16->value) : 0;

    const DBAttrib* a17 = root->FindAttrib(17);
    impulseChainB = a17 ? static_cast<s32>(a17->value) : 0;

    const DBAttrib* a18 = root->FindAttrib(18);
    impulseChainC = a18 ? static_cast<s32>(a18->value) : 0;

    if (impulseChainA || impulseChainB || impulseChainC) {
        hasImpulseChain = 1;
    }
}

void Pendulum::CreateModel(const char* name) {
    MARKFUNCTION(0x8002520C);
    Obstacle::CreateModel(name);
    AllocateAndCreateShadow();

    CSound* created = nullptr;
    if (CSoundFactory::CreateObject(10120, &created, modelHash) >= 0) {
        pendulumSound = static_cast<CPendulumSound*>(created);
        pendulumSound->Initialize(&pos);
    }
}

void Pendulum::DeleteModel() {
    MARKFUNCTION(0x8002525C);
    Obstacle::DeleteModel();

    delete pendulumSound;
    pendulumSound = nullptr;
}

void Pendulum::Reset() {
    MARKFUNCTION(0x800252AC);
}

void Pendulum::Think() {
    MARKFUNCTION(0x800252B4);

    const LVector previousOrientation = orientation;
    LVector nextOrientation = orientation;

    swingVelocityZ -= MulShift16(swingScale, rmSin16(nextOrientation.z));
    swingVelocityX -= MulShift16(swingScale, rmSin16(nextOrientation.x));

    nextOrientation.z += PendulumAngularStep(swingVelocityZ);
    nextOrientation.x += PendulumAngularStep(swingVelocityX);
    orientation = nextOrientation;

    const bool crossedX =
        ((previousOrientation.x <= 0 && nextOrientation.x > 0) ||
         (nextOrientation.x < 0 && previousOrientation.x >= 0));
    const bool crossedZ =
        ((previousOrientation.z <= 0 && nextOrientation.z > 0) ||
         (nextOrientation.z < 0 && previousOrientation.z >= 0));

    if ((crossedX || crossedZ) && pendulumSound) {
        pendulumSound->Swing();
    }

    s32 dominantSwing = nextOrientation.z;
    if (PsxAbsS32(dominantSwing) < PsxAbsS32(nextOrientation.x)) {
        dominantSwing = nextOrientation.x;
    }

    const s32 sinY = rmSin16(nextOrientation.y);
    const s32 sinSwing = rmSin16(dominantSwing);
    const s32 cosY = rmSin16(nextOrientation.y + 0x4000);
    const s32 cosSwing = rmSin16(dominantSwing + 0x4000);

    const s32 endX = -MulShift16(pendulumLength, MulShift16(sinSwing, cosY));
    const s32 endY = MulShift16(pendulumLength, cosSwing);
    const s32 endZ = -MulShift16(pendulumLength, MulShift16(sinSwing, sinY));

    tagCollisionBox armBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    armBox.minX = static_cast<s16>((endX >> 16) - armHalfX);
    armBox.minY = 0;
    armBox.minZ = static_cast<s16>((endZ >> 16) - armHalfZ);
    armBox.maxX = static_cast<s16>((endX >> 16) + armHalfX);
    armBox.maxY = 0;
    armBox.maxZ = static_cast<s16>((endZ >> 16) + armHalfZ);
    SetCollisionBox(armBox);
    UpdateShadowFloorHeight();

    tagCollisionBox bulbBox = armBox;
    bulbBox.minX = static_cast<s16>((endX >> 16) - bulbHalfX);
    bulbBox.minY = static_cast<s16>(endY >> 16);
    bulbBox.minZ = static_cast<s16>((endZ >> 16) - bulbHalfZ);
    bulbBox.maxX = static_cast<s16>((endX >> 16) + bulbHalfX);
    bulbBox.maxY = static_cast<s16>(static_cast<s32>(bulbHalfY) + (endY >> 16));
    bulbBox.maxZ = static_cast<s16>((endZ >> 16) + bulbHalfZ);
    SetCollisionBox(bulbBox);
}

void Pendulum::UpdatePosition() {
    MARKFUNCTION(0x800256C4);
}

void Pendulum::Draw() {
    MARKFUNCTION(0x800256CC);
    Obstacle::Draw();
}

void Pendulum::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800256EC);
    if (pickup) {
        pickup->Kill();
    }
}

void Pendulum::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80025720);
    if (!hum) {
        return;
    }

    LVector correctedPos = {};
    LVector correctionNormal = {};
    LVector pushedPos = {};
    CorrectThingPositionObstacle(
        pos,
        pos,
        orientation.y,
        orientation.y,
        collBox,
        hum->pos,
        hum->homePos,
        hum->collBboxMin.x,
        hum->collBboxMin.y,
        hum->collBboxMin.z,
        correctedPos,
        correctionNormal,
        pushedPos);
    correctedPos.y = hum->homePos.y;
    hum->homePos = correctedPos;

    if (impactRegionMode == 0) {
        return;
    }

    if (hum->thingType == AITypes::TT_PLAYER) {
        Shock(SHOCK_9);
    }

    s32 impulseX = PendulumLengthSinComponent(pendulumLength, PsxAbsS32(orientation.z));
    if (swingVelocityZ >= 0) {
        impulseX = -impulseX;
    }
    impulseX *= impulseScale;

    s32 impulseZ = PendulumLengthSinComponent(pendulumLength, PsxAbsS32(orientation.x));
    if (swingVelocityX >= 0) {
        impulseZ = -impulseZ;
    }
    impulseZ *= impulseScale;

    if (hasImpulseChain) {
        if (impulseZ < impulseX) {
            impulseX *= impulseChainA;
        }
        else {
            impulseZ *= impulseChainA;
        }

        if (impulseZ < impulseX) {
            impulseX *= impulseChainC;
        }
        else {
            impulseZ *= impulseChainC;
        }
    }

    hum->velocity.x = impulseX;
    hum->velocity.y = 0;
    hum->velocity.z = impulseZ;

    if (hum->actionState < 46) {
        if (strongHitMode) {
            hum->HandleCollision(
                this,
                1,
                static_cast<s32>(0x80000002u), 4,
                static_cast<s32>(0x80000003u), 3,
                static_cast<s32>(0x80000007u), damageAmount,
                0);
        }
        else {
            hum->HandleCollision(
                this,
                1,
                static_cast<s32>(0x80000002u), 4,
                static_cast<s32>(0x80000007u), damageAmount,
                0);
        }

        if (pendulumSound) {
            pendulumSound->HitHumanoid();
        }
    }
}
