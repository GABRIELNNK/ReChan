#include "common.h"
#include "gen/blockmgr.h"
#include "gen/database.h"

// __12BlockManager (BLKMGR.CPP:148)
BlockManager::BlockManager() {
    MARKFUNCTION(0x8004FE98);
    numBlocks = 0;
    totalBlocks = 0;
    currentBlockNum = 0;
    toBeLoadedCount = 0;
    memset(toBeLoadedList, 0, sizeof(toBeLoadedList));
    drawListCount = 0;
    memset(drawList, 0, sizeof(drawList));
    alreadyLoadedCount = 0;
    memset(alreadyLoadedList, 0, sizeof(alreadyLoadedList));
    flag1 = 1;
    flag2 = 1;
    flag3 = 1;
    loadingState = 0;
    lastAddedBlockNum = 0x1000;
}

// ~BlockManager (BLKMGR.CPP:169)
BlockManager::~BlockManager() {
    MARKFUNCTION(0x8004FF1C);
    InternalClose();
}

// _LoadBlocksFunc__12BlockManagerP8Callback (BLKMGR.CPP:207)
// PSX: reads block count from database, allocates Block array, calls Init on each
void BlockManager::LoadBlocksFunc(const std::vector<DBVolume*>& volumes) {
    MARKFUNCTION(0x8005010C);

    totalBlocks = static_cast<u32>(volumes.size());
    blocks.resize(totalBlocks);

    for (u32 i = 0; i < totalBlocks; i++) {
        blocks[i].Init(volumes[i]);
    }

    LOG("[BlockManager] Initialized %u blocks from WDB volumes", totalBlocks);
}

// InternalOpen__12BlockManager (BLKMGR.CPP:258)
// PSX: creates ccNode callback pairs for load/unload, adds to LevelMgr lists
// PC: just clear state = async loading not needed
void BlockManager::InternalOpen() {
    MARKFUNCTION(0x800502BC);
    drawListCount = 0;
    memset(drawList, 0, sizeof(drawList));
    alreadyLoadedCount = 0;
    memset(alreadyLoadedList, 0, sizeof(alreadyLoadedList));
    toBeLoadedCount = 0;
    memset(toBeLoadedList, 0, sizeof(toBeLoadedList));
    loadingState = 0;
    lastAddedBlockNum = 0x1000;
}

// InternalClose__12BlockManager (BLKMGR.CPP:280)
void BlockManager::InternalClose() {
    MARKFUNCTION(0x80050384);
    for (auto& b : blocks) b.Destroy();
    blocks.clear();
    totalBlocks = 0;
    drawListCount = 0;
    alreadyLoadedCount = 0;
    toBeLoadedCount = 0;
}

// LoadBlocks__12BlockManagerUl (BLKMGR.CPP:695)
// PSX: streams BLK data from disc via async load, then calls Parse on each block.
// PC: all blocks are in memory, parse directly. All blocks are "loaded".
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

    // PC: all blocks are loaded. Build the alreadyLoadedList with all block numbers.
    UpdateAlreadyLoadedList();

    // PC: on PSX, PopulateBlock is called here inside LoadBlocks.
    // On PC, the caller (World::LoadLevelIndex) calls g_ai->PopulateBlock() separately.

    loadingState = 0x1000; // mark loading complete

    LOG("[BlockManager] Parsed %u BLK entries, %u in active list", blkCount, alreadyLoadedCount);
}

// UpdateAlreadyLoadedList__12BlockManager (BLKMGR.CPP)
// PSX: copies block numbers from loaded block linked list into alreadyLoadedList array.
// PC: all blocks are loaded, so copy all block numbers.
void BlockManager::UpdateAlreadyLoadedList() {
    alreadyLoadedCount = 0;
    for (u32 i = 0; i < totalBlocks && alreadyLoadedCount < 8; i++) {
        alreadyLoadedList[alreadyLoadedCount] = blocks[i].blockNum;
        alreadyLoadedCount++;
    }
    // Also populate drawList = same as alreadyLoadedList on PC (all loaded = all drawable)
    drawListCount = alreadyLoadedCount;
    for (u32 i = 0; i < alreadyLoadedCount; i++) {
        drawList[i] = alreadyLoadedList[i];
    }
}

