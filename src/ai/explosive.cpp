#include "ai/explosive.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "gen/colvol.h"
#include "ai/obstacle_shared.h"

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
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
    savedCollBox = localBox;
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
}

void Explosive::Think() {
    MARKFUNCTION(0x80013A48);
}

void Explosive::Draw() {
    MARKFUNCTION(0x80013A18);
    Obstacle::Draw();
}

void Explosive::UpdatePosition() {
}


void Explosive::CheckObstacleCollisions() {
    MARKFUNCTION(0x8001333C);
}

void Explosive::ExplodeThing() {
    MARKFUNCTION(0x80013554);
}

void Explosive::MovePassengers() {
    MARKFUNCTION(0x80013C0C);
    MovePassengersBasic();
}

void Explosive::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80013C2C);
}

void Explosive::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80013CA8);
}

void Explosive::HandleObstacleCollision(Obstacle* other) {
    MARKFUNCTION(0x80013E40);
}

void Explosive::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x80013ECC);
}

void Explosive::Trigger() {
    MARKFUNCTION(0x80013BD0);
}
