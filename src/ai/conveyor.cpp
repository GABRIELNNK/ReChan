#include "ai/conveyor.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "ai/obstacle_shared.h"

Conveyor::Conveyor(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001BE34);
    field116 = 0;
    field120 = 0;
    beltSpeed = 10;
    field128 = 0;
    field132 = 0;
    field136 = 0;
    field140 = 0;
    field144 = 0;
}

Conveyor::~Conveyor() {
    MARKFUNCTION(0x8001BE98);
}

void Conveyor::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001BEE4);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
    // TODO: belt-speed and direction attrib reads
}

void Conveyor::CreateModel(const char* name) {
    MARKFUNCTION(0x8001C1DC);
    Obstacle::CreateModel(name);
}

void Conveyor::DeleteModel() {
    MARKFUNCTION(0x8001C214);
    Obstacle::DeleteModel();
}

void Conveyor::Reset() {
    MARKFUNCTION(0x8001C240);
}

void Conveyor::Think() {
    MARKFUNCTION(0x8001C260);
}

void Conveyor::UpdatePosition() {
    MARKFUNCTION(0x8001C2BC);
}

void Conveyor::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001C2C4);
}

void Conveyor::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001C2CC);
}
