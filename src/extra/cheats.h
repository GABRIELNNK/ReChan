#pragma once
#include "gen/config.h"

#if NEW_CHEATS

enum class CheatOption {
    AllDragons,
    AllLevels,
    GodMode,
    OnePunchMan,
    HeavenBound,
};

bool IsCheatEnabled(CheatOption option);
void SetCheatEnabled(CheatOption option, bool enabled);
void ApplyProgressCheats();
void ResetCheats();

#endif
