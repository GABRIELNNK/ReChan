#include "ai/humndata.h"

static const HashEnum kPreActiveIdleTable[] = {
    { 0x07A94FA1, 0x5A },
    { 0x07B40197, 0x5B },
    { 0x07A94FA2, 0x5C },
    { 0x0B0932E4, 0x5D },
    { 0x0B7A2772, 0x5E },
    { 0x08952727, 0x7C },
    { 0x0CDB0BE4, 0x7D },
    { 0x06B50D95, 0x7E },
    { 0x062F4B77, 0x7F },
    { 0x0076BF10, 0x80 },
    { 0x0255BB67, 0x82 },
    { 0x06EB1757, 0x81 },
    { 0x0B57FE67, 0x83 },
    { 0x0E093E97, 0x84 },
    { 0x06AA2523, 0x5F },
};

static const HashEnum kTauntAnimTable[] = {
    { 0x07A94FA1, 0x5A },
    { 0x07B40197, 0x5B },
    { 0x07A94FA2, 0x5C },
    { 0x0B0932E4, 0x5D },
    { 0x0B7A2772, 0x5E },
    { 0x08952727, 0x7C },
    { 0x0CDB0BE4, 0x7D },
    { 0x06B50D95, 0x7E },
};

static const HashEnum kCharSubTypeHashTable[] = {
    { 0x00A33F61, 1 },
    { 0x00A33F62, 2 },
    { 0x00A33F63, 3 },
    { 0x04784041, 4 },
    { 0x04784042, 5 },
    { 0x04784043, 6 },
    { 0x070E2A71, 7 },
    { 0x070E2A72, 8 },
    { 0x070E2A73, 9 },
    { 0x07738171, 10 },
    { 0x07738172, 11 },
    { 0x07738173, 12 },
    { 0x0A0E8B71, 13 },
    { 0x0A0E8B72, 14 },
    { 0x0A0E8B73, 15 },
    { 0x004C4831, 16 },
    { 0x004C4832, 17 },
    { 0x004C4833, 18 },
    { 0x06E66EF1, 19 },
    { 0x06E66EF2, 20 },
    { 0x06E66EF3, 21 },
    { 0x04D43A31, 22 },
    { 0x04D43A32, 23 },
    { 0x04D43A33, 24 },
    { 0x04DC4831, 25 },
    { 0x04DC4832, 26 },
    { 0x04DC4833, 27 },
    { 0x0869E761, 28 },
    { 0x0869E762, 29 },
    { 0x0869E763, 30 },
    { 0x02E90A51, 31 },
    { 0x02E90A52, 32 },
    { 0x02E90A53, 33 },
    { 0x064095F1, 34 },
    { 0x064095F2, 35 },
    { 0x064095F3, 36 },
    { 0x0889A261, 37 },
    { 0x0889A262, 38 },
    { 0x00586531, 39 },
    { 0x00586532, 40 },
    { 0x00586533, 41 },
    { 0x0C666461, 42 },
    { 0x0C666462, 43 },
    { 0x0C666463, 44 },
    { 0x005D6131, 45 },
    { 0x005D6132, 46 },
    { 0x005D6133, 47 },
    { 0x060AE4E1, 48 },
    { 0x060AE4E2, 49 },
    { 0x060AE4E3, 50 },
    { 0x08943561, 51 },
    { 0x08943562, 52 },
};

