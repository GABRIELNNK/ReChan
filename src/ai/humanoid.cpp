// humanoid.cpp - Humanoid class implementation
// Reversed from PSX C:\CHAN\GAME\SRC\AI\HUMANOID.CPP
#include "ai/humanoid.h"
#include "ai/player.h"
#include "p3d/p3dmath.h"
#include <cmath>

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
    stateDispatch = SD_STAND;
    field348 = 8;
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

    // PSX step 1: CHumanoidSound::Think (sound system not yet implemented)
    // PSX step 2-3: random() + LoadEnemyTaunts (dialog system not yet implemented)
    // PSX step 4: check flags2 bit 7 for dialog state (not yet implemented)

    // PSX step 6: clear flag bits
    flags &= ~TF_BIT1;
    flags2 &= ~TF2_BIT3;

    // PSX step 7: process AI behaviour
    ProcessControl();

    // PSX step 8: delta time computation (fixed-point 16.16 multiply)
    // result = (attackRange * deltaTime) >> 16
    s64 dt = (s64)attackRange * (s64)deltaTime;
    s32 scaledRange = (s32)((u64)dt >> 16);
    (void)scaledRange; // stored to PSX +212 (animation speed field, not yet wired)
    deltaTime = 0x10000; // reset to 1.0

    // PSX step 9: face player if not player and not in certain states
    // (requires FightingCollision system, simplified for now)

    // PSX step 10: ProcessAction dispatches to current state handler
    ProcessAction();

    // PSX step 11: UpdatePosition (virtual, moves entity based on velocity)
    UpdatePosition();

    // PSX step 12: clear per-frame field, increment think counter
    field368 = 0;
    thinkCounter++;
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

    // PSX: if this != global player, clear target
    if (this != (Thing*)Player::s_player) {
        SetTarget(nullptr);
    }

    turnRate = 2730;
    stateTimer = 0;
    thinkCounter = 0;

    // PSX: flags |= 4 (needs activation), flags &= ~0x100
    flags |= TF_NEEDS_ACTIVATION;
    flags &= ~TF_BIT8;
}

// PSX: Activate__8Humanoid (HUMANOID.CPP:760)
void Humanoid::Activate() {
    MARKFUNCTION(0x80063210);
    // PSX: save prior activated state, then call base
    bool wasActivated = (flags & TF_ACTIVATED) != 0;
    Thing::Activate();
    // PSX: if newly activated (wasn't before, is now), insert into FightingCollision
    bool isActivated = (flags & TF_ACTIVATED) != 0;
    if (!wasActivated && isActivated) {
        // FightingCollision::InsertHumanoid(this) not yet reversed
    }
}

// PSX: Deactivate__8Humanoid (HUMANOID.CPP:776)
void Humanoid::Deactivate() {
    MARKFUNCTION(0x80063270);
    Thing::Deactivate();
    // PSX: if was activated (bit 4 now clear after base call), remove from FightingCollision
    // FightingCollision::RemoveHumanoid(this) not yet reversed
}

// PSX: Move__8Humanoid (HUMANOID.CPP:1544)
void Humanoid::Move() {
    MARKFUNCTION(0x80064100);
    DynamicThing::Move();
    // PSX: HandleAnimationControl (animation system not yet implemented)
    // PSX: CheckSwitches for trigger volumes (world system not yet implemented)
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
// PSX: 904 bytes. Reads tag items for damage/force/impulse from the other
// Thing, applies state-dependent hit reactions, subtracts HP, applies knockback.
// Requires: tag item system, damage types enum, sound system.
void Humanoid::HandleCollision(Thing* other, s32 damage) {
    MARKFUNCTION(0x80064808);
    if (!other) return;
    if (damage <= 0) return;
    health -= damage;
    if (health <= 0) {
        health = 0;
        SetActionState(AS_COLLAPSE, 0);
    }
}

// PSX: AnalyzeMesh__8HumanoidP6DBRoot (HUMANOID.CPP:535)
void Humanoid::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80062E54);
    Thing::AnalyzeMesh(root);
}

