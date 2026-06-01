#include "ai/pushable.h"
#include "ai/humanoid.h"
#include "ai/pickup.h"
#include "gen/common.h"
#include "gen/database.h"
#include "gen/blockmgr.h"
#include "gen/colsect.h"
#include "gen/model.h"
#include "ai/obstacle_shared.h"
#include "snd/pushsnd.h"
#include "snd/sndfact.h"
#include "gen/scoremgr.h"

Pushable::Pushable(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80017CCC);
}

Pushable::~Pushable() {
    MARKFUNCTION(0x80017D08);
    if (field168) {
        CPushableSound* snd = reinterpret_cast<CPushableSound*>(field168);
        snd->Release();
        field168 = nullptr;
    }
}

void Pushable::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80017D70);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    field116 = pos.x;
    field120 = pos.y;
    field124 = pos.z;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
    const DBAttrib* attr6 = root->FindAttrib(6);
    stateCounter = attr6 ? (s32)((u32)attr6->value << 16) : 0x320000;
    const DBAttrib* attr7 = root->FindAttrib(7);
    field152 = attr7 ? rmDiv16i((s32)((u32)attr7->value << 16), 0x640000) : 0x51E;
    s32 scale = (s32)(((s64)stateCounter * 0x120000) >> 16);
    stateCounter = (s32)(s16)(stateCounter >> 16);
    field152 = (s32)((s64)field152 * scale >> 32);
}

void Pushable::CreateModel(const char* name) {
    MARKFUNCTION(0x80017EE0);
    Obstacle::CreateModel(name);

    if (!field168) {
        CSound* created = nullptr;
        const s32 result = CSoundFactory::CreateObject(0x2724, &created, modelHash);
        if (result >= 0) {
            CPushableSound* sound = static_cast<CPushableSound*>(created);
            sound->Initialize(&pos);
            field168 = reinterpret_cast<s32*>(sound);
        }
    }
}

void Pushable::DeleteModel() {
    MARKFUNCTION(0x80017F34);
    Obstacle::DeleteModel();

    if (field168) {
        CPushableSound* snd = reinterpret_cast<CPushableSound*>(field168);
        snd->Release();
        field168 = nullptr;
    }
}

void Pushable::Reset() {
    MARKFUNCTION(0x80017F84);
    field128 = 0;
    field132 = 0;
    field136 = 0;
    field140 = 0;
    field144 = 0;
    field148 = 0;
    field164 = 0;
    field160 = 0;
    field156 = 0;
}

void Pushable::Think() {
    MARKFUNCTION(0x80017FAC);

    // PSX: a "moving" test across all three velocity components
    const bool isMoving = (field128 != 0 || field132 != 0 || field136 != 0);

    // PSX: if (field168) CPushableSound::Think()
    if (field168) {
        CPushableSound* sound = reinterpret_cast<CPushableSound*>(field168);
        if (sound) sound->Think();
    }

    // PSX: if (field156 == 0 && field156_prev != 0 && isMoving) BeginPush
    if (field156 == 0 && field160 != 0 && isMoving) {
        if (field168) {
            CPushableSound* sound = reinterpret_cast<CPushableSound*>(field168);
            if (sound) sound->BeginPush();
        }
    }

    // PSX: if (!isMoving) EndPush
    if (!isMoving) {
        if (field168) {
            CPushableSound* sound = reinterpret_cast<CPushableSound*>(field168);
            if (sound) sound->EndPush();
        }
    }

    if (isMoving) {
        Move();
    }

    MovePassengers();

    if (g_blockManager) {
        const u16 newBlock = g_blockManager->GetBlockNumber(pos);
        if (newBlock != BLOCK_UNASSIGNED) {
            blockNum = newBlock;
        }
    }

    // PSX tail: update field160 (was_moving), then clear field156
    if (field156 == 0) {
        field160 = 0;
    }
    else {
        field160 = (field160 != 0 || isMoving) ? 1 : 0;
    }
    field156 = 0;
}

void Pushable::Move() {
    MARKFUNCTION(0x8001811C);

    if (field156 != 0) {
        return;
    }

    const LVector prevPos = pos;
    field116 = pos.x;
    field120 = pos.y;
    field124 = pos.z;

    pos.x += field128;
    pos.z += field136;

    if (HandleEnvironmentCollision(prevPos)) {
        // Wall collision: zero both velocity components
        field136 = 0;
        field128 = 0;
    }
    else {
        // No wall: apply friction deceleration.
        field128 += field140;
        if (((s64)field128 * (s64)field140) >= 0) {
            field128 = 0;
        }
        field136 += field148;
        if (((s64)field136 * (s64)field148) >= 0) {
            field136 = 0;
        }
    }

    field164 = 0;

}

