// behaviour.h - AI Behaviour base class
// Reversed from PSX C:\CHAN\GAME\SRC\AI\BEHAVIOU.CPP
#pragma once

#include "core.h"

class Humanoid;

// Behaviour - AI decision-making for humanoid entities
// PSX: 280 bytes. Controls enemy actions, patrol, combat decisions.
struct Behaviour {
    // PSX: Process__9Behaviour (BEHAVIOU.CPP)
    virtual void Process();

    virtual ~Behaviour() = default;
};