// PSX: SetActionState__8HumanoidUll (HUMANOID.CPP:2792)
// PSX: 5580 bytes, 74-case switch. Each case sets up animation, flags, and the
// method thunk (stateDispatch) that ProcessAction uses to call the state handler.
// On PC, we set stateDispatch to the vtable index corresponding to the handler.
// vtable indices: 22=_Stand, 23=_Run, 24=_Jump, 25=_Fall, 26=_Straif,
// 27=_DiveRoll, 28=_Taunt, 29=_Pause, 30=_GotHitHigh, 31=_GotHitMed,
// 32=_GotHitLow, 33=_Collapse, 34=_Dead, 35=_SpinBack, 36=_FlyingBack,
// 37=_Stunned, 38=_Throw, 39=_Pickup
void Humanoid::SetActionState(u32 state, s32 param) {
    MARKFUNCTION(0x80065680);

    s32 prevState = actionState;
    (void)prevState;

    // PSX preamble: clear combatFlag, set flags bit 11, end sounds
    combatFlag = 0;
    flags |= TF_DYNAMIC;
    // PSX: if sound object (field328) != null, EndAllSounds()

    if (state >= AS_COUNT) return;

    // Record action state
    actionState = (s32)state;

    // Map state number to handler dispatch index
    // PSX uses a 74-entry jump table; here we map the known cases.
    switch (state) {
    case AS_INACTIVE_IDLE: stateDispatch = SD_STAND; break;
    case AS_STAND:         stateDispatch = SD_STAND; break;
    case AS_STAND_ANIM:    stateDispatch = SD_STAND; break;
    case AS_RUN:           stateDispatch = SD_RUN; break;
    case AS_JUMP:          stateDispatch = SD_JUMP; break;
    case AS_TAUNT:         stateDispatch = SD_TAUNT; break;
    case AS_FALL:          stateDispatch = SD_FALL; break;
    case AS_STRAFE:        stateDispatch = SD_STRAFE; break;
    case AS_DIVE_ROLL:     stateDispatch = SD_DIVE_ROLL; break;
    case AS_PAUSE:         stateDispatch = SD_PAUSE; break;
    case AS_GOT_HIT_HIGH:  stateDispatch = SD_GOT_HIT_HIGH; break;
    case AS_COLLAPSE:      stateDispatch = SD_COLLAPSE; break;
    case AS_DEAD:          stateDispatch = SD_DEAD; break;
    case AS_SPIN_BACK:     stateDispatch = SD_SPIN_BACK; break;
    case AS_FLYING_BACK:   stateDispatch = SD_FLYING_BACK; break;
    case AS_STUNNED:       stateDispatch = SD_STUNNED; break;
    case AS_THROW:         stateDispatch = SD_THROW; break;
    case AS_PICKUP:        stateDispatch = SD_PICKUP; break;
    case AS_THROW_PUNCH:   stateDispatch = SD_THROW; break;
    case AS_THROW_KICK:    stateDispatch = SD_THROW; break;
    default:
        // Many states (19-33, 36-73) set up specific animations and dispatch
        // to one of the above handlers. Default to _Stand for safety.
        stateDispatch = SD_STAND;
        break;
    }

    stateTimer = 0;
    (void)param;
}