static const CharSubTypeData kCharSubTypeDataTable[] = {
    { 1, 0x0052C559, 0x10CCC, 0, 100 },
    { 2, 0x0052C559, 0x11999, 1, 125 },
    { 3, 0x0052C559, 0x11EB8, 2, 150 },
    { 4, 0x0A5C2E39, 0x10CCC, 0, 100 },
    { 5, 0x0A5C7E14, 0x11999, 1, 125 },
    { 6, 0x0A5C7E14, 0x11EB8, 2, 150 },
    { 7, 0x00493044, 0x1147A, 0, 100 },
    { 8, 0x00493044, 0x11999, 1, 125 },
    { 9, 0x00493044, 0x11EB8, 2, 150 },
    { 10, 0x0049959B, 0x1147A, 0, 100 },
    { 11, 0x0049959B, 0x11999, 1, 125 },
    { 12, 0x0049959B, 0x11EB8, 2, 150 },
    { 13, 0x004C30A5, 0x10CCC, 0, 100 },
    { 14, 0x004C30A5, 0x11999, 1, 125 },
    { 15, 0x004C30A5, 0x11EB8, 2, 150 },
    { 16, 0x00004E62, 0x10000, 0, 120 },
    { 17, 0x00004E62, 0x11999, 1, 130 },
    { 18, 0x00004E62, 0x11EB8, 2, 150 },
    { 19, 0x04E90884, 0x10000, 0, 100 },
    { 20, 0x04E90884, 0x11999, 1, 125 },
    { 21, 0x04E90884, 0x13333, 2, 150 },
    { 22, 0x0980B8E9, 0x10CCC, 0, 95 },
    { 23, 0x0980B8E9, 0x11999, 1, 120 },
    { 24, 0x0980E8C4, 0x11EB8, 2, 130 },
    { 25, 0x0004FE62, 0x10000, 0, 100 },
    { 26, 0x0004FE62, 0x11999, 1, 125 },
    { 27, 0x0004FE62, 0x11EB8, 2, 150 },
    { 28, 0x007A8B93, 0x10000, 0, 100 },
    { 29, 0x007A8B93, 0x11999, 1, 125 },
    { 30, 0x007A8B93, 0x11EB8, 2, 150 },
    { 31, 0x00850B32, 0x10000, 0, 100 },
    { 32, 0x00850B32, 0x11999, 1, 125 },
    { 33, 0x00850B32, 0x11EB8, 2, 150 },
    { 34, 0x0A5C7E14, 0x10000, 0, 125 },
    { 35, 0x0A5C7E14, 0x11999, 1, 135 },
    { 36, 0x0A5C7E14, 0x13333, 2, 150 },
    { 37, 0x005AABBC, 0x10000, 0, 100 },
    { 38, 0x005AABBC, 0x11999, 1, 150 },
    { 39, 0x00005A7F, 0x10000, 0, 100 },
    { 40, 0x00005A7F, 0x11999, 1, 125 },
    { 41, 0x00005A7F, 0x11EB8, 2, 150 },
    { 42, 0x0980B8E9, 0x10000, 0, 90 },
    { 43, 0x0980E8C4, 0x11999, 1, 110 },
    { 44, 0x0980E8C4, 0x11EB8, 2, 120 },
    { 45, 0x00005F7B, 0x10000, 0, 100 },
    { 46, 0x00005F7B, 0x11999, 1, 125 },
    { 47, 0x00005F7B, 0x11EB8, 2, 150 },
    { 48, 0x05F82D01, 0x10000, 0, 110 },
    { 49, 0x05F82D01, 0x11999, 1, 125 },
    { 50, 0x05F82D01, 0x11EB8, 2, 150 },
    { 51, 0x0C7AB6D9, 0x10000, 0, 100 },
    { 52, 0x0C7AB6D9, 0x11999, 1, 150 },
    { -1, 0, 0, -1, -1 },
};

