// blockmgr.cpp — BlockManager reversed from PSX BLKMGR.CPP
// Original: C:\CHAN\GAME\SRC\GEN\BLKMGR.CPP
#include "gen/blockmgr.h"

// __12BlockManager (BLKMGR.CPP:148)
BlockManager::BlockManager() {
    MARKFUNCTION(0x8004FE98);
    numBlocks = 0;
    totalBlocks = 0;
    currentBlockNum = 0;
    flag1 = 1;
    flag2 = 1;
    flag3 = 1;
    drawListCount = 0;
    loadingState = 0;
}

// ~BlockManager (BLKMGR.CPP:169)
BlockManager::~BlockManager() {
    MARKFUNCTION(0x8004FF1C);
    InternalClose();
}

// _LoadBlocksFunc__12BlockManagerP8Callback (BLKMGR.CPP:207)
// PSX: reads block count from database, allocates Block array, calls Init on each
void BlockManager::LoadBlocksFunc(const std::vector<DBVolume>& volumes) {
    MARKFUNCTION(0x8005010C);

    totalBlocks = static_cast<u32>(volumes.size());
    blocks.resize(totalBlocks);

    for (u32 i = 0; i < totalBlocks; i++) {
        blocks[i].Init(&volumes[i]);
    }

    RC_LOG("[BlockManager] Initialized %u blocks from WDB volumes", totalBlocks);
}

// InternalOpen__12BlockManager (BLKMGR.CPP:258)
// PSX: creates ccNode callback pairs for load/unload, adds to LevelMgr lists
// PC: just clear state — async loading not needed
void BlockManager::InternalOpen() {
    MARKFUNCTION(0x800502BC);
    drawListCount = 0;
    loadingState = 0;
}

// InternalClose__12BlockManager (BLKMGR.CPP:280)
void BlockManager::InternalClose() {
    MARKFUNCTION(0x80050384);
    for (auto& b : blocks) b.Destroy();
    blocks.clear();
    totalBlocks = 0;
    drawListCount = 0;
}

// LoadBlocks__12BlockManagerUl (BLKMGR.CPP:695)
// PSX: streams BLK data from disc via async load, then calls Parse on each block
// PC: we have all BLK data in memory, so parse directly
void BlockManager::LoadBlocks(u32 blockNum,
                              const u8* const* blkDataPtrs,
                              const u32* blkSizes,
                              u32 blkCount) {
    MARKFUNCTION(0x80050A98);

    currentBlockNum = blockNum;

    // Parse each BLK entry into its corresponding block
    for (u32 i = 0; i < blkCount && i < totalBlocks; i++) {
        if (blkDataPtrs[i] && blkSizes[i] > 0) {
            blocks[i].Parse(blkSizes[i], blkDataPtrs[i]);
        }
    }

    loadingState = 0x1000; // mark loading complete

    RC_LOG("[BlockManager] Parsed %u BLK entries", blkCount);
}

// GetBlock__12BlockManagerUl (BLKMGR.CPP:1374)
Block* BlockManager::GetBlock(u32 index) {
    MARKFUNCTION(0x800518C4);

    if (index >= totalBlocks) return nullptr;
    return &blocks[index];
}

// IsValidBlockNumber__12BlockManagerUl (BLKMGR.CPP:769)
bool BlockManager::IsValidBlockNumber(u32 index) const {
    MARKFUNCTION(0x80050C70);
    return index < totalBlocks;
}
