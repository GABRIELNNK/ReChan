#include "ai/destroy.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "ai/pickup.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/colvol.h"
#include "gen/common.h"
#include "gen/control.h"
#include "gen/database.h"
#include "gen/director.h"
#include "gen/geffect.h"
#include "gen/scoremgr.h"
#include "snd/dstrsnd.h"
#include "snd/sndfact.h"

#include <cstring>

static constexpr s32 COLLISION_TAG_HIT_TYPE = static_cast<s32>(0x80000003u);
static constexpr s32 COLLISION_TAG_DAMAGE = static_cast<s32>(0x80000007u);
static constexpr s32 COLLISION_TAG_END = 0;

static constexpr s32 SCORE_THROW_ENEMY_INTO_DESTRUCTIBLE = 0xC8;
static constexpr s32 COLLISION_SPHERE_RADIUS = 0x80;
static constexpr s32 COLLISION_HIT_TYPE_DESTRUCTIBLE = 0x10;
static constexpr s32 DESTRUCTIBLE_BREAK_FORCE_MAX = 0x3E8;
static constexpr s32 DESTRUCTIBLE_BREAK_VEL_Y_MIN = -0xFA;
static constexpr s32 DESTRUCTIBLE_BREAK_LAND_VEL_Y_MIN = -0x19;
static constexpr s32 DESTRUCTIBLE_EFFECT_PARAM_FLAG = 0x80000000;
static constexpr u32 DESTRUCTIBLE_DEFAULT_EFFECT_HASH = 0x065C8E90u;

static constexpr s32 ACTION_STATE_56 = 56;
static constexpr s32 ACTION_STATE_57 = 57;

DestructibleThing::DestructibleThing(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x800102B8);
    aliveFlag = 1;
}

DestructibleThing::~DestructibleThing() {
    MARKFUNCTION(0x80010308);

    if (itemModelName) {
        delete[] itemModelName;
        itemModelName = nullptr;
    }

    if (sound) {
        delete sound;
        sound = nullptr;
    }
}

void DestructibleThing::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80010388);

    if (!root) {
        return;
    }

    Obstacle::AnalyzeMesh(root);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);

    const DBAttrib* attrib = root->FindAttrib(7);
    if (attrib) {
        generateItemType = static_cast<u16>(attrib->value);
    }
    else {
        generateItemType = 0;
    }

    attrib = root->FindAttrib(8);
    if (attrib && generateItemType) {
        const char* str = attrib->GetAttribString();
        if (str) {
            s32 len = static_cast<s32>(std::strlen(str));
            itemModelName = new char[len + 1];
            std::strcpy(itemModelName, str);
        }
    }

    attrib = root->FindAttrib(0x1E);
    if (attrib) {
        itemParam1 = static_cast<s32>(attrib->value);
    }
    else {
        itemParam1 = 0;
    }

    attrib = root->FindAttrib(0x1F);
    if (attrib) {
        itemParam2 = static_cast<s32>(attrib->value);
    }
    else {
        itemParam2 = 0;
    }

    attrib = root->FindAttrib(0x20);
    if (attrib) {
        itemParam3 = static_cast<s32>(attrib->value);
    }
    else {
        itemParam3 = 0;
    }

    attrib = root->FindAttrib(0x21);
    if (attrib) {
        itemParam4 = static_cast<s32>(attrib->value);
    }
    else {
        itemParam4 = 0;
    }

    attrib = root->FindAttrib(9);
    if (attrib) {
        field132 = static_cast<s32>(attrib->value);
    }
    else {
        field132 = 0;
    }

    attrib = root->FindAttrib(0x0A);
    if (attrib) {
        field136 = static_cast<s32>(attrib->value);
    }
    else {
        field136 = 0;
    }

    health = 1;
    maxHealth = 1;
    attrib = root->FindAttrib(0x0B);
    if (attrib) {
        health = static_cast<u16>(attrib->value);
        maxHealth = health;
    }

    attrib = root->FindAttrib(0x14);
    if (attrib) {
        const char* effectName = attrib->GetAttribString();
        if (effectName && effectName[0] != '\0') {
            effectHash = p3dHash(effectName);
        }
        else {
            effectHash = DESTRUCTIBLE_DEFAULT_EFFECT_HASH;
        }
    }
    else {
        effectHash = DESTRUCTIBLE_DEFAULT_EFFECT_HASH;
    }

    attrib = root->FindAttrib(0x15);
    if (attrib) {
        effectParam = DESTRUCTIBLE_EFFECT_PARAM_FLAG;
    }
    else {
        effectParam = 0;
    }

    attrib = root->FindAttrib(0x19);
    if (attrib) {
        field124 = static_cast<s32>(attrib->value);
    }
    else {
        field124 = 0;
    }

    destroyFlag = 0;
}