// GetBlock__12BlockManagerUl (BLKMGR.CPP:1374)
Block* BlockManager::GetBlock(u32 index) {
    MARKFUNCTION(0x800518C4);

    if (index >= totalBlocks) return nullptr;
    return &blocks[index];
}

// IsValidBlockNumber__12BlockManagerUl (BLKMGR.CPP:769)
// PSX: iterates loaded block linked list, returns true if any block has that blockNum.
// PC: all blocks are loaded, iterate all blocks.
bool BlockManager::IsValidBlockNumber(u32 blockNum) const {
    MARKFUNCTION(0x80050C70);
    for (u32 i = 0; i < totalBlocks; i++) {
        if (blocks[i].blockNum == (u16)blockNum) {
            return true;
        }
    }
    return false;
}

// GetBlockNumber__12BlockManagerRC10tagLVector (BLKMGR.CPP:749)
// PSX: iterates loaded block linked list, returns block->blockNum (attrib 15) if pos is inside.
// Returns BLOCK_UNASSIGNED (0x1000) if no block contains the position.
// PC: all blocks are loaded, iterate all blocks.
u16 BlockManager::GetBlockNumber(const LVector& pos) const {
    MARKFUNCTION(0x80050C04);
    for (u32 i = 0; i < totalBlocks; i++) {
        const Block& blk = blocks[i];
        if (blk.PointInBlock(&pos)) {
            return blk.blockNum;
        }
    }
    return 0x1000;
}

// InActiveList__C12BlockManagerUl (BLKMGR.CPP:825, 0x80050D44)
// PSX: checks alreadyLoadedList u16 array for blockNum.
bool BlockManager::InActiveList(u32 blockNum) const {
    MARKFUNCTION(0x80050D44);
    for (u32 i = 0; i < alreadyLoadedCount; i++) {
        if (alreadyLoadedList[i] == (u16)blockNum) {
            return true;
        }
    }

    // PC fallback: all parsed blocks are resident (no PSX demand-loading pool yet),
    // so consider any valid block number as active.
    return IsValidBlockNumber(blockNum);
}

// InDrawList__C12BlockManagerUl (BLKMGR.CPP:807, 0x80050CF4)
// PSX: checks drawList u16 array for blockNum.
bool BlockManager::InDrawList(u32 blockNum) const {
    MARKFUNCTION(0x80050CF4);
    if (!drawListCount) {
        return IsValidBlockNumber(blockNum);
    }
    for (u32 i = 0; i < drawListCount; i++) {
        if (drawList[i] == (u16)blockNum) {
            return true;
        }
    }

    // PC fallback: all parsed blocks are drawable until demand-loading is reversed.
    return IsValidBlockNumber(blockNum);
}

// InLoadList__C12BlockManagerUl (BLKMGR.CPP:785, 0x80050CB4)
// PSX: checks toBeLoadedList u16 array for blockNum.
bool BlockManager::InLoadList(u32 blockNum) const {
    MARKFUNCTION(0x80050CB4);
    for (u32 i = 0; i < 6; i++) {
        if (toBeLoadedList[i] == (u16)blockNum) {
            return true;
        }
    }
    return false;
}

// CrossedBoundary__12BlockManager (BLKMGR.CPP:486, 0x800506BC)
// PSX: returns true if player blockNum != currentBlockNum
bool BlockManager::CrossedBoundary() const {
    MARKFUNCTION(0x800506BC);
    // PSX: MEMORY[0x54] = player->blockNum
    // PC: we'd need the player reference, but since all blocks are loaded,
    // demand loading isn't needed. Return false.
    return false;
}
