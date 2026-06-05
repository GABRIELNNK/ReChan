#include "ai/explosive.h"
#include "ai/humanoid.h"
#include "ai/pickup.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/colsect.h"
#include "gen/common.h"
#include "gen/control.h"
#include "gen/database.h"
#include "gen/colvol.h"
#include "gen/geffect.h"
#include "ai/obstacle_shared.h"
#include "snd/snddrct.h"

static s32 MulHigh32(s32 a, s32 b) {
    return (s32)(((s64)a * (s64)b) >> 32);
}

Explosive::Explosive(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001304C);
    aliveFlag = 1;
}

Explosive::~Explosive() {
    MARKFUNCTION(0x800130C0);
}

void Explosive::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800130E8);
    if (!root) {
        return;
    }

    Obstacle::AnalyzeMesh(root);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    tagCollisionBox localBox = INVALID_COLLISION_BOX;
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
    savedCollBox = localBox;

    if (const DBAttrib* attrib = root->FindAttrib(6)) {
        field132 = (s32)attrib->GetAttribValue();
    }
    else {
        field132 = 0x200;
    }

    if (const DBAttrib* attrib = root->FindAttrib(7)) {
        field136 = (s32)attrib->GetAttribValue();
    }
    else {
        field136 = 0x400;
    }

    if (const DBAttrib* attrib = root->FindAttrib(8)) {
        field120 = (s32)attrib->GetAttribValue();
    }
    else {
        field120 = 0;
    }

    if (const DBAttrib* attrib = root->FindAttrib(9)) {
        field124 = Div2TowardZero((s32)attrib->GetAttribValue());
    }
    else {
        field124 = 10;
    }

    if (const DBAttrib* attrib = root->FindAttrib(20)) {
        const char* effectName = attrib->GetAttribString();
        if (effectName) {
            field160 = (s32)p3dHash(effectName);
        }
    }
}

void Explosive::CreateModel(const char* name) {
    MARKFUNCTION(0x800132C8);
    Obstacle::CreateModel(name);
}

void Explosive::DeleteModel() {
    MARKFUNCTION(0x800132E8);
    Obstacle::DeleteModel();
}

void Explosive::Reset() {
    MARKFUNCTION(0x80013308);
    SetCollisionBox(savedCollBox);
    state = 0;
    aliveFlag = 1;
}

void Explosive::Think() {
    MARKFUNCTION(0x80013A48);

    switch (state) {
    case 0:
        CheckObstacleCollisions();
        MovePassengers();
        break;

    case 1:
        if (field156 != 0) {
            if (field160 != 0) {
                if (!(field156 < field120 && field128 == 0)) {
                    GEffect_Create((u32)field160, &pos, nullptr, nullptr, 0, 0, 0);
                    field128 = 0;
                }
            }
            field156--;
        }
        else {
            state = 2;
            aliveFlag = 0;
            field156 = 0;
        }

        MovePassengers();
        break;

    case 2:
        CheckObstacleCollisions();
        if (field156 < field124) {
            field156++;
        }
        else {
            state = 3;
        }
        AdjustCollisionBox();
        break;

    case 3:
        if (field156 != 0) {
            field156--;
        }
        else {
            state = 4;
            Kill();
        }
        AdjustCollisionBox();
        break;

    default:
        break;
    }
}

void Explosive::Draw() {
    MARKFUNCTION(0x80013A18);
    if (state <= 0) {
        Obstacle::Draw();
    }
}

void Explosive::UpdatePosition() {
}