void Pushable::MovePassengers() {
    MARKFUNCTION(0x80018244);

    Ticket* ticket = static_cast<Ticket*>(subNode.next);
    if (model) {
        Model* pushableModel = static_cast<Model*>(model);
        if (ticket) {
            pushableModel->modelFlags |= 8;
        }
        else {
            pushableModel->modelFlags &= ~8u;
        }
    }

    static const s32 kPassengerCarryThreshold = 40;

    while (ticket) {
        Humanoid* hum = static_cast<Humanoid*>(ticket->passenger);
        Ticket* nextTicket = static_cast<Ticket*>(ticket->next);
        if (!hum) {
            ticket = nextTicket;
            continue;
        }

        LVector savedHomePos = hum->homePos;
        LVector savedVelocity = hum->velocity;

        savedHomePos.y = pos.y + (s32)collBox.maxY - 2;
        if (savedVelocity.y <= 0) {
            savedVelocity.y = 0;
            hum->flags = (hum->flags | TF_ON_GROUND) & ~0x2000u;
        }

        s32 absVelX = field128;
        if (absVelX < 0) {
            absVelX = -absVelX;
        }

        s32 absVelZ = field136;
        if (absVelZ < 0) {
            absVelZ = -absVelZ;
        }

        if (absVelX > kPassengerCarryThreshold || absVelZ > kPassengerCarryThreshold) {
            if (hum->actionState < (s32)AS_HIT_REACT_PUNCH_A) {
                hum->HandleCollision(this, 1, 0x80000002, 4, 0x80000007, 10, 0x80000003, 14, 0);
                if (hum->thingType != 0) {
                    if (g_scoreManager) {
                        g_scoreManager->AddStylePoints(100);
                    }
                }
                if (field168) {
                    CPushableSound* snd = reinterpret_cast<CPushableSound*>(field168);
                    snd->HitHumanoid();
                }
            }
        }
        else {
            savedHomePos.x += field128;
            savedHomePos.z += field136;
        }

        hum->homePos = savedHomePos;
        hum->velocity = savedVelocity;
        ticket = nextTicket;
    }
}

void Pushable::UpdatePosition() {
    MARKFUNCTION(0x800184A8);
}

bool Pushable::HandleEnvironmentCollision(const LVector& prevPos) {
    MARKFUNCTION(0x800184B0);
    const s32 sinY = rmSin16(orientation.y);
    const s32 cosY = rmSin16(orientation.y + 0x4000);

    const s32 centreX = Div2TowardZero((s32)collBox.minX + (s32)collBox.maxX);
    const s32 centreZ = Div2TowardZero((s32)collBox.minZ + (s32)collBox.maxZ);

    const s32 worldOffX =
        (s32)(((s64)cosY * (s64)centreX) >> 16) +
        (s32)(((s64)sinY * (s64)centreZ) >> 16);
    const s32 worldOffZ =
        (s32)(((s64)(-sinY) * (s64)centreX) >> 16) +
        (s32)(((s64)cosY * (s64)centreZ) >> 16);

    LVector oldPos = prevPos;
    oldPos.x += worldOffX;
    oldPos.z += worldOffZ;

    LVector newPos = pos;
    newPos.x += worldOffX;
    newPos.z += worldOffZ;

    const s32 halfX = Div2TowardZero((s32)collBox.maxX - (s32)collBox.minX);
    const s32 halfZ = Div2TowardZero((s32)collBox.maxZ - (s32)collBox.minZ);

    const s32 radiusX =
        (s32)(((s64)cosY * (s64)halfX) >> 16) +
        (s32)(((s64)sinY * (s64)halfZ) >> 16);
    const s32 radiusZ =
        (s32)(((s64)(-sinY) * (s64)halfX) >> 16) +
        (s32)(((s64)cosY * (s64)halfZ) >> 16);

    const s32 deltaX = newPos.x - oldPos.x;
    const s32 deltaZ = newPos.z - oldPos.z;
    const s32 absDeltaX = (deltaX < 0) ? -deltaX : deltaX;
    const s32 absDeltaZ = (deltaZ < 0) ? -deltaZ : deltaZ;
    const s32 absRadiusX = (radiusX < 0) ? -radiusX : radiusX;
    const s32 absRadiusZ = (radiusZ < 0) ? -radiusZ : radiusZ;
    s32 radius = (absDeltaZ < absDeltaX) ? absRadiusX : absRadiusZ;
    if (radius >= 0) {
        radius -= 2;
    }

    const s32 minY = (s32)collBox.minY;
    const s32 maxY = (s32)collBox.maxY;

    s32 wallRatio = 0;
    LVector wallNormal = {};
    LVector wallHitPoint = {};
    s32 wallHorizontal = 0;
    const bool wallHit = CollisionSector::CheckWorldWallCollision(
        oldPos,
        newPos,
        radius,
        minY,
        maxY,
        wallRatio,
        wallNormal,
        wallHitPoint,
        wallHorizontal) != 0;
    (void)wallNormal;
    (void)wallHitPoint;
    (void)wallHorizontal;

    if (wallHit) {
        newPos.x = oldPos.x + (s32)(((s64)wallRatio * (s64)deltaX) >> 16);
        newPos.z = oldPos.z + (s32)(((s64)wallRatio * (s64)deltaZ) >> 16);
    }

    const s32 floorHeight = g_collisionSectors[0].GetWorldFloorHeight(newPos, radius);

    static constexpr s32 k_pushableGravityStep = -24;

    if (floorHeight < newPos.y) {
        field132 += k_pushableGravityStep;
        field144 = k_pushableGravityStep;
        newPos.y += field132;
        if (!(floorHeight < newPos.y)) {
            field144 = 0;
            field132 = 0;
            newPos.y = floorHeight;
        }
    }
    else {
        field144 = 0;
        field132 = 0;
        newPos.y = floorHeight;
    }

    newPos.x -= worldOffX;
    newPos.z -= worldOffZ;
    pos = newPos;

    return wallHit;
}

