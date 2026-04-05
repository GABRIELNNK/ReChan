// player.cpp - Player class implementation
// Reversed from PSX C:\CHAN\GAME\SRC\AI\PLAYER.CPP
#include "ai/player.h"
#include "gen/game.h"
#include "gen/control.h"
#include "gen/model.h"
#include "gen/animmat.h"
#include "p3d/p3dmath.h"

// Command bit positions (matching PSX Behaviour output → commandBits)
// PSX _Stand dispatches via ReturnMostSignificant32BitNumber(commandBits):
//   bit 1 → run, bit 2 → jump, bit 3 → dive roll, etc.
static constexpr s32 CB_RUN      = (1 << 1);  // movement input present
static constexpr s32 CB_JUMP     = (1 << 2);  // jump input (Cross / K)

// Movement tuning constants (PSX original values)
// PSX uses a ramping force accumulator (gp+392) that gradually increases from 0
// to runSpeed. AddForce accumulates into DynamicThing::force (80% damped per frame).
static constexpr s32 PLAYER_RUN_FORCE  = 2500;  // per-frame AddForce magnitude
static constexpr s32 PLAYER_JUMP_FORCE = 2000;  // upward velocity on jump
static constexpr s32 PLAYER_AIR_FORCE  = 800;   // air-control force

Player* Player::s_player = nullptr;

// PSX: __6PlayerPC10tagLVector (PLAYER.CPP:1014)
Player::Player(const LVector* initialPos)
    : Humanoid(initialPos, AITypes::TT_PLAYER) {
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

    // Store spawn ground level for simple floor clamping
    jumpReturnHeight = homePos.y;

    SetActionState(AS_INACTIVE_IDLE, 0);
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

    // PSX: Behaviour::Process() reads controller → commandBits + faceAngle
    // PC: read input directly since Behaviour system not yet reversed
    ReadPlayerInput();

    ProcessAction();
    Move();

    // PSX: UpdatePosition is called through handler system for the player.
    // PC: call explicitly to commit homePos → pos each frame.
    UpdatePosition();

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
    flags |= TF_DYNAMIC | TF_BIT5 | TF_BIT3;
    velocity = {};
    contactForce = {};
    turnRate = 5500;
    field616 = 0;
    field620 = 0;
    hitCombo = 0;
    comboTimer = 0;
    playerFlags |= PF_COMBAT_READY;
    field704 = 0;
    field706 = 0;
    field712 = 0;
    field720 = 0;
    field724 = 0;
    field728 = 0;
    lastPos = orientation;

    SetActionState(AS_STAND, 0);
}

// PSX: Move__6Player (PLAYER.CPP:1408)
void Player::Move() {
    MARKFUNCTION(0x80030100);

    Humanoid::Move();

    // PC: simple ground plane collision until collision system is reversed.
    // Clamp homePos.y to ground level when not airborne.
    if (stateDispatch != SD_JUMP && stateDispatch != SD_FALL) {
        if (homePos.y < jumpReturnHeight) {
            homePos.y = jumpReturnHeight;
            velocity.y = 0;
        }
    }
}

// PSX: CreateModel__6PlayerPCc (PLAYER.CPP:1111, 0x8002FD34)
// PSX: creates PlayerModel if not exists, calls Thing::CreateModel (NOT Humanoid::CreateModel),
// stores OriginalSTree_omPlayer global, calls InitBlendPose
void Player::CreateModel(const char* name) {
    MARKFUNCTION(0x8002FD34);

    // PSX: if model == null, create PlayerModel(136)
    if (!model) {
        PlayerModel* pm = new PlayerModel();
        model = pm;
        pm->backPtr = this;
    }

    // PSX: calls Thing::CreateModel directly (skips Humanoid::CreateModel)
    Thing::CreateModel(name);

    // PSX: virtual calls for animation setup (ApplyAnimToModel etc.)
    // Animation system not yet reversed - skip

    // PSX: OriginalSTree_omPlayer = model->drawable->original
    // Used for suit-change system - skip global for now

    // PSX: InitBlendPose - animation blending
    // Not yet reversed - skip

    // PSX: InitSemiTransMode (called via Humanoid path, also needed here)
    Model* m = static_cast<Model*>(model);
    if (m) {
        SModel* sm = static_cast<SModel*>(m);
        sm->InitSemiTransMode();
    }
}