void DestructibleThing::CreateModel(const char* name) {
    MARKFUNCTION(0x80010658);

    Obstacle::CreateModel(name);

    if (!sound) {
        CSound* created = nullptr;
        s32 result = CSoundFactory::CreateObject(0x276A, &created, modelHash);
        if (result >= 0) {
            sound = static_cast<CDestructibleSound*>(created);
            sound->Initialize(&pos);
        }
    }
}

void DestructibleThing::DeleteModel() {
    MARKFUNCTION(0x800106AC);

    Obstacle::DeleteModel();

    if (sound) {
        delete sound;
        sound = nullptr;
    }
}

void DestructibleThing::Reset() {
    MARKFUNCTION(0x800106FC);

    aliveFlag = 1;
    health = maxHealth;
}

void DestructibleThing::Think() {
    MARKFUNCTION(0x80010710);

    if (!destroyFlag && aliveFlag) {
        MovePassengers();
        if (sound) {
            sound->Think();
        }
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

    if (!hum) {
        return;
    }

    if (destroyFlag || !aliveFlag) {
        return;
    }

    if ((flags & TF_DIRECTOR_ACTIVE) != 0) {
        return;
    }

    const s32 actionState = hum->actionState;

    s32 breakHit = 0;
    if (field132 == 0) {
        breakHit = 1;
    }
    else if (field132 < DESTRUCTIBLE_BREAK_FORCE_MAX) {
        if (actionState == static_cast<s32>(AS_THROW_CHARACTER_RECEIVE)
            || actionState == static_cast<s32>(AS_THROW_FREE_FALL)
            || actionState == ACTION_STATE_56
            || actionState == ACTION_STATE_57) {
            breakHit = 1;
        }
        else {
            const s32 velY = hum->velocity.y;
            if (velY < DESTRUCTIBLE_BREAK_VEL_Y_MIN) {
                breakHit = 1;
            }
            else if (actionState == static_cast<s32>(AS_FLYING_BACK_LAND)
                     && velY < DESTRUCTIBLE_BREAK_LAND_VEL_Y_MIN) {
                breakHit = 1;
            }
        }
    }

    if (breakHit) {
        Destroy();

        if (g_director->scriptState != 0) {
            Shock(SHOCK_15);
            return;
        }

        hum->HandleCollision(
            this,
            1,
            COLLISION_TAG_DAMAGE,
            field136,
            COLLISION_TAG_HIT_TYPE,
            COLLISION_HIT_TYPE_DESTRUCTIBLE,
            COLLISION_TAG_END);

        if (hum != Player::s_player && g_scoreManager) {
            g_scoreManager->AddStylePoints(SCORE_THROW_ENEMY_INTO_DESTRUCTIBLE);
        }

        return;
    }

    LVector correctedPos = {};
    LVector correctionNormal = {};
    LVector correctionPushedPos = {};

    CorrectThingPositionObstacle(
        pos, pos,
        orientation.y, orientation.y,
        collBox,
        hum->pos, hum->homePos,
        hum->collBboxMin.x, 0, 0x300,
        correctedPos, correctionNormal, correctionPushedPos);

    hum->homePos = correctedPos;

    if (correctionNormal.y > 0 && hum->velocity.y <= 0) {
        hum->SetFloorHeight(pos.y + static_cast<s32>(collBox.maxY));
        AddPassenger(hum);
    }

    if (LedgeCheck(collBox, correctionNormal, correctionPushedPos, hum)) {
        if (hum == Player::s_player) {
            if (hum->actionState != static_cast<s32>(AS_LEDGE_LATCH)) {
                hum->SetActionState(static_cast<s32>(AS_LEDGE_LATCH), 0);
            }

            hum->PrepareLedgeLatch(correctionPushedPos, correctionNormal);

            if (hum->humanoidSound) {
                hum->humanoidSound->Grab(static_cast<CSoundMaterial>(GetFloorMaterial()));
            }

            AddPassenger(hum);
        }
    }
}

bool DestructibleThing::CareAboutAttack() const {
    MARKFUNCTION(0x80010B64);
    return true;
}

void DestructibleThing::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x80010B6C);
    (void)damageType;
    (void)attackMagnitude;

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

