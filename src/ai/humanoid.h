// humanoid.h - Humanoid class (combat-capable Thing)
// Reversed from PSX C:\CHAN\GAME\SRC\AI\HUMANOID.CPP
#pragma once

#include "ai/thing.h"

#include "ai/behaviour.h"

// Action state IDs for Humanoid::SetActionState
enum ActionState : u32 {
    AS_INACTIVE_IDLE = 0,
    AS_STAND         = 1,
    AS_STAND_ANIM    = 2,
    AS_RUN           = 4,
    AS_JUMP          = 5,
    AS_TAUNT         = 6,
    AS_FALL          = 7,
    AS_STRAFE        = 8,
    AS_DIVE_ROLL     = 9,
    AS_PAUSE         = 10,
    AS_GOT_HIT_HIGH  = 11,
    AS_COLLAPSE      = 12,
    AS_DEAD          = 13,
    AS_SPIN_BACK     = 14,
    AS_FLYING_BACK   = 15,
    AS_STUNNED       = 16,
    AS_THROW         = 17,
    AS_PICKUP        = 18,
    AS_THROW_PUNCH   = 34,
    AS_THROW_KICK    = 35,
    AS_COUNT         = 74,
};

// State dispatch indices for Humanoid::ProcessAction
enum StateDispatch : u16 {
    SD_NONE         = 0,
    SD_STAND        = 22,
    SD_RUN          = 23,
    SD_JUMP         = 24,
    SD_FALL         = 25,
    SD_STRAFE       = 26,
    SD_DIVE_ROLL    = 27,
    SD_TAUNT        = 28,
    SD_PAUSE        = 29,
    SD_GOT_HIT_HIGH = 30,
    SD_GOT_HIT_MED  = 31,
    SD_GOT_HIT_LOW  = 32,
    SD_COLLAPSE     = 33,
    SD_DEAD         = 34,
    SD_SPIN_BACK    = 35,
    SD_FLYING_BACK  = 36,
    SD_STUNNED      = 37,
    SD_THROW        = 38,
    SD_PICKUP       = 39,
};

// Humanoid - DynamicThing with combat, animation, and AI state
// PSX: ~560 bytes. Base class for Player and all enemy types.
// Source: C:\CHAN\GAME\SRC\AI\HUMANOID.CPP
class Humanoid : public DynamicThing {
public:
    // PSX +200 (s32): action state ID (-1 = none)
    s32 actionStateA = -1;
    // PSX +204 (s32): previous action state ID (-1 = none)
    s32 actionStateB = -1;

    // PSX +208 (s32): facing range / attack range
    s32 attackRange = 0;

    // PSX +216 (s32): reserved
    s32 field216 = 0;

    // PSX +220 (ptr): face angle data table
    void* faceAngleData = nullptr;

    // PSX +224 (s32): animation control
    s32 animControl = 0;

    // PSX +228 (s32): reserved
    s32 field228 = 0;

    // PSX +256 (s32): reserved
    s32 field256 = 0;

    // PSX +260 (u8): reserved
    u8 field260 = 0;

    // PSX +280 (s32,s32,s32): bounding box for collision
    // Initialized to {175, 0, 768} then {175, 0, 768} (two sets)
    LVector collBboxMin = {175, 0, 768};
    LVector collBboxMax = {175, 0, 768};

    // PSX +308 (s32): state timer (cleared on reset / state change)
    s32 stateTimer = 0;
    // PSX +312 (s32): think counter (incremented each Think)
    s32 thinkCounter = 0;

    // PSX +316..+336: combat state fields
    s32 field316 = 0;
    s32 field320 = 0;
    s32 field324 = 0;
    s32 field328 = 0;
    // PSX +332 (s32): sound handle
    s32 soundHandle = 0;
    // PSX +336 (s32): sound param
    s32 soundParam = 0;

    // PSX +340 (u8): combat flag
    u8 combatFlag = 0;

    // PSX +342 (u16): turn rate (2730 = ~15 degrees/frame)
    u16 turnRate = 2730;

    // PSX +344 (u16): reserved
    u16 field344 = 0;
    // PSX +346 (u16): state handler dispatch index (see StateDispatch enum)
    u16 stateDispatch = SD_STAND;

    // PSX +348 (u16): vtable pointer offset (always 8 on PSX)
    u16 field348 = 8;

    // PSX +356 (s32): current action state number
    s32 actionState = -1;
    // PSX +360 (s32): walk cycle flag
    s32 walkCycleFlag = 0;
    // PSX +364 (s32): reserved
    s32 field364 = 0;
    // PSX +368 (s32): cleared each Think
    s32 field368 = 0;
    // PSX +372 (s32): delta time multiplier (0x10000 = 1.0)
    s32 deltaTime = 0x10000;

    // PSX +376,+378,+380 (u16): punch/kick/combo direction indices
    u16 punchDir = 0;
    u16 kickDir = 0;
    u16 comboDir = 0;

    // PSX +384..+416: targeting / combat tracking
    s32 field384 = 0;
    s32 field388 = 0;
    s32 field392 = 0;
    s32 field396 = 0;
    s32 field400 = 0;
    s32 field404 = 0;
    s32 field408 = -1;
    s32 field412 = 0;
    s32 field416 = 0;

