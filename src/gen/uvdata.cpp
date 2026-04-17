#include "gen/uvdata.h"
#include "pc/log.h"
#include <cstring>

UVPrimData g_UVPrimDataArray[UV_PRIM_MAX];
u32 g_UVPrimDataCount = 0;
CBVPrimData g_CBVPrimDataArray[CBV_PRIM_MAX];
u32 g_CBVPrimDataCount = 0;

static u32 ReadU32LE(const u8* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static u16 ReadU16LE(const u8* p) {
    return static_cast<u16>(p[0] | (p[1] << 8));
}

// PSX: Load__10UVPrimDataR10tReadChunkPPv (UVDATA.CPP:81, 0x80098334)
void LoadUVPrimData(u16 chunkId, const u8* body, u32 bodySize,
                    const u8* permBase, u32& permCursor, u32 permSize)
{
    MARKFUNCTION(0x80098334);

    if (chunkId == 0x8C20) {
        if (g_UVPrimDataCount >= UV_PRIM_MAX) return;
        if (bodySize < 12) return;

        UVPrimData& d = g_UVPrimDataArray[g_UVPrimDataCount];
        memset(&d, 0, sizeof(d));
        d.hash = ReadU32LE(body + 0);
        d.blockNum = ReadU32LE(body + 4);
        d.numEntries = ReadU32LE(body + 8);
        // PSX: UVPrimData::Init / FindUVPrimInfo supply masks + steps. Some builds
        // embed them after the first 12 bytes in the 0x8C20 chunk body.
        if (bodySize >= 24) {
            d.uMask = ReadU32LE(body + 12);
            d.vMask = ReadU32LE(body + 16);
        }
        if (bodySize >= 28) {
            d.uStep = static_cast<u16>(ReadU16LE(body + 20));
            d.vStep = static_cast<u16>(ReadU16LE(body + 22));
        }
        g_UVPrimDataCount++;

        LOG("[UVPrimData] Load 0x8C20: idx=%u hash=0x%08X blockNum=%u numEntries=%u "
            "uMask=0x%X vMask=0x%X uStep=%u vStep=%u",
            g_UVPrimDataCount - 1, d.hash, d.blockNum, d.numEntries,
            d.uMask, d.vMask, d.uStep, d.vStep);
    }
    else if (chunkId == 0x8C21) {
        if (g_UVPrimDataCount == 0) return;
        UVPrimData& d = g_UVPrimDataArray[g_UVPrimDataCount - 1];

        if (permCursor + d.numEntries * 32 > permSize) {
            LOG("[UVPrimData] 0x8C21 perm overflow: need %u, have %u",
                permCursor + d.numEntries * 32, permSize);
            return;
        }

        d.entryRaw = permBase + permCursor;
        permCursor += d.numEntries * 32;
        d.valid = 1;

        // PSX: UVPrimData::FindUVPrimInfo(hash) + Init(...) supply masks/steps when
        // not present in the 0x8C20 chunk body — table lives in the executable on PSX.
        if (d.uMask == 0 && d.vMask == 0 && d.uStep == 0 && d.vStep == 0) {
            LOG("[UVPrimData] WARN: hash=0x%08X has no masks/steps (need FindUVPrimInfo from ROM)",
                d.hash);
        }

        LOG("[UVPrimData] Load 0x8C21: entries=%u entryRaw=%p valid=1", d.numEntries, d.entryRaw);
    }
}

// PSX: Load__11CBVPrimDataR10tReadChunkPPv (UVDATA.CPP:339, 0x8009864C)
void LoadCBVPrimData(u16 chunkId, const u8* body, u32 bodySize,
                     const u8* permBase, u32& permCursor, u32 permSize)
{
    MARKFUNCTION(0x8009864C);

    if (chunkId == 0x8C30) {
        if (g_CBVPrimDataCount >= CBV_PRIM_MAX) return;
        if (bodySize < 12) return;

        CBVPrimData& d = g_CBVPrimDataArray[g_CBVPrimDataCount];
        memset(&d, 0, sizeof(d));
        d.hash = ReadU32LE(body + 0);
        d.blockNum = ReadU32LE(body + 4);
        d.numEntries = ReadU32LE(body + 8);
        g_CBVPrimDataCount++;

        LOG("[CBVPrimData] Load 0x8C30: idx=%u hash=0x%08X blockNum=%u numEntries=%u",
            g_CBVPrimDataCount - 1, d.hash, d.blockNum, d.numEntries);
    }
    else if (chunkId == 0x8C31) {
        if (g_CBVPrimDataCount == 0) return;
        CBVPrimData& d = g_CBVPrimDataArray[g_CBVPrimDataCount - 1];

        if (permCursor + d.numEntries * 8 > permSize) {
            LOG("[CBVPrimData] 0x8C31 perm overflow");
            return;
        }

        d.entryRaw = permBase + permCursor;
        permCursor += d.numEntries * 8;
        d.valid = 1;

        LOG("[CBVPrimData] Load 0x8C31: entries=%u valid=1", d.numEntries);
    }
}

// PSX: Unload__10UVPrimData (UVDATA.CPP:154, 0x80098478)
void UnloadUVPrimData() {
    MARKFUNCTION(0x80098478);
    for (u32 i = 0; i < g_UVPrimDataCount; i++) {
        memset(&g_UVPrimDataArray[i], 0, sizeof(UVPrimData));
    }
    g_UVPrimDataCount = 0;
}

// PSX: Unload__11CBVPrimData (UVDATA.CPP:412, 0x80098790)
void UnloadCBVPrimData() {
    MARKFUNCTION(0x80098790);
    for (u32 i = 0; i < g_CBVPrimDataCount; i++) {
        memset(&g_CBVPrimDataArray[i], 0, sizeof(CBVPrimData));
    }
    g_CBVPrimDataCount = 0;
}

// Tick all UV accumulators once per frame
void TickAllUVPrimData() {
    for (u32 i = 0; i < g_UVPrimDataCount; i++) {
        if (g_UVPrimDataArray[i].valid) {
            g_UVPrimDataArray[i].Tick();
        }
    }
}
