// humanoid.cpp - Humanoid class implementation stubs
// Reversed from PSX C:\CHAN\GAME\SRC\AI\HUMANOID.CPP
#include "ai/humanoid.h"

// PSX: __8HumanoidPC10tagLVectorUs (HUMANOID.CPP:350)
Humanoid::Humanoid(const LVector* initialPos, u16 type)
    : DynamicThing(initialPos, type) {
    MARKFUNCTION(0x80062A34);

    actionStateA = -1;
    actionStateB = -1;
    collBboxMin = {175, 0, 768};
    collBboxMax = {175, 0, 768};
    field328 = 0;
    combatFlag = 0;
    turnRate = 2730;
    field344 = 0;
    idleTimer = 22;
    runSpeed = 8;
    distantTargetRange = 16000;
    stateCounter = 100; // PSX: this+52 = 100 for humanoids
    field424 = 0;
    field428 = 0;
    field432 = 0;
    field466 = 0;
    field468 = 0;
    comboCount = 1;
    animControl = 0;
    field528 = 0;
    attackRange = 3000; // PSX: set in constructor
    spawnCount = 1;
    field408 = -1;
    field484 = 0;
    field488 = 0;
    field364 = 0;
    field256 = 0;
    field260 = 0;
    field500 = 0;
    field504 = 0;
    field496 = 0;
    field532 = 0;
    field384 = 0;
    field388 = 0;
    field392 = 0;
    field396 = 0;
    field400 = 0;
    field404 = 0;
    field412 = 0;
    field416 = 0;
    field316 = 0;
    field320 = 0;
    field324 = 0;
    field452 = 0;
    field436 = 0;
    field216 = 0;
    soundHandle = 0;
    soundParam = 0;
    punchDir = 0;
    kickDir = 0;
    comboDir = 0;
    // PSX: activeRadius/initialActiveRadius set to 100 for humanoids
    activeRadius = 100;
    initialActiveRadius = 100;
}

// PSX: _._8Humanoid (HUMANOID.CPP:490)
Humanoid::~Humanoid() {
    MARKFUNCTION(0x80062C58);
    // PSX: KillDialog, delete sound, delete behaviour, etc.
    behaviour = nullptr;
    fightingSystem = nullptr;
    defaultFightingSystem = nullptr;
    humanoidData = nullptr;
    trails = nullptr;
}

// PSX: Think__8Humanoid (HUMANOID.CPP:1133)
void Humanoid::Think() {
    MARKFUNCTION(0x80063808);
    // PSX: ProcessControl(), ProcessAction(), HandleAnimationControl()
    ProcessControl();
    ProcessAction();
    UpdatePosition();
}

// PSX: Draw__8Humanoid (HUMANOID.CPP:1280)
void Humanoid::Draw() {
    MARKFUNCTION(0x80063A88);
    // PSX: complex draw with animation frame, shadow, etc.
    Thing::Draw();
}

// PSX: Reset__8Humanoid (HUMANOID.CPP:513)
void Humanoid::Reset() {
    MARKFUNCTION(0x80062DC0);
    DynamicThing::Reset();
    actionStateA = -1;
    actionStateB = -1;
}

// PSX: Activate__8Humanoid (HUMANOID.CPP:760)
void Humanoid::Activate() {
    MARKFUNCTION(0x80063210);
    Thing::Activate();
}

// PSX: Deactivate__8Humanoid (HUMANOID.CPP:776)
void Humanoid::Deactivate() {
    MARKFUNCTION(0x80063270);
    Thing::Deactivate();
}

// PSX: Move__8Humanoid (HUMANOID.CPP:1544)
void Humanoid::Move() {
    MARKFUNCTION(0x80064100);
    DynamicThing::Move();
}

// PSX: CreateModel__8HumanoidPCc (HUMANOID.CPP:795)
void Humanoid::CreateModel(const char* name) {
    MARKFUNCTION(0x800632B4);
    // PSX: CharacterManager::LoadCharacter, set up animation, etc.
    Thing::CreateModel(name);
}

// PSX: DeleteModel__8Humanoid (HUMANOID.CPP:910)
void Humanoid::DeleteModel() {
    MARKFUNCTION(0x80063514);
    Thing::DeleteModel();
}

// PSX: HandleCollision__8HumanoidP5Thingle (HUMANOID.CPP:1997)
void Humanoid::HandleCollision(Thing* other, s32 damage) {
    MARKFUNCTION(0x80064808);
    (void)other;
    (void)damage;
}

// PSX: AnalyzeMesh__8HumanoidP6DBRoot (HUMANOID.CPP:535)
void Humanoid::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80062E54);
    Thing::AnalyzeMesh(root);
}

// PSX: SetActionState__8HumanoidUll (HUMANOID.CPP:2792)
void Humanoid::SetActionState(u32 /*state*/, s32 /*param*/) {
    MARKFUNCTION(0x80065680);
}

// PSX: ProcessAction__8Humanoid (HUMANOID.CPP:2659)
void Humanoid::ProcessAction() {
    MARKFUNCTION(0x8006538C);
}

// PSX: ProcessControl__8Humanoid (HUMANOID.CPP:961)
void Humanoid::ProcessControl() {
    MARKFUNCTION(0x80063660);
}

void Humanoid::FaceThing(Thing* /*target*/, s32 /*immediate*/) {
    MARKFUNCTION(0x80064B98);
}

void Humanoid::FacePoint(const LVector& /*point*/, s32 /*immediate*/) {
    MARKFUNCTION(0x80064BD0);
}

void Humanoid::FindFoe(u32 /*range*/, s32 /*param*/, s32 /*immediate*/) {
    MARKFUNCTION(0x80064F94);
}

void Humanoid::SetTarget(Humanoid* /*target*/) {
    MARKFUNCTION(0x8006511C);
}

void Humanoid::ReleaseTarget() {
    MARKFUNCTION(0x80065200);
}

// Action state handler stubs
void Humanoid::_Stand() { MARKFUNCTION(0x80066CA0); }
void Humanoid::_Run() { MARKFUNCTION(0x800672EC); }
void Humanoid::_Jump() { MARKFUNCTION(0x80067DBC); }
void Humanoid::_Fall() { MARKFUNCTION(0x80067F2C); }
void Humanoid::_Straif() { MARKFUNCTION(0x80067610); }
void Humanoid::_DiveRoll() { MARKFUNCTION(0x80066E3C); }
void Humanoid::_Taunt() { MARKFUNCTION(0x8006710C); }
void Humanoid::_Pause() { MARKFUNCTION(0x80067288); }
void Humanoid::_GotHitHigh() { MARKFUNCTION(0x8006882C); }
void Humanoid::_GotHitMed() { MARKFUNCTION(0x800688B4); }
void Humanoid::_GotHitLow() { MARKFUNCTION(0x800689B4); }
void Humanoid::_Collapse() { MARKFUNCTION(0x80068DD4); }
void Humanoid::_Dead() { MARKFUNCTION(0x800691DC); }
void Humanoid::_SpinBack() { MARKFUNCTION(0x80068B78); }
void Humanoid::_FlyingBack() { MARKFUNCTION(0x80068BC8); }
void Humanoid::_Stunned() { MARKFUNCTION(0x80068AB4); }
void Humanoid::_Throw() { MARKFUNCTION(0x800685A8); }
void Humanoid::_Pickup() { MARKFUNCTION(0x80068508); }
