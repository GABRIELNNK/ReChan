// cominter.h - Command interpreter
// Reversed from PSX C:\CHAN\GAME\SRC\AI\COMINTER.CPP
#pragma once

#include "core.h"

// PSX: g_maxAttackRange at gp+2052
static constexpr s32 g_maxAttackRange = 1500;

// PSX: FindActionRequest__FRUlUlUlPC7Control (COMINTER.CPP:84, 0x800B0E18)
s32 FindActionRequest(u32* state, u32 buttons, s32 direction, u16 padIndex);
