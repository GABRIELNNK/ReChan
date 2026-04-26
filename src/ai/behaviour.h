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

    // PSX +56: constructor AI parameter.
    s32 aiParam = 0;

    // PSX +196 (s16): controller pad port index.
    s16 padPort = 1;

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

    // PSX +260 (s32): button hold counter (incremented each frame same buttons held)
    s32 buttonHoldCounter = 0;

    // PSX +272 (u32): previous controller mask cache.
    u32 previousButtons = 0;

    // PSX +276 (u32): unmapped behaviour state used by AiFollowPath/Jumping.
    u32 field276 = 0;

    Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 aiParam);
    void SetAIHandler(u32 handlerType);
    bool InActiveZone() const;
    s32 MoveToDestinationPoint(u32 threshold);
    virtual void Process();
    virtual ~Behaviour() = default;
    void DisableInputProcessing() { behaviourFlags &= ~BF_INPUT_PROCESSING; }

    static void PlayerUserControl(Behaviour* behaviour);
    static void NisControl(Behaviour* behaviour);
    static void NDMS(Behaviour* behaviour);
    static void SubwayDodgeRight(Behaviour* behaviour);
    static void SubwayDodgeLeft(Behaviour* behaviour);
    static void SubwayDodgeJump(Behaviour* behaviour);
};
