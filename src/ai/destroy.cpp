#include "ai/destroy.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "ai/pickup.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/common.h"
#include "gen/control.h"
#include "gen/database.h"
#include "gen/geffect.h"

DestructibleThing::DestructibleThing(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x800102B8);
    aliveFlag = 1;
}

DestructibleThing::~DestructibleThing() {
    MARKFUNCTION(0x80010308);
}

void DestructibleThing::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80010388);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void DestructibleThing::CreateModel(const char* name) {
    MARKFUNCTION(0x80010658);
    Obstacle::CreateModel(name);
}

void DestructibleThing::DeleteModel() {
    MARKFUNCTION(0x800106AC);
    Obstacle::DeleteModel();
}

void DestructibleThing::Reset() {
    MARKFUNCTION(0x800106FC);
}

void DestructibleThing::Think() {
    MARKFUNCTION(0x80010710);
    if (!destroyFlag && aliveFlag) {
        MovePassengers();
        // CDestructibleSound::Think(sound) at 0x800AC5F4.
    }
}

void DestructibleThing::UpdatePosition() {
    MARKFUNCTION(0x80010770);
}

void DestructibleThing::Draw() {
    MARKFUNCTION(0x80010778);
    if (!destroyFlag && aliveFlag) {
        Obstacle::Draw();
    }
}

void DestructibleThing::MovePassengers() {
    MARKFUNCTION(0x800107B8);
    MovePassengersBasic();
}

void DestructibleThing::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800107D8);
    if (!destroyFlag && aliveFlag && pickup) {
        static_cast<Pickup*>(pickup)->PlayEffect();
        pickup->Kill();
    }
}

void DestructibleThing::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80010834);
    LVector correctedPos = {};
    LVector correctionNormal = {};
    LVector correctionPushedPos = {};
    CorrectThingPositionObstacle(
        pos, pos,
        orientation.y, orientation.y,
        collBox,
        hum->pos, hum->homePos,
        hum->collBboxMin.x, hum->collBboxMin.y, hum->collBboxMin.z,
        correctedPos, correctionNormal, correctionPushedPos);
    hum->homePos = correctedPos;
}

bool DestructibleThing::CareAboutAttack() const {
    MARKFUNCTION(0x80010B64);
    return true;
}

void DestructibleThing::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {
    MARKFUNCTION(0x80010B6C);
    (void)damageType;

    const s16 hitDamage = static_cast<s16>(damage);
    if (field132 >= hitDamage) {
        return;
    }

    const u16 hitDamageU = static_cast<u16>(hitDamage);
    if (hitDamageU < health) {
        health = static_cast<u16>(health - hitDamage);
    } else {
        health = 0;
    }

    if (health == 0) {
        Destroy();
        if (attacker == Player::s_player) {
            Shock(SHOCK_12);
        }
    }
}

void DestructibleThing::HandleObstacleCollision(Obstacle* other) {
    MARKFUNCTION(0x80010BFC);
}

s32 DestructibleThing::GetFloorMaterial() const {
    MARKFUNCTION(0x80010CF8);
    return 3;
}

void DestructibleThing::GenerateItem() {
    MARKFUNCTION(0x800100F0);
    if (!g_ai || !itemModelName) return;

    LVector center;
    FillBoxCentre(center, pos, orientation, collBox);

    DBRoot tempRoot;
    tempRoot.AllocatePermanentAttributeArray(6);
    tempRoot.AddAttribString(0, 5, itemModelName);
    tempRoot.AddAttribNumber(1, 15, blockNum);
    tempRoot.AddAttribNumber(2, 6, (u32)itemParam1);
    tempRoot.AddAttribNumber(3, 7, (u32)itemParam2);
    tempRoot.AddAttribNumber(4, 8, (u32)itemParam3);
    tempRoot.AddAttribNumber(5, 9, (u32)itemParam4);

    g_ai->AddThingNoTagList(nullptr, generateItemType, &center, nullptr, itemModelName, &tempRoot);
    tempRoot.DeallocatePermanentAttributeArray();
}

void DestructibleThing::Destroy() {
    MARKFUNCTION(0x80010000);
    if (destroyFlag) return;
    if (!aliveFlag) return;

    if (generateItemType)
        GenerateItem();

    if (effectHash) {
        LVector center;
        FillBoxCentre(center, pos, orientation, collBox);
        GEffect_Create(effectHash, &center, nullptr, nullptr, 0, 0, effectParam);
    }

    // Smash__18CDestructibleSound(sound)
    // CDestructibleSound

    SetCollisionBox(INVALID_COLLISION_BOX);
    aliveFlag = 0;
    health = 0;
}
