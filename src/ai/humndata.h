#pragma once

#include "core.h"

struct HashEnum {
    u32 hash;
    s32 value;
};

struct CharSubTypeData {
    s32 enumValue;
    u32 behaviourNameHash;
    s32 scale;
    s16 clut;
    s16 hitPoints;
};

// PSX HUMNDATA.CPP table entry (type -> data pointer + threshold/id)
struct HumanoidDataEntry {
    s32 thingType;
    void* data;
    u16 dataID;
};

extern const HumanoidDataEntry* humanoidDataTable;

s32 GetEnumFromHashTable(const HashEnum* table, u32 count, s32 hash);
s32 GetPreActiveIdle(s32 hash);
s32 GetTauntAnim(s32 hash);
s32 GetCharSubTypeEnumFromHashID(s32 hash);
const CharSubTypeData* CharSubTypeDataTableElement(s32 charSubType);
s32 GetCharSubTypeScale(s32 charSubType);
s16 GetCharSubTypeClut(s32 charSubType);
s16 GetCharSubTypeHitPoints(s32 charSubType);
u32 GetBehaviourNameHash(s32 charSubType);
const HumanoidDataEntry* GetHumanoidData(u16 thingType);