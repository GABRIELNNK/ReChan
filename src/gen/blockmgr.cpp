#include "common.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/blockmgr.h"
#include "gen/database.h"
#include "gen/game.h"
#include "gen/world.h"
#include "gen/weffect.h"

static u32 ClampLoadedBlockCount(u32 totalBlockCount) {
    const u32 maxLoaded = (g_game && g_game->GetWorld() && g_game->GetWorld()->GetCurLevelID() == 7) ? 7u : 6u;
    return (totalBlockCount < maxLoaded) ? totalBlockCount : maxLoaded;
}

static const Block* GetBlockByNumber(const std::vector<Block>& blocks, u32 blockNum) {
    for (u32 index = 0; index < blocks.size(); index++) {
        if (blocks[index].blockNum == (u16)blockNum) {
            return &blocks[index];
        }
    }

    return nullptr;
}

static Block* GetBlockByNumber(std::vector<Block>& blocks, u32 blockNum) {
    for (u32 index = 0; index < blocks.size(); index++) {
        if (blocks[index].blockNum == (u16)blockNum) {
            return &blocks[index];
        }
    }

    return nullptr;
}

static bool ContainsBlockNumber(const u16* list, u32 count, u16 blockNum) {
    for (u32 index = 0; index < count; index++) {
        if (list[index] == blockNum) {
            return true;
        }
    }

    return false;
}

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
    numBlocks = ClampLoadedBlockCount(totalBlocks);

    // Parse each BLK entry into its corresponding block
    for (u32 i = 0; i < blkCount && i < totalBlocks; i++) {
        if (blkDataPtrs[i] && blkSizes[i] > 0) {
            blocks[i].Parse(blkSizes[i], blkDataPtrs[i]);
        }
    }

    UpdateToBeLoadedList(blockNum);
    UpdateAlreadyLoadedList();

    // PC: on PSX, PopulateBlock is called here inside LoadBlocks.
    // On PC, the caller (World::LoadLevelIndex) calls g_ai->PopulateBlock() separately.

    loadingState = 0x1000; // mark loading complete

    LOG("[BlockManager] Parsed %u BLK entries, %u in active list", blkCount, alreadyLoadedCount);
}