// PSX: SetActionState__6PlayerUll (PLAYER.CPP:1579)
// PSX: 73-case switch. Player adds states for platforming, weapon combos, etc.
// Player-specific stateDispatch indices use virtual overrides of the state handlers.
void Player::SetActionState(u32 state, s32 param) {
    MARKFUNCTION(0x800303BC);
    actionStateFlag = 0;
    flags |= TF_DYNAMIC;

    // Delegate to Humanoid for the core state mapping and preamble
    Humanoid::SetActionState(state, param);
}

// PSX: GetViewSpot__6PlayerP10tagLVectorT1 (PLAYER.CPP:1460)
void Player::GetViewSpot(LVector* outPos, LVector* outTarget) {
    MARKFUNCTION(0x8003027C);

    s32 state = actionState;

    if (outPos) {
        if (state == 18) {
            *outPos = pos;
        } else {
            *outPos = homePos;
        }
    }

    if (outTarget) {
        if (state == 18) {
            *outTarget = pos;
        } else if (state == 23) {
            bool usedAnimMatrix = false;

            if (model) {
                Model* baseModel = static_cast<Model*>(model);
                HumanoidModel* humanoidModel = static_cast<HumanoidModel*>(baseModel);
                if (humanoidModel->animMatrices) {
                    const s32* joint5 = humanoidModel->animMatrices->GetMatrix(5);
                    if (joint5) {
                        outTarget->x = joint5[5];
                        outTarget->y = joint5[6];
                        outTarget->z = joint5[7];
                        usedAnimMatrix = true;
                    }
                }
            }

            if (!usedAnimMatrix) {
                *outTarget = homePos;
                outTarget->y += 450;
            }
        } else {
            *outTarget = homePos;
            outTarget->y += 450;
        }
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

void Player::SetLivesLeft(s32 lives) {
    MARKFUNCTION(0x80033D9C);
    if (lives < 0) {
        lives = 0;
    }
    livesLeft = lives;
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

// PC: reads InputManager state and sets commandBits + faceAngle.
// On PSX this is done by the player's Behaviour::Process() object.
void Player::ReadPlayerInput() {
    commandBits = 0;

    u32 buttons = (u32)g_game->GetControlVal(0);

    // D-pad → movement direction and faceAngle
    s32 dx = 0, dz = 0;
    if (buttons & PsxPad::Up)    dz += 1;
    if (buttons & PsxPad::Down)  dz -= 1;
    if (buttons & PsxPad::Right) dx += 1;
    if (buttons & PsxPad::Left)  dx -= 1;

    if (dx != 0 || dz != 0) {
        commandBits |= CB_RUN;
        // atan2(dx, dz) → PSX binary angle (0=+Z, 0x4000=+X)
        f32 rad = atan2((f32)dx, (f32)dz);
        faceAngle = ((s32)(rad * P3D_RAD_TO_ANGLE)) & 0xFFFF;
    }

    // Cross button → jump
    if (buttons & PsxPad::Cross) commandBits |= CB_JUMP;
}

void Player::LoadPlayerTauntResponse(Humanoid* /*target*/) {
    MARKFUNCTION(0x80034290);
}

void Player::PlayPlayerTauntResponse() {
    MARKFUNCTION(0x8003431C);
}

// Action state handler stubs
void Player::_InactiveIdle() { MARKFUNCTION(0x8003123C); }

// PSX: _Stand__6Player (PLAYER.CPP:2134) — 1832 bytes, 117 blocks
// Checks commandBits for movement/jump/combat transitions.
// Full PSX version has combat combo dispatching; PC implements core movement.
void Player::_Stand() {
    MARKFUNCTION(0x80031350);
    stateTimer++;

    // Movement input → run
    if (commandBits & CB_RUN) {
        SetActionState(AS_RUN, 0);
        return;
    }

    // Jump input → jump
    if (commandBits & CB_JUMP) {
        SetActionState(AS_JUMP, 0);
        return;
    }
}

void Player::_Flip() { MARKFUNCTION(0x80031A78); }

// PSX: _Jump__6Player (PLAYER.CPP:2539) — sets upward velocity, air control
void Player::_Jump() {
    MARKFUNCTION(0x80031C68);

    // First frame: set upward velocity
    if (stateTimer == 0) {
        DoJump(PLAYER_JUMP_FORCE);
    }
    stateTimer++;

    // Air control: apply reduced force in input direction
    if (commandBits & CB_RUN) {
        FaceAngleY(faceAngle, 1);
        f32 rad = (f32)orientation.y * P3D_ANGLE_TO_RAD;
        SVector dir;
        dir.x = (s16)(sin(rad) * 4096.0f);
        dir.y = 0;
        dir.z = (s16)(cos(rad) * 4096.0f);
        dir.pad = 0;
        AddForce(PLAYER_AIR_FORCE, &dir);
    }

    // Transition to fall when velocity turns negative
    if (stateTimer > 1 && velocity.y <= 0) {
        stateDispatch = SD_FALL;
    }
}

// PSX: _Fall__6Player (PLAYER.CPP:3173) — gravity fall, landing check
void Player::_Fall() {
    MARKFUNCTION(0x80032444);
    stateTimer++;

    // Air control
    if (commandBits & CB_RUN) {
        FaceAngleY(faceAngle, 1);
        f32 rad = (f32)orientation.y * P3D_ANGLE_TO_RAD;
        SVector dir;
        dir.x = (s16)(sin(rad) * 4096.0f);
        dir.y = 0;
        dir.z = (s16)(cos(rad) * 4096.0f);
        dir.pad = 0;
        AddForce(PLAYER_AIR_FORCE, &dir);
    }

    // Check for landing: homePos.y at or below ground level
    if (homePos.y <= jumpReturnHeight) {
        homePos.y = jumpReturnHeight;
        Land();
        SetActionState(AS_STAND, 0);
    }
}

void Player::_HardFall() { MARKFUNCTION(0x800324E8); }
void Player::_HardLand() { MARKFUNCTION(0x80032560); }

// PSX: _Run__6Player (PLAYER.CPP:3004) — 1148 bytes, 73 blocks
// Applies locomotion force in facing direction, checks transitions.
void Player::_Run() {
    MARKFUNCTION(0x800325CC);
    stateTimer++;

    // Jump input takes priority
    if (commandBits & CB_JUMP) {
        SetActionState(AS_JUMP, 0);
        return;
    }

    // No movement input → stop
    if (!(commandBits & CB_RUN)) {
        SetActionState(AS_STAND, 0);
        return;
    }

    // Turn toward input direction (gradual, limited by turnRate)
    FaceAngleY(faceAngle, 1);

    // Build direction vector from current orientation angle
    // PSX binary angles: 0=+Z, 0x4000=+X, 0x8000=-Z, 0xC000=-X
    f32 rad = (f32)orientation.y * P3D_ANGLE_TO_RAD;
    SVector dir;
    dir.x = (s16)(sin(rad) * 4096.0f);
    dir.y = 0;
    dir.z = (s16)(cos(rad) * 4096.0f);
    dir.pad = 0;

    // Apply locomotion force (PSX: AddForce with gp+392 accumulated magnitude)
    AddForce(PLAYER_RUN_FORCE, &dir);
}
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
