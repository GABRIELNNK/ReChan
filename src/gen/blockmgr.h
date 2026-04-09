// blockmgr.h = BlockManager reversed from PSX BLKMGR.CPP
#pragma once

#include "core.h"
#include "gen/manager.h"
#include "gen/block.h"
#include <vector>

// BlockManager = manages level block loading, draw lists, demand loading
class BlockManager : public Manager {
public:
    BlockManager();
    ~BlockManager() override;

    // allocate block pool from WDB database
    void LoadBlocksFunc(const std::vector<DBVolume*>& volumes);

    // set up draw/load callback nodes
    void InternalOpen() override;

    void InternalClose() override;

    // load and parse block data from stream
    void LoadBlocks(u32 blockNum,
                    const u8* const* blkDataPtrs,
                    const u32* blkSizes,
                    u32 blkCount);

    // get block by index
    Block* GetBlock(u32 index);
    bool IsValidBlockNumber(u32 index) const;

    // find block containing position
    u16 GetBlockNumber(const LVector& pos) const;

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
