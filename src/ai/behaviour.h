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

    // PSX +220/+222/+224: behaviour handler dispatch thunk.
    s16 handlerThisOffset = 0;
    s16 handlerDispatch = -1;
    AIHandler handler = nullptr;

    // PSX +272: previous controller mask cache.
    u32 previousButtons = 0;
    bool previousRunPressed = false;

    // PSX: _9BehaviourP8HumanoidUll (BEHAVIOU.CPP)
    Behaviour(Humanoid* ownerHumanoid, u32 handlerType, s32 aiParam);

    // PSX: SetAIHandler__9BehaviourUl (BEHAVIOU.CPP)
    void SetAIHandler(u32 handlerType);

    // PSX: Process__9Behaviour (BEHAVIOU.CPP)
    virtual void Process();

    virtual ~Behaviour() = default;

private:
    // PSX: PlayerUserControl__9Behaviour (BEHAVIOU.CPP)
    static void PlayerUserControl(Behaviour* behaviour);
};
