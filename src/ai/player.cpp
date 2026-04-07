// player.cpp - Player class implementation
// Reversed from PSX C:\CHAN\GAME\SRC\AI\PLAYER.CPP
#include "ai/player.h"
#include "gen/game.h"
#include "gen/control.h"
#include "gen/model.h"
#include "gen/animmat.h"
#include "gen/animstruct.h"
#include "gen/charmgr.h"
#include "p3d/p3dmath.h"
#include "pc/log.h"

// Command bit positions - PSX Behaviour action IDs from FindActionRequest/RequestAction
// RequestAction does: commandBits |= (1 << actionID)
// PSX _Stand dispatches by checking bits in priority order.
static constexpr s32 CB_RUN      = (1 << 2);  // bit 2: directional movement
static constexpr s32 CB_JUMP     = (1 << 3);  // bit 3: jump (Cross)
static constexpr s32 CB_GUARD    = (1 << 4);  // bit 4: guard/taunt (Circle)
static constexpr s32 CB_STRAFE   = (1 << 5);  // bit 5: strafe (R1)
static constexpr s32 CB_BACKFLIP = (1 << 6);  // bit 6: backflip (L1)
static constexpr s32 CB_ATTACK   = (1 << 7);  // bit 7: attack (Square)

// Movement tuning constants (PSX original values)
// PSX uses a ramping force accumulator (gp+392) that gradually increases from 0
// to runSpeed. AddForce accumulates into DynamicThing::force (80% damped per frame).
static constexpr s32 PLAYER_RUN_FORCE  = 2500;  // per-frame AddForce magnitude
static constexpr s32 PLAYER_JUMP_FORCE = 17000;  // upward contactForce on jump (gp+460, PSX 0x4268)
static constexpr s32 PLAYER_RUNNING_JUMP_BONUS = -1500; // running jump Y offset (gp+464, PSX 0xFFFFFA24)
static constexpr s32 PLAYER_RUNNING_JUMP_BURST = 2500; // initial horizontal burst (runJumpHold[0], PSX 0x9C4)

// PSX jump parameter tables at 0x800D652C-0x800D6558: [force, gravity, maxFallDivisor]
static const s32 s_runJumpTap[3]       = { 2000,  20480, 20 };  // 0x7D0, 0x5000, 0x14
static const s32 s_runJumpHold[3]      = { 2500,  16000, 23 };  // 0x9C4, 0x3E80, 0x17
static const s32 s_standingJumpTap[3]  = { 150,   4096,  20 };  // 0x96,  0x1000, 0x14
static const s32 s_standingJumpHold[3] = { 150,   4096,  25 };  // 0x96,  0x1000, 0x19

// PSX: gp+492 - hard-fall velocity threshold (velocity.y must be <= this to trigger)
static constexpr s32 g_hardFallThreshold = -8192;

enum PlayerAnimEnum : s32 {
    PLAYER_ANIM_INACTIVE_IDLE = 1,
    PLAYER_ANIM_RUN = 2,
    PLAYER_ANIM_IDLE_UNARMED = 22,
    PLAYER_ANIM_LEDGE_LATCH = 31,
    PLAYER_ANIM_WALL_JUMP_START = 32,  // PSX: 0x20
    PLAYER_ANIM_WALL_JUMP_LAUNCH = 33, // PSX: 0x21
    PLAYER_ANIM_FALL = 39,
    PLAYER_ANIM_HARD_FALL = 40,
    PLAYER_ANIM_TABLE_ROLL = 86,       // PSX: 0x56
    PLAYER_ANIM_TABLE_ROLL_END = 87,
    PLAYER_ANIM_POLE_SWING_BACK = 285,
    PLAYER_ANIM_POLE_SWING_FWD = 286,
    PLAYER_ANIM_FORWARD_FLIP = 287,    // PSX: 0x11F
    PLAYER_ANIM_TURNING_FLIP = 288,    // PSX: 0x120
    PLAYER_ANIM_FLIP_VARIANT = 295,    // PSX: 0x127
};

static bool EnsurePlayerAnimationLoaded(s32 animEnum) {
    if (!g_characterManager || animEnum < 0) {
        return false;
    }
    if (g_characterManager->GetAnimation(0, animEnum)) {
        return true;
    }

    // Current load path is synchronous on PC.
    g_characterManager->LoadAnimationBatch(0, animEnum, nullptr);
    return g_characterManager->GetAnimation(0, animEnum) != nullptr;
}

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
    field712 = nullptr;
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
    field712 = nullptr;
    field720 = 0;
    field724 = 0;
    field728 = 0;
    lastPos = orientation;

    SetActionState(AS_STAND, 0);
}

// PSX: Move__6Player (PLAYER.CPP:1408, 0x80030100)
void Player::Move() {
    MARKFUNCTION(0x80030100);
    Humanoid::Move();
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

    // PSX: OriginalSTree_omPlayer = model->drawable->original
    // Used for suit-change system - skip global for now

    // PSX: InitBlendPose - animation blending
    // Not yet reversed - skip

    // PSX: InitSemiTransMode (called via Humanoid path, also needed here)
    Model* m = static_cast<Model*>(model);
    if (m) {
        m->ApplyAnimToModel(0, 0, 2, 0, 0);
        SModel* sm = static_cast<SModel*>(m);
        sm->InitSemiTransMode();
    }
}