void Pushable::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800188E8);
    if (!pickup) {
        return;
    }

    Pickup* p = static_cast<Pickup*>(pickup);
    p->PlayEffect();
    p->Kill();
}

bool Pushable::CareAboutAttack() const {
    MARKFUNCTION(0x80019298);
    return true;
}

void Pushable::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x80018928);
    (void)damage;

    if (field128 || field136) {
        return;
    }

    LVector boxCenter = {};
    FillBoxCentre(boxCenter, pos, orientation, collBox);

    const s32 dx = boxCenter.x - attacker->pos.x;
    const s32 dz = boxCenter.z - attacker->pos.z;

    if (damageType != 4) {
        return;
    }

    if (field168) {
        CPushableSound* sound = reinterpret_cast<CPushableSound*>(field168);
        if (sound)
            sound->Kick();
    }

    // PSX divides the attack force payload by pushable strength (+52).
    if (stateCounter == 0) {
        return;
    }
    const s32 impulse = attackMagnitude / stateCounter;

    const s32 magXZ = (s32)rmMag2((f32)dx, (f32)dz);
    if (magXZ == 0) {
        return;
    }

    const s32 dirX = rmDiv16i(dx, magXZ);
    const s32 dirZ = rmDiv16i(dz, magXZ);

    if (dirX > 0x8000) {
        field128 = impulse;
        field136 = 0;
        field148 = 0;
        field140 = -field152;
    }
    else if (dirX < -0x8000) {
        field128 = -impulse;
        field136 = 0;
        field148 = 0;
        field140 = field152;
    }
    else if (dirZ > 0x8000) {
        field128 = 0;
        field136 = impulse;
        field140 = 0;
        field148 = -field152;
    }
    else {
        field128 = 0;
        field136 = -impulse;
        field140 = 0;
        field148 = field152;
    }

    field132 = 0;
    field144 = 0;
}

