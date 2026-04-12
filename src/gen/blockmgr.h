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

    // PSX: IsValidBlockNumber (BLKMGR.CPP:769, 0x80050C70)
    // Iterates loaded block list, returns true if blockNum is loaded.
    bool IsValidBlockNumber(u32 blockNum) const;

    // PSX: GetBlockNumber (BLKMGR.CPP:749, 0x80050C04)
    // Iterates loaded blocks, returns block->blockNum if pos is inside.
    u16 GetBlockNumber(const LVector& pos) const;

    // PSX: InActiveList (BLKMGR.CPP:825, 0x80050D44)
    // Checks alreadyLoadedList for blockNum.
    bool InActiveList(u32 blockNum) const;

    // PSX: InDrawList (BLKMGR.CPP:807, 0x80050CF4)
    // Checks drawList for blockNum.
    bool InDrawList(u32 blockNum) const;

    // PSX: InLoadList (BLKMGR.CPP:785, 0x80050CB4)
    // Checks toBeLoadedList for blockNum.
    bool InLoadList(u32 blockNum) const;

    // PSX: CrossedBoundary (BLKMGR.CPP:486, 0x800506BC)
    bool CrossedBoundary() const;

    // PSX: UpdateAlreadyLoadedList (updates alreadyLoadedList from loaded blocks)
    void UpdateAlreadyLoadedList();

    u32 GetNumBlocks() const { return totalBlocks; }

    // Block array access (for rendering iteration)
    Block* GetBlocks() { return blocks.data(); }
    const Block* GetBlocks() const { return blocks.data(); }

private:
    // +32: numBlocks (max blocks that can be loaded at once)
    u32 numBlocks;
    // +40: Block array (pointer on PSX, vector on PC)
    std::vector<Block> blocks;
    // +48: totalBlocks (total blocks in level)
    u32 totalBlocks;
    // +52: currentBlockNum
    u32 currentBlockNum;

    // +64: toBeLoadedList (u16[8]) - block numbers queued for loading
    u16 toBeLoadedList[8];
    u32 toBeLoadedCount;

    // +92: drawListCount + drawList (u16[8]) - blocks in draw range
    u32 drawListCount;
    u16 drawList[8];

    // +112: alreadyLoadedCount + alreadyLoadedList (u16[8]) - currently loaded block nums
    u32 alreadyLoadedCount;
    u16 alreadyLoadedList[8];

    // +132-140: flags (all initialized to 1)
    u32 flag1;
    u32 flag2;
    u32 flag3;

    // +160: loadingState (0x1000 = complete)
    u32 loadingState;

    // +164: lastAddedBlockNum (set when a block is added to loaded list)
    u32 lastAddedBlockNum;
};

// PSX: gp scope, defined in world.cpp
extern BlockManager* g_blockManager;
