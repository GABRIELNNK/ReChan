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

    Obstacle::AnalyzeMesh(root);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);

    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6) {
        field136 = (s32)a6->value;
    }
    else {
        field136 = 5;
    }

    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7) {
        field140 = (s32)a7->value;
    }
    else {
        field140 = 0;
    }

    const DBAttrib* a8 = root->FindAttrib(8);
    if (a8) {
        field144 = (s32)a8->value;
    }
    else {
        field144 = 0;
    }

    const DBAttrib* a9 = root->FindAttrib(9);
    if (a9) {
        field148 = (s32)a9->value;
    }
    else {
        field148 = 5;
    }

    const DBAttrib* a10 = root->FindAttrib(10);
    if (a10) {
        field152 = (s32)a10->value;
    }
    else {
        field152 = 0;
    }

    const DBAttrib* a11 = root->FindAttrib(11);
    if (a11) {
        field156 = (s32)a11->value;
    }
    else {
        field156 = 0;
    }

    const DBAttrib* a12 = root->FindAttrib(12);
    if (a12) {
        field132 = (s32)a12->value;
    }
    else {
        field132 = 0;
    }

    field160 = field148;
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
