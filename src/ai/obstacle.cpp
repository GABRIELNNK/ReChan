#include "ai/obstacle.h"
#include "ai/humanoid.h"
#include "gen/ai.h"
#include "gen/config.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "p3d/p3dmath.h"

const LVector ZERO_DELTA_VELOCITY = { 0, 0, 0 };

Obstacle::Obstacle(const LVector* pos, u16 type) : Thing(pos, type) {
    MARKFUNCTION(0x8007CA08);
    collBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    physicalType = 0;
    lightingFlag = 1;
    shadowFlag = 1;
}

Obstacle::~Obstacle() {
    MARKFUNCTION(0x8007CA7C);
}

void Obstacle::Think() {
    MARKFUNCTION(0x8007CDA4);
    Move();
}

void Obstacle::Draw() {
    MARKFUNCTION(0x8007AE04);
    if (model) {
        // PSX: Obstacle::Draw sets up GModel transform from pos/orient
        // and calls GModel::Draw. Simplified for now.
    }
}

void Obstacle::Reset() {
    MARKFUNCTION(0x8007CD9C);
}

void Obstacle::Move() {
    MARKFUNCTION(0x8007CDD4);
}

void Obstacle::UpdatePosition() {
    MARKFUNCTION(0x8007CDDC);
}

void Obstacle::CreateModel(const char* name) {
    MARKFUNCTION(0x8007CC64);
}

void Obstacle::DeleteModel() {
    MARKFUNCTION(0x8007CD94);
}

void Obstacle::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8007CBC4);
    Thing::AnalyzeMesh(root);

    const DBAttrib* a = root->FindAttrib(50);
    if (a) {
        physicalType = (u8)a->value;
    }

    if (thingType == 436) {
        lightingFlag = 0;
    } else {
        const DBAttrib* a51 = root->FindAttrib(51);
        if (a51 && a51->value == 2) {
            lightingFlag = 0;
        }
    }

    a = root->FindAttrib(52);
    if (a) {
        shadowFlag = 0;
    }
}

void Obstacle::FillSphere(tSphere& sphere) const {
    MARKFUNCTION(0x8007CE08);
}

void Obstacle::HandlePickupCollision(Thing* pickup) {
}

void Obstacle::HandleHumanoidCollision(Humanoid* hum) {
}

void Obstacle::Trigger() {
    MARKFUNCTION(0x8007CE40);
}

void Obstacle::TriggerByName(Thing* source, const char* name, const char* param) {
    MARKFUNCTION(0x8007CE94);
}

void Obstacle::ExplosiveTrigger(s32 damage, const char* name) {
    MARKFUNCTION(0x8007CE60);
}

const LVector* Obstacle::GetDeltaVelocity() const {
    MARKFUNCTION(0x8007CE70);
    return &ZERO_DELTA_VELOCITY;
}

bool Obstacle::CareAboutAttack() const {
    return false;
}

void Obstacle::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {
}

s32 Obstacle::GetFloorMaterial() const {
    MARKFUNCTION(0x8007D0B4);
    return 0;
}

s32 Obstacle::GetObstacleFloorHeight(const LVector& pos) const {
    MARKFUNCTION(0x8007D0CC);
    return 1;
}

s32 Obstacle::GetPhysical() const {
    MARKFUNCTION(0x8007D048);
    // PSX: switch on physicalType
    // 0: return 0  (no floor)
    // 3: return 0  (no floor)
    // default: return non-zero based on type
    if (physicalType == 0 || physicalType == 3) {
        return 0;
    }
    return 1;
}

void Obstacle::SetCollisionBox(const tagCollisionBox& box) {
    MARKFUNCTION(0x8007BE24);
    collBox = box;
    SetCollisionBoxExtent(collBox);
}

// PSX: Obstacle::HandleHumanoidObstacleCollision (OBSTACLE.CPP:1301, 0x8007C178)
// Broad-phase: extent + humanoid radius vs homePos distance.
// Then calls HandleHumanoidCollision via vtable for passing obstacles.
void Obstacle::HandleHumanoidObstacleCollision(Humanoid* hum) {
    MARKFUNCTION(0x8007C178);

    if (!g_ai) {
        return;
    }

    DynamicThing* dt = (DynamicThing*)hum;

    for (ccMinNode* node = g_ai->moveList.head; node != nullptr;) {
        ccMinNode* next = node->next;
        Obstacle* obs = static_cast<Obstacle*>((Thing*)node);

        if (obs->flags & TF_MODEL_CREATED) {
            // PSX broad-phase: extent + humanoid collBboxMin.x
            s32 threshold = (s32)obs->collBox.extent + hum->collBboxMin.x;

            // PSX: abs(homePos.x - obs.pos.x) < threshold
            s32 dx = dt->homePos.x - obs->pos.x;
            if (dx < 0) dx = -dx;

            // PSX: abs(homePos.z - obs.pos.z) < threshold
            s32 dz = dt->homePos.z - obs->pos.z;
            if (dz < 0) dz = -dz;

            if (dx < threshold && dz < threshold) {
                // PSX Y check: homePos.y - obs.pos.y in range [collBox.minY - collBboxMin.z, collBox.maxY - collBboxMin.y]
                s32 dy = dt->homePos.y - obs->pos.y;
                s32 yLow = (s32)obs->collBox.minY - hum->collBboxMin.z;
                s32 yHigh = (s32)obs->collBox.maxY - hum->collBboxMin.y;
                if (dy >= yLow && yHigh >= dy) {
                    obs->HandleHumanoidCollision(hum);
                }
            }
        }

        node = next;
    }
}
