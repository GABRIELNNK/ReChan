// charmgr.cpp - CharacterManager reversed from PSX C:\CHAN\GAME\SRC\GEN\CHARMGR.CPP
// PC port: .RR files loaded from disk (assets/RCHARS/) via standard file I/O.
#include "gen/charmgr.h"
#include "p3d/hash.h"
#include "p3d/loadmanager.h"
#include "p3d/texture.h"
#include "p3d/inventory.h"
#include "p3d/context.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>

// Global singleton (PSX: gp+796)
CharacterManager* g_characterManager = nullptr;

// Global CharFile list head (PSX: gp+800)
CharFile* g_charFileList = nullptr;

// PSX: gp+3468 - mesh type for reload
static s32 g_playerMeshType = 0;

// PSX: 0x800D6880 - character name table indexed by ThingType
// 29 entries (0-28), index 29 is EMPTY_SENTINEL
const char* g_charNameTable[] = {
    "JACKIE",   // 0 - player
    "LENNY",    // 1
    "IGOR",     // 2
    "ROSCOE",   // 3
    "MIME",      // 4
    "DISCO",    // 5
    "HOOD",     // 6
    "CLOWN",    // 7
    "BROCK",    // 8
    "WAITER",   // 9
    "CHEF",     // 10
    "JACQUES",  // 11
    "DANTE",    // 12
    "JANITOR",  // 13
    "SHAOLIN",  // 14
    "BLIND",    // 15
    "VAGRANT",  // 16
    "ELITE",    // 17
    "TAO",      // 18
    "HAZARD",   // 19
    "FACTORY",  // 20
    "SHO",      // 21
    "STEEL",    // 22
    "GRONTAR",  // 23
    "GOR",      // 24
    "YAKUZA",   // 25
    "LORNA",    // 26
    "YAK",      // 27
    "BLIND",    // 28 (duplicate)
};


// Free functions
// PSX: FreeAnimMemory (CHARMGR.CPP:201, 0x800395F8)
void FreeAnimMemory(void* ptr) {
    MARKFUNCTION(0x800395F8);
    // PSX: rPFree(g_MemoryHeap, ptr)
    std::free(ptr);
}

// PSX: GetCompositeAnimationNameHash (CHARMGR.CPP:267, 0x80039624)
u32 GetCompositeAnimationNameHash(const char* name) {
    MARKFUNCTION(0x80039624);
    // PSX: builds "RCHARS\\{name}.P3D" then hashes with p3dHash
    char buf[96] = {};
    std::strcpy(buf, "RCHARS\\");
    std::strcat(buf, name);
    std::strcat(buf, ".P3D");
    return p3dHash(buf);
}

// PSX: GetPlayerMeshType (CHARMGR.CPP:301, 0x800396BC)
// PSX returns pointer to global: return gp+3468
s32* GetPlayerMeshType() {
    MARKFUNCTION(0x800396BC);
    return &g_playerMeshType;
}


// CharMgrCallback
// PSX: ~CharMgrCallback (CHARMGR.HPP:83, 0x8003BA98)
CharMgrCallback::~CharMgrCallback() {
    MARKFUNCTION(0x8003BA98);
}

// PSX: Callback__14CharMgrCallback (CHARMGR.HPP:82, 0x8003BACC)
void CharMgrCallback::Callback() {
    MARKFUNCTION(0x8003BACC);
    done = 1;
}


// AnimCallback
// PSX: __12AnimCallback (CHARMGR.CPP:2935, 0x8003B964)
AnimCallback::AnimCallback(u32 type, s32 animEnum, u32 count, CharMgrCallback* cb) {
    MARKFUNCTION(0x8003B964);
    done = 0;
    thingType = type;
    animEnumCounter = animEnum;
    remainingCount = (s32)count;
    userCallback = cb;
}

// PSX: _._12AnimCallback (CHARMGR.CPP, 0x8003BA4C)
AnimCallback::~AnimCallback() {
    MARKFUNCTION(0x8003BA4C);
}