// PSX: SetActionState__6PlayerUll (PLAYER.CPP:1579, 0x800303BC)
// PSX: 25 Player-specific cases, rest delegate to Humanoid::SetActionState.
void Player::SetActionState(u32 state, s32 param) {
    MARKFUNCTION(0x800303BC);
    actionStateFlag = 0;
    flags |= TF_DYNAMIC;

    // PSX preamble
    combatFlag = 0;
    flags |= TF_DYNAMIC;

    // PSX: if previous actionState was 3, unload stored animation
    if (actionState == 3) {
        if (g_characterManager) {
            g_characterManager->UnloadAnimationBatch(0, currentAnimEnum);
        }
    }

    switch (state) {
    case AS_INACTIVE_IDLE: {
        // PSX case 0: clear animations, set dispatch to none
        playerFlags &= ~4;
        field344 = 0;
        stateDispatch = SD_NONE;
        field348 = 0;
        PlayAnimation(PLAYER_ANIM_INACTIVE_IDLE, ANIM_LOOP);
        actionState = (s32)state;
        return;
    }
    case AS_STAND: {
        // PSX case 1: stateDispatch=22, SetIdleAnimation, reset fields, playerFlags|=4
        field344 = 0;
        stateDispatch = SD_STAND;
        field348 = 8;
        PlayAnimation(PLAYER_ANIM_IDLE_UNARMED, ANIM_LOOP);
        field616 = 0;
        field424 = 0;
        playerFlags |= PF_COMBAT_READY;
        flags |= TF_DYNAMIC;
        flags2 &= ~0x70; // clear bits 4,5,6
        actionState = (s32)state;
        stateTimer = 0;
        return;
    }
    case AS_WALL_JUMP_TAUNT: {
        // PSX case 3: wall jump taunt/idle - loads angle-based animations (308-315)
        // Uses InactiveIdle handler. Complex anim selection not yet reversed.
        field344 = 0;
        stateDispatch = SD_INACTIVE_IDLE;
        animCallbackData = 0;
        currentAnimEnum = PLAYER_ANIM_FLIP_VARIANT; // placeholder
        animLoadState = 2;
        actionState = (s32)state;
        return;
    }
    case AS_PAUSE: {
        // PSX case 6: running jump (from _Run context).
        // stateDispatch=28(SD_JUMP), DoJump with combined base+running force,
        // field704=1 (hold flag), runJumpHold table, AddForce initial burst.
        if (!field500 && !field504) {
            // PSX: model->ClearSemiTransMode() if no pickups
        }
        field344 = 0;
        stateDispatch = SD_JUMP;
        field348 = 8;
        field704 = 1;  // jump hold flag (running jump has hold detection)
        field700 = 0;
        field706 = 0;
        field712 = s_runJumpHold;
        DoJump(PLAYER_JUMP_FORCE + PLAYER_RUNNING_JUMP_BONUS);

        PlayAnimation(PLAYER_ANIM_FALL, ANIM_RUN_TO_LAST);

        // PSX: AddForce(jumpTable[0], orientation) - initial horizontal burst
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(orientation.y & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(PLAYER_RUNNING_JUMP_BURST, &dir);

        turnRate = 1500;
        playerFlags = (playerFlags & ~3) | 2;
        actionState = (s32)state;
        return;
    }
    case AS_JUMP: {
        // PSX case 8: standing jump. DoJump, set direction cosines, playerFlags|=3.
        field344 = 0;
        stateDispatch = SD_JUMP;
        field348 = 8;
        PlayAnimation(PLAYER_ANIM_FALL, ANIM_RUN_TO_LAST);

        DoJump();

        field700 = 0;
        field712 = nullptr;
        field704 = 0;
        field706 = 0;
        playerFlags |= 3;

        // PSX: store jump direction cosines
        field720 = rmSin16(orientation.y);
        field724 = 0;
        field728 = rmSin16((s16)(orientation.y + 0x4000));

        field616 = 0;
        jumpReturnHeight = homePos.y;
        actionState = (s32)state;
        stateTimer = 0;
        return;
    }
    case AS_LEDGE_LATCH: {
        // PSX case 9: ledge grab. stateDispatch=33(SD_WALLJUMP), zero velocity/force.
        field344 = 0;
        stateDispatch = SD_LEDGE_LATCH;
        field348 = 8;
        PlayAnimation(PLAYER_ANIM_LEDGE_LATCH, ANIM_LOOP);
        velocity = {};
        contactForce = {};
        maxFallDivisor = 0;
        playerFlags &= ~2;
        field616 = 0;
        actionState = (s32)state;
        return;
    }
    case AS_RUN: {
        // PSX case 10: turnRate=4000, field616=0, gp+392=0
        turnRate = 4000;
        field616 = 0;
        // Fall through to Humanoid for stateDispatch + animation setup
        break;
    }
    case AS_BACKFLIP: {
        // PSX case 11: face enemy + strafe init.
        // FindFoe, SetHumanoidTarget, stateDispatch=26(SD_BACKFLIP)
        stateTimer = 0;
        FindFoe(distantTargetRange, 0, 0);
        field344 = 0;
        stateDispatch = SD_BACKFLIP;
        field348 = 8;
        actionState = (s32)state;
        return;
    }
    case AS_STRAFE: {
        // PSX case 12: stateDispatch=27(SD_STRAFE), play anim 24
        field344 = 0;
        stateDispatch = SD_STRAFE;
        field348 = 8;
        field488 = 0;
        actionState = (s32)state;
        return;
    }
    case AS_FALL: {
        // PSX case 13: stateDispatch=29, play anim 39 frame 5,
        // field616=0, playerFlags|=1, jumpReturnHeight=homePos.y
        field344 = 0;
        stateDispatch = SD_FALL;
        field348 = 8;
        PlayAnimation(PLAYER_ANIM_FALL, ANIM_RUN_TO_LAST);
        field616 = 0;
        playerFlags |= 1;
        jumpReturnHeight = homePos.y;
        actionState = (s32)state;
        stateTimer = 0;
        return;
    }
    case AS_HARDFALL: {
        // PSX case 14: stateDispatch=-1, HardFall handler
        field344 = 0;
        stateDispatch = SD_HARDFALL;
        actionState = (s32)state;
        stateTimer = 0;
        return;
    }
    case AS_HARDLAND: {
        // PSX case 15: stateDispatch=-1, HardLand handler
        field344 = 0;
        stateDispatch = SD_HARDLAND;
        actionState = (s32)state;
        stateTimer = 0;
        return;
    }
    case AS_FLIP: {
        // PSX case 16: stateDispatch=-1, Flip handler, play anim 295, playerFlags|=2
        playerFlags |= 2;
        field344 = 0;
        stateDispatch = SD_FLIP;
        PlayAnimation(PLAYER_ANIM_FLIP_VARIANT, ANIM_LOOP);
        field616 = 0;
        actionState = (s32)state;
        return;
    }
    case AS_FLIP_VARIANT: {
        // PSX case 17: stateDispatch=-1, Flip handler, play default anim
        field344 = 0;
        stateDispatch = SD_FLIP;
        actionState = (s32)state;
        return;
    }
    case AS_POLE_IDLE: {
        // PSX case 18: stateDispatch=44, zero velocity/force, pole idle setup
        field344 = 0;
        stateDispatch = SD_POLE_IDLE;
        field348 = 8;
        velocity = {};
        contactForce = {};
        flags2 &= ~0x70; // clear bits 4,5,6
        field616 = 0;
        actionStateFlag = 1;
        field424 = -38000;
        actionState = (s32)state;
        return;
    }
    case AS_PUSH_OBJECT: {
        // PSX case 19: stateDispatch=-1, PushObject handler
        field344 = 0;
        stateDispatch = SD_PUSH_OBJECT;
        actionState = (s32)state;
        return;
    }
    case AS_SLOPE_SLIDE: {
        // PSX case 20: stateDispatch=47, play anim 34 frame 3, field616=0
        field344 = 0;
        stateDispatch = SD_SLOPE_SLIDE;
        field348 = 8;
        field616 = 0;
        actionState = (s32)state;
        return;
    }
    case AS_TABLE_ROLL: {
        // PSX case 21: stateDispatch=-1, TableRoll handler.
        // Zero velocity/force/maxFallDivisor, setup flags2.
        u32 f2 = (u32)flags2;
        if (((f2 >> 4) & 1) == 0 || (((f2 >> 5) & 1) != 0 && ((f2 >> 6) & 1) != 0)) {
            flags2 = (flags2 & ~0x70) | 0x10;
        }
        velocity = {};
        contactForce = {};
        maxFallDivisor = 0;
        field344 = 0;
        stateDispatch = SD_TABLE_ROLL;
        actionState = (s32)state;
        return;
    }
    case AS_POLE_SWING: {
        // PSX case 23: stateDispatch=45, zero velocity/force, field616=0
        field344 = 0;
        stateDispatch = SD_POLE_SWING;
        field348 = 8;
        velocity = {};
        contactForce = {};
        field616 = 0;
        actionState = (s32)state;
        return;
    }
    case AS_PUNCH_ATTACK:
    case AS_KICK_ATTACK: {
        // PSX cases 32,34: FindFoe, SetHumanoidTarget, then fall through to Humanoid
        FindFoe(750, 10922, 0);
        break;
    }
    case AS_PICKUP: {
        // PSX case 44: pickup object - stateDispatch=-1, Pickup handler
        // TODO: full pickup system with GetPickupMove, WeaponPickupDialog
        field344 = 0;
        stateDispatch = SD_PICKUP;
        actionState = (s32)state;
        return;
    }
    case AS_THROW_PICKUP: {
        // PSX case 45: throw object - stateDispatch=-1, Throw handler
        // TODO: full throw system with GetThrowMove, target finding
        field344 = 0;
        stateDispatch = SD_THROW;
        actionState = (s32)state;
        return;
    }
    case AS_GET_UP: {
        // PSX case 68: get up from knockdown. stateDispatch=43, field616=0
        flags2 &= ~0x70;
        field344 = 0;
        stateDispatch = SD_GET_UP;
        field348 = 8;
        field616 = 0;
        stateTimer = 0;
        actionState = (s32)state;
        return;
    }
    case AS_DEAD: {
        // PSX case 72: player death. Complex death dialog/anim sequence.
        // stateDispatch=48, flags|=0x80, field616=0
        field616 = 0;
        stateTimer = 0;
        flags |= 0x80;
        field344 = 0;
        stateDispatch = SD_DEAD_PLAYER;
        field348 = 8;
        actionState = (s32)state;
        return;
    }
    default:
        break;
    }

    // Delegate to Humanoid for the core state mapping and remaining cases
    Humanoid::SetActionState(state, param);

    // Animation fallbacks for states that go through Humanoid path
    switch (state) {
    case AS_RUN:
        PlayAnimation(PLAYER_ANIM_RUN, ANIM_LOOP);
        break;
    default:
        break;
    }
}

// PSX: ProcessAction dispatches via method thunk (field344/346/348).
// PSX stateDispatch > 0 = vtable index, stateDispatch < 0 = direct function pointer.
// On PC, Player::ProcessAction handles all Player-specific dispatches via switch,
// then falls through to Humanoid::ProcessAction for shared Humanoid dispatches.
void Player::ProcessAction() {
    switch (stateDispatch) {
    // Player-specific handlers (PSX: direct function pointer, stateDispatch = -1)
    case SD_HARDFALL:      _HardFall(); return;
    case SD_HARDLAND:      _HardLand(); return;
    case SD_FLIP:          _Flip(); return;
    case SD_INACTIVE_IDLE: _InactiveIdle(); return;
    case SD_PUSH_OBJECT:   _PushObject(); return;
    case SD_TABLE_ROLL:    _TableRoll(); return;
    case SD_LEDGE_LATCH:   _LedgeLatch(); return;
    case SD_LEDGE_PULLUP:  _LedgePullup(); return;
    case SD_DO_STAND:      _DoStand(); return;
    // Player-specific handlers (PSX: vtable index dispatch)
    case SD_GET_UP:        _DoStand(); return;
    case SD_POLE_IDLE:     _Push(); return;
    case SD_POLE_SWING:    _HorizontalPoleSwing(); return;
    case SD_SLOPE_SLIDE:   _SlopeSlide(); return;
    case SD_DEAD_PLAYER:   _Dead(); return;
    case SD_WALLJUMP:      _WallJump(); return;
    default: Humanoid::ProcessAction(); return;
    }
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

// PSX: DoJump__6Player (PLAYER.CPP:1424, 0x80030120)
void Player::DoJump() {
    MARKFUNCTION(0x80030120);
    // PSX: contactForce += {0, gp+460, 0}
    contactForce.y += PLAYER_JUMP_FORCE;
    flags &= ~TF_ON_GROUND;
    // PSX: sets field344=0, stateDispatch=28, field348=8
    field344 = 0;
    stateDispatch = SD_JUMP;
    field348 = 8;
    jumpReturnHeight = homePos.y;
}

// PSX: DoJump__6Playerl (PLAYER.CPP:1437, 0x800301D8)
// PSX: adds height to contactForce.y, clears TF_ON_GROUND
void Player::DoJump(s32 height) {
    MARKFUNCTION(0x800301D8);
    contactForce.y += height;
    flags &= ~TF_ON_GROUND;
    // PSX: sets field344=0, stateDispatch=28, field348=8
    field344 = 0;
    stateDispatch = SD_JUMP;
    field348 = 8;
    jumpReturnHeight = homePos.y;
}

// PSX: FallingPhysics__6Player (PLAYER.CPP:3187, 0x80032368)
void Player::FallingPhysics() {
    MARKFUNCTION(0x80032368);

    // PSX: v2 = a1[88] (commandBits at +352)
    // PSX: checks bits 2, 3, 4
    u32 cb = (u32)commandBits;
    s32 hasInput = 0;
    if (((cb >> 2) & 1) || ((cb >> 4) & 1) || ((cb >> 3) & 1)) {
        hasInput = 1;
    }

    if (hasInput) {
        // PSX: FaceAngleY(this, faceAngle, 1)
        FaceAngleY(faceAngle, 1);
        // PSX: stack = {int32:0, int32:faceAngle, int32:0}
        // As SVector: {x=0, y=0, z=faceAngle, pad=0} - rotY in SVector.z slot
        SVector dir;
        dir.x = 0;
        dir.y = 0;
        dir.z = (s16)(faceAngle & 0xFFFF);
        dir.pad = 0;
        AddForce(150, &dir);
        gravity = 4096;
    } else {
        gravity = 2048;
    }
}

// PSX: CheckForLanding__6Player (PLAYER.CPP:4366, 0x80033C00)
// PSX: checks TF_ON_GROUND (set by collision system via Land() in HandleThingFloor)
void Player::CheckForLanding() {
    MARKFUNCTION(0x80033C00);
    if (flags & TF_ON_GROUND) {
        if (commandBits & CB_RUN) {
            SetActionState(AS_RUN, 0);
        } else {
            SetActionState(AS_STAND, 0);
        }
    }
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

    // D-pad - movement direction and faceAngle
    s32 dx = 0, dz = 0;
    if (buttons & PsxPad::Up)    dz += 1;
    if (buttons & PsxPad::Down)  dz -= 1;
    if (buttons & PsxPad::Right) dx += 1;
    if (buttons & PsxPad::Left)  dx -= 1;

    if (dx != 0 || dz != 0) {
        commandBits |= CB_RUN;
        // atan2(dx, dz) - PSX binary angle (0=+Z, 0x4000=+X)
        f32 rad = atan2((f32)dx, (f32)dz);
        faceAngle = RAD2ANGLE(rad) & 0xFFFF;
    }

    // Cross - jump (bit 3)
    if (buttons & PsxPad::Cross) commandBits |= CB_JUMP;
    // Square - attack (bit 7)
    if (buttons & PsxPad::Square) commandBits |= CB_ATTACK;
    // Circle - guard/taunt (bit 4)
    if (buttons & PsxPad::Circle) commandBits |= CB_GUARD;
    // R1 - strafe (bit 5)
    if (buttons & PsxPad::R1) commandBits |= CB_STRAFE;
    // L1 - backflip (bit 6)
    if (buttons & PsxPad::L1) commandBits |= CB_BACKFLIP;
}

void Player::LoadPlayerTauntResponse(Humanoid* /*target*/) {
    MARKFUNCTION(0x80034290);
}

void Player::PlayPlayerTauntResponse() {
    MARKFUNCTION(0x8003431C);
}

bool Player::PlayAnimation(s32 animEnum, s32 loopType) {
    if (!model || animEnum < 0) {
        return false;
    }
    if (!EnsurePlayerAnimationLoaded(animEnum)) {
        return false;
    }

    Model* m = static_cast<Model*>(model);
    m->ApplyAnimToModel(0, animEnum, loopType, 0, 0);
    currentAnimEnum = animEnum;
    animLoadState = 1;
    return true;
}

void Player::PauseAnimation() {
    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }
    anim->speed = 0;
    animLoadState = 2;
}

void Player::ResumeAnimation() {
    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }
    anim->speed = FIX16_ONE;
    animLoadState = 1;
}

