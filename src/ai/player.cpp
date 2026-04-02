// player.cpp - Player class implementation stubs
// Reversed from PSX C:\CHAN\GAME\SRC\AI\PLAYER.CPP
#include "ai/player.h"

Player* Player::s_player = nullptr;

// PSX: __6PlayerPC10tagLVector (PLAYER.CPP:1014)
Player::Player(const LVector* initialPos)
    : Humanoid(initialPos, AI::TT_PLAYER) {
    MARKFUNCTION(0x8002FA80);

    initialActiveRadius = 200;
    attackRange = 3000;
    comboCount = 1;

    // PSX: embedded sub-object init (+636..+688)
    for (int i = 0; i < 13; i++) {
        subObject[i] = 0;
    }
    subObject[7] = -1; // invalid ID sentinel at PSX +664
    subVtable = nullptr;

    hitCombo = 0;
    comboTimer = 0;
    playerFlags = 0;
    field704 = 0;
    field706 = 0;
    field712 = 0;
    field720 = 0;
    field724 = 0;
    field728 = 0;
    actionStateFlag = 0;
    animCallbackData = 0;
    animCallbackVtable = nullptr;
    currentAnimEnum = 0;
    animLoadState = 0;

    // PSX: gp+3432 = this (global player pointer)
    s_player = this;

    SetActionState(0, 0);
}

// PSX: _._6Player (PLAYER.CPP:1050)
Player::~Player() {
    MARKFUNCTION(0x8002FBC0);
    if (s_player == this) {
        s_player = nullptr;
    }
}

// PSX: Think__6Player (PLAYER.CPP:1155)
void Player::Think() {
    MARKFUNCTION(0x8002FE30);
    // PSX: CHumanoidSound think, encounter check, behaviour process,
    // ProcessAction, Move, combo tracking, input read
    PlayerSingleEncounterCheak();
    ProcessAction();
    Move();

    // Combo tracking
    if (hitCombo < 3) {
        // not enough for combo
    }
    else if (comboTimer < 1800) {
        comboTimer++;
    }
    else {
        hitCombo = 0;
        comboTimer = 0;
    }
}

// PSX: Reset__6Player (PLAYER.CPP:1056)
void Player::Reset() {
    MARKFUNCTION(0x8002FC24);
    Humanoid::Reset();

    stateCounter = 100;
    activeRadius = 200;
    flags |= 0x0800 | 0x0028;
    velocity = {};
    contactForce = {};
    turnRate = 5500;
    field616 = 0;
    field620 = 0;
    hitCombo = 0;
    comboTimer = 0;
    playerFlags |= 0x04;
    field704 = 0;
    field706 = 0;
    field712 = 0;
    field720 = 0;
    field724 = 0;
    field728 = 0;
    lastPos = orientation;

    SetActionState(1, 0);
}

// PSX: Move__6Player (PLAYER.CPP:1408)
void Player::Move() {
    MARKFUNCTION(0x80030100);
    Humanoid::Move();
}

// PSX: CreateModel__6PlayerPCc (PLAYER.CPP:1111)
void Player::CreateModel(const char* name) {
    MARKFUNCTION(0x8002FD34);
    // PSX: CharacterManager::LoadCharacter, set up animation matrices, etc.
    Humanoid::CreateModel(name);
}

// PSX: SetActionState__6PlayerUll (PLAYER.CPP:1579)
void Player::SetActionState(u32 state, s32 param) {
    MARKFUNCTION(0x800303BC);
    actionStateFlag = 0;
    flags |= 0x0800;

    // PSX: giant switch on state (73 cases)
    // For now, delegate to Humanoid base
    Humanoid::SetActionState(state, param);
}

// PSX: GetViewSpot__6PlayerP10tagLVectorT1 (PLAYER.CPP:1460)
void Player::GetViewSpot(LVector* outPos, LVector* outTarget) {
    MARKFUNCTION(0x8003027C);
    if (outPos) {
        *outPos = homePos;
    }
    if (outTarget) {
        *outTarget = homePos;
        outTarget->y += 450;
    }
}