// PSX: Callback__12AnimCallback (CHARMGR.CPP:2963, 0x8003B99C)
void AnimCallback::Callback() {
    MARKFUNCTION(0x8003B99C);
    animEnumCounter++;
    remainingCount--;

    if (remainingCount != 0) {
        // More to load - chain next animation
        if (g_characterManager) {
            g_characterManager->LoadAnimationBatch(thingType, animEnumCounter, this);
        }
        return;
    }

    // Done - invoke user callback
    if (userCallback) {
        userCallback->Callback();
    }
    delete this;
}


// CharFile
// PSX: __8CharFile (CHARMGR.CPP:2682, 0x8003B694)
// PSX: builds "RCHARS\\{name}.RR", calls rCDOpen, rrLoadHeaderOnly,
//      allocates buffer, reads resource 1 synchronously.
// PC: opens file from disk, reads header, reads resource 1.
CharFile::CharFile(u32 type) {
    MARKFUNCTION(0x8003B694);
    refCount = 1;
    thingType = type;
    dataBuffer = nullptr;
    dataSize = 0;
    fileHandle = nullptr;
    rrHeader = nullptr;
    rrHeaderEntries = 0;

    // Build path: "RCHARS/{name}.RR"
    char pathBuf[128];
    std::snprintf(pathBuf, sizeof(pathBuf), "RCHARS/%s.RR", g_charNameTable[type]);

    // Open file (PSX: rCDOpen)
    fileHandle = std::fopen(pathBuf, "rb");
    if (!fileHandle) {
        RC_ERR("[CharFile] Failed to open: %s", pathBuf);
        next = g_charFileList;
        g_charFileList = this;
        return;
    }

    // PSX: rrLoadHeaderOnly - reads first 2048 bytes (or file size if smaller)
    // The header is an array of RREntry (8 bytes each)
    // PSX reads up to min(fileSize, 2048) bytes
    std::fseek(fileHandle, 0, SEEK_END);
    s32 fileSize = (s32)std::ftell(fileHandle);
    std::fseek(fileHandle, 0, SEEK_SET);

    s32 headerSize = (fileSize < 2048) ? fileSize : 2048;
    rrHeader = (RREntry*)std::malloc(headerSize);
    std::fread(rrHeader, 1, headerSize, fileHandle);
    rrHeaderEntries = headerSize / (s32)sizeof(RREntry);

    // Read resource 1 (the animation hash data section)
    // PSX: dataBuffer = rPMalloc(heap, rrSize(header, 1), 0)
    //      rCDSeekA(handle, rrOffset(header, 1), 0)
    //      rCDReadA(handle, dataBuffer, rrSize(header, 1))
    //      rCDWaitUntilDone()
    s32 dataByteSize = rrSize(rrHeader, 1);
    if (dataByteSize > 0) {
        dataBuffer = std::malloc(dataByteSize);
        std::fseek(fileHandle, rrOffset(rrHeader, 1), SEEK_SET);
        std::fread(dataBuffer, 1, dataByteSize, fileHandle);
        dataSize = dataByteSize / 4; // PSX stores size in words
    }

    // Link into global list (PSX: next = gp+800; gp+800 = this)
    next = g_charFileList;
    g_charFileList = this;

    RC_LOG("[CharFile] Opened %s (type %u, %d resources)", pathBuf, type, rrHeaderEntries);
}

// PSX: _._8CharFile (CHARMGR.CPP:2751, 0x8003B798)
CharFile::~CharFile() {
    MARKFUNCTION(0x8003B798);
    // Unlink from global list
    CharFile** prev = &g_charFileList;
    while (*prev && *prev != this) {
        prev = &(*prev)->next;
    }
    if (*prev == this) {
        *prev = next;
    }

    // PSX: rCDCloseA, rPFree(rrHeader), rPFree(dataBuffer)
    if (fileHandle) {
        std::fclose(fileHandle);
        fileHandle = nullptr;
    }
    std::free(rrHeader);
    rrHeader = nullptr;
    std::free(dataBuffer);
    dataBuffer = nullptr;
}

// PSX: AddRef__8CharFile (0x8003B83C)
void CharFile::AddRef() {
    MARKFUNCTION(0x8003B83C);
    refCount++;
}

