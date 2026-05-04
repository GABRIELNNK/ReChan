#include "ai/untouchable.h"
#include "gen/common.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "ai/obstacle_shared.h"

Untouchable::Untouchable(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x800A6330);
    // PSX: allocates a ParticleSystemMgr (28 bytes) and stores at +116.
    particleMgr = nullptr;
    soundPtr = nullptr;
}

Untouchable::~Untouchable() {
    MARKFUNCTION(0x800A6380);
}

void Untouchable::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800A63DC);
    Obstacle::AnalyzeMesh(root);

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    FillCollisionBox(localBox, *static_cast<const DBVolume*>(root));
    SetCollisionBox(localBox);

    const DBAttrib* attrib = root->FindAttrib(6);
    damageType = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 2;

    attrib = root->FindAttrib(7);
    damageValue = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 3;

}

void Untouchable::CreateModel(const char* name) {
    MARKFUNCTION(0x800A64C8);
    (void)name;
    flags |= TF_MODEL_CREATED;
}

void Untouchable::DeleteModel() {
    MARKFUNCTION(0x800A64DC);
    flags &= ~TF_MODEL_CREATED;
}

void Untouchable::Reset() {
    MARKFUNCTION(0x800A64F0);
    field148 = 0;
    lastHitCounter = 0;
    countdownTimer = damageValue;
    // InitMgr(FindParticleSystem(60675069)) and ReleaseSound.
}

void Untouchable::Think() {
    MARKFUNCTION(0x800A6540);

    if (countdownTimer <= 0) {
        countdownTimer = damageValue;
    }
    countdownTimer -= 1;

    if (!field148) {
        return;
    }

    // particle update/purge and sound update/release. Deferred.
}
