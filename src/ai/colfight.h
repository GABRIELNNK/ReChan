// colfight.h - FightingCollision system
// Reversed from PSX C:\CHAN\GAME\SRC\AI\COLFIGHT.CPP
#pragma once

#include "core.h"

class Humanoid;

static constexpr s32 FIGHTING_COLLISION_MAX = 12;

namespace FightingCollision {

    Humanoid** GetHumanoidArray();
    void InsertHumanoid(Humanoid* h);
    void RemoveHumanoid(const Humanoid* h);
    void Init();

}