// PSX: DeleteRef__8CharFile (CHARMGR.CPP:2805, 0x8003B850)
void CharFile::DeleteRef() {
    MARKFUNCTION(0x8003B850);
    refCount--;
    if (refCount <= 0) {
        delete this;
    }
}

// PSX: Find__8CharFileUs (CHARMGR.CPP:2831, 0x8003B88C)
CharFile* CharFile::Find(u32 type) {
    MARKFUNCTION(0x8003B88C);
    CharFile* cf = g_charFileList;
    while (cf) {
        if (cf->thingType == type) {
            return cf;
        }
        cf = cf->next;
    }
    return nullptr;
}

// PSX: FindAnim__8CharFileUl (CHARMGR.CPP:2861, 0x8003B8C4)
// Scans the RR data buffer (word array) from index 2 to dataSize for a matching hash.
// Returns 0-based index (i-2) on match, 0xFFFF if not found.
s32 CharFile::FindAnim(u32 hash) {
    MARKFUNCTION(0x8003B8C4);
    u32* data = (u32*)dataBuffer;
    for (s32 i = 2; i < dataSize; i++) {
        if (data[i] == hash) {
            return i - 2;
        }
    }
    return (s32)0xFFFF;
}

// PSX: EnableCache__8CharFilei (CHARMGR.CPP:2893, 0x8003B914)
// PSX: rCDCacheInit / rCDCacheTerm on the CD handle.
// PC: no-op (file I/O is already buffered)
void CharFile::EnableCache(s32 enable) {
    MARKFUNCTION(0x8003B914);
    (void)enable;
}

// PC helper: read a resource from the .RR file into a freshly allocated buffer.
// Caller owns the returned pointer (free with std::free).
u8* CharFile::ReadResource(s32 index, s32* outSize) {
    if (!fileHandle || !rrHeader || index < 0 || index >= rrHeaderEntries) {
        if (outSize) *outSize = 0;
        return nullptr;
    }
    s32 size = rrSize(rrHeader, index);
    s32 offset = rrOffset(rrHeader, index);
    if (size <= 0) {
        if (outSize) *outSize = 0;
        return nullptr;
    }
    u8* buf = (u8*)std::malloc(size);
    std::fseek(fileHandle, offset, SEEK_SET);
    std::fread(buf, 1, size, fileHandle);
    if (outSize) *outSize = size;
    return buf;
}


// CharacterManager
// PSX: __16CharacterManager (CHARMGR.CPP:334, 0x800396C8)
CharacterManager::CharacterManager() {
    MARKFUNCTION(0x800396C8);

    // Build free list: animPtrs[i] = &animPtrs[i+1]
    for (s32 i = 0; i < CHAR_MAX_ANIMS - 1; i++) {
        animRefCounts[i] = 0;
        animPtrs[i] = &animPtrs[i + 1];
    }
    animPtrs[CHAR_MAX_ANIMS - 1] = nullptr;
    animRefCounts[CHAR_MAX_ANIMS - 1] = 0;

    animCount = 0;
    freeListHead = &animPtrs[0];

    // Initialize 4 character slots
    for (s32 s = 0; s < CHAR_MAX_SLOTS; s++) {
        slots[s].thingType = CharSlot::EMPTY_SENTINEL;
        slots[s].field48 = 0;
        slots[s].field52 = 0;
        slots[s].field56 = 0;
        std::memset(slots[s].animIndexTable, 0xFF, CharSlot::ANIM_TABLE_SIZE);
    }

    // Set global singleton (PSX: gp+796 = this)
    g_characterManager = this;
}

// PSX: _._16CharacterManager (CHARMGR.CPP:388, 0x80039794)
CharacterManager::~CharacterManager() {
    MARKFUNCTION(0x80039794);
    g_characterManager = nullptr;
}

// Helper: find slot index for type
s32 CharacterManager::FindSlot(u32 type) {
    for (s32 i = 0; i < CHAR_MAX_SLOTS; i++) {
        if (slots[i].thingType == type) return i;
    }
    return -1;
}

