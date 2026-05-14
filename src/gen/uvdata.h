#pragma once
#include "core.h"

struct tPrimGeom;

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

    // PSX: Init__10UVPrimDataiiii (UVDATA.CPP:251)
    // Sets masks/steps and marks this UVPrimData entry valid.
    void Init(s32 newUMask, s32 newVMask, s32 newUStep, s32 newVStep);

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

bool UpdateUVPrimData(int blockNum, tPrimGeom* geom);

// PSX ColourInfo [28 bytes]
struct ColourInfo {
    u16 startR = 0;
    u16 startG = 0;
    u16 startB = 0;
    u16 endR = 0;
    u16 endG = 0;
    u16 endB = 0;
    u16 stepR = 0;
    u16 stepG = 0;
    u16 stepB = 0;
    u16 currentR = 0;
    u16 currentG = 0;
    u16 currentB = 0;
    u32 reverse = 0;

    int Init(u32 startColour, u32 endColour, int steps);
    int Reset(int useEndColour);
    int Update();
    u32 GetColour() const;
};

// PSX layout/order: CBVPrimData [72 bytes]
struct CBVPrimData {
    u32 valid;          // +0x00
    u32 hash;           // +0x04
    u32 blockNum;       // +0x08
    u32 numEntries;     // +0x0C
    const u8* entryRaw; // +0x10 (8 bytes per entry: u32 primListOffset + u32 colour)
    s32 mode;           // +0x14
    s32 randomFrames;   // +0x18
    s32 frameCount;     // +0x1C
    s32 frameCountMax;  // +0x20
    s32 frameCountMin;  // +0x24
    s32 currentIndex;   // +0x28
    u32 fixedColour;    // +0x30
    ColourInfo** colourInfo; // +0x2C on PSX
    s32 restartDelay;   // +0x34
    s32 holdDelay;      // +0x38
    s32 restartDelayCurrent; // +0x3C
    s32 holdDelayCurrent;    // +0x40
    s32 direction;      // +0x44

    CBVPrimData();
    int Init(int frameCount, int frameCountMin, u32 fixedColour, int restartDelay, int holdDelay, int mode);
    void Release();
    void FreeColourInfo();
    int Update();
};

bool UpdateCBVPrimData(int blockNum, tPrimGeom* geom);

static constexpr u32 UV_PRIM_MAX = 64;
static constexpr u32 CBV_PRIM_MAX = 64;

// PSX globals: gp+0xA64 = gUVPrimDataCount, gp+0xA68 = gCBVPrimDataCount
extern UVPrimData g_UVPrimDataArray[UV_PRIM_MAX];
extern u32 g_UVPrimDataCount;
extern CBVPrimData g_CBVPrimDataArray[CBV_PRIM_MAX];
extern u32 g_CBVPrimDataCount;

// PSX: FindUVPrimInfo__10UVPrimDataUl (UVDATA.CPP:129)
UVPrimData* FindUVPrimInfo(u32 hash);

// PSX: FindCBVPrimInfo__11CBVPrimDataUl (UVDATA.CPP:387)
CBVPrimData* FindCBVPrimInfo(u32 hash);

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
