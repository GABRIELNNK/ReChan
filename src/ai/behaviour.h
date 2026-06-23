#pragma once
#include "core.h"
#include "p3d/p3dmath.h"

class Humanoid;
struct LinearPath;
struct BehaviourAttrib;

// Behaviour - AI decision-making for humanoid entities
// PSX: 280 bytes. Controls enemy actions, patrol, combat decisions.
struct Behaviour {
    using AIHandler = void (*)(Behaviour*);
    static constexpr u32 BF_INPUT_PROCESSING = 1u;

    // PSX +24: owning Humanoid pointer.
    Humanoid* owner = nullptr;

    // PSX +28: copied AI/thing type used by behaviour dispatch.
    u32 aiType = 0;

    // PSX +32/+36/+40: path-AI state.
    LinearPath* currentPath = nullptr;
    u32 field36 = 0;
    u32 currentPathNodeIndex = 0;

    // PSX +44/+48/+52: NDMS think timers/decision cache.
    s32 ndmsThinkCounter = 0;
    s32 ndmsThinkDelay = 0;
    s32 ndmsDecision = 0;

    // PSX +56: constructor AI parameter.
    s32 aiParam = 0;

    // PSX +60/+64/+68: NDMS state used by backoff/navigation helpers.
    s32 field60 = 0;
    s32 navDecisionCounter = 0;
    s32 navDecision = 0;

    // PSX +72/+76: ComplexAttack script cursor and selected script index.
    s32 comboScriptCursor = 0;
    s32 comboScriptIndex = -1;

    // PSX +80..+176: currently unmapped behaviour working state.
    s32 field80To176[25] = {};

    // PSX +180/+184/+188/+192: NavigateWorld cache/collision state.
    s32 worldNavCached = 0;
    s32 worldNavCachedTicks = 0;
    s32 worldNavBlocked = 0;
    s32 worldNavDirection = 0;

    // PSX +196 (s16): controller pad port index.
    s16 padPort = 1;
    s16 field198 = 0;

    // PSX +200..+203: action request state data (used by FindActionRequest)
    u32 actionRequestState[9] = {};

    // PSX +204/+208/+212: destination point for MoveToDestinationPoint
    LVector destPoint = {};

    // PSX +216 (ptr): behaviour attrib / animation config data pointer
    BehaviourAttrib* animConfigPtr = nullptr;

    // PSX +220/+222/+224: behaviour handler dispatch thunk.
    s16 handlerThisOffset = 0;
    s16 handlerDispatch = -1;
    AIHandler handler = nullptr;

    // PSX +228/+230/+232: deferred handler dispatch thunk used by jump/sub-state flows.
    s16 nextHandlerThisOffset = 0;
    s16 nextHandlerDispatch = -1;
    AIHandler nextHandler = nullptr;

    // PSX +236 (u32): flags (bit 0 = first frame init)
    u32 behaviourFlags = 0;

    // PSX +240: NDMS distance/range band.
    s32 ndmsRangeBand = 0;

    // PSX +244..+256: currently unmapped behaviour state.
    s32 field244 = 0;
    s32 field248 = 0;
    s32 field252 = 0;
    s32 field256 = 0;

    // PSX +260 (s32): button hold counter (incremented each frame same buttons held)
    s32 buttonHoldCounter = 0;

    // PSX +264/+268: currently unmapped behaviour state.
    s32 field264 = 0;
    s32 field268 = 0;

    // PSX +272 (u32): previous controller mask cache.
    u32 previousButtons = 0;

    // PSX +276 (u32): unmapped behaviour state used by AiFollowPath/Jumping.
    u32 field276 = 0;

    Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 aiParam);
    void SetAIHandler(u32 handlerType);
    bool InActiveZone() const;
    s32 MoveToDestinationPoint(u32 threshold);
    s32 BreakOffPathAndFight();
    void InitPathAIState(LinearPath* path);
    void ComplexAttack();
    s32 CounterAttack();
    s32 NavigateWorld(s32& moveDirection);
    s32 NavigateEnemies(s32 mode);
    s32 LookAheadFloorCheck(s16 angleOffset, s32 distance, s32 radius);
    s32 LookAheadWallCheck(s16 angleOffset, s32 distance, s32 radius);
    virtual void Process();
    virtual ~Behaviour() = default;
    void DisableInputProcessing() { behaviourFlags &= ~BF_INPUT_PROCESSING; }

    static void PlayerUserControl(Behaviour* behaviour);
    static void Idle(Behaviour* behaviour);
    static void AiFollowPath(Behaviour* behaviour);
    static void NisControl(Behaviour* behaviour);
    static void GetBackIntoActiveZone(Behaviour* behaviour);
    static void BackoffAndTaunt(Behaviour* behaviour);
    static void BackOutOfTheFight(Behaviour* behaviour);
    static void NDMS(Behaviour* behaviour);
    static void ButchDMS(Behaviour* behaviour);
    static void ButchDMS_Charge(Behaviour* behaviour);
    static void GrontarDMS(Behaviour* behaviour);
    static void PaulDMS(Behaviour* behaviour);
    static void OscarDMS(Behaviour* behaviour);
    static void OscarHenchmanDMS(Behaviour* behaviour);
    static void DanteDMS_Phase1(Behaviour* behaviour);
    static void DanteDMS_Phase2(Behaviour* behaviour);
    static void DanteDMS_Phase3(Behaviour* behaviour);
    static void Jumping(Behaviour* behaviour);
    static void DieWhenWeHitTheGround(Behaviour* behaviour);
    static void SubwayDodgeRight(Behaviour* behaviour);
    static void SubwayDodgeLeft(Behaviour* behaviour);
    static void SubwayDodgeJump(Behaviour* behaviour);

    // PSX (Dead__8Humanoid, 0x800691FC): clears the OscarsHenchman global
    // when the dying humanoid is the active Oscar henchman, so a solo Oscar
    // switches from the circling/backoff tree to his relentless chase.
    static void ClearOscarsHenchmanOnDeath(Humanoid* dying);
};
