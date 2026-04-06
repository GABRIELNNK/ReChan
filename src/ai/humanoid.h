// humanoid.h - Humanoid class (combat-capable Thing)
// Reversed from PSX C:\CHAN\GAME\SRC\AI\HUMANOID.CPP
#pragma once

#include "ai/thing.h"
#include "p3d/p3dmath.h"
#include "ai/behaviour.h"

// Action state IDs for Humanoid::SetActionState
// PSX: 74-case switch at 0x80065680. IDs confirmed from handler transitions.
enum ActionState : u32 {
    AS_INACTIVE_IDLE     = 0,
    AS_STAND             = 1,
    AS_STAND_ANIM        = 2,
    AS_DIVE_ROLL         = 4,
    AS_PAUSE             = 6,
    AS_JUMP              = 8,
    AS_RUN               = 10,
    AS_BACKFLIP          = 11,
    AS_STRAFE            = 12,
    AS_FALL              = 13,
    AS_HARDFALL          = 14,
    AS_HARDLAND          = 15,
    AS_STRAFE_SPECIAL    = 20,
    AS_PUNCH_ATTACK      = 32,
    AS_KICK_ATTACK       = 34,
    AS_COMBAT_IDLE       = 36,
    AS_THROW_PICKUP      = 45,
    AS_FLYING_BACK_LAND  = 58,
    AS_BACK_GRAB_RECOVER = 62,
    AS_GET_UP            = 68,
    AS_FLYING_BACK_CHECK = 70,
    AS_SPIN_BACK_RECOVER = 71,
    AS_DEAD              = 72,
    AS_HIT_EXPLOSION     = 74,
    AS_HIT_ENVIRONMENT   = 75,
    AS_COUNT             = 76,
};

// State dispatch indices for Humanoid::ProcessAction
enum StateDispatch : u16 {
    SD_NONE         = 0,
    SD_STAND        = 22,
    SD_DIVE_ROLL    = 23,
    SD_PAUSE        = 24,
    SD_RUN          = 25,
    SD_BACKFLIP     = 26,
    SD_STRAFE       = 27,
    SD_JUMP         = 28,
    SD_FALL         = 29,
    SD_GOT_HIT_HIGH = 30,
    SD_GOT_HIT_MED  = 31,
    SD_GOT_HIT_LOW  = 32,
    SD_WALLJUMP     = 33,
    SD_COLLAPSE     = 34,
    SD_DEAD         = 35,
    SD_SPIN_BACK    = 36,
    SD_FLYING_BACK  = 37,
    SD_STUNNED      = 38,
    SD_THROW        = 39,
    SD_PICKUP       = 40,
    // Player-specific dispatch via direct function pointer (stateDispatch = -1 on PSX)
    SD_HARDFALL     = 250,
    SD_HARDLAND     = 251,
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

    // PSX +212 (s32): run speed for AddForce
    s32 runSpeed = 0;

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

    // PSX +264..275: undeclared fields
    s32 field264 = 0;
    s32 field268 = 0;
    s32 field272 = 0;

    // PSX +276 (s32): desired face angle (binary angle units)
    s32 faceAngle = 0;

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

    // PSX +352 (s32): input command bitfield (set by AI/controls, read by action handlers)
    // Bits: 1=guard, 2=kick, 3=punch, 4=taunt, 5=strafe, 6=backflip,
    //        7=combat, 8-20=combos, 21=dive roll, 30=env hit, 31=explosion
    s32 commandBits = 0;

    // PSX +356 (s32): current action state number
    s32 actionState = -1;
    // PSX +360 (s32): walk cycle flag
    s32 walkCycleFlag = 0;
    // PSX +364 (s32): reserved
    s32 field364 = 0;
    // PSX +368 (s32): cleared each Think
    s32 field368 = 0;
    // PSX +372 (s32): delta time multiplier (0x10000 = 1.0)
    s32 deltaTime = FIX16_ONE;

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

    // PSX: FaceAngleY__8Humanoidli (HUMANOID.CPP:2402)
    void FaceAngleY(s32 angle, s32 immediate);

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
