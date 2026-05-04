#include "ai/crusher.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "ai/obstacle_shared.h"

Crusher::Crusher(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001F6A4);
    field116 = 0;
    field120 = 0;
    field160 = 0;
    field168 = 1;
    field172 = 0;
    aliveFlag = 0;
}

Crusher::~Crusher() {
    MARKFUNCTION(0x8001F6F8);
}

void Crusher::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001F760);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
    // TODO
}

void Crusher::CreateModel(const char* name) {
    MARKFUNCTION(0x8001F924);
    Obstacle::CreateModel(name);
}

void Crusher::DeleteModel() {
    MARKFUNCTION(0x8001F980);
    Obstacle::DeleteModel();
}

void Crusher::Reset() {
    MARKFUNCTION(0x8001F9D0);
}

void Crusher::Think() {
    MARKFUNCTION(0x8001F9F0);
}

void Crusher::UpdatePosition() {
    MARKFUNCTION(0x8001FAE0);
}

void Crusher::Draw() {
    MARKFUNCTION(0x8001FAE8);
    Obstacle::Draw();
}

void Crusher::Move() {
    MARKFUNCTION(0x8001FB08);
}

void Crusher::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001FC58);
}

void Crusher::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001FC98);
}
