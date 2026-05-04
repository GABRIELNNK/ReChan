#include "ai/generator.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "p3d/p3dmath.h"
#include "ai/obstacle_shared.h"

Generator::Generator(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80010D54);
}

Generator::~Generator() {
    MARKFUNCTION(0x80010DE8);

    if (modelNameBuffer) {
        delete[] static_cast<char*>(modelNameBuffer);
        modelNameBuffer = nullptr;
    }
    if (attribArray) {
        delete[] static_cast<void**>(attribArray);
        attribArray = nullptr;
    }
    dbRoot.DeallocatePermanentAttributeArray();
}

void Generator::GenerateObject(s32 param) {
    MARKFUNCTION(0x80010EEC);
}

void Generator::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80011018);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void Generator::CreateModel(const char* name) {
    MARKFUNCTION(0x8001121C);
}

void Generator::DeleteModel() {
    MARKFUNCTION(0x80011230);
    Obstacle::DeleteModel();
}

void Generator::Reset() {
    MARKFUNCTION(0x80011250);
}

void Generator::Think() {
    MARKFUNCTION(0x80011300);
}

void Generator::UpdatePosition() {
    MARKFUNCTION(0x800113F4);
}

void Generator::Trigger() {
    MARKFUNCTION(0x800113FC);
}

void Generator::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80011404);
}

void Generator::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001140C);
}

EnemyGenerator::EnemyGenerator(const LVector* pos, u16 type)
    : Generator(pos, type) {
}

EnemyGenerator::~EnemyGenerator() {
    MARKFUNCTION(0x80012274);
}

void EnemyGenerator::GenerateObject(s32 param) {
    MARKFUNCTION(0x80011414);
}

void EnemyGenerator::Reset() {
    MARKFUNCTION(0x8001157C);
}

void EnemyGenerator::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001158C);
    orientation.x = 0;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void EnemyGenerator::Think() {
    MARKFUNCTION(0x800117D8);
}

void EnemyGenerator::SetupTargets(const char* pointNames) {
    MARKFUNCTION(0x80011914);
}

ThrowingGenerator::ThrowingGenerator(const LVector* pos, u16 type)
    : Generator(pos, type) {
}

ThrowingGenerator::~ThrowingGenerator() {
    MARKFUNCTION(0x8001224C);
}

void ThrowingGenerator::GenerateObject(s32 param) {
    MARKFUNCTION(0x80011A80);
}

bool ThrowingGenerator::TargetInFOF() const {
    MARKFUNCTION(0x80011DC8);
    const s32 angle = rmATan216((s32)((playerTrackX - pos.x) << 16), (s32)((playerTrackZ - pos.z) << 16));
    return angle >= fofAngleStart && angle <= fofAngleEnd;
}

void ThrowingGenerator::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80011E34);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);
}

void ThrowingGenerator::Reset() {
    MARKFUNCTION(0x800120F4);
}

void ThrowingGenerator::Think() {
    MARKFUNCTION(0x80012104);
}