// PSX: HUMNDATA.CPP inline table at 0x800D9090, scanned in 12-byte entries
// by GetHumanoidData__FQ22AI10ThingTypes until thingType == -1.
static const HumanoidDataEntry kHumanoidDataTable[] = {
    { 0, nullptr, 25 },
    { 1, nullptr, 25 },
    { 2, nullptr, 32 },
    { 3, nullptr, 32 },
    { 4, nullptr, 30 },
    { 5, nullptr, 32 },
    { 6, nullptr, 30 },
    { 7, nullptr, 32 },
    { 8, nullptr, 30 },
    { 9, nullptr, 28 },
    { 10, reinterpret_cast<void*>(1), 34 },
    { 11, nullptr, 32 },
    { 12, nullptr, 30 },
    { 13, reinterpret_cast<void*>(1), 34 },
    { 14, nullptr, 30 },
    { 15, reinterpret_cast<void*>(1), 34 },
    { 16, nullptr, 30 },
    { 17, reinterpret_cast<void*>(1), 30 },
    { 18, nullptr, 30 },
    { 19, nullptr, 28 },
    { 20, nullptr, 25 },
    { 21, nullptr, 30 },
    { 23, nullptr, 35 },
    { 24, nullptr, 30 },
    { 25, nullptr, 30 },
    { 26, nullptr, 30 },
    { 27, nullptr, 30 },
    { -1, nullptr, 0xFFFF },
};

const HumanoidDataEntry* humanoidDataTable = kHumanoidDataTable;

s32 GetEnumFromHashTable(const HashEnum* table, u32 count, s32 hash) {
    if (!hash) {
        return -1;
    }

    for (u32 index = 0; index < count; index++) {
        if ((s32)table[index].hash == hash) {
            return table[index].value;
        }
    }

    return -1;
}

s32 GetPreActiveIdle(s32 hash) {
    const s32 count = (s32)(sizeof(kPreActiveIdleTable) / sizeof(kPreActiveIdleTable[0]));
    const s32 result = GetEnumFromHashTable(kPreActiveIdleTable, count, hash);
    return (result != -1) ? result : 22;
}

s32 GetTauntAnim(s32 hash) {
    const s32 count = (s32)(sizeof(kTauntAnimTable) / sizeof(kTauntAnimTable[0]));
    const s32 result = GetEnumFromHashTable(kTauntAnimTable, count, hash);
    return (result != -1) ? result : 22;
}

s32 GetCharSubTypeEnumFromHashID(s32 hash) {
    const s32 count = (s32)(sizeof(kCharSubTypeHashTable) / sizeof(kCharSubTypeHashTable[0]));
    const s32 result = GetEnumFromHashTable(kCharSubTypeHashTable, count, hash);
    return (result != -1) ? result : 0;
}

const CharSubTypeData* CharSubTypeDataTableElement(s32 charSubType) {
    const CharSubTypeData* entry = kCharSubTypeDataTable;
    while (entry->enumValue != -1) {
        if (entry->enumValue == charSubType) {
            return entry;
        }
        entry++;
    }

    return nullptr;
}

s32 GetCharSubTypeScale(s32 charSubType) {
    const CharSubTypeData* entry = CharSubTypeDataTableElement(charSubType);
    return entry ? entry->scale : 0x10000;
}

s16 GetCharSubTypeClut(s32 charSubType) {
    const CharSubTypeData* entry = CharSubTypeDataTableElement(charSubType);
    return entry ? entry->clut : 0;
}

s16 GetCharSubTypeHitPoints(s32 charSubType) {
    const CharSubTypeData* entry = CharSubTypeDataTableElement(charSubType);
    return entry ? entry->hitPoints : 0;
}

u32 GetBehaviourNameHash(s32 charSubType) {
    const CharSubTypeData* entry = CharSubTypeDataTableElement(charSubType);
    return entry ? entry->behaviourNameHash : 0;
}

// PSX: GetHumanoidData__FQ22AI10ThingTypes (HUMNDATA.CPP:297)
const HumanoidDataEntry* GetHumanoidData(u16 thingType) {
    MARKFUNCTION(0x8007DB94);

    const HumanoidDataEntry* entry = humanoidDataTable;
    if (!entry) {
        return nullptr;
    }

    while (entry->thingType != -1) {
        if (static_cast<u16>(entry->thingType) == thingType) {
            return entry;
        }
        entry++;
    }

    return nullptr;
}