void Explosive::CheckObstacleCollisions() {
    MARKFUNCTION(0x8001333C);

    tagCollisionSphere sphere = {};
    if (state < 2) {
        const s32 spanX = (s32)collBox.maxX - (s32)collBox.minX;
        const s32 spanY = (s32)collBox.maxY - (s32)collBox.minY;
        const s32 spanZ = (s32)collBox.maxZ - (s32)collBox.minZ;
        s32 maxSpan = spanX;
        if (maxSpan < spanY) {
            maxSpan = spanY;
        }
        if (maxSpan < spanZ) {
            maxSpan = spanZ;
        }
        sphere.radius = Div2TowardZero(maxSpan);
    }
    else {
        sphere.radius = (field124 != 0) ? ((field156 * field136) / field124) : 0;
    }

    if (!g_ai) {
        return;
    }

    for (ccMinNode* node = g_ai->moveList.head; node; node = node->next) {
        Obstacle* other = static_cast<Obstacle*>(static_cast<Thing*>(node));
        if (!other || other == this) {
            continue;
        }

        if (other->thingType != AITypes::TT_EXPLOSIVE &&
            other->thingType != AITypes::TT_KICKNROLL &&
            other->thingType != AITypes::TT_EXPLOSIVE_OBJ) {
            continue;
        }

        const s16 yRot = (s16)GetYRotation(other->orientation.x, other->orientation.z);
        if (CheckStaticBoxSphereCollision(other->pos, other->collBox, yRot, pos, sphere)) {
            HandleObstacleCollision(other);
        }
    }
}

void Explosive::AdjustCollisionBox() {
    MARKFUNCTION(0x800134BC);

    const s32 range = (field124 != 0) ? ((field156 * field136) / field124) : 0;
    tagCollisionBox newBox = INVALID_COLLISION_BOX;
    newBox.minX = (s16)-range;
    newBox.minY = (s16)-range;
    newBox.minZ = (s16)-range;
    newBox.maxX = (s16)range;
    newBox.maxY = (s16)range;
    newBox.maxZ = (s16)range;
    SetCollisionBox(newBox);
}

void Explosive::ExplodeThing(Thing* target) {
    MARKFUNCTION(0x80013554);

    if (!target) {
        return;
    }

    tagCollisionSphere sphere = {};
    sphere.radius = (field124 != 0) ? ((field156 * field132) / field124) : 0;

    LVector targetPos = target->pos;
    const LVector targetOrientation = target->orientation;

    LVector normal = {};
    normal.x = (targetPos.x - pos.x) << 16;
    normal.y = (targetPos.y - pos.y) << 16;
    normal.z = (targetPos.z - pos.z) << 16;
    rmV3Normalize(&normal, &normal);
    if (normal.y < 0) {
        normal.y = 0;
    }

    const s32 scale = (field124 != 0) ? rmDiv16i(field136 << 16, field124 << 16) : 0;

    if (target->thingType < AITypes::TT_OBSTACLE_FIRST) {
        Humanoid* hum = static_cast<Humanoid*>(target);

        if (hum->actionState < (s32)AS_HIT_REACT_PUNCH_A) {
            const bool directHit = CheckStaticCylinderSphereCollision(
                targetPos,
                *reinterpret_cast<const tagCollisionCylinder*>(&hum->collBboxMin),
                pos,
                sphere);
            hum->HandleCollision(
                this,
                1,
                0x80000002,
                4,
                0x80000003,
                14,
                0x80000007,
                directHit ? 10000 : 50,
                0);
        }
        else if (hum->actionState == (s32)AS_COLLAPSE_STUN) {
            hum->HandleCollision(this, 1, 0x80000007, 50, 0);
            hum->SetActionState(AS_FLYING_BACK_LAND, 0);
        }

        targetPos.x += MulHigh32(normal.x, scale);
        targetPos.y += MulHigh32(normal.y, scale);
        targetPos.z += MulHigh32(normal.z, scale);
        hum->homePos = targetPos;

        if (target->thingType == AITypes::TT_PLAYER) {
            hum->LoadDialog(24, 60);
            Shock(SHOCK_15);
        }

        return;
    }

    if (target->thingType == AITypes::TT_COLLECTIBLE) {
        if ((targetPos.x - pos.x) < sphere.radius &&
            (targetPos.y - pos.y) < sphere.radius &&
            (targetPos.z - pos.z) < sphere.radius) {
            Pickup* pickup = static_cast<Pickup*>(target);
            pickup->PlayEffect();
            pickup->Kill();
            return;
        }
    }
    else if (target->thingType < 0x191) {
        return;
    }

    const s16 yRot = (s16)GetYRotation(targetOrientation.x, targetOrientation.z);
    const bool overlap = CheckStaticBoxSphereCollision(
        targetPos,
        static_cast<Obstacle*>(target)->collBox,
        yRot,
        pos,
        sphere);
    static_cast<Obstacle*>(target)->ExplosiveTrigger(overlap ? 1 : 0, nullptr);

    if (target->thingType == AITypes::TT_EXPLOSIVE ||
        target->thingType == AITypes::TT_EXPLOSIVE_OBJ) {
        return;
    }

    targetPos.x += MulHigh32(normal.x, scale);
    targetPos.y += MulHigh32(normal.y, scale);
    targetPos.z += MulHigh32(normal.z, scale);
    target->pos = targetPos;
}

