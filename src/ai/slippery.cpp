#include "ai/slippery.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/animmat.h"
#include "gen/common.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/model.h"
#include "gen/trail.h"
#include "ai/obstacle_shared.h"

SlipperyFloor::SlipperyFloor(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x800122C4);
}

SlipperyFloor::~SlipperyFloor() {
    MARKFUNCTION(0x80012304);
    delete trailA;
    trailA = nullptr;
    delete trailB;
    trailB = nullptr;
}

void SlipperyFloor::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80012390);

    Obstacle::AnalyzeMesh(root);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    FillCollisionBox(localBox, *static_cast<const DBVolume*>(root));
    SetCollisionBox(localBox);

    trailA = new Trails(4);
    trailB = new Trails(4);
}

void SlipperyFloor::CreateModel(const char* name) {
    MARKFUNCTION(0x80012454);
    (void)name;
    flags |= TF_MODEL_CREATED;
}

void SlipperyFloor::DeleteModel() {
    MARKFUNCTION(0x80012468);
    Thing::DeleteModel();
}

void SlipperyFloor::Reset() {
    MARKFUNCTION(0x80012488);
}

void SlipperyFloor::Think() {
    MARKFUNCTION(0x80012490);
}

void SlipperyFloor::UpdatePosition() {
    MARKFUNCTION(0x80012498);
}

void SlipperyFloor::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800124A0);
    static_cast<DynamicThing*>(pickup)->gravity = FIX16_HALF;
}

void SlipperyFloor::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800124AC);

    if ((hum->flags & TF_ON_GROUND) == 0) {
        return;
    }

    hum->deltaTime = 0x3333;
    hum->gravity = 0xCCC;
    DoTrailEffect(hum);
}

void SlipperyFloor::DoTrailEffect(Humanoid* hum) {
    MARKFUNCTION(0x800125A4);

    if (!trailA || !trailB) {
        return;
    }

    if (hum != Player::s_player) {
        return;
    }

    HumanoidModel* model = static_cast<HumanoidModel*>(hum->model);
    if (!model || !model->animMatrices) {
        return;
    }

    AnimationMatrices* const animMatrices = model->animMatrices;
    static constexpr u32 kTrailColor = 0x00646464;
    static constexpr s32 kTrailSteps = 4;

    LVector trailStart;
    LVector trailEnd;
    animMatrices->GetAttack(3u, trailStart, trailEnd);

    {
        const s32 deltaX = trailEnd.x - trailStart.x;
        const s32 deltaZ = trailEnd.z - trailStart.z;

        trailStart.x = trailEnd.x + deltaZ;
        trailStart.y = trailEnd.y;
        trailStart.z = trailEnd.z - deltaX;

        trailEnd.x = trailEnd.x - deltaZ;
        trailEnd.y = trailEnd.y;
        trailEnd.z = trailEnd.z + deltaX;
    }

    trailA->Add(&trailStart, &trailEnd, kTrailColor, kTrailSteps, nullptr);

    animMatrices->GetAttack(4u, trailStart, trailEnd);

    {
        const s32 deltaX = trailEnd.x - trailStart.x;
        const s32 deltaZ = trailEnd.z - trailStart.z;

        trailStart.x = trailEnd.x + deltaZ;
        trailStart.y = trailEnd.y;
        trailStart.z = trailEnd.z - deltaX;

        trailEnd.x = trailEnd.x - deltaZ;
        trailEnd.y = trailEnd.y;
        trailEnd.z = trailEnd.z + deltaX;
    }

    trailB->Add(&trailStart, &trailEnd, kTrailColor, kTrailSteps, nullptr);
}
