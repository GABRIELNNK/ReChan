// world.h - Level world: loads BLK blocks from an LCF stream
#pragma once

#include "core.h"
#include "gen/manager.h"
#include "gen/block.h"
#include "gen/blockmgr.h"
#include <vector>
#include <string>
#include <cstring>

// PSX VRAM simulation (1024x512 16-bit words), heap-allocated
struct PsxVRAM {
    u16* data; // [y * 1024 + x], 1024x512

    PsxVRAM() : data(new u16[1024 * 512]()) {}
    ~PsxVRAM() { delete[] data; }
    PsxVRAM(const PsxVRAM&) = delete;
    PsxVRAM& operator=(const PsxVRAM&) = delete;

    u16  Get(int x, int y) const { return data[y * 1024 + x]; }
    void Set(int x, int y, u16 v) { data[y * 1024 + x] = v; }

    void Clear() { std::memset(data, 0, 1024 * 512 * sizeof(u16)); }

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
    void Render(const LVector* camPos);
    void Unload();

    u32 GetBlockCount() const { return blockMgr.GetNumBlocks(); }
    BlockManager* GetBlockManager() { return &blockMgr; }
    const LVector& GetLevelMin() const { return levelMin; }
    const LVector& GetLevelMax() const { return levelMax; }

private:
    BlockManager blockMgr;
    PsxVRAM vram;
    u32 vramHandle = 0;
    std::vector<u8> streamData; // LCF file data (kept alive for block pointers)
    LVector levelMin = {}, levelMax = {};

    void LoadTPGTextures(const u8* lcfData, u32 lcfSize);

    // DrawEverythingHandler (GAME.CPP:2211) - sorting + rendering pipeline
    void DrawEverythingHandler(const LVector* camPos);  // 0x8002A98C

    // computeBlockToPointDistances (GAME.CPP:1976) - 8-corner bbox distance + frustum cull
    void computeBlockToPointDistances(const Block* block, const LVector* point,
                                      s32* outDistSq, s32* outZDepth);  // 0x8002A238

    // OffsetToPreventSeams (GAME.CPP:2482) - shifts block pos toward camera
    void OffsetToPreventSeams(LVector& pos, const LVector& camPos);  // 0x8002AF88
};
