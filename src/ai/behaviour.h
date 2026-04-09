// behaviour.h - AI Behaviour base class
// Reversed from PSX C:\CHAN\GAME\SRC\AI\BEHAVIOU.CPP
#pragma once

#include "core.h"

class Humanoid;

// Behaviour - AI decision-making for humanoid entities
// PSX: 280 bytes. Controls enemy actions, patrol, combat decisions.
struct Behaviour {
    using AIHandler = void (*)(Behaviour*);

    // PSX +24: owning Humanoid pointer.
    Humanoid* owner = nullptr;

    // PSX +196 (s16): controller pad port index (0 or 1)
    s16 padPort = 0;

    // PSX +200..+235: action request state data (used by FindActionRequest)
    u32 actionRequestState[9] = {};

    // PSX +236 (u32): flags (bit 0 = first frame init)
    u32 behaviourFlags = 0;

    // PSX +220/+222/+224: behaviour handler dispatch thunk.
    s16 handlerThisOffset = 0;
    s16 handlerDispatch = -1;
    AIHandler handler = nullptr;

    // PSX +260 (s32): button hold counter (incremented each frame same buttons held)
    s32 buttonHoldCounter = 0;

    // PSX +272 (u32): previous controller mask cache.
    u32 previousButtons = 0;

    Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 aiParam);
    void SetAIHandler(u32 handlerType);
    virtual void Process();
    virtual ~Behaviour() = default;

private:
    static void PlayerUserControl(Behaviour* behaviour);
};
