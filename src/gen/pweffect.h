#pragma once

#include "core.h"

struct DBPoint;
struct LVector;

// PSX: InitPWorldEffects__8PWEffectP7DBPoint (PWEFFECT.CPP:80, 0x8009AC58)
void PWEffect_InitWorldEffects(DBPoint* firstPoint);

// PSX: Unload__8PWEffect (PWEFFECT.CPP:528, 0x8009B534)
void PWEffect_Unload();

// PSX: Create__8PWEffecti (PWEFFECT.CPP:542, 0x8009B554)
void PWEffect_CreateForBlock(s32 blockNum);

// PSX: Create__9FPWEffecti (PWEFFECT.CPP:822, 0x8009BACC)
void FPWEffect_CreateForBlock(s32 blockNum);

// PSX: Create2__9FPWEffectUlP10tagLVectorP9_RMVECT16ii (PWEFFECT.CPP:911, 0x8009BC38)
s32 FPWEffect_Create2(u32 effectHash,
                      const LVector* pos,
                      const LVector* direction,
                      s32 lifeFrames,
                      s32 followPos);