    // PSX +424,+428,+432: reserved (combat state)
    s32 field424 = 0;
    s32 field428 = 0;
    s32 field432 = 0;

    // PSX +436 (s32): reserved
    s32 field436 = 0;

    // PSX +440 (ptr): Behaviour* (AI behaviour tree)
    Behaviour* behaviour = nullptr;

    // PSX +444 (s32): reserved
    s32 field444 = 0;

    // PSX +448 (s32): spawn count (set to 1)
    s32 spawnCount = 1;

    // PSX +452 (s32): reserved
    s32 field452 = 0;

    // PSX +460 (s32): humanoid data ptr (from GetHumanoidData)
    void* humanoidData = nullptr;

    // PSX +464 (u16): humanoid data ID
    u16 humanoidDataID = 20;

    // PSX +466,+468,+470 (u16): combat counters
    u16 field466 = 0;
    u16 field468 = 0;
    u16 comboCount = 1;

    // PSX +476 (ptr): FightingSystem* (current)
    void* fightingSystem = nullptr;
    // PSX +480 (ptr): FightingSystem* (default)
    void* defaultFightingSystem = nullptr;

    // PSX +484,+488: reserved
    s32 field484 = 0;
    s32 field488 = 0;

    // PSX +492 (s32): distant target range (16000)
    s32 distantTargetRange = 16000;

    // PSX +496 (s32): reserved
    s32 field496 = 0;
    // PSX +500,+504 (s32): reserved
    s32 field500 = 0;
    s32 field504 = 0;

    // PSX +508 (u16): from global (gp+1756)
    u16 field508 = 0;

    // PSX +512 (ptr): Trails* (visual trail effect)
    void* trails = nullptr;

    // PSX +528 (s32): reserved
    s32 field528 = 0;

    // PSX +532 (s32): reserved
    s32 field532 = 0;

    // PSX +536: embedded ccNode for combat/targeting list
    ccNode combatNode;


    // PSX: __8HumanoidPC10tagLVectorUs (HUMANOID.CPP:350)
    Humanoid(const LVector* initialPos, u16 type);

    // PSX: _._8Humanoid (HUMANOID.CPP:490)
    ~Humanoid() override;


    // PSX: Think__8Humanoid (HUMANOID.CPP:1133)
    void Think() override;

    // PSX: Draw__8Humanoid (HUMANOID.CPP:1280)
    void Draw() override;

    // PSX: Reset__8Humanoid (HUMANOID.CPP:513)
    void Reset() override;

    // PSX: Activate__8Humanoid (HUMANOID.CPP:760)
    void Activate() override;

    // PSX: Deactivate__8Humanoid (HUMANOID.CPP:776)
    void Deactivate() override;

    // PSX: Move__8Humanoid (HUMANOID.CPP:1544)
    void Move() override;

    // PSX: CreateModel__8HumanoidPCc (HUMANOID.CPP:795)
    void CreateModel(const char* name) override;

    // PSX: DeleteModel__8Humanoid (HUMANOID.CPP:910)
    void DeleteModel() override;

    // PSX: HandleCollision__8HumanoidP5Thingle (HUMANOID.CPP:1997)
    void HandleCollision(Thing* other, s32 damage) override;

    // PSX: AnalyzeMesh__8HumanoidP6DBRoot (HUMANOID.CPP:535)
    void AnalyzeMesh(DBRoot* root) override;


    // PSX: SetActionState__8HumanoidUll (HUMANOID.CPP:2792)
    virtual void SetActionState(u32 state, s32 param);

    // PSX: ProcessAction__8Humanoid (HUMANOID.CPP:2659)
    virtual void ProcessAction();

    // PSX: ProcessControl__8Humanoid (HUMANOID.CPP:961)
    virtual void ProcessControl();

    // PSX: FaceThing__8HumanoidP5Thingi (HUMANOID.CPP:2252)
    void FaceThing(Thing* target, s32 immediate);

    // PSX: FacePoint__8HumanoidRC10tagLVectori (HUMANOID.CPP:2260)
    void FacePoint(const LVector& point, s32 immediate);

    // PSX: FindFoe__8HumanoidUlli (HUMANOID.CPP:2446)
    void FindFoe(u32 range, s32 param, s32 immediate);

    // PSX: SetTarget__8HumanoidP8Humanoid (HUMANOID.CPP:2502)
    void SetTarget(Humanoid* target);

    // PSX: ReleaseTarget__8Humanoid (HUMANOID.CPP:2553)
    void ReleaseTarget();

    virtual void _Stand();
    virtual void _Run();
    virtual void _Jump();
    virtual void _Fall();
    virtual void _Straif();
    virtual void _DiveRoll();
    virtual void _Taunt();
    virtual void _Pause();
    virtual void _GotHitHigh();
    virtual void _GotHitMed();
    virtual void _GotHitLow();
    virtual void _Collapse();
    virtual void _Dead();
    virtual void _SpinBack();
    virtual void _FlyingBack();
    virtual void _Stunned();
    virtual void _Throw();
    virtual void _Pickup();
};
