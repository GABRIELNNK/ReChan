#pragma once

#include "core.h"

struct ScaleData;

// PSX: Load__9ScaleDataR10tReadChunkPPv (SCALEDAT.CPP:56, 0x8009C6CC)
bool LoadScaleData(u16 chunkId, const u8* body, u32 bodySize,
                   const u8* permBase, u32& permCursor, u32 permSize);

// PSX: FindScaleInfo__9ScaleDatai (SCALEDAT.CPP:179, 0x8009C918)
ScaleData* FindScaleInfo(u32 hash);

// PSX: CommonScaleData__9ScaleDatai (SCALEDAT.CPP:365, 0x8009CC20)
void ScaleData_SetCommonMode(s32 enable);

// PSX: SetFrame__9ScaleDatai (SCALEDAT.CPP:379, 0x8009CC2C)
void ScaleData_SetFrame(ScaleData* scaleData, s16 frame);

// PSX: GetScale__9ScaleDataP9_RMVECT16P14ScaleKeyFrames (SCALEDAT.CPP:398, 0x8009CC34)
s32 ScaleData_GetScale(const ScaleData* scaleData, LVector* outScale, const u8* channelData);

u32 ScaleData_GetChannelCount(const ScaleData* scaleData);
const u8* ScaleData_GetChannelByIndex(const ScaleData* scaleData, u32 index);
u32 ScaleData_GetChannelJointHash(const u8* channelData);

// PSX: Unload__9ScaleData (SCALEDAT.CPP:312, 0x8009CB40)
void UnloadScaleData();

// PSX: UnloadLevel__9ScaleData (SCALEDAT.CPP:340, 0x8009CBB4)
void UnloadLevelScaleData();
