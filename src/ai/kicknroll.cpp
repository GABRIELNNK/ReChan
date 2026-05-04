#include "ai/kicknroll.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "ai/obstacle_shared.h"

KickNRoll::KickNRoll(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001C398);
    sound = nullptr;
    aliveFlag = 1;
}

KickNRoll::~KickNRoll() {
    MARKFUNCTION(0x8001C3DC);
}

void KickNRoll::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001C444);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void KickNRoll::CreateModel(const char* name) {
    MARKFUNCTION(0x8001C6FC);
    Obstacle::CreateModel(name);
}

void KickNRoll::DeleteModel() {
    MARKFUNCTION(0x8001C750);
    Obstacle::DeleteModel();
}

void KickNRoll::Reset() {
    MARKFUNCTION(0x8001C7A0);
}

void KickNRoll::Think() {
    MARKFUNCTION(0x8001C7B8);
    Move();
    MovePassengers();
}

void KickNRoll::Move() {
    MARKFUNCTION(0x8001C850);
}

void KickNRoll::HandleEnvironmentCollision(const LVector& normal) {
    MARKFUNCTION(0x8001CB60);
}

void KickNRoll::Destroy() {
    MARKFUNCTION(0x8001CEEC);
    aliveFlag = 0;
    // TODO: if effectHash: Create__7GEffect
    extern const tagCollisionBox INVALID_COLLISION_BOX;
    SetCollisionBox(INVALID_COLLISION_BOX);
}

void KickNRoll::UpdatePosition() {
    MARKFUNCTION(0x8001CF74);
}

void KickNRoll::Draw() {
    MARKFUNCTION(0x8001CF7C);
    Obstacle::Draw();
}

void KickNRoll::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001CFAC);
}

void KickNRoll::MovePassengers() {
    MARKFUNCTION(0x8001CFEC);
    MovePassengersBasic();
}

void KickNRoll::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001D178);
}

void KickNRoll::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {
    MARKFUNCTION(0x8001D45C);
}

KnockDown::KnockDown(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001D5D4);
    field116 = 0;
    field168 = 0;
    field172 = 0;
    field184 = 0;
    aliveFlag = 1;
}

KnockDown::~KnockDown() {
    MARKFUNCTION(0x8001D650);
}

void KnockDown::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001D6B8);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void KnockDown::CreateModel(const char* name) {
    MARKFUNCTION(0x8001D8D8);
    Obstacle::CreateModel(name);
}

void KnockDown::Draw() {
    MARKFUNCTION(0x8001D92C);
    Obstacle::Draw();
}

void KnockDown::DeleteModel() {
    MARKFUNCTION(0x8001D9A8);
    Obstacle::DeleteModel();
}

void KnockDown::Reset() {
    MARKFUNCTION(0x8001D9F8);
}

void KnockDown::Think() {
    MARKFUNCTION(0x8001DA48);
}

void KnockDown::Move() {
    MARKFUNCTION(0x8001DB8C);
}


void KnockDown::UpdateCollisionBox() {
    MARKFUNCTION(0x8001DCA0);
}

void KnockDown::UpdatePosition() {
    MARKFUNCTION(0x8001F674);
}

void KnockDown::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001E120);
}

void KnockDown::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001E16C);
}

void KnockDown::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {
    MARKFUNCTION(0x8001E4EC);
}

s32 Stack::LoadDialog(u32 a, u32 b, u32 c) {
    MARKFUNCTION(0x8001E6E0);
    return 0;
}

Stack::Stack(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001E768);
    field124 = 0;
    field128 = 0;
    field132 = 0;
    field136 = 0;
    field140 = 0;
    field156 = 0;
}

Stack::~Stack() {
    MARKFUNCTION(0x8001E7B8);
}

void Stack::Draw() {
    MARKFUNCTION(0x8001E820);
    Obstacle::Draw();
}

void Stack::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001E850);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void Stack::CreateModel(const char* name) {
    MARKFUNCTION(0x8001EA18);
    Obstacle::CreateModel(name);
}

void Stack::DeleteModel() {
    MARKFUNCTION(0x8001EB0C);
    Obstacle::DeleteModel();
}

void Stack::Reset() {
    MARKFUNCTION(0x8001EB5C);
}

void Stack::UpdatePosition() {
    MARKFUNCTION(0x8001EB68);
}

void Stack::Wobble() {
    MARKFUNCTION(0x8001EB70);
}

void Stack::Fall() {
    MARKFUNCTION(0x8001EBE8);
}

void Stack::FinishStack() {
    MARKFUNCTION(0x8001EC24);
}

void Stack::Think() {
    MARKFUNCTION(0x8001EE28);
}

void Stack::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001EEE8);
}

void Stack::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001EF1C);
}

void Stack::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {
    MARKFUNCTION(0x8001F320);
}

void Stack::UpdateCollisionBox() {
    MARKFUNCTION(0x8001F370);
}

void Stack::TriggerStackAnimation() {
    MARKFUNCTION(0x8001F3E8);
}

void Stack::SetupCallbacks() {
    MARKFUNCTION(0x8001F3F4);
}

void Stack::SetupJointPosition(s32 index, LVector pos) {
    MARKFUNCTION(0x8001F55C);
}
