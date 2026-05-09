#pragma once
#include "core.h"

struct MiscAnimNode;
class ComEffect;
class Effects;

// PSX: Load__7GEffectR10tReadChunkPPv (GEFFECT.CPP:85, 0x8008DB2C)
// Parses chunk 0x8A10 body and builds a hash -> ComEffect(anim) table.
void GEffect_LoadChunk(const u8* body, u32 bodySize);

// PSX: Unload__7GEffect (GEFFECT.CPP:163, 0x8008DCE8)
void GEffect_Unload();

// PSX: FindEffect__7GEffectUl (GEFFECT.CPP:193, 0x8008DDD0)
ComEffect* GEffect_FindEffect(u32 effectHash);

// Host bridge for obstacle animation lookup via preloaded GEffect ComEffects.
// Returns true when an effect hash exists; outAnim may be nullptr when the
// source ComEffect has no misc animation bound.
bool GEffect_FindEffectAnim(u32 effectHash, MiscAnimNode** outAnim);

// PSX: Create__7GEffectUlP10tagLVectorP9_RMVECT16T3iiUl (GEFFECT.CPP:226, 0x8008DE18)
Effects* GEffect_Create(u32 effectHash,
                        const LVector* pos,
                        const LVector* scale,
                        const LVector* rotation,
                        s32 frameDelay,
                        s32 lifeFrames,
                        u32 createFlags);