// Helper: find empty slot
s32 CharacterManager::FindEmptySlot() {
    for (s32 i = 0; i < CHAR_MAX_SLOTS; i++) {
        if (slots[i].thingType == CharSlot::EMPTY_SENTINEL) return i;
    }
    return -1;
}

// PSX: OpenCharacter__16CharacterManagerUs (CHARMGR.CPP:425, 0x800397C4)
// PSX: CharFile::Find(type), if null creates new CharFile(type). Does NOT touch slots.
void CharacterManager::OpenCharacter(u32 type) {
    MARKFUNCTION(0x800397C4);
    CharFile* cf = CharFile::Find(type);
    if (!cf) {
        cf = new CharFile(type);
    }
}

// PSX: CloseCharacter__16CharacterManagerUs (CHARMGR.CPP:467, 0x80039808)
// PSX: CharFile::Find(type) then cf->DeleteRef(). Does NOT touch slots.
void CharacterManager::CloseCharacter(u32 type) {
    MARKFUNCTION(0x80039808);
    CharFile* cf = CharFile::Find(type);
    if (cf) {
        cf->DeleteRef();
    }
}

// PSX: LoadCharacter__16CharacterManagerUsP14CharMgrCallback (CHARMGR.CPP:496, 0x80039830)
// PSX: Full async CD pipeline. PC: synchronous file read from .RR.
// Finds CharFile, allocates slot (0 for player, 1-3 for NPCs), reads P3D model data.
void CharacterManager::LoadCharacter(u32 type, CharMgrCallback* callback) {
    MARKFUNCTION(0x80039830);

    CharFile* cf = CharFile::Find(type);
    if (!cf) {
        RC_ERR("[CharMgr] LoadCharacter: no CharFile for type %u", type);
        if (callback) callback->Callback();
        return;
    }

    // Find slot - PSX: type 0 -> slot 0, else search slots 1-3 for empty
    s32 slotIdx;
    if (type == 0) {
        slotIdx = 0;
    } else {
        slotIdx = -1;
        for (s32 i = 1; i < CHAR_MAX_SLOTS; i++) {
            if (slots[i].thingType == CharSlot::EMPTY_SENTINEL) {
                slotIdx = i;
                break;
            }
        }
        if (slotIdx < 0) {
            RC_ERR("[CharMgr] LoadCharacter: no empty slot for type %u", type);
            if (callback) callback->Callback();
            return;
        }
    }

    CharSlot& slot = slots[slotIdx];
    slot.thingType = type;
    slot.charFile = cf;
    cf->AddRef();

    // PSX: rrIdx = slotIdx * 2 + 3 (resource index for P3D model data)
    s32 rrIdx = slotIdx * 2 + 3;

    // Read P3D model data (resource rrIdx)
    s32 dataSize = 0;
    u8* dataBuf = cf->ReadResource(rrIdx, &dataSize);

    if (type == 0) {
        // Player: also read skeleton/extra buffer (resource rrIdx-1)
        // PSX: takes max of rrSize(hdr, rrIdx-1) and rrSize(hdr, slotIdx*2+4)
        s32 skelSize1 = rrSize(cf->rrHeader, rrIdx - 1);
        s32 skelSize2 = rrSize(cf->rrHeader, slotIdx * 2 + 4);
        s32 skelSize = std::max(skelSize1, skelSize2);
        u8* skelBuf = (u8*)std::malloc(skelSize);
        std::fseek(cf->fileHandle, rrOffset(cf->rrHeader, rrIdx - 1), SEEK_SET);
        std::fread(skelBuf, 1, skelSize, cf->fileHandle);
        slot.dataBuffer = skelBuf;
        g_playerMeshType = 0;
    } else {
        // NPC: read extra buffer (resource rrIdx-1)
        s32 extraSize = rrSize(cf->rrHeader, rrIdx - 1);
        u8* extraBuf = (u8*)std::malloc(extraSize);
        std::fseek(cf->fileHandle, rrOffset(cf->rrHeader, rrIdx - 1), SEEK_SET);
        std::fread(extraBuf, 1, extraSize, cf->fileHandle);
        slot.dataBuffer = extraBuf;
    }

    // PSX: sets loadCount=2, then CharDataLoadCallback processes the P3D data.
    // PC: synchronous - process inline.
    slot.loadCount = 1;
    std::memset(slot.animIndexTable, 0xFF, CharSlot::ANIM_TABLE_SIZE);

    // PSX: CharDataLoadCallback creates 8 loaders on stack, calls P3DLoad.
    // The P3D data (resource rrIdx) is a TexturePage (0xFF04) stream.
    // P3DLoad parses the 0xFF04 container and loads 0x6008 Texture sub-chunks
    // into the p3d::inventory. The mesh data is in slot.dataBuffer (raw PRIM).
    P3DLoadTextures(dataBuf, dataSize);
    std::free(dataBuf);

    // PSX: CharDataLoadCallback post-processing:
    // 1. Frees the P3D data buffer (done above)
    // 2. Looks up the loaded model in p3d::inventory by name hash
    // 3. For player: finds model in LevelManager, creates OriginalSTree (skeleton copy)
    // 4. Associates textures with the model
    // TODO: post-load model lookup and OriginalSTree creation

    RC_LOG("[CharMgr] Loaded character type %u into slot %d", type, slotIdx);

    if (callback) {
        callback->Callback();
    }
}

