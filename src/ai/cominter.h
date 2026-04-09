#pragma once
#include "core.h"

static constexpr s32 g_maxAttackRange = 1500;

s32 FindActionRequest(u32* state, u32 buttons, s32 direction, u16 padIndex);