void HandleObstacleDestructibleThingCollision(Obstacle* obstacle) {
    MARKFUNCTION(0x80010BFC);

    if (!obstacle || !g_ai) {
        return;
    }

    tagCollisionSphere sphere = { COLLISION_SPHERE_RADIUS };

    Obstacle* node = static_cast<Obstacle*>(g_ai->moveList.head);
    while (node) {
        Obstacle* next = static_cast<Obstacle*>(node->next);

        if (node->thingType == AITypes::TT_DESTRUCTIBLE_DP) {
            DestructibleThing* destructible = static_cast<DestructibleThing*>(node);

            if ((destructible->flags & TF_MODEL_CREATED) != 0
                && g_director->scriptState == 0
                && destructible->aliveFlag
                && destructible->destroyFlag == 0) {
                if (CheckStaticBoxSphereCollision(
                        obstacle->pos,
                        obstacle->collBox,
                        obstacle->orientation.y,
                        destructible->pos,
                        sphere)) {
                    destructible->Destroy();
                }
            }
        }

        node = next;
    }
}

void DestructibleThing::HandleObstacleCollision(Obstacle* other) {
    (void)other;
    HandleObstacleDestructibleThingCollision(this);
}

s32 DestructibleThing::GetFloorMaterial() const {
    MARKFUNCTION(0x80010CF8);

    CSoundMaterial material = SMAT_WOOD;
    if (sound) {
        sound->GetMaterial(&material);
    }
    return static_cast<s32>(material);
}

void DestructibleThing::GenerateItem() {
    MARKFUNCTION(0x800100F0);

    if (!g_ai || !itemModelName) {
        return;
    }

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

    Thing* generated = g_ai->AddThingNoTagList(nullptr, generateItemType, &center, nullptr, itemModelName, &tempRoot);
    tempRoot.DeallocatePermanentAttributeArray();

    generated->pos = pos;
    generated->orientation = orientation;

    if (generateItemType == AITypes::TT_COLLECTIBLE_OBJ) {
        tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
        localBox.minX = -0x100;
        localBox.maxX = 0x100;
        localBox.minY = -0x100;
        localBox.maxY = 0x100;
        localBox.minZ = -0x100;
        localBox.maxZ = 0x100;

        static_cast<Obstacle*>(generated)->SetCollisionBox(localBox);
    }
}

void DestructibleThing::Destroy() {
    MARKFUNCTION(0x80010000);

    if (destroyFlag || !aliveFlag) {
        return;
    }

    if (generateItemType) {
        GenerateItem();
    }

    if (effectHash) {
        LVector center;
        FillBoxCentre(center, pos, orientation, collBox);
        GEffect_Create(effectHash, &center, nullptr, nullptr, 0, 0, effectParam);
    }

    if (sound) {
        sound->Smash();
    }

    SetCollisionBox(INVALID_COLLISION_BOX);
    aliveFlag = 0;
    health = 0;
}