// PSX: UnloadCharacter__16CharacterManagerUs (CHARMGR.CPP:693, 0x80039C3C)
void CharacterManager::UnloadCharacter(u32 type) {
    MARKFUNCTION(0x80039C3C);

    s32 idx = FindSlot(type);
    if (idx < 0) return;

    CharSlot& slot = slots[idx];
    slot.loadCount--;
    if (slot.loadCount > 0) return;

    // TODO: delete model from P3D inventory (drawable + skeleton sections)
    // TODO: find/delete composite anim via GetCompositeAnimationNameHash
    // TODO: find/delete OriginalSTree from LevelManager

    // Clear slot
    slot.thingType = CharSlot::EMPTY_SENTINEL;

    // Release CharFile
    if (slot.charFile) {
        slot.charFile->DeleteRef();
        slot.charFile = nullptr;
    }

    // Free data buffer
    if (slot.dataBuffer) {
        std::free(slot.dataBuffer);
        slot.dataBuffer = nullptr;
    }

    slot.model = nullptr;
}

// PSX: ReloadCharacter__16CharacterManagerUslP14CharMgrCallback (CHARMGR.CPP:792, 0x80039DC4)
void CharacterManager::ReloadCharacter(u32 type, s32 meshType, CharMgrCallback* callback) {
    MARKFUNCTION(0x80039DC4);

    s32 idx = FindSlot(type);
    if (idx < 0) {
        if (callback) callback->Callback();
        return;
    }

    g_playerMeshType = meshType;

    CharSlot& slot = slots[idx];
    CharFile* cf = slot.charFile;
    if (!cf) {
        if (callback) callback->Callback();
        return;
    }

    // TODO: remove old model from P3D inventory (same as UnloadCharacter)
    // TODO: delete OriginalSTree from LevelManager

    // Re-read from .RR (same resource indices as LoadCharacter)
    s32 rrIdx = idx * 2 + 3;

    // Free old data buffer and re-read
    if (slot.dataBuffer) {
        std::free(slot.dataBuffer);
        slot.dataBuffer = nullptr;
    }

    s32 extraSize = rrSize(cf->rrHeader, rrIdx - 1);
    u8* extraBuf = (u8*)std::malloc(extraSize);
    std::fseek(cf->fileHandle, rrOffset(cf->rrHeader, rrIdx - 1), SEEK_SET);
    std::fread(extraBuf, 1, extraSize, cf->fileHandle);
    slot.dataBuffer = extraBuf;

    s32 dataSize = 0;
    u8* dataBuf = cf->ReadResource(rrIdx, &dataSize);

    // PSX: CharDataLoadCallback processes the TexturePage via P3DLoad
    P3DLoadTextures(dataBuf, dataSize);
    std::free(dataBuf);

    // TODO: post-load model lookup and OriginalSTree creation

    if (callback) callback->Callback();
}

