#pragma once
#include "core.h"
#include "p3d/lvector.h"

class Humanoid;

static constexpr s32 FIGHTING_COLLISION_MAX = 12;

struct FightingCollisionAttackType {
    s32 attackType = 0;
    LVector startA = {};
    LVector endA = {};
    s32 radiusA = 0;
    s32 hasSecondary = 0;
    LVector startB = {};
    LVector endB = {};
    s32 radiusB = 0;
};

namespace FightingCollision {

    s32 Init();
    Humanoid** GetHumanoidArray();
    s32 InsertHumanoid(Humanoid* h);
    s32 RemoveHumanoid(const Humanoid* h);
    s32 ClearAttack(const Humanoid* h);
    s32 CheckAttack(
        Humanoid** outHumanoids,
        s32 maxCount,
        const Humanoid* source,
        const FightingCollisionAttackType* attackType);
    s32 Set(const Humanoid* source, const Humanoid* target);

}
