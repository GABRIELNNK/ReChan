#pragma once
#include "core.h"
#include "p3d/p3dmath.h"

class Humanoid;

// Behaviour - AI decision-making for humanoid entities
// PSX: 280 bytes. Controls enemy actions, patrol, combat decisions.
struct Behaviour {
    using AIHandler = void (*)(Behaviour*);

    // PSX +24: owning Humanoid pointer.
    Humanoid* owner = nullptr;

    // PSX +196 (s16): controller pad port index (0 or 1)
    s16 padPort = 0;

    // PSX +200..+203: action request state data (used by FindActionRequest)
    u32 actionRequestState[9] = {};

    // PSX +204/+208/+212: destination point for MoveToDestinationPoint
    LVector destPoint = {};

    // PSX +216 (ptr): animation/config data pointer (speed reference)
    void* animConfigPtr = nullptr;

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

    // PSX +260 (s32): button hold counter (incremented each frame same buttons held)
    s32 buttonHoldCounter = 0;

    // PSX +272 (u32): previous controller mask cache.
    u32 previousButtons = 0;

    Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 aiParam);
    void SetAIHandler(u32 handlerType);
    s32 MoveToDestinationPoint(u32 threshold);
    virtual void Process();
    virtual ~Behaviour() = default;

    static void PlayerUserControl(Behaviour* behaviour);
    static void NisControl(Behaviour* behaviour);
    static void NDMS(Behaviour* behaviour);
    static void SubwayDodgeRight(Behaviour* behaviour);
    static void SubwayDodgeLeft(Behaviour* behaviour);
    static void SubwayDodgeJump(Behaviour* behaviour);
};
