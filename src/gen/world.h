// world.h - Level world: loads BLK blocks from an LCF stream
#pragma once

#include "core.h"
#include "gen/manager.h"
#include "gen/block.h"
#include "gen/blockmgr.h"
#include <vector>
#include <string>

// PSX VRAM simulation (1024x512 16-bit words), heap-allocated
struct PsxVRAM {
    u16* data; // [y * 1024 + x], 1024x512

    PsxVRAM() : data(new u16[1024 * 512]()) {}
    ~PsxVRAM() { delete[] data; }
    PsxVRAM(const PsxVRAM&) = delete;
    PsxVRAM& operator=(const PsxVRAM&) = delete;

    u16  Get(int x, int y) const { return data[y * 1024 + x]; }
    void Set(int x, int y, u16 v) { data[y * 1024 + x] = v; }

    void Clear() { memset(data, 0, 1024 * 512 * sizeof(u16)); }

    void Upload(s16 x, s16 y, s16 w, s16 h, const u8* raw) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int vx = x + col, vy = y + row;
                if (vx >= 0 && vx < 1024 && vy >= 0 && vy < 512) {
                    int idx = (row * w + col) * 2;
                    Set(vx, vy, static_cast<u16>(raw[idx] | (raw[idx + 1] << 8)));
                }
            }
        }
    }

    // Decode a 256x256 texture page to RGBA8 (out must be 256*256*4 bytes)
    void DecodePage(u16 tpage, u16 cba, u8* rgbaOut) const;
};

class World : public Manager {
public:
    World();
    ~World() override;

    bool Load(const std::string& lcfPath);
    bool LoadLevelIndex(u32 levelIndex);
    void LoadPetal(u32 petalIndex);
    void LoadLevelNames();
    void LoadPermanent();
    void Render(const LVector* camPos);
    void Unload();
    void UnloadPetal();
    void ResetLevel();

    u32 GetCurrentLevelIndex() const { return currentLevelIndex; }
    u32 GetTargetLevelIndex() const { return targetLevelIndex; }
    u32 GetCurrentPetalIndex() const { return currentPetalIndex; }
    u32 GetTargetPetalIndex() const { return targetPetalIndex; }

    void SetTargetLevelIndex(u32 levelIndex) { targetLevelIndex = levelIndex; }
    void SetTargetPetalIndex(u32 petalIndex) { targetPetalIndex = petalIndex; }
    void SetTargetLevelPetal(u32 levelIndex, u32 petalIndex) {
        targetLevelIndex = levelIndex;
        targetPetalIndex = petalIndex;
    }

    u32 GetBlockCount() const { return blockMgr.GetNumBlocks(); }
    BlockManager* GetBlockManager() { return &blockMgr; }
    const LVector& GetLevelMin() const { return levelMin; }
    const LVector& GetLevelMax() const { return levelMax; }

    // Upload raw PSX texture data to VRAM and refresh the GL texture
    void UploadToVRAM(s16 x, s16 y, s16 w, s16 h, const u8* raw);
    void RefreshVRAMTexture();
    u32 GetVRAMHandle() const { return vramHandle; }

    // PSX: LevelIDToIndex__5Worldi (WORLD.CPP:1889, 0x80046D88)
    // Converts a level ID (e.g. 7=hub) to its index in the level list.
    s32 LevelIDToIndex(s32 levelID) const {
        for (s32 i = 0; i < levelCount; i++) {
            if (levelList && levelList[i * 2] == levelID)
                return i;
        }
        return 0;
    }

    // PSX: GetCurLevelPetals__5World (WORLD.CPP:1871, 0x80046D3C)
    // Returns the number of petals for the current level.
    s32 GetCurLevelPetals() const {
        if (levelList && currentLevelIndex < (u32)levelCount)
            return levelList[currentLevelIndex * 2 + 1];
        return 1;
    }

    // PSX: GetCurLevelID__5World (WORLD.CPP:1863, 0x80046D14)
    // Returns the level ID for the current level index.
    s32 GetCurLevelID() const {
        if (levelList && currentLevelIndex < (u32)levelCount)
            return levelList[currentLevelIndex * 2];
        return 0;
    }

private:
    BlockManager blockMgr;
    PsxVRAM vram;
    u32 vramHandle = 0;
    std::vector<u8> streamData; // LCF file data (kept alive for block pointers)
    LVector levelMin = {}, levelMax = {};

    // Level table data (from RTARGET/GAME_LN.TXT via LoadLevelNames)
    // PSX offsets: +0x24 through +0x38
    s32* levelList = nullptr;        // [levelCount * 2]: pairs of {levelID, petalCount}
    char** levelNames = nullptr;     // [levelCount + 1]: level name strings
    char*** petalNames = nullptr;    // [levelCount + 1]: per-level petal name arrays
    u8** petalSoundIDs = nullptr;    // [levelCount]: per-level sound byte arrays
    s32* highestPetal = nullptr;     // [levelCount]: highest petal index per level
    s32 levelCount = 0;

    // PSX world progression fields (offsets +0x3C..+0x4C in original layout)
    u32 currentLevelIndex = 6;
    u32 targetLevelIndex = 6;
    u32 currentPetalIndex = 0;
    u32 targetPetalIndex = 0;
    u32 previousLevelIndex = 0;

    void LoadTPGTextures(const u8* lcfData, u32 lcfSize);

    // DrawEverythingHandler (GAME.CPP:2211) - sorting + rendering pipeline
    void DrawEverythingHandler(const LVector* camPos);  // 0x8002A98C

    // computeBlockToPointDistances (GAME.CPP:1976) - 8-corner bbox distance + frustum cull
    void computeBlockToPointDistances(const Block* block, const LVector* point,
                                      s32* outDistSq, s32* outZDepth);  // 0x8002A238

    // OffsetToPreventSeams (GAME.CPP:2482) - shifts block pos toward camera
    void OffsetToPreventSeams(LVector& pos, const LVector& camPos);  // 0x8002AF88
};