void Pushable::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80018AF8);

    const LVector prevPos = pos;

    LVector correctedPos = {};
    LVector correctionNormal = {};
    LVector correctionPushedPos = {};

    SetCorrectThingPositionRadiusBias(hum->collBboxMin.x);

    CorrectThingPositionObstacle(
        LVector{ field116, field120, field124 },
        prevPos,
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
        correctionPushedPos);

    const s16 normX_hi = (s16)(correctionNormal.x >> 16);
    const s16 normZ_hi = (s16)(correctionNormal.z >> 16);
    static const s32 kImpactThresholdX = 40;
    static const s32 kImpactThresholdZ = 40;
    const bool impact =
        (field128 * normX_hi > kImpactThresholdX) ||
        (field136 * normZ_hi > kImpactThresholdZ);

    if (impact) {
        if (hum->actionState < (s32)AS_HIT_REACT_PUNCH_A) {
            hum->HandleCollision(this, 1, 0x80000002, 4, 0x80000007, 20, 0x80000003, 14, 0);
            if (hum->thingType != 0) {
                if (g_scoreManager) {
                    g_scoreManager->AddStylePoints(100);
                }
            }
            if (field168) {
                CPushableSound* snd = reinterpret_cast<CPushableSound*>(field168);
                snd->HitHumanoid();
            }
            return;
        }
    }

    if (LedgeCheck(collBox, correctionNormal, correctionPushedPos, hum)) {
        if (hum->thingType != 0) {
            return;
        }
        if (hum->actionState != (s32)AS_LEDGE_LATCH) {
            hum->SetActionState(AS_LEDGE_LATCH, 0);
            hum->PrepareLedgeLatch(correctionPushedPos, correctionNormal);
            if (hum->humanoidSound) {
                hum->humanoidSound->Grab((CSoundMaterial)GetFloorMaterial());
            }
        }
        AddPassenger(hum);
        return;
    }

    if (correctionNormal.y > 0 && hum->velocity.y <= 0) {
        hum->SetFloorHeight(prevPos.y + (s32)collBox.maxY);
        AddPassenger(hum);
        hum->homePos = correctedPos;
        return;
    }

    if (hum->actionState >= (s32)AS_KICK_ATTACK && hum->actionState < (s32)AS_COMBAT_IDLE) {
        hum->homePos = correctedPos;
        return;
    }

    const bool doImpulse = (hum->velocity.x != 0 || hum->velocity.z != 0) &&
        hum->actionState != (s32)AS_STAND &&
        hum->actionState != (s32)AS_PAUSE &&
        hum->actionState != (s32)AS_DIVE_ROLL;

    if (doImpulse) {
        const s32 sinY = rmSin16(hum->orientation.y);
        const s32 cosY = rmSin16(hum->orientation.y + 0x4000);
        const s32 normXmid = (s32)(((s64)correctionNormal.x * sinY) >> 16);
        const s32 normZmid = (s32)(((s64)correctionNormal.z * cosY) >> 16);
        const s32 xz_component = normXmid + normZmid;

        static const s32 k_impulseThreshold = -58982;
        if (xz_component < k_impulseThreshold) {
            const s32 old164 = field164;
            field164++;
            if (old164 < 5) {
                field156 = 1;
            }
            else {
                field116 = pos.x;
                field120 = pos.y;
                field124 = pos.z;
                const s32 dot = (s32)(((s64)correctionNormal.x * hum->velocity.x) >> 16) +
                    (s32)(((s64)correctionNormal.z * hum->velocity.z) >> 16);
                const s32 impX = (s32)(((s64)correctionNormal.x * dot) >> 16);
                const s32 impZ = (s32)(((s64)correctionNormal.z * dot) >> 16);
                const LVector impulseBasePos = pos;
                LVector newPos = impulseBasePos;
                newPos.x += Div2TowardZero(impX);
                newPos.z += Div2TowardZero(impZ);
                pos = newPos;
                HandleEnvironmentCollision(impulseBasePos);
                field128 = pos.x - impulseBasePos.x;
                field132 = 0;
                field136 = pos.z - impulseBasePos.z;
                field140 = Div2TowardZero((s32)(((s64)field152 * correctionNormal.x) >> 32));
                field144 = 0;
                field148 = Div2TowardZero((s32)(((s64)field152 * correctionNormal.z) >> 32));
                correctedPos.x += Div2TowardZero(hum->velocity.x);
                correctedPos.z += Div2TowardZero(hum->velocity.z);
                if (hum->actionState != (s32)AS_PUSH_OBJECT) {
                    hum->SetActionState(AS_PUSH_OBJECT, 0);
                }
                hum->field368 |= 4;
                field156 = 1;
            }
        }
    }

    const s32 vel_dot = field128 * correctionNormal.x + field136 * correctionNormal.z;
    if (vel_dot <= 0) {
        hum->homePos = correctedPos;
    }
    else {
        pos.x = field116;
        pos.y = field120;
        pos.z = field124;
        Reset();
    }
}

s32 Pushable::GetFloorMaterial() const {
    MARKFUNCTION(0x80019264);
    s32 mat = 3;
    if (field168) {
        CPushableSound* sound = reinterpret_cast<CPushableSound*>(field168);
        sound->GetMaterial(&mat);
    }
    return mat;
}