// PSX: LoadCharTexture__16CharacterManagerUs (CHARMGR.CPP:931, 0x8003A078)
void CharacterManager::LoadCharTexture(u32 type) {
    MARKFUNCTION(0x8003A078);

    s32 idx = FindSlot(type);
    if (idx < 0) return;

    CharSlot& slot = slots[idx];
    CharFile* cf = slot.charFile;
    if (!cf) return;

    // PSX: texture resource index depends on meshType and IsDrunkenMasterSuitEnabled
    // Base: rrIdx = idx * 2 + 4
    s32 rrIdx = idx * 2 + 4;
    // TODO: if type == 0 && IsDrunkenMasterSuitEnabled(), rrIdx = idx * 2 + 6

    s32 texSize = 0;
    u8* texBuf = cf->ReadResource(rrIdx, &texSize);
    if (!texBuf) return;

    // PSX: creates tTexLoader on stack, calls P3DLoad(loaders, texBuf, 0)
    // PC: use tP3DFileHandler to load the TexturePage resource
    P3DLoadTextures(texBuf, texSize);
    std::free(texBuf);
}

// PSX: IsCharacterLoaded__16CharacterManagerUs (CHARMGR.CPP:1010, 0x8003A1A4)
bool CharacterManager::IsCharacterLoaded(u32 type) {
    MARKFUNCTION(0x8003A1A4);
    return FindSlot(type) >= 0;
}

// PSX: GetNumberCharactersLoaded__16CharacterManager (CHARMGR.CPP:1038, 0x8003A1D4)
s32 CharacterManager::GetNumberCharactersLoaded() {
    MARKFUNCTION(0x8003A1D4);
    s32 count = 0;
    for (s32 i = 0; i < CHAR_MAX_SLOTS; i++) {
        if (slots[i].thingType != CharSlot::EMPTY_SENTINEL) {
            count++;
        }
    }
    return count;
}

// PSX: EnableCache__16CharacterManagerUsi (CHARMGR.CPP:1073, 0x8003A20C)
// PSX: uses CharFile::Find(type) directly, not FindSlot.
void CharacterManager::EnableCache(u32 type, s32 enable) {
    MARKFUNCTION(0x8003A20C);
    CharFile* cf = CharFile::Find(type);
    if (cf) {
        cf->EnableCache(enable);
    }
}

// PSX: LoadAnimation (hash overload) (CHARMGR.CPP:1309, 0x8003A240)
// PSX: looks up hash in animHashTable to find animEnum, then delegates.
void CharacterManager::LoadAnimation(u32 type, u32 hash, CharMgrCallback* callback) {
    MARKFUNCTION(0x8003A240);

    // TODO: look up hash in animHashTable (0x800D68F4) to get animEnum
    // Table format: { u32 hash, s32 animEnum } terminated by hash==-1
    s32 animEnum = -1;
    (void)hash;

    if (animEnum < 0) {
        if (callback) callback->Callback();
        return;
    }

    LoadAnimation(type, animEnum, hash, callback);
}

// PSX: LoadAnimation (enum+hash overload) (CHARMGR.CPP:1383, 0x8003A328)
// PSX: creates AnimCallback, delegates to LoadAnimationBatch.
void CharacterManager::LoadAnimation(u32 type, s32 animEnum, u32 hash, CharMgrCallback* callback) {
    MARKFUNCTION(0x8003A328);

    AnimCallback* ac = new AnimCallback(type, animEnum, hash, callback);
    LoadAnimationBatch(type, animEnum, ac);
}

