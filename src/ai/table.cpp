#include "ai/table.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "gen/common.h"
#include "gen/database.h"
#include "gen/levelmgr.h"
#include "gen/colvol.h"
#include "p3d/hash.h"

DynamicObstacle::DynamicObstacle(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80013F24);
    effectHash = 0;
    aliveFlag = 1;
}

DynamicObstacle::~DynamicObstacle() {
    MARKFUNCTION(0x80013FD0);
}

void DynamicObstacle::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80013FFC);

    // Copy orientation from DBRoot (fields 40,44,48)
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    Obstacle::AnalyzeMesh(root);

    // Fill collision box from attrib 5 (geo name)
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);

    // PSX: *(_WORD*)(a1+58) = 1 → maxHealth = 1
    maxHealth = 1;

    // attrib 20 = effect name → effectHash; attrib 21 = effect variant flag
    const DBAttrib* a20 = root->FindAttrib(20);
    if (a20 && a20->strValue) {
        effectHash = p3dHash(a20->strValue);
        effectParam = root->FindAttrib(21) ? 0x80000000u : 0u;
    } else {
        // PSX default: 106729104 = p3dHash of default effect name
        effectHash = 106729104u;
        effectParam = 0x80000000u;
    }
}

void DynamicObstacle::CreateModel(const char* name) {
    MARKFUNCTION(0x80014120);
    Obstacle::CreateModel(name);
}

void DynamicObstacle::DeleteModel() {
    MARKFUNCTION(0x80014140);
    Obstacle::DeleteModel();
}

void DynamicObstacle::Reset() {
    MARKFUNCTION(0x80014148);
}

void DynamicObstacle::Think() {
    MARKFUNCTION(0x80014158);
    Move();
    MovePassengersBasic();
}

void DynamicObstacle::Move() {
    MARKFUNCTION(0x80014198);
}

void DynamicObstacle::Draw() {
    MARKFUNCTION(0x80014484);
    Obstacle::Draw();
}

void DynamicObstacle::AddForce(s32 damage, const LVector* matrix) {
    MARKFUNCTION(0x800144B4);
}

void DynamicObstacle::AddMomentVector(const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x8001456C);
}

void DynamicObstacle::MovePassengers() {
    MARKFUNCTION(0x80014738);
    MovePassengersBasic();
}

void DynamicObstacle::Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x80014758);
}

void DynamicObstacle::UpdatePosition() {
    MARKFUNCTION(0x80014818);
}

void DynamicObstacle::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80014820);
}

void DynamicObstacle::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80014828);
}

void DynamicObstacle::HandleObjectInterAction(Humanoid* hum) {
    MARKFUNCTION(0x80014934);
}

void DynamicObstacle::Destroy() {
    MARKFUNCTION(0x80014BF8);
}

void DynamicObstacle::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {
    MARKFUNCTION(0x80014CA8);
}

void DynamicObstacle::HandleEnvironmentCollision(const LVector& normal) {
    MARKFUNCTION(0x80014CB0);
}

bool DynamicObstacle::CareAboutAttack() const {
    MARKFUNCTION(0x80015470);
    return false;
}


Table::Table(const LVector* pos, u16 type)
    : DynamicObstacle(pos, type) {
    MARKFUNCTION(0x80015190);
    aliveFlag = 1;
}

Table::~Table() {
    MARKFUNCTION(0x800151C8);
}

void Table::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800151F0);
    DynamicObstacle::AnalyzeMesh(root);
}

void Table::CreateModel(const char* name) {
    MARKFUNCTION(0x80015210);
    DynamicObstacle::CreateModel(name);
}

void Table::DeleteModel() {
    MARKFUNCTION(0x80015230);
    DynamicObstacle::DeleteModel();
}

void Table::Think() {
    MARKFUNCTION(0x80015238);
    DynamicObstacle::Think();
}

void Table::UpdatePosition() {
    MARKFUNCTION(0x80015258);
}

void Table::Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x80015260);
    DynamicObstacle::Throw(a, b, matrix, contactPos);
}

void Table::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800152A0);
}

void Table::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800152A8);
    DynamicObstacle::HandleHumanoidCollision(hum);
}

Chair::Chair(const LVector* pos, u16 type)
    : DynamicObstacle(pos, type) {
    MARKFUNCTION(0x8001531C);
    aliveFlag = 1;
}

Chair::~Chair() {
    MARKFUNCTION(0x80015354);
}

void Chair::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001537C);
    DynamicObstacle::AnalyzeMesh(root);
}

void Chair::CreateModel(const char* name) {
    MARKFUNCTION(0x8001539C);
    DynamicObstacle::CreateModel(name);
}

void Chair::DeleteModel() {
    MARKFUNCTION(0x800153BC);
    DynamicObstacle::DeleteModel();
}

void Chair::Think() {
    MARKFUNCTION(0x800153C4);
    DynamicObstacle::Think();
}

void Chair::UpdatePosition() {
    MARKFUNCTION(0x800153E4);
}

void Chair::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800153EC);
}

void Chair::Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x800153F4);
    DynamicObstacle::Throw(a, b, matrix, contactPos);
}

void Chair::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80015434);
    DynamicObstacle::HandleHumanoidCollision(hum);
}
