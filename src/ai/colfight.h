// colfight.h - FightingCollision system
// Reversed from PSX C:\CHAN\GAME\SRC\AI\COLFIGHT.CPP
#pragma once

#include "core.h"

class Humanoid;

static constexpr s32 FIGHTING_COLLISION_MAX = 12;

// PSX: FightingCollision - global array of active combatant Humanoids
// Source: C:\CHAN\GAME\SRC\AI\COLFIGHT.CPP
namespace FightingCollision {

// PSX: GetHumanoidArray (COLFIGHT.CPP:258, 0x80072768)
// Returns pointer to the global array of 12 Humanoid pointers.
Humanoid** GetHumanoidArray();

// PSX: InsertHumanoid (COLFIGHT.CPP:181, 0x80072668)
void InsertHumanoid(Humanoid* h);

// PSX: RemoveHumanoid (COLFIGHT.CPP:224, 0x800726E0)
void RemoveHumanoid(const Humanoid* h);

// PSX: Init (COLFIGHT.CPP:159, 0x800725FC)
void Init();

} // namespace FightingCollision