// PSX: LoadAnimationBatch (CHARMGR.CPP:1448, 0x8003A3EC)
// PSX: 1020 bytes. Checks if anim already loaded, allocates handle from free list,
//      reads P3D data from .RR, invokes callback.
void CharacterManager::LoadAnimationBatch(u32 type, s32 animEnum, CharMgrCallback* callback) {
    MARKFUNCTION(0x8003A3EC);

    s32 idx = FindSlot(type);
    if (idx < 0) {
        if (callback) callback->Callback();
        return;
    }

    CharSlot& slot = slots[idx];

    // Check if already loaded
    if (animEnum >= 0 && animEnum < (s32)CharSlot::ANIM_TABLE_SIZE) {
        if (slot.animIndexTable[animEnum] != 0xFF) {
            u8 handle = slot.animIndexTable[animEnum];
            if (handle < CHAR_MAX_ANIMS) {
                animRefCounts[handle]++;
            }
            if (callback) callback->Callback();
            return;
        }
    }

    CharFile* cf = slot.charFile;
    if (!cf) {
        if (callback) callback->Callback();
        return;
    }

    // TODO: check if animation already in P3D inventory

    // Allocate handle from free list
    if (freeListHead == nullptr) {
        if (callback) callback->Callback();
        return;
    }

    void** freeSlot = (void**)freeListHead;
    freeListHead = *freeSlot;
    s32 handleIdx = (s32)(freeSlot - &animPtrs[0]);

    animRefCounts[handleIdx] = 2; // PSX: initial refcount
    slot.loadCount++;

    // PSX: p3dIdx = animEnum * 2 + 9, paramIdx = animEnum * 2 + 8
    // Resource at paramIdx is raw animation data (P3D-independent binary format).
    // Resource at p3dIdx is a TexturePage (0xFF04) for animation-specific textures.
    s32 p3dIdx = animEnum * 2 + 9;
    s32 paramIdx = animEnum * 2 + 8;

    // Read raw animation data from .RR (not P3D format)
    s32 animSize = 0;
    u8* animBuf = cf->ReadResource(paramIdx, &animSize);

    // Read and process TexturePage for this animation
    s32 p3dSize = 0;
    u8* p3dBuf = cf->ReadResource(p3dIdx, &p3dSize);
    if (p3dBuf) {
        // PSX: AnimLoadCallback creates loaders, calls P3DLoad for textures
        P3DLoadTextures(p3dBuf, p3dSize);
        std::free(p3dBuf);
    }

    // TODO: parse the raw animation data (animBuf) into tCompositeAnim/tSequenceAnim
    // For now, store the raw buffer pointer for future use
    animPtrs[handleIdx] = animBuf;

    // Store handle in slot's anim table
    if (animEnum >= 0 && animEnum < (s32)CharSlot::ANIM_TABLE_SIZE) {
        slot.animIndexTable[animEnum] = (u8)handleIdx;
    }

    // Decrement initial refcount (PSX does this after callback)
    animRefCounts[handleIdx]--;

    if (callback) callback->Callback();
}

// PSX: UnloadAnimation (hash overload) (CHARMGR.CPP:1740, 0x8003A7E8)
void CharacterManager::UnloadAnimation(u32 type, u32 hash) {
    MARKFUNCTION(0x8003A7E8);

    // TODO: look up hash in animHashTable to get animEnum
    s32 animEnum = -1;
    (void)hash;

    if (animEnum < 0) return;

    s32 idx = FindSlot(type);
    if (idx < 0) return;

    UnloadAnimationBatch(type, animEnum);
}

// PSX: UnloadAnimation (enum+hash batch range) (CHARMGR.CPP:1813, 0x8003A8C0)
// PSX: third param is COUNT, not hash. Loops animEnum..animEnum+count.
void CharacterManager::UnloadAnimation(u32 type, s32 startEnum, u32 count) {
    MARKFUNCTION(0x8003A8C0);

    s32 endEnum = startEnum + (s32)count;
    for (s32 e = startEnum; e < endEnum; e++) {
        UnloadAnimationBatch(type, e);
    }
}