void Player::StopAnimation() {
    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }
    anim->ForceFrame(0);
    anim->speed = 0;
    animLoadState = 0;
}

bool Player::IsAnimationPaused() const {
    if (!model) {
        return false;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return false;
    }
    return anim->speed == 0;
}

// PSX: _InactiveIdle__6Player (PLAYER.CPP:2331, 0x8003123C)
// Waits for the inactive idle animation callback. When triggered, plays
// currentAnimEnum with RUN_TO_LAST, then plays dialog. When animation
// completes or guard bit released, transitions to Stand.
void Player::_InactiveIdle() {
    MARKFUNCTION(0x8003123C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    bool animDone = false;

    // PSX: a1[187] = field748 (animCallbackData)
    if (animCallbackData) {
        // Animation callback triggered - play the stored animation
        animCallbackData = 0;
        m->ApplyAnimToModel(0, currentAnimEnum, ANIM_RUN_TO_LAST, 0, 0);
        // PSX: PlayDialogBasedOnPriority(54, 54) - dialog system not reversed
    } else if (animLoadState == 2 && anim) {
        // Check if animation enum matches and has completed a loop
        if (anim->animEnum == currentAnimEnum && anim->loopCount > 0) {
            animDone = true;
        }
    }

    // PSX: check commandBits bit 1 (guard)
    bool guardBit = ((commandBits >> 1) & 1) != 0;
    if (!guardBit || animDone) {
        // PSX: UnloadAnimation(0, 0, currentAnimEnum)
        if (g_characterManager) {
            g_characterManager->UnloadAnimationBatch(0, currentAnimEnum);
        }
        SetActionState(AS_STAND, 0);
        CheckForLanding();
    }
}

// PSX: _Stand__6Player (PLAYER.CPP:2378, 0x80031350) - 1832 bytes, 117 blocks
// Priority-ordered command dispatch from commandBits.
// PSX: checks strafe, pickup/throw, combat, taunt, jump, run, idle anims.
void Player::_Stand() {
    MARKFUNCTION(0x80031350);

    u32 cb = (u32)commandBits;

    // PSX: bit 5 (strafe) -> face + strafe
    if ((cb >> 5) & 1) {
        FaceAngleY(faceAngle, 0);
        SetActionState(AS_STRAFE, 0);
        return;
    }

    // PSX: bits 7,15,16,19 -> pickup/throw or combat idle
    s32 hasPickupBits = 0;
    if (((cb >> 7) & 1) || ((cb >> 15) & 1) || (cb & 0x10000) || ((cb >> 19) & 1)) {
        hasPickupBits = 1;
    }
    if (hasPickupBits) {
        if (field500 != 0 || field504 != 0) {
            SetActionState(AS_THROW_PICKUP, 0);
        } else {
            SetActionState(AS_COMBAT_IDLE, 0);
        }
        return;
    }

    // PSX: bit 4 (taunt) -> pause
    if ((cb >> 4) & 1) {
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // PSX: bit 3 (jump) -> jump
    if ((cb >> 3) & 1) {
        SetActionState(AS_JUMP, 0);
        return;
    }

    // PSX: bit 2 (run) or bit 6 (backflip) -> run with angle check
    s32 hasMove = 0;
    if (((cb >> 2) & 1) || ((cb >> 6) & 1)) {
        hasMove = 1;
    }
    if (hasMove) {
        // PSX: backflip bit -> backflip
        if ((cb >> 6) & 1) {
            SetActionState(AS_BACKFLIP, 0);
            return;
        }

        // PSX: angle difference check for turn-around vs direct run
        s32 diff = orientation.y - faceAngle;
        if (diff > (s32)0xFFFF) {
            diff -= 0xFFFF;
            while (diff > (s32)0xFFFF) diff -= 0xFFFF;
        }
        if (diff < 0) {
            diff += 0xFFFF;
            while (diff < 0) diff += 0xFFFF;
        }
        s32 absDiff = (diff >= 0) ? diff : -diff;

        // PSX: angle diff in range 24577..40959 -> turn around first
        if (faceAngle != orientation.y && (u32)(absDiff - 24577) < 0x3FFF) {
            SetActionState(AS_RUN, 0);
            return;
        }

        // PSX: first 3 frames with movement -> AddForce(2500, faceAngle)
        // then transition to run
        stateTimer++;
        if (stateTimer < 4) {
            SVector dir;
            dir.x = 0;
            dir.y = 0;
            dir.z = (s16)(faceAngle & 0xFFFF);
            dir.pad = 0;
            AddForce(2500, &dir);
        } else {
            SetActionState(AS_RUN, 0);
        }
        return;
    }

    // PSX: combat bits 7-20 -> punch attack
    s32 hasCombat = 0;
    if (((cb >> 7) & 1) || ((cb >> 8) & 1) || ((cb >> 9) & 1) ||
        ((cb >> 10) & 1) || ((cb >> 11) & 1) || ((cb >> 12) & 1) ||
        ((cb >> 13) & 1) || ((cb >> 14) & 1) || ((cb >> 15) & 1) ||
        (cb & 0x10000) || ((cb >> 17) & 1) || ((cb >> 18) & 1) ||
        ((cb >> 19) & 1) || ((cb >> 20) & 1)) {
        hasCombat = 1;
    }
    if (hasCombat) {
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // PSX: not on ground -> fall off ledge
    if (!(flags & TF_ON_GROUND)) {
        SetActionState(AS_FALL, 3);
        return;
    }

    // PSX: face toward input direction (gradual turn while idle)
    FaceAngleY(faceAngle, 1);
}

// PSX: _Flip__6Player (PLAYER.CPP:2683, 0x80031A78)
// Handles flip animations (forward flip, turning flip, flip variant).
// Applies directional force based on animEnum, checks landing.
void Player::_Flip() {
    MARKFUNCTION(0x80031A78);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    s32 animE = anim->animEnum;
    s32 force;

    // PSX: if animEnum == 295 (flip variant), maxFallDivisor=13, force=1500
    // Otherwise maxFallDivisor=15, force=1000
    if (animE == PLAYER_ANIM_FLIP_VARIANT) {
        maxFallDivisor = 13;
        force = 1500;
    } else {
        maxFallDivisor = 15;
        force = 1000;
    }

    if (animE == PLAYER_ANIM_FORWARD_FLIP) {
        // Forward flip: AddForce in facing direction at runSpeed
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(faceAngle & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(runSpeed, &dir);
    } else if (animE == PLAYER_ANIM_TURNING_FLIP) {
        // Turning flip: temporarily boost turnRate, face target angle
        u16 savedTurnRate = turnRate;
        turnRate = 5000;
        FaceAngleY(faceAngle, 1);
        turnRate = savedTurnRate;
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(faceAngle & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(runSpeed, &dir);
    } else {
        // Default flip: check CB_RUN for directional control
        if (!(commandBits & CB_RUN)) {
            goto handleLanding;
        }
        // Check angle difference for immediate vs gradual turn
        s32 diff = orientation.y - faceAngle;
        s32 absDiff = (diff >= 0) ? diff : -diff;
        s32 immediate = (absDiff < 16385) ? 1 : 0;
        FaceAngleY(faceAngle, immediate);
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(faceAngle & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(force, &dir);
    }

handleLanding:
    // PSX: call HandleLand
    HandleLand(0);

    // PSX: check TF_ON_GROUND (flags bit 12)
    if (flags & TF_ON_GROUND) {
        gravity = 0x8000;
    } else {
        gravity = 4999;
        CheckForLanding();
    }
}

// PSX: _Jump__6Player (PLAYER.CPP:2830, 0x80031C68)
// PSX: wall jump check, combat transitions, jump phase tracking via field700/704/706/712,
// jump table system (standingJumpHold/Tap, runJumpHold/Tap), air control,
// HandleLand call, transition to fall when velocity negative AND height threshold met.
// DoJump is called from SetActionState(AS_JUMP/AS_PAUSE), NOT from here.
void Player::_Jump() {
    MARKFUNCTION(0x80031C68);

    // PSX: check playerFlags bit 1 for wall jump eligibility
    // PSX: check commandBits bits 8,9,14 for combat air attack

    // PSX: vtable+204 call = HandleLand (empty on DynamicThing, fall damage on Humanoid)
    HandleLand(0);

    // PSX: jump phase initialization (field700 == 0 = first _Jump frame)
    if (field700 == 0) {
        if (playerFlags & 1) {
            // Standing jump: determine hold vs tap on first frame
            if (commandBits & CB_JUMP) {
                // Button still held
                field700 = 1;
                field704 = 1;
                field706 = 0;
                field712 = s_standingJumpHold;
            } else {
                // Button released (tap)
                field700 = 2;
                field706 = 1;
                field704 = 0;
                field712 = s_standingJumpTap;
            }
        } else {
            // Running jump: field712 already set to s_runJumpHold from SetActionState
            if (field712 == s_runJumpHold) {
                if (!(commandBits & CB_JUMP)) {
                    // Button released - switch to tap table
                    field712 = s_runJumpTap;
                    field700 = 2;
                } else {
                    field700 = 1;
                }
            }
        }
    }

    // PSX: apply jump table parameters (LABEL_37 in decompile)
    if (field712) {
        FaceAngleY(faceAngle, 1);

        // maxFallDivisor from jump table
        // PSX: if holding button, reduce gravity by 4 for higher jump
        if (field704 && (commandBits & CB_JUMP)) {
            maxFallDivisor = field712[2] - 4;
        } else {
            if (field704) {
                field704 = 0;  // clear hold flag on release
            }
            maxFallDivisor = field712[2];
        }

        // Air control direction check: bits 2(run), 3(jump), 4(guard), 6(backflip)
        u32 cb = (u32)commandBits;
        s32 hasDir = 0;
        if (((cb >> 2) & 1) || ((cb >> 6) & 1) || ((cb >> 4) & 1) || ((cb >> 3) & 1)) {
            hasDir = 1;
        }

        // Gravity (XZ drag): full from table with input, half without
        if (hasDir) {
            gravity = field712[1];
        } else {
            gravity = field712[1] / 2;
        }

        // Air movement force with directional input
        if (hasDir) {
            SVector dir = {};
            dir.z = (s16)(faceAngle & 0xFFFF);
            AddForce(field712[0], &dir);

            // PSX: standing jump momentum preservation (field720/728 cosines)
            if (field712 == s_standingJumpTap || field712 == s_standingJumpHold) {
                s32 sinY = rmSin16(faceAngle);
                s32 cosY = rmSin16((s16)(faceAngle + 0x4000));
                s64 momentum = ((s64)field720 * (s64)sinY >> 16)
                             + ((s64)field728 * (s64)cosY >> 16);
                if ((s32)momentum > 58982) {
                    AddForce(6000, &dir);
                    field720 = 0;
                    field724 = 0;
                    field728 = 0;
                }
            }
        }
    }

    // PSX: landing check - if TF_ON_GROUND set by collision system, land
    if (flags & TF_ON_GROUND) {
        CheckForLanding();
        return;
    }

    // PSX: fall transition: velocity.y <= 0 AND jumpReturnHeight - homePos.y >= 2561
    if (velocity.y <= 0) {
        if (jumpReturnHeight - homePos.y >= 2561) {
            SetActionState(AS_FALL, 3);
            field616 = 100;
        }
    }
}

// PSX: _Fall__6Player (PLAYER.CPP:3226, 0x80032444)
// PSX: FallingPhysics, hard-fall velocity check, HandleLand (vtable+204),
// TF_ON_GROUND check, vtable+228 (CheckForLanding) if not on ground.
void Player::_Fall() {
    MARKFUNCTION(0x80032444);

    // PSX: air control + gravity setting
    FallingPhysics();

    // PSX: if fall speed exceeds threshold -> hard fall (AS 14)
    if (g_hardFallThreshold >= velocity.y) {
        SetActionState(AS_HARDFALL, 0);
        return;
    }

    // PSX: vtable+204 = HandleLand (Humanoid override calculates fall damage)
    HandleLand(0);

    // PSX: check TF_ON_GROUND (set by HandleThingFloor collision system)
    if (flags & TF_ON_GROUND) {
        // PSX: returns immediately when on ground; landing transitions handled
        // by next frame's state dispatch (CheckForLanding to stand/run)
        CheckForLanding();
        return;
    }
}

// PSX: _HardFall__6Player (PLAYER.CPP:3343, 0x800324E8)
// Applies falling physics, checks for ground contact.
// On landing: transitions to HardLand, applies hard-fall animation.
void Player::_HardFall() {
    MARKFUNCTION(0x800324E8);

    FallingPhysics();

    // PSX: (flags >> 12) & 1 = TF_ON_GROUND
    if (flags & TF_ON_GROUND) {
        SetActionState(AS_HARDLAND, 0);
        // PSX: model->SetAnim(40, 0, 0, 0) = play hard-fall landing anim
        if (model) {
            Model* m = static_cast<Model*>(model);
            m->ApplyAnimToModel(0, PLAYER_ANIM_HARD_FALL, ANIM_LOOP, 0, 0);
        }
    }
}

// PSX: _HardLand__6Player (PLAYER.CPP:3365, 0x80032560)
// Waits for hard-land animation to complete (loopCount > 0).
// Transitions to GET_UP if alive, DEAD if health is 0.
void Player::_HardLand() {
    MARKFUNCTION(0x80032560);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: check loopCount > 0 (animation completed)
    if (anim->loopCount > 0) {
        // PSX: if health > 0, GET_UP (68); else DEAD (72)
        if (health > 0) {
            SetActionState(AS_GET_UP, 0);
        } else {
            SetActionState(AS_DEAD, 0);
        }
    }
}

// PSX: _Run__6Player (PLAYER.CPP:3397, 0x800325CC) - 1148 bytes, 73 blocks
// Priority-ordered transitions: pickup/backflip, guard->stand, slope,
// jump/taunt, combat attacks, ledge detection, directional force.
void Player::_Run() {
    MARKFUNCTION(0x800325CC);

    u32 cb = (u32)commandBits;

    // PSX: bits 7,15,16 -> pickup/throw transitions
    s32 hasPickup = 0;
    if (((cb >> 7) & 1) || ((cb >> 15) & 1) || (cb & 0x10000)) {
        hasPickup = 1;
    }
    if (hasPickup) {
        if (field500 != 0 || field504 != 0) {
            SetActionState(AS_THROW_PICKUP, 0);
        } else {
            // PSX: CheckForPickup
        }
    }

    // PSX: bit 6 -> backflip
    if ((cb >> 6) & 1) {
        SetActionState(AS_BACKFLIP, 0);
        return;
    }

    // PSX: bit 1 (guard) without bit 2 (run) -> stand with turn anim
    if (((cb >> 1) & 1) && !((cb >> 2) & 1)) {
        SetActionState(AS_STAND, 0);
        return;
    }

    // PSX: flags bit 17 (slope) -> slope strafe
    if ((flags >> 17) & 1) {
        SetActionState(AS_SLOPE_SLIDE, 0);
        return;
    }

    // PSX: bits 3 (jump) or 4 (taunt) -> running jump (state 6)
    // PSX _Run dispatches BOTH jump and taunt bits to SetActionState(6, 0)
    s32 hasJumpTaunt = 0;
    if (((cb >> 3) & 1) || ((cb >> 4) & 1)) {
        hasJumpTaunt = 1;
    }
    if (hasJumpTaunt) {
        SetActionState(AS_PAUSE, 0); // AS_PAUSE=6 = running jump on Player
        return;
    }

    // PSX: combat bits 7-20 -> combat attack
    s32 hasCombat = 0;
    if (((cb >> 7) & 1) || ((cb >> 8) & 1) || ((cb >> 9) & 1) ||
        ((cb >> 10) & 1) || ((cb >> 11) & 1) || ((cb >> 12) & 1) ||
        ((cb >> 13) & 1) || ((cb >> 14) & 1) || ((cb >> 15) & 1) ||
        (cb & 0x10000) || ((cb >> 17) & 1) || ((cb >> 18) & 1) ||
        ((cb >> 19) & 1) || ((cb >> 20) & 1)) {
        hasCombat = 1;
    }
    if (hasCombat) {
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // PSX: bit 5 -> ledge detection + strafe
    if ((cb >> 5) & 1) {
        // PSX: CheckForLedges2, ledge grab detection
        // Simplified: face and strafe
        SetActionState(AS_STRAFE, 0);
        FaceAngleY(faceAngle, 0);
        return;
    }

    // PSX: face toward input direction
    FaceAngleY(faceAngle, 1);

    // PSX: if NOT on ground, check for falling off ledge
    if (!(flags & TF_ON_GROUND)) {
        // PSX: checks floor height; if homePos.y - floorHeight >= 129 -> fall
        SetActionState(AS_FALL, 3);
        return;
    }

    // PSX: no run input -> stand
    if (!((cb >> 2) & 1)) {
        SetActionState(AS_STAND, 0);
        return;
    }

    // PSX: builds SVector with faceAngle as Y rotation, AddForce with ramped magnitude
    // PSX uses gp+392 accumulator that ramps from 0 to runSpeed (gp+396 is ramp rate)
    SVector dir;
    dir.x = 0;
    dir.y = 0;
    dir.z = (s16)(faceAngle & 0xFFFF);
    dir.pad = 0;

    AddForce(PLAYER_RUN_FORCE, &dir);
}

// PSX: _Push__6Player (PLAYER.CPP:3550, 0x80032A48)
// Handles push state: checks guard/jump/run bits for transitions.
// If running toward push target (angle within threshold), zeros velocity.
void Player::_Push() {
    MARKFUNCTION(0x80032A48);

    u32 cb = (u32)commandBits;

    // PSX: bit 1 (guard) -> Stand with param 3
    if ((cb >> 1) & 1) {
        SetActionState(AS_STAND, 3);
        return;
    }

    // PSX: bit 3 (jump) -> Pause
    if ((cb >> 3) & 1) {
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // PSX: bit 2 (run) -> check angle for push direction
    if (!((cb >> 2) & 1)) {
        return;
    }

    // PSX: angle check between faceAngle and orientation.y
    s32 targetAngle = faceAngle;
    if (targetAngle == 0) {
        targetAngle = PSX_ANGLE_360;
    } else if (targetAngle > PSX_ANGLE_360) {
        targetAngle %= PSX_ANGLE_360;
    }

    s32 diff = targetAngle - orientation.y;
    if (diff < 0) {
        diff = -diff;
    }
    if (diff > PSX_ANGLE_180) {
        s32 correction = (diff > 0) ? -PSX_ANGLE_360 : PSX_ANGLE_360;
        diff = targetAngle - orientation.y + correction;
    }
    s32 absDiff = (diff >= 0) ? diff : -diff;

    // PSX: threshold 4552 - if angle difference too large, transition to strafe
    if (absDiff >= 4552) {
        SetActionState(AS_RUN, 0);
        return;
    }

    // Facing toward push target - zero velocity
    velocity = {};
}

// PSX: _PushObject__6Player (PLAYER.CPP:3604, 0x80032B80)
// Handles pushing an object. Checks guard/jump/taunt bits.
// If running, applies force in facing direction.
void Player::_PushObject() {
    MARKFUNCTION(0x80032B80);

    u32 cb = (u32)commandBits;

    // PSX: bit 1 (guard) -> Stand with param 3
    if ((cb >> 1) & 1) {
        SetActionState(AS_STAND, 3);
        return;
    }

    // PSX: bit 3 (jump) or bit 4 (taunt) -> Stand
    s32 hasJumpOrTaunt = 0;
    if (((cb >> 3) & 1) || ((cb >> 4) & 1)) {
        hasJumpOrTaunt = 1;
    }
    if (hasJumpOrTaunt) {
        SetActionState(AS_STAND, 0);
        return;
    }

    // PSX: bit 2 (run) -> apply force and face direction
    if (!((cb >> 2) & 1)) {
        return;
    }

    // PSX: checks field368 bit 2 for push confirmation
    if ((field368 >> 2) & 1) {
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(orientation.y & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(faceAngle, &dir);
        FaceAngleY(faceAngle, 1);
        return;
    }

    SetActionState(AS_STAND, 0);
}

// PSX: _Teetering__6Player (PLAYER.CPP:3659, 0x80032C70)
// Empty function on PSX (COLLAPSED, 8 bytes)
void Player::_Teetering() {
    MARKFUNCTION(0x80032C70);
}

// PSX: _WallJump__6Player (PLAYER.CPP:3688, 0x80032D8C)
// Handles wall jump sequence: start (32) -> launch (33) with force.
// Transitions to fall or stand on ground contact.
void Player::_WallJump() {
    MARKFUNCTION(0x80032D8C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    s32 animE = anim->animEnum;

    if (animE == PLAYER_ANIM_WALL_JUMP_START) {
        // Phase 1: wall contact, zero maxFallDivisor
        maxFallDivisor = 0;
        // Fall through to check completion
    } else if (animE == PLAYER_ANIM_WALL_JUMP_LAUNCH) {
        // Phase 2: launching off wall - apply forward force
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(orientation.y & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(4000, &dir);
        maxFallDivisor = 0;
    }

    // PSX: check if anim 33 completed (loopCount > 0)
    bool animDone = false;
    if (anim->animEnum == PLAYER_ANIM_WALL_JUMP_LAUNCH) {
        animDone = (anim->loopCount > 0);
    }

    if (animDone) {
        // Wall jump complete - transition to fall
        SetActionState(AS_FALL, 0);
        // Set desired move direction and face angle from orientation
        FaceAngleY(orientation.y, 0);
        return;
    }

    // PSX: call CheckForLanding, then HandleLand
    CheckForLanding();
    HandleLand(0);

    // If on ground after HandleLand, set move direction
    if (flags & TF_ON_GROUND) {
        FaceAngleY(orientation.y, 0);
    }
}

// PSX: _Collapse__6Player (PLAYER.CPP:3732, 0x80032EB0)
// Player-specific collapse: calls vtable handler, increments counter,
// checks humanoidDataID threshold for GET_UP vs DEAD transition.
void Player::_Collapse() {
    MARKFUNCTION(0x80032EB0);

    // PSX: vtable+260 call (ProcessControl or animation handler)
    ProcessControl();

    // PSX: increment field616 counter
    field616++;

    // PSX: check if counter exceeds humanoidDataID threshold
    s16 counter = (s16)field616;
    if ((s16)humanoidDataID < counter) {
        if (health > 0) {
            SetActionState(AS_GET_UP, 0);
        } else {
            field616 = 0;
            SetActionState(AS_DEAD, 0);
        }
    }
}

// PSX: _DoStand__6Player (PLAYER.CPP:3757, 0x80032F48)
// Transitions to Stand and zeros velocity vector.
void Player::_DoStand() {
    MARKFUNCTION(0x80032F48);
    SetActionState(AS_STAND, 0);
    velocity = {};
}
// PSX: _HorizontalPoleSwing__6Player (PLAYER.CPP:3776, 0x80032F8C)
// Complex pole swing physics: pendulum motion around X axis,
// applies angular velocity, checks for dismount on jump/taunt.
void Player::_HorizontalPoleSwing() {
    MARKFUNCTION(0x80032F8C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: pendulum physics
    // field424 = angular velocity accumulator for pole swing
    // Compute torque from gravity: torque = -660 * sin(orientation.x) * stateCounter
    s32 sinX = rmSin16(orientation.x);
    s64 torque = (-660LL * sinX) >> 16;
    torque = (4000 * stateCounter * torque);
    s32 accel = (s32)((torque >> 16) / 1); // simplified from rmDiv16i

    // PSX: accumulate angular velocity
    s32 angVel = field424 + (s32)((2182LL * (accel / 2000)) >> 16);
    field424 = angVel;

    // PSX: update orientation.x with angular velocity
    s32 newAngleX = orientation.x + (s32)((2182LL * angVel) >> 16);
    if (newAngleX < 0) {
        newAngleX += PSX_ANGLE_360;
    }
    newAngleX %= PSX_ANGLE_360;
    orientation.x = newAngleX;

    // PSX: check anim midpoint for swing count tracking
    s32 halfFrame = ((anim->endFrame >> 16) + (anim->endFrame >> 31)) >> 1;
    if (halfFrame < anim->currentFrame) {
        field616++;
    }

    // PSX: switch animation based on swing direction
    s32 targetAnim = (field424 >= 0) ? PLAYER_ANIM_POLE_SWING_FWD : PLAYER_ANIM_POLE_SWING_BACK;
    if (anim->animEnum != targetAnim) {
        m->ApplyAnimToModel(0, targetAnim, ANIM_HOLD_FIRST, 0, 0);
        actionStateFlag = 1;
    }

    // PSX: check if swing passed through bottom (actionStateFlag + angle range)
    if (actionStateFlag) {
        if (newAngleX < 5461 || newAngleX > 0xE38F) {
            // Passed through bottom of swing
            actionStateFlag = 0;
            // PSX: PoleSwing sound - not yet reversed
        }
    }

    // PSX: check dismount (jump bit 3 or taunt bit 4)
    maxFallDivisor = 0;
    u32 cb = (u32)commandBits;
    s32 wantDismount = 0;
    if (((cb >> 3) & 1) || ((cb >> 4) & 1)) {
        wantDismount = 1;
    }

    if (wantDismount && field616 > 0) {
        // Dismount from pole
        // PSX: complex matrix math for launch direction
        // Simplified: transition to flip and apply launch force
        SetActionState(AS_FLIP, 0); // PSX: SetActionState(16, 0) - flip from pole

        m->ApplyAnimToModel(0, PLAYER_ANIM_FORWARD_FLIP, ANIM_LOOP, 0, 0);

        // PSX: set faceAngle based on swing direction
        if (newAngleX <= PSX_ANGLE_180) {
            faceAngle = orientation.y + PSX_ANGLE_180;
        }

        // Reset orientation and swing state
        orientation.x = 0;
        field424 = 0;
    }
}
// PSX: _LedgeLatch__6Player (PLAYER.CPP:4062, 0x8003352C)
// Holds player on a ledge. Zeros velocity/force, waits 4 frames,
// then checks for dismount (jump/taunt) or pull-up.
void Player::_LedgeLatch() {
    MARKFUNCTION(0x8003352C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: zero velocity and contactForce
    velocity = {};
    contactForce = {};

    // PSX: check animEnum == 31 (LEDGE_LATCH)
    if (anim->animEnum != PLAYER_ANIM_LEDGE_LATCH) {
        return;
    }

    // PSX: increment field616, wait at least 4 frames
    field616++;
    if (field616 < 4) {
        return;
    }

    u32 cb368 = (u32)field368;

    // PSX: check field368 bit 3 - ledge grab confirmation from collision
    if ((cb368 >> 3) & 1) {
        // Move position back from ledge and drop
        // PSX: offset position by -300 * sin(orientation.y) in X/Z
        s32 sinY = rmSin16(orientation.y);
        s32 cosY = rmSin16((s16)(orientation.y + 0x4000));
        homePos.x += (s32)((-300LL * sinY) >> 16);
        homePos.y -= 850;
        homePos.z += (s32)((-300LL * cosY) >> 16);
        gravity = 0;
        return;
    }

    // PSX: check commandBits for directional input
    u32 cb = (u32)commandBits;
    s32 hasInput = 0;
    if (((cb >> 2) & 1) || ((cb >> 3) & 1) || ((cb >> 4) & 1)) {
        hasInput = 1;
    }

    if (hasInput) {
        // Check if input direction is facing the ledge (angle within threshold)
        s32 diff = orientation.y - faceAngle;
        if (diff > PSX_ANGLE_180) {
            diff -= PSX_ANGLE_360;
        }
        if (diff < -PSX_ANGLE_180) {
            diff += PSX_ANGLE_360;
        }
        s32 absDiff = (diff >= 0) ? diff : -diff;

        // PSX: threshold 9103 - if facing ledge, pull up
        s32 shouldPullUp = 0;
        if ((absDiff < 9103 || ((cb >> 3) & 1)) && !((u8)(field368 >> 7))) {
            shouldPullUp = 1;
        }

        if (shouldPullUp) {
            // PSX: transition to ledge pull-up
            SetActionState(AS_LEDGE_PULLUP, 0);
        } else {
            // PSX: LetGoOfLedge - not yet reversed, transition to fall
            SetActionState(AS_FALL, 0);
        }
    }
}

// PSX: _LedgePullup__6Player (PLAYER.CPP:4194, 0x800337A8)
// Zeros velocity/force, waits for animation completion,
// then restores position with clamped Y.
void Player::_LedgePullup() {
    MARKFUNCTION(0x800337A8);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);

    // PSX: zero velocity and contactForce
    velocity = {};
    contactForce = {};

    if (!anim) {
        return;
    }

    // PSX: check loopCount > 0 (animation completed)
    if (anim->loopCount > 0) {
        // PSX: vtable+240 call (ProcessAction)
        ProcessAction();

        // PSX: clamp homePos.y to >= 0
        if (homePos.y < 0) {
            homePos.y = 0;
        }
    }
}

// PSX: _Dead__6Player (PLAYER.CPP:4256, 0x80033858)
// Sets death flag and triggers death sequence via Director.
void Player::_Dead() {
    MARKFUNCTION(0x80033858);

    if (!field620) {
        field620 = 1;
        // PSX: SetCodeSnip(theDirector, death, 0)
        // Director system not yet reversed
    }
}

// PSX: _SlopeSlide__6Player (PLAYER.CPP:4280, 0x8003389C)
// Handles sliding on slopes. Applies force along slope normal,
// checks for dismount or falling off.
void Player::_SlopeSlide() {
    MARKFUNCTION(0x8003389C);

    u32 cb = (u32)commandBits;
    field616++;

    // PSX: check jump (bit 3) or taunt (bit 4) for dismount
    s32 wantDismount = 0;
    if (((cb >> 3) & 1) || ((cb >> 4) & 1)) {
        wantDismount = 1;
    }
    if (wantDismount && field616 >= 8) {
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // PSX: check flags bit 16 (on slope surface)
    if (!(flags & 0x10000)) {
        // Not on slope - transition to fall
        SetActionState(AS_FALL, 0);
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(orientation.y & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(5000, &dir);
        return;
    }

    // PSX: check flags bit 17 (slope physics active)
    if (!((flags >> 17) & 1)) {
        SetActionState(AS_STAND, 0);
        return;
    }

    // PSX: compute slide direction from collision normal
    // field148[6] = normalX at DynamicThing +172
    // field148[8] = normalZ at DynamicThing +180
    s32 normalX = field148[6]; // +172 relative to DynThing
    s32 normalZ = field148[8]; // +180 relative to DynThing

    if (normalX == 0 && normalZ == 0) {
        return;
    }

    // PSX: determine face direction from dominant slope axis
    s32 slideAngle = 0;
    if (normalX != 0) {
        slideAngle = (normalX > 0) ? 0x4000 : 0xC000;
    } else {
        if (normalZ <= 0) {
            slideAngle = PSX_ANGLE_180;
        }
    }

    // PSX: apply slide force along slope
    s32 sinA = rmSin16(faceAngle);
    s32 cosA = rmSin16((s16)(faceAngle + 0x4000));
    s32 velX = (s32)((sinA * (s64)runSpeed) >> 16);
    s32 velZ = (s32)((cosA * (s64)runSpeed) >> 16);

    // Project out the slope-normal component
    // (simplified from PSX matrix projection)
    contactForce.x += velX;
    contactForce.z += velZ;

    FaceAngleY(slideAngle, 0);
}
// PSX: _Straif__6Player (PLAYER.CPP:4606, 0x80033FF8)
// Player override of Humanoid::_Straif. Finds target if none,
// checks flag transitions, scales runSpeed by deltaTime.
void Player::_Straif() {
    MARKFUNCTION(0x80033FF8);

    // PSX: if no target (field256 == 0), find one via FindFoe
    if (field256 == 0) {
        // PSX: FindFoe(gp+468, gp+472, 0) - use default range values
        FindFoe(distantTargetRange, 0, 0);
        // PSX: SetHumanoidTarget with result - using FindFoe side effect
    }

    // PSX: check flags bit 17 (slope/special surface)
    if ((flags >> 17) & 1) {
        SetActionState(AS_SLOPE_SLIDE, 0);
        return;
    }

    u32 cb = (u32)commandBits;

    // PSX: bit 4 (taunt) -> release target, run
    if ((cb >> 4) & 1) {
        ReleaseTarget();
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // PSX: bit 3 (jump) -> release target, jump
    if ((cb >> 3) & 1) {
        ReleaseTarget();
        SetActionState(AS_JUMP, 0);
        return;
    }

    // PSX: scale runSpeed by global deltaTime multiplier
    // PSX: a1[52] = (gp+532 * a1[52]) >> 16
    s64 scaled = (s64)deltaTime * (s64)runSpeed;
    s32 scaledSpeed = (s32)(scaled >> 16);
    (void)scaledSpeed;

    // Delegate to base Humanoid strafe logic
    Humanoid::_Straif();
}

// PSX: _LadderDismount__6Player (PLAYER.CPP:4660, 0x80033DB8)
// Delegates to Humanoid::_LadderDismount (not yet reversed).
void Player::_LadderDismount() {
    MARKFUNCTION(0x80033DB8);
    // PSX: direct call to _LadderDismount__8Humanoid
    // Humanoid ladder system not yet reversed - stub
}

// PSX: _ClimbLadder__6Player (PLAYER.CPP:4680, 0x80033DD8)
// Delegates to Humanoid::_ClimbLadder (not yet reversed).
void Player::_ClimbLadder() {
    MARKFUNCTION(0x80033DD8);
    // PSX: direct call to _ClimbLadder__8Humanoid
    // Humanoid ladder system not yet reversed - stub
}

// PSX: _TableRoll__6Player (PLAYER.CPP:4700, 0x80033DF8)
// Handles rolling across a table surface. Applies small forward force
// during early frames, checks for end of surface, transitions to stand.
void Player::_TableRoll() {
    MARKFUNCTION(0x80033DF8);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    s16 frame = (s16)anim->currentFrame;

    // PSX: check animEnum == 86 (TABLE_ROLL)
    if (anim->animEnum != PLAYER_ANIM_TABLE_ROLL) {
        maxFallDivisor = 0;

        // PSX: check loopCount for animation completion
        if (anim->loopCount > 0) {
            // PSX: clear flags2 bits 4-6, transition to stand
            flags2 &= ~0x70u;
            SetActionState(AS_STAND, 0);
            // PSX: RestorePositionFromBip01 - not yet reversed
        }
        return;
    }

    // PSX: early frames (< 5): apply small forward force
    if (frame < 5) {
        SVector dir;
        dir.x = (s16)(orientation.x & 0xFFFF);
        dir.y = (s16)(orientation.y & 0xFFFF);
        dir.z = (s16)(orientation.z & 0xFFFF);
        dir.pad = 0;
        AddForce(20, &dir);
        maxFallDivisor = 0;
    }

    // PSX: at frame 13, check for table edge (floor height query)
    if (frame == 13) {
        // PSX: GetWorldFloorHeight and check if within 150 units
        // Collision system floor height query not yet reversed
        // If at edge, switch to TABLE_ROLL_END animation
        m->ApplyAnimToModel(0, PLAYER_ANIM_TABLE_ROLL_END, ANIM_LOOP, 0, 0);
    }

    // PSX: check loopCount for completion
    maxFallDivisor = 0;
    if (anim->loopCount > 0) {
        flags2 &= ~0x70u;
        SetActionState(AS_STAND, 0);
    }
}
