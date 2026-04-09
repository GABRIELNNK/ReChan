#include "ai/colfight.h"
#include "ai/humanoid.h"

// PSX: global array of 12 Humanoid pointers (gArray in decompile)
static Humanoid* g_fightArray[FIGHTING_COLLISION_MAX] = {};

namespace FightingCollision {

// PSX: Init (COLFIGHT.CPP:159, 0x800725FC)
void Init() {
    MARKFUNCTION(0x800725FC);
    for (s32 i = 0; i < FIGHTING_COLLISION_MAX; ++i) {
        g_fightArray[i] = nullptr;
    }
}

// PSX: GetHumanoidArray (COLFIGHT.CPP:258, 0x80072768)
Humanoid** GetHumanoidArray() {
    MARKFUNCTION(0x80072768);
    return g_fightArray;
}

// PSX: InsertHumanoid (COLFIGHT.CPP:181, 0x80072668)
void InsertHumanoid(Humanoid* h) {
    MARKFUNCTION(0x80072668);
    if (!h) {
        return;
    }
    for (s32 i = 0; i < FIGHTING_COLLISION_MAX; ++i) {
        if (g_fightArray[i] == nullptr) {
            g_fightArray[i] = h;
            return;
        }
    }
}

// PSX: RemoveHumanoid (COLFIGHT.CPP:224, 0x800726E0)
void RemoveHumanoid(const Humanoid* h) {
    MARKFUNCTION(0x800726E0);
    if (!h) {
        return;
    }
    for (s32 i = 0; i < FIGHTING_COLLISION_MAX; ++i) {
        if (g_fightArray[i] == h) {
            g_fightArray[i] = nullptr;
            return;
        }
    }
}

} // namespace FightingCollision