// PSX: UnloadAnimationBatch (single enum) (CHARMGR.CPP:1838, 0x8003A930)
// PSX: 788 bytes. Complex cleanup of composite vs non-composite anims.
void CharacterManager::UnloadAnimationBatch(u32 type, s32 animEnum) {
    MARKFUNCTION(0x8003A930);

    s32 idx = FindSlot(type);
    if (idx < 0) return;

    CharSlot& slot = slots[idx];

    if (animEnum < 0 || animEnum >= (s32)CharSlot::ANIM_TABLE_SIZE) return;

    u8 handleIdx = slot.animIndexTable[animEnum];
    if (handleIdx == 0xFF) return;

    // Clear table entry
    slot.animIndexTable[animEnum] = 0xFF;

    // Decrement slot loadCount
    slot.loadCount--;

    // Decrement animation refcount
    animRefCounts[handleIdx]--;

    if ((animRefCounts[handleIdx] & 0x7F) != 0) {
        // Still referenced
        return;
    }

    // TODO: full P3D inventory cleanup for composite (0x1000B) vs non-composite
    // For now: free memory and return handle to free list
    bool isCached = (animRefCounts[handleIdx] & 0x80) != 0;
    if (!isCached) {
        FreeAnimMemory(animPtrs[handleIdx]);
    }

    animRefCounts[handleIdx] = 0;
    animPtrs[handleIdx] = freeListHead;
    freeListHead = &animPtrs[handleIdx];
}

// PSX: GetAnimation__16CharacterManagerUsQ2_2AI9AnimEnums (CHARMGR.CPP:1968, 0x8003AC44)
void* CharacterManager::GetAnimation(u32 type, s32 animEnum) {
    MARKFUNCTION(0x8003AC44);

    for (s32 i = 0; i < CHAR_MAX_SLOTS; i++) {
        if (slots[i].thingType == type) {
            if (animEnum < 0 || animEnum >= (s32)CharSlot::ANIM_TABLE_SIZE) return nullptr;
            u8 handle = slots[i].animIndexTable[animEnum];
            if (handle == 0xFF) return nullptr;
            return animPtrs[handle];
        }
    }
    return nullptr;
}

// PSX: LookUpAnimation__16CharacterManagerUsPCc (CHARMGR.CPP:2023, 0x8003ACBC)
// PSX: uses p3dHash(name) directly, NOT GetCompositeAnimationNameHash.
s32 CharacterManager::LookUpAnimation(u32 type, const char* name) {
    MARKFUNCTION(0x8003ACBC);

    u32 hash = p3dHash(name);

    for (s32 i = 0; i < CHAR_MAX_SLOTS; i++) {
        if (slots[i].thingType == type) {
            CharFile* cf = slots[i].charFile;
            if (cf) return cf->FindAnim(hash);
            return (s32)0xFFFF;
        }
    }
    return (s32)0xFFFF;
}

// PSX: PurgeLevel__16CharacterManager (CHARMGR.CPP:2054, 0x8003AD44)
// PSX: sets abort flag, drains task list, unloads slots 1-3 (NPCs),
//      deletes CharFiles for types 1-28, partially unloads player slot 0 anims.
void CharacterManager::PurgeLevel() {
    MARKFUNCTION(0x8003AD44);

    // PSX: g_abortFlag=1, drain task list, g_abortFlag=0
    // PC: no async tasks, skip drain

    // Unload slots 1-3 (NPCs)
    for (s32 i = 1; i < CHAR_MAX_SLOTS; i++) {
        u32 type = slots[i].thingType;
        if (type == CharSlot::EMPTY_SENTINEL) continue;

        // PSX: UnloadAnimation(type, 0, 392) = unload ALL animations
        UnloadAnimation(type, 0, CharSlot::ANIM_TABLE_SIZE);
        UnloadCharacter(type);
        CloseCharacter(type);
    }

    // Delete all CharFiles for types 1-28
    for (u32 t = 1; t < 29; t++) {
        CharFile* cf = CharFile::Find(t);
        if (cf) cf->DeleteRef();
    }

    // Partially unload player slot 0 anims (range 124..392 = 268 count)
    u32 playerType = slots[0].thingType;
    if (playerType != CharSlot::EMPTY_SENTINEL) {
        UnloadAnimation(playerType, 124, 268);
    }
}

// PSX: InternalReset__16CharacterManager (0x8003BA80)
void CharacterManager::InternalReset() {
    MARKFUNCTION(0x8003BA80);
}

// PSX: InternalOpen__16CharacterManager (0x8003BA88)
void CharacterManager::InternalOpen() {
    MARKFUNCTION(0x8003BA88);
}

// PSX: InternalClose__16CharacterManager (0x8003BA90)
void CharacterManager::InternalClose() {
    MARKFUNCTION(0x8003BA90);
}