// UpdateAlreadyLoadedList__12BlockManager (BLKMGR.CPP)
// PSX: copies block numbers from the loaded block list into alreadyLoadedList.
// PC parses BLKs synchronously, so the logical resident list mirrors toBeLoadedList.
void BlockManager::UpdateAlreadyLoadedList() {
    alreadyLoadedCount = toBeLoadedCount;
    for (u32 index = 0; index < 8; index++) {
        alreadyLoadedList[index] = (index < alreadyLoadedCount) ? toBeLoadedList[index] : 6969;
    }

    flag2 = 1;

    s32 startBlockNum = (s32)currentBlockNum - 1;
    if (startBlockNum < 0) {
        startBlockNum = 0;
    }
    else if ((u32)startBlockNum > totalBlocks) {
        startBlockNum = (s32)totalBlocks;
    }

    s32 endBlockNum = (s32)currentBlockNum + 1;
    if (endBlockNum < 0) {
        endBlockNum = 0;
    }
    else if ((u32)endBlockNum > totalBlocks) {
        endBlockNum = (s32)totalBlocks;
    }

    for (s32 blockNum = startBlockNum; blockNum <= endBlockNum; blockNum++) {
        if (InLoadList((u32)blockNum) && !IsValidBlockNumber((u32)blockNum)) {
            flag2 = 0;
        }
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
    for (u32 index = 0; index < alreadyLoadedCount; index++) {
        if (alreadyLoadedList[index] == (u16)blockNum && GetBlockByNumber(blocks, blockNum)) {
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
    for (u32 index = 0; index < alreadyLoadedCount; index++) {
        const Block* block = GetBlockByNumber(blocks, alreadyLoadedList[index]);
        if (block && block->PointInBlock(&pos)) {
            return block->blockNum;
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
    return false;
}

// InDrawList__C12BlockManagerUl (BLKMGR.CPP:807, 0x80050CF4)
// PSX: checks drawList u16 array for blockNum.
bool BlockManager::InDrawList(u32 blockNum) const {
    MARKFUNCTION(0x80050CF4);
    for (u32 i = 0; i < drawListCount; i++) {
        if (drawList[i] == (u16)blockNum) {
            return true;
        }
    }
    return false;
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
    if (!Player::s_player) {
        return false;
    }

    const u32 playerBlockNum = Player::s_player->blockNum;
    return playerBlockNum != BLOCK_UNASSIGNED && playerBlockNum != currentBlockNum;
}

s32 BlockManager::DemandLoading() {
    MARKFUNCTION(0x800506E8);

    if (!flag3 || !CrossedBoundary() || !Player::s_player) {
        return 0;
    }

    currentBlockNum = Player::s_player->blockNum;
    UpdateToBeLoadedList(currentBlockNum);
    UpdateAlreadyLoadedList();

    if (g_ai) {
        g_ai->UnpopulateBlock();
        g_ai->PopulateBlock();
    }

    WEffect_UnPopulateWEffects(-1);
    WEffect_PopulateWEffects();

    return 0;
}

bool BlockManager::UpdateToBeLoadedList(u32 blockNum) {
    MARKFUNCTION(0x80050D94);

    for (u32 index = 0; index < 8; index++) {
        toBeLoadedList[index] = 6969;
        alreadyLoadedList[index] = 6969;
        drawList[index] = 6969;
    }
    toBeLoadedCount = 0;
    alreadyLoadedCount = 0;
    drawListCount = 0;

    if (numBlocks == 0) {
        return false;
    }

    const Block* currentBlock = GetBlockByNumber(blocks, blockNum);
    if (!currentBlock) {
        return false;
    }

    u16 previousBlocks[8] = {};
    u16 nextBlocks[8] = {};

    const Block* previousFrontier[2] = { currentBlock, nullptr };
    u32 previousFrontierIndex = 0;
    u32 previousCount = 0;
    while (previousCount < 7) {
        const u32 otherIndex = (previousFrontierIndex + 1) & 1u;
        const Block* block = previousFrontier[previousFrontierIndex];
        if (!block) {
            if (!previousFrontier[otherIndex]) {
                break;
            }
            previousFrontierIndex = otherIndex;
            continue;
        }

        if (block != currentBlock && !ContainsBlockNumber(previousBlocks, previousCount, block->blockNum)) {
            previousBlocks[previousCount++] = block->blockNum;
        }

        if (block->blockNum == 0) {
            previousFrontier[previousFrontierIndex] = nullptr;
            previousFrontierIndex = otherIndex;
            continue;
        }

        const s16 branch = (s16)block->unk38;
        if (branch >= 0 || (previousFrontier[0] && previousFrontier[1])) {
            previousFrontier[previousFrontierIndex] = GetBlockByNumber(blocks, block->GetPrevBlockNumber());
        }
        else {
            const Block* previousBlock = GetBlockByNumber(blocks, block->GetPrevBlockNumber());
            const Block* alternateBlock = GetBlockByNumber(blocks, (u32)(~branch));
            if (previousBlock && previousBlock->lodExtra1 == 2) {
                previousFrontier[previousFrontierIndex] = previousBlock;
                previousFrontier[otherIndex] = nullptr;
            }
            else if (alternateBlock && alternateBlock->lodExtra1 == 2) {
                previousFrontier[previousFrontierIndex] = alternateBlock;
                previousFrontier[otherIndex] = nullptr;
            }
            else if (previousBlock && previousBlock->lodExtra1 == 1) {
                previousFrontier[otherIndex] = previousBlock;
                previousFrontier[previousFrontierIndex] = alternateBlock;
            }
            else if (!alternateBlock || alternateBlock->lodExtra1 != 1) {
                previousFrontier[otherIndex] = previousBlock;
                previousFrontier[previousFrontierIndex] = alternateBlock;
            }
            else {
                previousFrontier[otherIndex] = alternateBlock;
                previousFrontier[previousFrontierIndex] = previousBlock;
            }
        }

        previousFrontierIndex = otherIndex;
    }

    const Block* nextFrontier[2] = { currentBlock, nullptr };
    u32 nextFrontierIndex = 0;
    u32 nextCount = 0;
    while (nextCount < 7) {
        const u32 otherIndex = (nextFrontierIndex + 1) & 1u;
        const Block* block = nextFrontier[nextFrontierIndex];
        if (!block) {
            if (!nextFrontier[otherIndex]) {
                break;
            }
            nextFrontierIndex = otherIndex;
            continue;
        }

        if (block != currentBlock && !ContainsBlockNumber(nextBlocks, nextCount, block->blockNum)) {
            nextBlocks[nextCount++] = block->blockNum;
        }

        const u32 nextBlockNum = block->GetNextBlockNumber();
        const Block* nextBlockCandidate = GetBlockByNumber(blocks, nextBlockNum);
        if (!nextBlockCandidate) {
            nextFrontier[nextFrontierIndex] = nullptr;
        }
        else {
            const s16 branch = (s16)block->unk38;
            if (branch <= 0 || (nextFrontier[0] && nextFrontier[1])) {
                nextFrontier[nextFrontierIndex] = nextBlockCandidate;
            }
            else {
                const Block* alternateBlock = GetBlockByNumber(blocks, (u32)(branch - 1));
                if (nextBlockCandidate && nextBlockCandidate->lodExtra0 == 2) {
                    nextFrontier[nextFrontierIndex] = nextBlockCandidate;
                    nextFrontier[otherIndex] = nullptr;
                }
                else if (alternateBlock && alternateBlock->lodExtra0 == 2) {
                    nextFrontier[nextFrontierIndex] = alternateBlock;
                    nextFrontier[otherIndex] = nullptr;
                }
                else if (nextBlockCandidate && nextBlockCandidate->lodExtra0 == 1) {
                    nextFrontier[otherIndex] = nextBlockCandidate;
                    nextFrontier[nextFrontierIndex] = alternateBlock;
                }
                else if (!alternateBlock || alternateBlock->lodExtra0 != 1) {
                    nextFrontier[otherIndex] = nextBlockCandidate;
                    nextFrontier[nextFrontierIndex] = alternateBlock;
                }
                else {
                    nextFrontier[otherIndex] = alternateBlock;
                    nextFrontier[nextFrontierIndex] = nextBlockCandidate;
                }
            }
        }

        nextFrontierIndex = otherIndex;
    }

    u32 previousLoad = (previousCount < currentBlock->lodFarX) ? previousCount : (u32)currentBlock->lodFarX;
    u32 nextLoad = (u32)currentBlock->lodNearX;
    u32 nextSpare = 0;
    if (nextCount >= nextLoad) {
        nextSpare = nextCount - nextLoad;
    }
    else {
        nextLoad = nextCount;
    }

    const u32 loadTotal = previousLoad + nextLoad + 1;
    if (loadTotal >= numBlocks) {
        if (loadTotal >= 8) {
            const u32 overflow = loadTotal - 7;
            if (nextLoad < overflow) {
                previousLoad -= overflow - nextLoad;
                nextLoad = 0;
            }
            else {
                nextLoad -= overflow;
            }
        }
    }
    else {
        const u32 deficit = numBlocks - loadTotal;
        if (nextSpare < deficit) {
            nextLoad += nextSpare;
            previousLoad += deficit - nextSpare;
        }
        else {
            nextLoad += deficit;
        }
    }

    toBeLoadedCount = 1;
    toBeLoadedList[0] = (u16)blockNum;
    for (u32 index = 0; index < previousLoad && toBeLoadedCount < 8; index++) {
        toBeLoadedList[toBeLoadedCount++] = previousBlocks[index];
    }
    for (u32 index = 0; index < nextLoad && toBeLoadedCount < 8; index++) {
        toBeLoadedList[toBeLoadedCount++] = nextBlocks[index];
    }

    if (toBeLoadedCount < numBlocks) {
        u16 minBlockNum = toBeLoadedList[0];
        for (u32 index = 1; index < toBeLoadedCount; index++) {
            if (toBeLoadedList[index] < minBlockNum) {
                minBlockNum = toBeLoadedList[index];
            }
        }

        for (s32 filler = (s32)minBlockNum - 1; filler >= 0 && toBeLoadedCount < numBlocks; filler--) {
            if (GetBlockByNumber(blocks, (u32)filler)) {
                toBeLoadedList[toBeLoadedCount++] = (u16)filler;
            }
        }
    }

    for (u32 left = 0; left < toBeLoadedCount; left++) {
        u32 minIndex = left;
        for (u32 right = left + 1; right < toBeLoadedCount; right++) {
            if (toBeLoadedList[right] < toBeLoadedList[minIndex]) {
                minIndex = right;
            }
        }

        if (minIndex != left) {
            const u16 tmp = toBeLoadedList[left];
            toBeLoadedList[left] = toBeLoadedList[minIndex];
            toBeLoadedList[minIndex] = tmp;
        }
    }

    u32 previousDraw = (previousCount < currentBlock->lodFarY) ? previousCount : (u32)currentBlock->lodFarY;
    u32 nextDraw = (u32)currentBlock->lodNearY;
    if (nextCount < nextDraw) {
        nextDraw = nextCount;
    }
    if (previousDraw + nextDraw + 1 >= 8) {
        const u32 overflow = previousDraw + nextDraw + 1 - 7;
        if (nextDraw < overflow) {
            previousDraw -= overflow - nextDraw;
            nextDraw = 0;
        }
        else {
            nextDraw -= overflow;
        }
    }

    drawListCount = 1;
    drawList[0] = (u16)blockNum;
    for (u32 index = 0; index < previousDraw && drawListCount < 8; index++) {
        drawList[drawListCount++] = previousBlocks[index];
    }
    for (u32 index = 0; index < nextDraw && drawListCount < 8; index++) {
        drawList[drawListCount++] = nextBlocks[index];
    }

    u32 previousActive = (previousCount < currentBlock->lodFarZ) ? previousCount : (u32)currentBlock->lodFarZ;
    u32 nextActive = (u32)currentBlock->lodNearZ;
    if (nextCount < nextActive) {
        nextActive = nextCount;
    }
    if (previousActive + nextActive + 1 >= 8) {
        const u32 overflow = previousActive + nextActive + 1 - 7;
        if (nextActive < overflow) {
            previousActive -= overflow - nextActive;
            nextActive = 0;
        }
        else {
            nextActive -= overflow;
        }
    }

    alreadyLoadedCount = 1;
    alreadyLoadedList[0] = (u16)blockNum;
    for (u32 index = 0; index < previousActive && alreadyLoadedCount < 8; index++) {
        alreadyLoadedList[alreadyLoadedCount++] = previousBlocks[index];
    }
    for (u32 index = 0; index < nextActive && alreadyLoadedCount < 8; index++) {
        alreadyLoadedList[alreadyLoadedCount++] = nextBlocks[index];
    }

    return toBeLoadedCount < numBlocks;
}
