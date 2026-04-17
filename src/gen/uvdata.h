#pragma once
#include "core.h"

// PSX: UVPrimData (UVDATA.CPP) - per-block UV animation data
// Loaded from P3D chunks 0x8C20 (header) / 0x8C21 (entries).
// Each entry stores 4 primList byte offsets and 4 base UV values
// for a quad whose UVs are animated (scrolling textures, etc.).
//
// Per-frame update: uAccum += uStep; uAccum &= uMask (same for v).
// Then uvCombined = (vAccum & 0xFF) << 8 | (uAccum & 0xFF).
// Update per block: for each entry, newUV = baseUV + uvCombined,
// written to the primList at the stored byte offset.
//
// On PC we don't have primList GPU packets. Instead we store a mapping
// from primList byte offsets to vertex buffer indices, built during
// ParseBLKPrims. Block::Draw patches the vertex buffer UVs each frame.

struct UVPrimEntry {
    u32 primOffsets[4];
    u16 baseUV[4];
};

// PSX layout: 0x28 (40) bytes
struct UVPrimData {
    u32 valid;          // +0x00
    u32 hash;           // +0x04 (FindUVPrimInfo key)
    u32 blockNum;       // +0x08 (matched in Update per block)
    u32 uMask;          // +0x0C
    u32 vMask;          // +0x10
    u32 numEntries;     // +0x14
    const u8* entryRaw; // +0x18 (raw pointer into P3D chunk data, 32 bytes per entry)
    u16 uStep;          // +0x1C
    u16 vStep;          // +0x1E
    u16 uAccum;         // +0x20
    u16 vAccum;         // +0x22
    u16 uvCombined;     // +0x24

    // PSX: Update__10UVPrimData (UVDATA.CPP:288) - per-frame accumulator tick
    void Tick() {
        MARKFUNCTION(0x80098604);
        uAccum = static_cast<u16>((uAccum + uStep) & uMask);
        vAccum = static_cast<u16>((vAccum + vStep) & vMask);
        uvCombined = static_cast<u16>(((vAccum & 0xFF) << 8) | (uAccum & 0xFF));
    }

    UVPrimEntry GetEntry(u32 idx) const {
        UVPrimEntry e = {};
        if (!entryRaw || idx >= numEntries) return e;
        const u8* p = entryRaw + idx * 32;
        for (int i = 0; i < 4; i++)
            e.primOffsets[i] = p[i * 4] | (p[i * 4 + 1] << 8) | (p[i * 4 + 2] << 16) | (p[i * 4 + 3] << 24);
        for (int i = 0; i < 4; i++)
            e.baseUV[i] = static_cast<u16>(p[16 + i * 4] | (p[16 + i * 4 + 1] << 8));
        return e;
    }
};

// PSX layout: 0x48 (72) bytes
struct CBVPrimData {
    u32 valid;          // +0x00
    u32 hash;           // +0x04
    u32 blockNum;       // +0x08
    u32 numEntries;     // +0x0C
    const u8* entryRaw; // +0x10 (8 bytes per entry: u32 primListOffset + pad)
    const u8* colorRaw; // +0x2C (u32 per entry: color value)
};

static constexpr u32 UV_PRIM_MAX = 64;
static constexpr u32 CBV_PRIM_MAX = 64;

// PSX globals: gp+0xA64 = gUVPrimDataCount, gp+0xA68 = gCBVPrimDataCount
extern UVPrimData g_UVPrimDataArray[UV_PRIM_MAX];
extern u32 g_UVPrimDataCount;
extern CBVPrimData g_CBVPrimDataArray[CBV_PRIM_MAX];
extern u32 g_CBVPrimDataCount;

// PSX: Load__10UVPrimDataR10tReadChunkPPv (UVDATA.CPP:81)
// Called for P3D chunk IDs 0x8C20 / 0x8C21.
// chunkId = the P3D sub-chunk ID, body = chunk body data, bodySize = body length,
// permBase/permCursor = permanent data base + current offset (for 0x8C21 entry data).
void LoadUVPrimData(u16 chunkId, const u8* body, u32 bodySize,
                    const u8* permBase, u32& permCursor, u32 permSize);

// PSX: Load__11CBVPrimDataR10tReadChunkPPv (UVDATA.CPP:339)
void LoadCBVPrimData(u16 chunkId, const u8* body, u32 bodySize,
                     const u8* permBase, u32& permCursor, u32 permSize);

// PSX: Unload__10UVPrimData (UVDATA.CPP:154)
void UnloadUVPrimData();

// PSX: Unload__11CBVPrimData (UVDATA.CPP:412)
void UnloadCBVPrimData();

// PSX: Update__10UVPrimData (tick all entries, called each frame)
void TickAllUVPrimData();