// PSX: ProcessAction__8Humanoid (HUMANOID.CPP:2659)
// PSX uses a method thunk at fields +344/+346/+348 to dispatch to the current
// state handler. On PC, we dispatch via stateDispatch (the vtable index).
void Humanoid::ProcessAction() {
    MARKFUNCTION(0x8006538C);
    if (stateDispatch == SD_NONE) return;

    switch (stateDispatch) {
    case SD_STAND:        _Stand(); break;
    case SD_RUN:          _Run(); break;
    case SD_JUMP:         _Jump(); break;
    case SD_FALL:         _Fall(); break;
    case SD_STRAFE:       _Straif(); break;
    case SD_DIVE_ROLL:    _DiveRoll(); break;
    case SD_TAUNT:        _Taunt(); break;
    case SD_PAUSE:        _Pause(); break;
    case SD_GOT_HIT_HIGH: _GotHitHigh(); break;
    case SD_GOT_HIT_MED:  _GotHitMed(); break;
    case SD_GOT_HIT_LOW:  _GotHitLow(); break;
    case SD_COLLAPSE:     _Collapse(); break;
    case SD_DEAD:         _Dead(); break;
    case SD_SPIN_BACK:    _SpinBack(); break;
    case SD_FLYING_BACK:  _FlyingBack(); break;
    case SD_STUNNED:      _Stunned(); break;
    case SD_THROW:        _Throw(); break;
    case SD_PICKUP:       _Pickup(); break;
    default: break;
    }
}

// PSX: ProcessControl__8Humanoid (HUMANOID.CPP:961)
void Humanoid::ProcessControl() {
    MARKFUNCTION(0x80063660);
    if (behaviour) {
        behaviour->Process();
    }
}

// PSX: FaceThing__8HumanoidP5Thingi (HUMANOID.CPP:2252)
void Humanoid::FaceThing(Thing* target, s32 immediate) {
    MARKFUNCTION(0x80064B98);
    if (!target) return;
    LVector point = target->pos;
    FacePoint(point, immediate);
}

// PSX: FacePoint__8HumanoidRC10tagLVectori (HUMANOID.CPP:2260)
// Computes the angle from this->pos to point, then either snaps or gradually
// turns orientation.y towards it, limited by turnRate.
void Humanoid::FacePoint(const LVector& point, s32 immediate) {
    MARKFUNCTION(0x80064BD0);

    s32 dx = point.x - pos.x;
    s32 dz = point.z - pos.z;

    // Compute target angle using atan2(dx, dz) in PSX binary angle units (0-65535)
    // PSX convention: 0 = +Z, 0x4000 = +X, 0x8000 = -Z, 0xC000 = -X
    f32 rad = std::atan2((f32)dx, (f32)dz);
    s32 targetAngle = ((s32)(rad * P3D_RAD_TO_ANGLE)) & 0xFFFF;

    if (immediate == 0) {
        // Snap directly to target angle
        orientation.y = targetAngle;
        return;
    }

    // Gradually turn towards target, limited by turnRate
    s32 diff = targetAngle - orientation.y;

    // Wrap difference to -32768..32767
    if (diff > 0x8000) diff -= 0x10000;
    if (diff < -0x8000) diff += 0x10000;

    s32 absDiff = (diff >= 0) ? diff : -diff;

    if (absDiff < (s32)turnRate) {
        // Close enough, snap to target
        orientation.y = targetAngle;
    } else if (diff >= 0) {
        orientation.y += turnRate;
    } else {
        orientation.y -= turnRate;
    }
}

// PSX: FindFoe__8HumanoidUlli (HUMANOID.CPP:2446)
// Searches nearby humanoids within range for a combat target.
// Requires FightingCollision system for entity iteration.
void Humanoid::FindFoe(u32 /*range*/, s32 /*param*/, s32 /*immediate*/) {
    MARKFUNCTION(0x80064F94);
}

// PSX: SetTarget__8HumanoidP8Humanoid (HUMANOID.CPP:2502)
void Humanoid::SetTarget(Humanoid* target) {
    MARKFUNCTION(0x8006511C);
    if (target == (Humanoid*)this) return;
    // PSX: sets field384 = target, increments target's refcount
    field384 = (s32)(intptr_t)target;
}

// PSX: ReleaseTarget__8Humanoid (HUMANOID.CPP:2553)
void Humanoid::ReleaseTarget() {
    MARKFUNCTION(0x80065200);
    field384 = 0;
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