void Explosive::MovePassengers() {
    MARKFUNCTION(0x80013C0C);
    MovePassengersBasic();
}

void Explosive::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80013C2C);

    if (state == 0) {
        state = 1;
        field156 = field120;

        if (pickup) {
            Pickup* p = static_cast<Pickup*>(pickup);
            p->PlayEffect();
            p->Kill();
        }
    }
    else if (state == 2) {
        ExplodeThing(pickup);
    }
}

void Explosive::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80013CA8);

    if (!hum) {
        return;
    }

    if (state == 2 || state == 3) {
        ExplodeThing(hum);
        return;
    }

    if (state == 0) {
        const bool triggerNow =
            hum->actionState == 59 ||
            hum->actionState == 63 ||
            hum->actionState == 56 ||
            hum->actionState == 57 ||
            hum->velocity.y < -250 ||
            (hum->actionState == 58 && hum->velocity.y < -25);

        if (triggerNow) {
            CSoundDirect::PlayTransient(209, &pos, 0, 0);
            state = 1;
            field156 = field120;
        }
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
    hum->homePos = correctedPos;

    if (correctionNormal.y > 0) {
        hum->SetFloorHeight(pos.y + (s32)collBox.maxY);
        AddPassenger(hum);
    }
}

void Explosive::HandleObstacleCollision(Obstacle* other) {
    MARKFUNCTION(0x80013E40);

    if (!other) {
        return;
    }

    if (state == 0) {
        if (other->thingType == AITypes::TT_KICKNROLL) {
            CSoundDirect::PlayTransient(209, &pos, 0, 0);
            state = 1;
            field128 = 1;
            field156 = 1;
        }
    }
    else if (state == 2) {
        ExplodeThing(other);
    }
}

void Explosive::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x80013ECC);

    (void)attacker;
    (void)damageType;
    (void)attackMagnitude;
    (void)damage;

    if (state == 0) {
        CSoundDirect::PlayTransient(209, &pos, 0, 0);
        state = 1;
        field156 = field120;
    }
}

bool Explosive::CareAboutAttack() const {
    MARKFUNCTION(0x80013F1C);
    return true;
}

void Explosive::TriggerByName(Thing* source, const char* name, const char* param) {
    MARKFUNCTION(0x80013BD0);
    (void)source;
    (void)name;
    (void)param;

    if (state == 0) {
        state = 1;
        field156 = field120;
    }
}

void Explosive::ExplosiveTrigger(s32 damage, const char* name) {
    MARKFUNCTION(0x80013BFC);
    (void)name;

    if (state >= 2) {
        return;
    }

    state = 1;
    if (damage != 0) {
        field156 = 1;
        field128 = 1;
    }
    else {
        field156 = field120;
    }
}
