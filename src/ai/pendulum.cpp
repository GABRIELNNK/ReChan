#include "ai/pendulum.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "ai/obstacle_shared.h"

Pendulum::Pendulum(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80024DF0);
    field116 = 0;
    field120 = 0;
    field160 = 0;
    field176 = 0;
}

Pendulum::~Pendulum() {
    MARKFUNCTION(0x80024E38);
}

void Pendulum::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80024EA0);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void Pendulum::CreateModel(const char* name) {
    MARKFUNCTION(0x8002520C);
    Obstacle::CreateModel(name);
}


void Pendulum::DeleteModel() {
    MARKFUNCTION(0x8002525C);
    Obstacle::DeleteModel();
}

void Pendulum::Reset() {
    MARKFUNCTION(0x800252AC);
}

void Pendulum::Think() {
    MARKFUNCTION(0x800252B4);
}

void Pendulum::UpdatePosition() {
    MARKFUNCTION(0x800256C4);
}

void Pendulum::Draw() {
    MARKFUNCTION(0x800256CC);
    Obstacle::Draw();
}

void Pendulum::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800256EC);
}

void Pendulum::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80025720);
}
