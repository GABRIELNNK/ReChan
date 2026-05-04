#include "ai/blast.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "ai/obstacle_shared.h"

Blast::Blast(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80015AB0);
}


Blast::~Blast() {
    MARKFUNCTION(0x80015AF8);
}

void Blast::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80015B54);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void Blast::CreateSound() {
    MARKFUNCTION(0x80016284);
}

void Blast::UpdateSound() {
    MARKFUNCTION(0x800162E4);
}

void Blast::ReleaseSound() {
    MARKFUNCTION(0x8001631C);
}

void Blast::CreateModel(const char* name) {
    MARKFUNCTION(0x80016368);
    Obstacle::CreateModel(name);
}

void Blast::DeleteModel() {
    MARKFUNCTION(0x8001639C);
    Obstacle::DeleteModel();
}

void Blast::Reset() {
    MARKFUNCTION(0x800163C8);
}

void Blast::Activate() {
    MARKFUNCTION(0x800164A4);
}

void Blast::Deactivate() {
    MARKFUNCTION(0x800164C4);
}

void Blast::Think() {
    MARKFUNCTION(0x80016528);
}

void Blast::Trigger() {
    MARKFUNCTION(0x80016A10);
}

void Blast::Draw() {
    MARKFUNCTION(0x80016ACC);
    Obstacle::Draw();
}

void Blast::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80016B3C);
}

void Blast::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80016B44);
}
