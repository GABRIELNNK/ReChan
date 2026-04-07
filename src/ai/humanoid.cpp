// humanoid.cpp - Humanoid class implementation
// Reversed from PSX C:\CHAN\GAME\SRC\AI\HUMANOID.CPP
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/model.h"
#include "gen/animmat.h"
#include "gen/animstruct.h"
#include "p3d/p3dmath.h"

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
    deltaTime = FIX16_ONE; // reset to 1.0

    // PSX step 9: face player if not player and not in certain states
    // (requires FightingCollision system, simplified for now)

    ProcessAction();

    Move();

    field368 = 0;
    thinkCounter++;
}

// PSX: Draw__8Humanoid (HUMANOID.CPP:1280)
// PSX: swaps animation matrices, sets pos/orient on model, Show(0),
// then updates collision bbox from skeleton joints (debug draw skipped).
void Humanoid::Draw() {
    MARKFUNCTION(0x80063A88);
    if (model) {
        HumanoidModel* hm = static_cast<HumanoidModel*>(model);
        // PSX: Swap__17AnimationMatrices(v2[24]) - swap double-buffered joint matrices
        if (hm->animMatrices) {
            hm->animMatrices->Swap();
        }
        // PSX: copy pos/orientation to model, then Show(0)
        Model* m = static_cast<Model*>(model);
        m->posX = pos.x;
        m->posY = pos.y;
        m->posZ = pos.z;
        m->rotX = (u16)(orientation.x & 0xFFFF);
        m->rotY = (u16)(orientation.y & 0xFFFF);
        m->rotZ = (u16)(orientation.z & 0xFFFF);
        m->Show(0);
        return;
    }
    // No model: fallback to debug wireframe
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

// PSX: CreateModel__8HumanoidPCc (HUMANOID.CPP:795, 0x80063248)
// PSX: creates HumanoidModel if not exists, creates Behaviour, then calls Thing::CreateModel
void Humanoid::CreateModel(const char* name) {
    MARKFUNCTION(0x800632B4);

    // PSX: if model == null, create HumanoidModel(136)
    if (!model) {
        HumanoidModel* hm = new HumanoidModel();
        model = hm;
        hm->backPtr = this;
    }

    // PSX: creates Behaviour if not exists (AI system)
    // Behaviour system not yet reversed - skip

    // PSX: calls Thing::CreateModel which does the LevelManager lookup
    Thing::CreateModel(name);

    // PSX: ApplyAnimToModel(thingType, 0, 2, 0, 0) then InitSemiTransMode
    Model* m = static_cast<Model*>(model);
    if (m) {
        m->ApplyAnimToModel(0, 0, 2, 0, 0);
        SModel* sm = static_cast<SModel*>(m);
        sm->InitSemiTransMode();
    }
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
        SetActionState(AS_DEAD, 0);
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
void Humanoid::SetActionState(u32 state, s32 param) {
    MARKFUNCTION(0x80065680);

    s32 prevState = actionState;
    (void)prevState;

    // PSX preamble: clear combatFlag, set flags bit 11, end sounds
    combatFlag = 0;
    flags |= TF_DYNAMIC;

    if (state >= AS_COUNT) return;

    // Record action state
    actionState = (s32)state;

    // Map state number to handler dispatch index
    // PSX uses a 74-entry jump table; here we map the known cases.
    switch (state) {
    case AS_INACTIVE_IDLE:     stateDispatch = SD_STAND; break;
    case AS_STAND:             stateDispatch = SD_STAND; break;
    case AS_STAND_ANIM:        stateDispatch = SD_STAND; break;
    case AS_DIVE_ROLL:         stateDispatch = SD_DIVE_ROLL; break;
    case AS_PAUSE:             stateDispatch = SD_PAUSE; break;
    case AS_JUMP:              stateDispatch = SD_JUMP; break;
    case AS_RUN:               stateDispatch = SD_RUN; break;
    case AS_BACKFLIP:          stateDispatch = SD_BACKFLIP; break;
    case AS_STRAFE:            stateDispatch = SD_STRAFE; break;
    case AS_SLOPE_SLIDE:      stateDispatch = SD_STRAFE; break;
    case AS_PUNCH_ATTACK:      stateDispatch = SD_THROW; break;
    case AS_KICK_ATTACK:       stateDispatch = SD_THROW; break;
    case AS_COMBAT_IDLE:       stateDispatch = SD_STAND; break;
    case AS_THROW_PICKUP:      stateDispatch = SD_THROW; break;
    case AS_FLYING_BACK_LAND:  stateDispatch = SD_FLYING_BACK; break;
    case AS_BACK_GRAB_RECOVER: stateDispatch = SD_STAND; break;
    case AS_GET_UP:            stateDispatch = SD_STAND; break;
    case AS_FLYING_BACK_CHECK: stateDispatch = SD_FLYING_BACK; break;
    case AS_SPIN_BACK_RECOVER: stateDispatch = SD_STAND; break;
    case AS_DEAD:              stateDispatch = SD_DEAD; break;
    case AS_HIT_EXPLOSION:     stateDispatch = SD_GOT_HIT_HIGH; break;
    case AS_HIT_ENVIRONMENT:   stateDispatch = SD_GOT_HIT_HIGH; break;
    default:
        // Many states set up specific animations and dispatch
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
    case SD_BACKFLIP:     _Jump(); break;
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
    f32 rad = atan2((f32)dx, (f32)dz);
    s32 targetAngle = RAD2ANGLE(rad) & 0xFFFF;

    if (immediate == 0) {
        // Snap directly to target angle
        orientation.y = targetAngle;
        return;
    }

    // Gradually turn towards target, limited by turnRate
    s32 diff = targetAngle - orientation.y;

    // Wrap difference to -32768..32767
    if (diff > PSX_ANGLE_180) diff -= PSX_ANGLE_360;
    if (diff < -PSX_ANGLE_180) diff += PSX_ANGLE_360;

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

// PSX: FaceAngleY__8Humanoidli (HUMANOID.CPP:2402)
// Turns orientation.y toward the given angle, limited by turnRate.
// If immediate == 0: snap directly. Otherwise: gradual turn.
void Humanoid::FaceAngleY(s32 angle, s32 immediate) {
    MARKFUNCTION(0x80064EB0);

    if (immediate == 0) {
        orientation.y = angle;
        return;
    }

    s32 diff = angle - orientation.y;

    // Wrap difference to -32768..32767
    if (diff > PSX_ANGLE_180) diff -= PSX_ANGLE_360;
    if (diff < -PSX_ANGLE_180) diff += PSX_ANGLE_360;

    s32 absDiff = (diff >= 0) ? diff : -diff;

    if (absDiff < (s32)turnRate) {
        orientation.y = angle;
    } else if (diff < 0) {
        orientation.y -= turnRate;
    } else {
        orientation.y += turnRate;
    }
}

// PSX: ReturnMostSignificant32BitNumber__FUl (HUMANOID.CPP:3826)
// Returns the 1-based index of the highest set bit, or 0 if input is 0.
// PSX uses binary search: test top 16 bits, then 8, then 4, etc.
static s32 ReturnMostSignificant32BitNumber(u32 value) {
    MARKFUNCTION(0x80066C4C);
    if (value == 0) return 0;
    s32 result = 0;
    s32 shift = 16;
    while (shift > 0) {
        u32 upper = value >> shift;
        if (upper != 0) {
            result += shift;
            value = upper;
        }
        shift >>= 1;
    }
    return result;
}

// PSX: _Stand__8Humanoid (HUMANOID.CPP:3859)
// Dispatches input commands from commandBits via highest-bit priority.
void Humanoid::_Stand() {
    MARKFUNCTION(0x80066CA0);

    s32 cmd = ReturnMostSignificant32BitNumber((u32)commandBits);
    flags2 |= 0x0008; // ground sticking

    if (cmd < 1 || cmd > 31) return;

    SVector dir;
    dir.x = (s16)orientation.x;
    dir.y = (s16)orientation.y;
    dir.z = (s16)orientation.z;
    dir.pad = 0;

    switch (cmd) {
    case 1: // guard/face
        FaceAngleY(faceAngle, 0);
        return;
    case 2: // kick -> run
        SetActionState(AS_RUN, runSpeed);
        return;
    case 3: // punch -> jump
        SetActionState(AS_JUMP, 0);
        return;
    case 4: // facing -> dive roll
        SetActionState(AS_DIVE_ROLL, 0);
        return;
    case 5: // roll to stand -> backflip
        SetActionState(AS_BACKFLIP, 0);
        return;
    case 6: // backflip -> pickup/throw or combat idle
        if (field500 != 0 || field504 != 0) {
            if (field500 != 0 && field316 != 0) {
                SetActionState(AS_COMBAT_IDLE, 0);
            } else {
                SetActionState(AS_THROW_PICKUP, 0);
            }
        } else {
            SetActionState(AS_COMBAT_IDLE, 0);
        }
        return;
    case 7: // combat attack -> punch
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    case 8: // dodge -> combat idle
        SetActionState(AS_COMBAT_IDLE, 0);
        return;
    case 9: // face + stand
        FaceAngleY(faceAngle, 0);
        SetActionState(AS_STAND, 0);
        return;
    case 10: // run -> strafe
        FaceAngleY(faceAngle, 0);
        SetActionState(AS_STRAFE, 0);
        return;
    case 11: // hit explosion
        SetActionState(AS_HIT_ENVIRONMENT, 0);
        return;
    case 12: // hit environment
        SetActionState(AS_HIT_EXPLOSION, 0);
        return;
    default:
        return;
    }
}

// PSX: _DiveRoll__8Humanoid (HUMANOID.CPP:3977)
// Phase-based dive roll: early frames apply forward force, mid/late check transitions.
void Humanoid::_DiveRoll() {
    MARKFUNCTION(0x80066E3C);

    // PSX: checks animation frame for phase transitions
    // Without animation system, check commandBits for immediate transitions
    s32 cb = commandBits;

    if (cb & 0x0008) { // punch -> jump
        SetActionState(AS_JUMP, 0);
        return;
    }
    if (cb & 0x0010) { // taunt -> pause
        SetActionState(AS_PAUSE, 0);
        return;
    }
    if (cb & 0x0004) { // kick -> run
        SetActionState(AS_RUN, 0);
        return;
    }
    if (cb & 0x0020) { // strafe
        SetActionState(AS_STRAFE, 0);
        return;
    }

    // PSX: apply forward force during early frames
    SVector dir;
    dir.x = (s16)orientation.x;
    dir.y = (s16)orientation.y;
    dir.z = (s16)orientation.z;
    dir.pad = 0;
    AddForce(runSpeed, &dir);

    // PSX: check stateTimer as frame proxy
    stateTimer++;
    if (stateTimer > 20) {
        SetActionState(AS_STAND, 0);
    }
}

// PSX: _Taunt__8Humanoid (HUMANOID.CPP:4069)
// Wait for animation to complete, then dispatch commands.
void Humanoid::_Taunt() {
    MARKFUNCTION(0x8006710C);

    // PSX: waits for animation complete flag, then dispatches
    // Without animation system, use stateTimer as frame proxy
    stateTimer++;
    if (stateTimer < 30) return;

    s32 cmd = ReturnMostSignificant32BitNumber((u32)commandBits);
    if (cmd < 2 || cmd > 31) {
        SetActionState(AS_STAND, 0);
        return;
    }

    switch (cmd) {
    case 2: // kick -> run
        SetActionState(AS_RUN, runSpeed);
        return;
    case 3: // taunt -> backflip
        SetActionState(AS_BACKFLIP, 0);
        return;
    case 5: // face+roll -> punch
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    case 6: // backflip -> pickup/throw or combat idle
        if (field500 != 0 || field504 != 0) {
            if (field500 != 0 && field316 != 0) {
                SetActionState(AS_COMBAT_IDLE, 0);
            } else {
                SetActionState(AS_THROW_PICKUP, 0);
            }
        } else {
            SetActionState(AS_COMBAT_IDLE, 0);
        }
        return;
    case 7: // combat idle
        SetActionState(AS_COMBAT_IDLE, 0);
        return;
    case 8: // dodge -> combat idle
        SetActionState(AS_COMBAT_IDLE, 0);
        return;
    case 9: // face+strafe
        FaceAngleY(faceAngle, 0);
        return;
    case 11: // hit env
        SetActionState(AS_HIT_ENVIRONMENT, 0);
        return;
    case 12: // hit explosion
        SetActionState(AS_HIT_EXPLOSION, 0);
        return;
    default:
        SetActionState(AS_STAND, 0);
        return;
    }
}

// PSX: _Pause__8Humanoid (HUMANOID.CPP:4153)
// Simple counter decrement, then return to stand.
void Humanoid::_Pause() {
    MARKFUNCTION(0x80067288);

    FaceAngleY(faceAngle, 0);

    if (field324 != 0) {
        field324--;
    } else {
        SetActionState(AS_STAND, 0);
    }
}

// PSX: _Run__8Humanoid (HUMANOID.CPP:4172)
// Extensive bit dispatch for attack/move transitions.
void Humanoid::_Run() {
    MARKFUNCTION(0x800672EC);

    flags2 |= 0x0008; // ground sticking
    s32 sd = commandBits;

    // Backflip (bit 6)
    if (sd & 0x0040) {
        SetActionState(AS_BACKFLIP, 0);
        return;
    }

    // Attack punch group (bits 8,10,12,20)
    if ((sd >> 8) & 1 || (sd >> 10) & 1 || (sd >> 12) & 1 || (sd >> 20) & 1) {
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // Attack kick group (bits 9,11,13,14)
    if ((sd >> 9) & 1 || (sd >> 11) & 1 || (sd >> 13) & 1 || (sd >> 14) & 1) {
        SetActionState(AS_KICK_ATTACK, 0);
        return;
    }

    // Multi-hit combat (bits 7,19,15,16,17,18) -> pickup/throw or combat idle
    if ((sd >> 7) & 1 || (sd >> 19) & 1 || (sd >> 15) & 1
            || (sd >> 16) & 1 || (sd >> 17) & 1 || (sd >> 18) & 1) {
        if (field500 != 0 || field504 != 0) {
            if (field500 != 0 && field316 != 0) {
                SetActionState(AS_COMBAT_IDLE, 0);
            } else {
                SetActionState(AS_THROW_PICKUP, 0);
            }
        } else {
            SetActionState(AS_COMBAT_IDLE, 0);
        }
        return;
    }

    // Taunt (bit 4) -> pause
    if (sd & 0x0010) {
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // Punch (bit 3) -> jump
    if (sd & 0x0008) {
        SetActionState(AS_JUMP, 0);
        return;
    }

    // Dive roll (bit 21)
    if (sd & 0x200000) {
        SetActionState(AS_DIVE_ROLL, 0);
        return;
    }

    // Guard (bit 1) -> stand
    if (sd & 0x0002) {
        SetActionState(AS_STAND, 0);
        return;
    }

    // Kick (bit 2) -> face + run forward
    if (sd & 0x0004) {
        FaceAngleY(faceAngle, 0);
        SVector dir;
        dir.x = (s16)orientation.x;
        dir.y = (s16)orientation.y;
        dir.z = (s16)orientation.z;
        dir.pad = 0;
        AddForce(runSpeed, &dir);
        return;
    }

    // Explosion (bit 31)
    if (sd < 0) {
        SetActionState(AS_HIT_ENVIRONMENT, 0);
        return;
    }

    // Env hit (bit 30)
    if ((sd >> 30) & 1) {
        SetActionState(AS_HIT_EXPLOSION, 0);
        return;
    }

    // Strafe (bit 5) -> face and strafe
    if (sd & 0x0020) {
        FaceAngleY(faceAngle, 0);
        SetActionState(AS_STRAFE, 0);
    }
}

// PSX: _Straif__8Humanoid (HUMANOID.CPP:4307)
// Complex: targeting, angle-based animation, movement.
void Humanoid::_Straif() {
    MARKFUNCTION(0x80067610);

    s32 sd = commandBits;

    // Attack punch group (bits 8,10,12,20)
    if ((sd >> 8) & 1 || (sd >> 10) & 1 || (sd >> 12) & 1 || (sd >> 20) & 1) {
        faceAngle = orientation.y;
        ReleaseTarget();
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // Attack kick group (bits 9,11,13,14)
    if ((sd >> 9) & 1 || (sd >> 11) & 1 || (sd >> 13) & 1 || (sd >> 14) & 1) {
        faceAngle = orientation.y;
        ReleaseTarget();
        SetActionState(AS_KICK_ATTACK, 0);
        return;
    }

    // Multi-hit combat (bits 7,19,15,16,17,18) -> pickup/throw or combat idle
    if ((sd >> 7) & 1 || (sd >> 19) & 1 || (sd >> 15) & 1
            || (sd >> 16) & 1 || (sd >> 17) & 1 || (sd >> 18) & 1) {
        if (field500 != 0 || field504 != 0) {
            if (field500 != 0 && field316 != 0) {
                ReleaseTarget();
                SetActionState(AS_COMBAT_IDLE, 0);
            } else {
                ReleaseTarget();
                SetActionState(AS_THROW_PICKUP, 0);
            }
        } else {
            ReleaseTarget();
            SetActionState(AS_COMBAT_IDLE, 0);
        }
        return;
    }

    // Explosion (bit 31)
    if (sd < 0) {
        ReleaseTarget();
        SetActionState(AS_HIT_ENVIRONMENT, 0);
        return;
    }

    // Env hit (bit 30)
    if ((sd >> 30) & 1) {
        ReleaseTarget();
        SetActionState(AS_HIT_EXPLOSION, 0);
        return;
    }

    // Guard release (bit 1)
    if (sd & 0x0002) {
        faceAngle = orientation.y;
        ReleaseTarget();
        SetActionState(AS_STAND, 0);
        return;
    }

    // Dive roll (bit 21)
    if (sd & 0x200000) {
        ReleaseTarget();
        SetActionState(AS_DIVE_ROLL, 0);
        return;
    }

    // Taunt (bit 4) -> pause
    if (sd & 0x0010) {
        ReleaseTarget();
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // Punch (bit 3) -> jump
    if (sd & 0x0008) {
        ReleaseTarget();
        SetActionState(AS_JUMP, 0);
        return;
    }

    // Kick (bit 2) -> face + run
    if (sd & 0x0004) {
        faceAngle = orientation.y;
        ReleaseTarget();
        SetActionState(AS_RUN, 0);
        return;
    }

    // Strafe (bit 5) -> face + strafe
    if (sd & 0x0020) {
        FaceAngleY(faceAngle, 0);
        ReleaseTarget();
        SetActionState(AS_STRAFE, 0);
        return;
    }

    // No commands -> strafe movement
    // PSX: face target, select strafe animation based on angle difference
    if (field256 != 0) {
        // field256 = target thing pointer (stored as s32)
        // FaceAngleY toward target's direction
        FaceAngleY(faceAngle, 1);
    }

    // Apply movement force in facing direction
    if (attackRange != 0) {
        SVector dir;
        dir.x = (s16)orientation.x;
        dir.y = (s16)orientation.y;
        dir.z = (s16)orientation.z;
        dir.pad = 0;
        AddForce(attackRange, &dir);
    }
}

// PSX: _Jump__8Humanoid (HUMANOID.CPP:4569)
// Apply forces, check air attack, call HandleLand.
void Humanoid::_Jump() {
    MARKFUNCTION(0x80067DBC);

    flags2 |= 0x0008; // ground sticking

    // If kick bit set (bit 2), apply directional jump
    if (commandBits & 0x0004) {
        FaceAngleY(faceAngle, 1);
        SVector dir;
        dir.x = (s16)orientation.x;
        dir.y = (s16)orientation.y;
        dir.z = (s16)orientation.z;
        dir.pad = 0;
        AddForce(runSpeed, &dir);
    }

    // PSX: check animation frame > threshold for air attack
    // Check attack bits (8,9,14)
    if ((commandBits >> 8) & 1 || (commandBits >> 9) & 1 || (commandBits >> 14) & 1) {
        commandBits = (commandBits | 0x4000) & ~0x0100 & ~0x0200;
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // PSX: call HandleLand (checks if landed on ground)
    HandleLand(0);
}

// PSX: _Fall__8Humanoid (HUMANOID.CPP:4620)
// Empty function on PSX (8 bytes, just jr $ra + nop)
void Humanoid::_Fall() {
    MARKFUNCTION(0x80067F2C);
}

// PSX: _Pickup__8Humanoid (HUMANOID.CPP:4959)
// Grab item at animation frame threshold.
void Humanoid::_Pickup() {
    MARKFUNCTION(0x80068508);

    // PSX: checks model->animStruct frame >= grabFrame, then sets up pickup
    // Without animation system, use stateTimer as proxy
    stateTimer++;

    if (field500 != 0 && stateTimer > 10) {
        flags2 |= 0x0001; // carrying flag
    }

    // PSX: if animation complete, return to stand
    if (stateTimer > 20) {
        SetActionState(AS_STAND, 0);
    }
}

// PSX: _Throw__8Humanoid (HUMANOID.CPP:4998, 0x800685A8)
// Face target during early frames, release thrown object at animation
// frame threshold, transition to stand when animation completes.
void Humanoid::_Throw() {
    MARKFUNCTION(0x800685A8);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    s16 frame = (s16)anim->currentFrame;

    // PSX: face target during first 6 frames
    if (frame < 6 && field256 != 0) {
        Thing* target = (Thing*)(intptr_t)field256;
        FaceThing(target, 1);
    }

    // PSX: if pickup object exists and past throw frame, release it
    if (field500 != 0) {
        // PSX: GetThrowMoveThrowFrame (not reversed) - use frame 8 as proxy
        if (frame >= 8) {
            if (flags2 & 0x0001) {
                // PSX: carrying flag set - release with direction
                // PSX: PlayDialog(84, 10) if this == thePlayer
                field500 = 0;
                flags2 &= ~0x0001;
            } else {
                // PSX: release without direction
                field500 = 0;
            }
        }
    }

    // PSX: if animation completed (loopCount > 0)
    if (anim->loopCount > 0) {
        ReleaseTarget();
        SetActionState(AS_STAND, 0);
    }
}

// PSX: _GotHitHigh__8Humanoid (HUMANOID.CPP:5114, 0x8006882C)
// First frame: force animation to specific global frame. Adjusts speed
// based on field466 knockback. Sets death state if health gone.
void Humanoid::_GotHitHigh() {
    MARKFUNCTION(0x8006882C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: on first frame (walkCycleFlag == 46), force specific global frame
    if (walkCycleFlag == 46) {
        // PSX: ForceFrame(gp+1856) - global value, use 0 as default
        anim->ForceFrame(0);
        walkCycleFlag = 1;
    }

    // PSX: if field466 (knockback speed) nonzero, adjust animation speed
    if (field466 != 0) {
        // PSX: speed = rmDiv16i(endFrame, field466 << 16)
        if (anim->endFrame != 0) {
            anim->speed = rmDiv16i(anim->endFrame, (s32)field466 << 16);
        }
    }

    // PSX: if health == 0, set walkCycleFlag to AS_DEAD (72)
    if (health <= 0) {
        walkCycleFlag = (s32)AS_DEAD;
    }
}

// PSX: _GotHitMed__8Humanoid (HUMANOID.CPP:5161, 0x800688B4)
// Adjusts animation speed from knockback. Sets death state if HP gone.
void Humanoid::_GotHitMed() {
    MARKFUNCTION(0x800688B4);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: if field466 nonzero, adjust animation speed
    if (field466 != 0) {
        if (anim->endFrame != 0) {
            anim->speed = rmDiv16i(anim->endFrame, (s32)field466 << 16);
        }
    }

    // PSX: if health == 0, set walkCycleFlag to AS_DEAD
    if (health <= 0) {
        walkCycleFlag = (s32)AS_DEAD;
    }
}

// PSX: _GotHitLow__8Humanoid (HUMANOID.CPP:5232, 0x800689B4)
// Identical logic to _GotHitMed: speed adjust + death check.
void Humanoid::_GotHitLow() {
    MARKFUNCTION(0x800689B4);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    if (field466 != 0) {
        if (anim->endFrame != 0) {
            anim->speed = rmDiv16i(anim->endFrame, (s32)field466 << 16);
        }
    }

    if (health <= 0) {
        walkCycleFlag = (s32)AS_DEAD;
    }
}

// PSX: _Stunned__8Humanoid (HUMANOID.CPP:5333, 0x80068AB4)
// Countdown stun timer (field468). On expire, clean up animControl
// and return to stand. On health depletion, go dead.
void Humanoid::_Stunned() {
    MARKFUNCTION(0x80068AB4);

    if ((s16)field468 > 0) {
        // PSX: decrement stun timer by rate (field468 - comboCount)
        field468 = (u16)((u16)field468 - comboCount);
    } else {
        // Stun expired
        field468 = 0;

        // PSX: if animControl target exists, signal and clear
        if (animControl != 0) {
            // PSX: *(animControl + 108) = 1 — signal stun target complete
            animControl = 0;
        }

        SetActionState(AS_STAND, 0);
    }

    // PSX: death check (health == 0)
    if (health <= 0) {
        if (animControl != 0) {
            animControl = 0;
        }
        SetActionState(AS_DEAD, 0);
    }
}

// PSX: _SpinBack__8Humanoid (HUMANOID.CPP:5373, 0x80068B78)
// Wait for spin-back animation to complete (loopCount > 0),
// then transition to recovery state.
void Humanoid::_SpinBack() {
    MARKFUNCTION(0x80068B78);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: if loopCount > 0, transition to spin-back recovery
    if (anim->loopCount > 0) {
        SetActionState(AS_SPIN_BACK_RECOVER, 0);
    }
}

// PSX: _FlyingBack__8Humanoid (HUMANOID.CPP:5397, 0x80068BC8)
// Scale velocity by global knockback factor, check animation complete
// for landing transition, check ground for ground-check transition.
void Humanoid::_FlyingBack() {
    MARKFUNCTION(0x80068BC8);

    // PSX: velocity.x *= gp+1860 (knockback damping factor)
    // PSX: maxFallDivisor = 18 / gp+1764
    // Global values not reversed - use defaults
    maxFallDivisor = 18;

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: if animation complete (loopCount > 0), transition to landing
    if (anim->loopCount > 0) {
        SetActionState(AS_FLYING_BACK_LAND, 0);
    }

    // PSX: if on ground (flags bit 12), transition to ground check
    if (flags & TF_ON_GROUND) {
        SetActionState(AS_FLYING_BACK_CHECK, 0);
    }
}

// PSX: _Collapse__8Humanoid (HUMANOID.CPP:5476, 0x80068DD4)
// Play collapse groan dialog, call ProcessControl, check animation
// complete + on-ground for get-up/death transition.
void Humanoid::_Collapse() {
    MARKFUNCTION(0x80068DD4);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: LoadDialog(1, 50) - groan sound (dialog system not reversed)

    // PSX: vtable+260 call - ProcessControl equivalent
    ProcessControl();

    // PSX: check loopCount > 0 AND on-ground
    if (anim->loopCount <= 0) {
        return;
    }
    if (!(flags & TF_ON_GROUND)) {
        return;
    }

    // PSX: if health == 0, die
    if (health <= 0) {
        SetActionState(AS_DEAD, 0);
        return;
    }

    // PSX: check stateTimer against humanoidDataID threshold
    if ((s16)humanoidDataID < (s16)stateTimer) {
        // PSX: if not this player AND model has bit 4 flag, signal get-up
        if (this != (Humanoid*)Player::s_player) {
            // PSX: check model->modelFlags bit 4
            if (m->modelFlags & 0x10) {
                Player::s_player->SignalEnemyGetUp();
            }
        }
        SetActionState(AS_GET_UP, 0);
    } else {
        stateTimer++;
    }
}

// PSX: _Dead__8Humanoid (HUMANOID.CPP:5723, 0x800691DC)
// Complex death handler: type-specific checks, signal player,
// toggle flags, cleanup, remove from fighting system.
void Humanoid::_Dead() {
    MARKFUNCTION(0x800691DC);

    // PSX: type check for respawn eligibility
    // Types 10, 12, 13, 15, 17 are boss types that don't signal player
    bool isBossType = false;
    switch (thingType) {
    case AITypes::TT_GRONTAR:
    case AITypes::TT_PAUL:
    case AITypes::TT_OSCAR:
    case AITypes::TT_DANTE:
    case AITypes::TT_BUTCH:
        isBossType = true;
        break;
    }

    if (!isBossType) {
        // PSX: SignalEnemyDead(thePlayer, this)
        if (Player::s_player) {
            Player::s_player->SignalEnemyDead(this);
        }

        // PSX: toggle flags bit 8 based on thinkCounter state
        if ((thinkCounter & 0x03) == 2) {
            if (flags & TF_BIT8) {
                flags &= ~TF_BIT8;
            } else {
                flags |= TF_BIT8;
            }
        }

        // PSX: check if death animation complete + enough time elapsed
        if (!model) {
            goto cleanup;
        }
        {
            Model* m = static_cast<Model*>(model);
            if (m->modelFlags & 0x10) {
                // Animation still playing
                return;
            }
            if (thinkCounter < 41) {
                return;
            }
        }

cleanup:
        // PSX: RemoveHumanoid from FightingCollision (not reversed)
        ReleaseTarget();
        flags &= ~0x0080; // clear bit 7

        if (field260 != 0) {
            // PSX: set model flag for fade-out
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->modelFlags |= 0x20;
            }
        } else {
            // PSX: call Kill virtual to deactivate
            Kill();
        }
    }

    // PSX: KillDialog(0, 0, 512) - dialog system not reversed
}