// PSX: SignalEnemyGetUp__6Player (PLAYER.CPP:1382)
void Player::SignalEnemyGetUp() {
    MARKFUNCTION(0x800300B0);
    hitCombo++;
}

void Player::DoJump() {
    MARKFUNCTION(0x80030120);
    jumpReturnHeight = homePos.y;
}

void Player::DoJump(s32 height) {
    MARKFUNCTION(0x800301D8);
    jumpReturnHeight = homePos.y;
    velocity.y = height;
}

void Player::FallingPhysics() {
    MARKFUNCTION(0x80032368);
}

void Player::CheckForLanding() {
    MARKFUNCTION(0x80033C00);
}

void Player::OnCheckpoint() {
    MARKFUNCTION(0x80033D0C);
    homePos = pos;
}

void Player::SetLivesLeft(s32 /*lives*/) {
    MARKFUNCTION(0x80033D9C);
}

void Player::SignalEnemyDead(Humanoid* /*enemy*/) {
    MARKFUNCTION(0x8003431C);
}

void Player::EnterCombatCombo() {
    MARKFUNCTION(0x800343D4);
}

void Player::LoadCombatDialog() {
    MARKFUNCTION(0x800343F4);
}

void Player::PlayCombatKnockDownDialog(s32 /*damageType*/) {
    MARKFUNCTION(0x800345B8);
}

void Player::HandleHitShock(s32 /*damageType*/) {
    MARKFUNCTION(0x800345B8);
}

void Player::PlayCombatThrowDialog() {
    MARKFUNCTION(0x80034618);
}

void Player::PlayerSingleEncounterCheak() {
    MARKFUNCTION(0x80034210);
}

void Player::LoadPlayerTauntResponse(Humanoid* /*target*/) {
    MARKFUNCTION(0x80034290);
}

void Player::PlayPlayerTauntResponse() {
    MARKFUNCTION(0x8003431C);
}

// Action state handler stubs
void Player::_InactiveIdle() { MARKFUNCTION(0x8003123C); }
void Player::_Stand() { MARKFUNCTION(0x80031350); }
void Player::_Flip() { MARKFUNCTION(0x80031A78); }
void Player::_Jump() { MARKFUNCTION(0x80031C68); }
void Player::_Fall() { MARKFUNCTION(0x80032444); }
void Player::_HardFall() { MARKFUNCTION(0x800324E8); }
void Player::_HardLand() { MARKFUNCTION(0x80032560); }
void Player::_Run() { MARKFUNCTION(0x800325CC); }
void Player::_Push() { MARKFUNCTION(0x80032A48); }
void Player::_PushObject() { MARKFUNCTION(0x80032B80); }
void Player::_Teetering() { MARKFUNCTION(0x80032C70); }
void Player::_WallJump() { MARKFUNCTION(0x80032D8C); }
void Player::_Collapse() { MARKFUNCTION(0x80032EB0); }
void Player::_DoStand() { MARKFUNCTION(0x80032F48); }
void Player::_HorizontalPoleSwing() { MARKFUNCTION(0x80032F8C); }
void Player::_LedgeLatch() { MARKFUNCTION(0x8003352C); }
void Player::_LedgePullup() { MARKFUNCTION(0x800337A8); }
void Player::_Dead() { MARKFUNCTION(0x80033858); }
void Player::_SlopeSlide() { MARKFUNCTION(0x8003389C); }
void Player::_Straif() { MARKFUNCTION(0x80033FF8); }
void Player::_LadderDismount() { MARKFUNCTION(0x80033DB8); }
void Player::_ClimbLadder() { MARKFUNCTION(0x80033DD8); }
void Player::_TableRoll() { MARKFUNCTION(0x80033DF8); }
