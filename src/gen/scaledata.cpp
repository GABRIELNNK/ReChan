#include "common.h"

#include "gen/scaledata.h"

#include "pc/log.h"

#include <cstring>

static constexpr u32 SCALE_DATA_MAX = 64;

struct ScaleData {
    const u8* rawHeader = nullptr;
    const u8* firstChannel = nullptr;
    void** runtimeBindings = nullptr;
    s16 frame = 0;

    u32 targetHash = 0;
    u8 channelCount = 0;
    u8 targetType = 0;

    const u8** channels = nullptr;
};

static ScaleData* g_scaleDataArray[SCALE_DATA_MAX] = {};
static ScaleData* g_commonScaleDataArray[SCALE_DATA_MAX] = {};
static u16 g_numScaleData = 0;
static u16 g_numCommonScaleData = 0;
static s32 g_commonScaleDataTest = 0;

static u16 ReadU16Scale(const u8* p) {
    return static_cast<u16>(p[0] | (p[1] << 8));
}

static u32 ReadU32Scale(const u8* p) {
    return static_cast<u32>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static bool GetScaleChannelSpanBytes(const u8* channelData, u32 remainingBytes, u32* outSpanBytes) {
    if (!channelData || !outSpanBytes || remainingBytes < 8) {
        return false;
    }

    const u32 keyCount = ReadU32Scale(channelData + 4);
    const u64 wordsToSkip = static_cast<u64>(keyCount) * 2u + 2u;
    const u64 bytesToSkip = wordsToSkip * 4u;
    if (bytesToSkip > 0xFFFFFFFFull || bytesToSkip > remainingBytes) {
        return false;
    }

    *outSpanBytes = static_cast<u32>(bytesToSkip);
    return true;
}

static void DestroyScaleData(ScaleData* scaleData) {
    MARKFUNCTION(0x8009CAEC);

    if (!scaleData) {
        return;
    }

    delete[] scaleData->runtimeBindings;
    scaleData->runtimeBindings = nullptr;

    delete[] scaleData->channels;
    scaleData->channels = nullptr;

    delete scaleData;
}

bool LoadScaleData(u16 chunkId, const u8* /*body*/, u32 /*bodySize*/,
                   const u8* permBase, u32& permCursor, u32 permSize)
{
    MARKFUNCTION(0x8009C6CC);

    if (chunkId != 0x8B01) {
        return false;
    }

    if (!permBase || permCursor + 8 > permSize) {
        LOG("[ScaleData] 0x8B01 short header: cursor=%u size=%u", permCursor, permSize);
        return false;
    }

    const u8* header = permBase + permCursor;
    const u8 channelCount = header[6];

    const u8** channelTable = nullptr;
    void** runtimeBindings = nullptr;
    if (channelCount > 0) {
        channelTable = new const u8*[channelCount];
        runtimeBindings = new void*[static_cast<u32>(channelCount) * 2u];
        if (!channelTable || !runtimeBindings) {
            delete[] channelTable;
            delete[] runtimeBindings;
            return false;
        }
        std::memset(runtimeBindings, 0, sizeof(void*) * static_cast<u32>(channelCount) * 2u);
    }

    u32 cursor = permCursor + 8;
    for (u32 channelIndex = 0; channelIndex < channelCount; channelIndex++) {
        if (cursor + 8 > permSize) {
            LOG("[ScaleData] 0x8B01 short channel header: channel=%u cursor=%u size=%u",
                channelIndex,
                cursor,
                permSize);
            delete[] channelTable;
            delete[] runtimeBindings;
            return false;
        }

        const u8* channelData = permBase + cursor;
        channelTable[channelIndex] = channelData;

        u32 spanBytes = 0;
        if (!GetScaleChannelSpanBytes(channelData, permSize - cursor, &spanBytes)) {
            LOG("[ScaleData] 0x8B01 channel overflow: channel=%u cursor=%u size=%u",
                channelIndex,
                cursor,
                permSize);
            delete[] channelTable;
            delete[] runtimeBindings;
            return false;
        }

        cursor += spanBytes;
    }

    ScaleData** outArray = g_scaleDataArray;
    u16* outCount = &g_numScaleData;
    if (g_commonScaleDataTest) {
        outArray = g_commonScaleDataArray;
        outCount = &g_numCommonScaleData;
    }

    if (*outCount >= SCALE_DATA_MAX) {
        LOG("[ScaleData] array full (common=%d, count=%u)", g_commonScaleDataTest, *outCount);
        delete[] channelTable;
        delete[] runtimeBindings;
        permCursor = cursor;
        return true;
    }

    ScaleData* scaleData = new ScaleData();
    if (!scaleData) {
        delete[] channelTable;
        delete[] runtimeBindings;
        return false;
    }

    scaleData->rawHeader = header;
    scaleData->firstChannel = header + 8;
    scaleData->runtimeBindings = runtimeBindings;
    scaleData->frame = 0;
    scaleData->targetHash = ReadU32Scale(header + 0);
    scaleData->channelCount = channelCount;
    scaleData->targetType = header[7];
    scaleData->channels = channelTable;

    outArray[*outCount] = scaleData;
    *outCount = static_cast<u16>(*outCount + 1);

    permCursor = cursor;
    return true;
}

ScaleData* FindScaleInfo(u32 hash) {
    MARKFUNCTION(0x8009C918);

    for (u16 i = 0; i < g_numScaleData; i++) {
        ScaleData* scaleData = g_scaleDataArray[i];
        if (scaleData && scaleData->targetHash == hash) {
            return scaleData;
        }
    }

    for (u16 i = 0; i < g_numCommonScaleData; i++) {
        ScaleData* scaleData = g_commonScaleDataArray[i];
        if (scaleData && scaleData->targetHash == hash) {
            return scaleData;
        }
    }

    return nullptr;
}

void ScaleData_SetCommonMode(s32 enable) {
    MARKFUNCTION(0x8009CC20);

    g_commonScaleDataTest = enable;
}

void ScaleData_SetFrame(ScaleData* scaleData, s16 frame) {
    MARKFUNCTION(0x8009CC2C);

    if (scaleData) {
        scaleData->frame = frame;
    }
}

s32 ScaleData_GetScale(const ScaleData* scaleData, LVector* outScale, const u8* channelData) {
    MARKFUNCTION(0x8009CC34);

    if (!outScale) {
        return 0;
    }

    outScale->x = 0x10000;
    outScale->y = 0x10000;
    outScale->z = 0x10000;

    if (!scaleData || !channelData) {
        return 0;
    }

    const u32 keyCount = ReadU32Scale(channelData + 4);
    if (keyCount == 0) {
        return 0;
    }

    const s16 frame = scaleData->frame;
    const u8* keyData = channelData + 8;

    for (u32 keyIndex = 0; keyIndex < keyCount; keyIndex++) {
        const s16 keyFrame = static_cast<s16>(ReadU16Scale(keyData + 0));
        const s32 keyX = static_cast<s32>(ReadU16Scale(keyData + 2));
        const s32 keyY = static_cast<s32>(ReadU16Scale(keyData + 4));
        const s32 keyZ = static_cast<s32>(ReadU16Scale(keyData + 6));

        if (frame == keyFrame) {
            outScale->x = keyX * 4;
            outScale->y = keyY * 4;
            outScale->z = keyZ * 4;
            return outScale->z;
        }

        if (frame < keyFrame) {
            if (keyIndex == 0) {
                outScale->x = keyX * 4;
                outScale->y = keyY * 4;
                outScale->z = keyZ * 4;
                return outScale->z;
            }

            const u8* prevKeyData = keyData - 8;
            const s16 prevFrame = static_cast<s16>(ReadU16Scale(prevKeyData + 0));
            const s32 prevX = static_cast<s32>(ReadU16Scale(prevKeyData + 2));
            const s32 prevY = static_cast<s32>(ReadU16Scale(prevKeyData + 4));
            const s32 prevZ = static_cast<s32>(ReadU16Scale(prevKeyData + 6));

            const s32 frameOffset = static_cast<s16>(frame - prevFrame);
            const s32 frameDelta = static_cast<s16>(keyFrame - prevFrame + 1);

            if (frameDelta == 0) {
                outScale->x = prevX * 4;
                outScale->y = prevY * 4;
                outScale->z = prevZ * 4;
                return outScale->z;
            }

            const s32 lerpX = prevX + ((keyX - prevX) * frameOffset) / frameDelta;
            const s32 lerpY = prevY + ((keyY - prevY) * frameOffset) / frameDelta;
            const s32 lerpZ = prevZ + ((keyZ - prevZ) * frameOffset) / frameDelta;

            outScale->x = lerpX * 4;
            outScale->y = lerpY * 4;
            outScale->z = lerpZ * 4;
            return outScale->z;
        }

        keyData += 8;
    }

    const u8* lastKeyData = channelData + 8 + static_cast<u32>(keyCount - 1u) * 8u;
    outScale->x = static_cast<s32>(ReadU16Scale(lastKeyData + 2)) * 4;
    outScale->y = static_cast<s32>(ReadU16Scale(lastKeyData + 4)) * 4;
    outScale->z = static_cast<s32>(ReadU16Scale(lastKeyData + 6)) * 4;
    return outScale->z;
}

u32 ScaleData_GetChannelCount(const ScaleData* scaleData) {
    return scaleData ? static_cast<u32>(scaleData->channelCount) : 0;
}

const u8* ScaleData_GetChannelByIndex(const ScaleData* scaleData, u32 index) {
    if (!scaleData || !scaleData->channels || index >= scaleData->channelCount) {
        return nullptr;
    }

    return scaleData->channels[index];
}

u32 ScaleData_GetChannelJointHash(const u8* channelData) {
    if (!channelData) {
        return 0;
    }

    return ReadU32Scale(channelData + 0);
}

void UnloadLevelScaleData() {
    MARKFUNCTION(0x8009CBB4);

    for (u16 i = 0; i < g_numScaleData; i++) {
        if (g_scaleDataArray[i]) {
            DestroyScaleData(g_scaleDataArray[i]);
            g_scaleDataArray[i] = nullptr;
        }
    }

    g_numScaleData = 0;
}

void UnloadScaleData() {
    MARKFUNCTION(0x8009CB40);

    for (u16 i = 0; i < g_numCommonScaleData; i++) {
        if (g_commonScaleDataArray[i]) {
            DestroyScaleData(g_commonScaleDataArray[i]);
            g_commonScaleDataArray[i] = nullptr;
        }
    }

    g_numCommonScaleData = 0;
    UnloadLevelScaleData();
}
