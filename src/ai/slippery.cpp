#include "ai/slippery.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "ai/obstacle_shared.h"

SlipperyFloor::SlipperyFloor(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x800122C4);
}

SlipperyFloor::~SlipperyFloor() {
    MARKFUNCTION(0x80012304);
}

void SlipperyFloor::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80012390);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void SlipperyFloor::CreateModel(const char* name) {
    MARKFUNCTION(0x80012454);
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
}

void SlipperyFloor::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800124AC);
}

void SlipperyFloor::DoTrailEffect() {
    MARKFUNCTION(0x800125A4);
}
