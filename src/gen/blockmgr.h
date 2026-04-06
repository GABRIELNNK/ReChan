// blockmgr.h = BlockManager reversed from PSX BLKMGR.CPP
#pragma once

#include "core.h"
#include "gen/manager.h"
#include "gen/block.h"
#include <vector>

// BlockManager = manages level block loading, draw lists, demand loading
// Reversed from PSX struct layout (BLKMGR.CPP)
class BlockManager : public Manager {
public:
    BlockManager();
    ~BlockManager() override;

    // _LoadBlocksFunc (BLKMGR.CPP:207) = allocate block pool from WDB database
    void LoadBlocksFunc(const std::vector<DBVolume*>& volumes);   // 0x8005010C

    // InternalOpen (BLKMGR.CPP:258) = set up draw/load callback nodes
    void InternalOpen() override;                                          // 0x800502BC

    // InternalClose (BLKMGR.CPP:280)
    void InternalClose() override;                                         // 0x80050384

    // LoadBlocks (BLKMGR.CPP:695) = load and parse block data from stream
    void LoadBlocks(u32 blockNum,
                    const u8* const* blkDataPtrs,
                    const u32* blkSizes,
                    u32 blkCount);                                // 0x80050A98

    // GetBlock (BLKMGR.CPP:1374) = get block by index
    Block* GetBlock(u32 index);                                   // 0x800518C4

    // IsValidBlockNumber (BLKMGR.CPP:769)
    bool IsValidBlockNumber(u32 index) const;                     // 0x80050C70

    // GetBlockNumber (BLKMGR.CPP:749) = find block containing position
    u16 GetBlockNumber(const LVector& pos) const;                 // 0x80050C04

    u32 GetNumBlocks() const { return totalBlocks; }

    // Block array access (for rendering iteration)
    Block* GetBlocks() { return blocks.data(); }
    const Block* GetBlocks() const { return blocks.data(); }

private:
    // +32: numBlocks (max blocks that can be loaded at once)
    u32 numBlocks;
    // +40: Block array (pointer, but we use vector on PC)
    std::vector<Block> blocks;
    // +48: totalBlocks (total blocks in level)
    u32 totalBlocks;
    // +52: currentBlockNum
    u32 currentBlockNum;
    // +132-140: flags (all initialized to 1)
    u32 flag1;
    u32 flag2;
    u32 flag3;
    // +156: drawListCount
    u32 drawListCount;
    // +160: loadingState (0x1000 = complete)
    u32 loadingState;
};

// PSX: gp scope, defined in world.cpp
extern BlockManager* g_blockManager;
