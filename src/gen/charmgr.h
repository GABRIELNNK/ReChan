// charmgr.h - CharacterManager reversed from PSX C:\CHAN\GAME\SRC\GEN\CHARMGR.CPP
// Manages character slot loading, model lifecycle, and animation handles.
// PC port: .RR files loaded from disk (assets/RCHARS/) via standard file I/O.
#pragma once

#include "core.h"
#include "gen/manager.h"
#include "ai/thing.h"
#include <cstdio>

// Forward declarations
class CharacterManager;
struct CharFile;

// PSX: 0x800D6880 - character name table indexed by ThingType
extern const char* g_charNameTable[];

// RR file header entry (8 bytes each in the .RR file header)
// PSX: PETLATL.C - rrSize reads entry[index].sizeShifted >> 8
//                   rrOffset reads entry[index].offset
struct RREntry {
    u32 offset;       // +0: byte offset into .RR file
    u32 sizeShifted;  // +4: size << 8 (actual size = value >> 8)
};

// Inline RR accessors matching PSX PETLATL.C
inline s32 rrSize(RREntry* header, s32 index) { return (s32)(header[index].sizeShifted >> 8); }
inline s32 rrOffset(RREntry* header, s32 index) { return (s32)header[index].offset; }

// PSX: CharMgrCallback base class (CHARMGR.HPP:82)
// Simple callback interface for async character/animation load completion.
struct CharMgrCallback {
    s32 done = 0;       // PSX +0: set to 1 when callback fires

    virtual ~CharMgrCallback();
    // PSX: Callback__14CharMgrCallback (CHARMGR.HPP:82)
    virtual void Callback();
};

// PSX: character slot (424 bytes, 4 slots in CharacterManager)
// Layout from CM base: slots[0] at CM+28, each 424 bytes
// thingType at slot+28, animIndexTable at slot+32
struct CharSlot {
    static constexpr u32 EMPTY_SENTINEL = 29;
    static constexpr u32 ANIM_TABLE_SIZE = 392;

    u32 thingType = EMPTY_SENTINEL;   // PSX slot+28: AI::ThingTypes, 29 = empty
    s32 loadCount = 0;                // PSX slot+4: reference count / CD reads pending
    CharFile* charFile = nullptr;     // PSX slot+8: CharFile handle
    void* model = nullptr;            // PSX slot+12: OriginalSTree*
    void* dataBuffer = nullptr;       // PSX slot+16: P3D data buffer
    s32 field48 = 0;                  // PSX slot+48
    s32 field52 = 0;                  // PSX slot+52
    s32 field56 = 0;                  // PSX slot+56
    u8 animIndexTable[ANIM_TABLE_SIZE] = {}; // PSX slot+32: maps animEnum byte -> handle index (0xFF = empty)
};

// PSX: CharFile (32 bytes) - file handle for a character .RR type
// PC: FILE* replaces rCDOpen handle; RREntry* replaces rrHeader void*
struct CharFile {
    void* dataBuffer = nullptr;       // PSX +0: raw .RR data section (resource 1)
    s32 dataSize = 0;                 // PSX +4: size in WORDS (bytes/4)
    FILE* fileHandle = nullptr;       // PSX +8: rCDOpen -> PC FILE*
    RREntry* rrHeader = nullptr;      // PSX +12: rrLoadHeaderOnly result
    s32 rrHeaderEntries = 0;          // PC: number of entries in header
    CharFile* next = nullptr;         // PSX +20: linked list next
    s32 refCount = 0;                 // PSX +24: reference count
    u32 thingType = 0;               // PSX +28: AI::ThingTypes

    // PSX: __8CharFileUs (CHARMGR.CPP:2682)
    CharFile(u32 type);
    // PSX: _._8CharFile (CHARMGR.CPP:2751)
    ~CharFile();

    // PSX: AddRef__8CharFile (CHARMGR.CPP)
    void AddRef();
    // PSX: DeleteRef__8CharFile (CHARMGR.CPP:2805)
    void DeleteRef();
    // PSX: Find__8CharFileUs (CHARMGR.CPP:2831) - static
    static CharFile* Find(u32 type);
    // PSX: FindAnim__8CharFileUl (CHARMGR.CPP:2861)
    s32 FindAnim(u32 hash);
    // PSX: EnableCache__8CharFilei (CHARMGR.CPP:2893)
    void EnableCache(s32 enable);

    // PC helper: read a resource from the .RR file into a new buffer
    u8* ReadResource(s32 index, s32* outSize = nullptr);
};

// PSX: AnimCallback (24 bytes) - chains batch animation loads
struct AnimCallback : public CharMgrCallback {
    u32 thingType = 0;               // PSX +8
    s32 animEnumCounter = 0;          // PSX +12
    s32 remainingCount = 0;           // PSX +16
    CharMgrCallback* userCallback = nullptr; // PSX +20

    // PSX: __12AnimCallbackUsil14CharMgrCallback (CHARMGR.CPP:2935)
    AnimCallback(u32 type, s32 animEnum, u32 hash, CharMgrCallback* cb);
    ~AnimCallback() override;
    // PSX: Callback__12AnimCallback (CHARMGR.CPP:2963)
    void Callback() override;
};

// PSX: CharacterManager (~3004 bytes)
// Singleton manager for character slots, model loading, animation handles.
// Global: gp+796
// Source: C:\CHAN\GAME\SRC\GEN\CHARMGR.CPP
static constexpr s32 CHAR_MAX_SLOTS = 4;
static constexpr s32 CHAR_MAX_ANIMS = 255;

class CharacterManager : public Manager {
public:
    CharSlot slots[CHAR_MAX_SLOTS];        // PSX +28: 4 slots of 424 bytes each
    void* animPtrs[CHAR_MAX_ANIMS] = {};   // PSX +1724: animation pointer array
    s32 animCount = 0;                     // PSX +2740
    void* freeListHead = nullptr;          // PSX +2744: free list into animPtrs
    u8 animRefCounts[CHAR_MAX_ANIMS] = {}; // PSX +2748: ref counts (bit 7 = cached)

    // PSX: __16CharacterManager (CHARMGR.CPP:334)
    CharacterManager();
    // PSX: _._16CharacterManager (CHARMGR.CPP:388)
    ~CharacterManager() override;

    // PSX: OpenCharacter__16CharacterManagerUs (CHARMGR.CPP:425)
    void OpenCharacter(u32 type);
    // PSX: CloseCharacter__16CharacterManagerUs (CHARMGR.CPP:467)
    void CloseCharacter(u32 type);

    // PSX: LoadCharacter__16CharacterManagerUsP14CharMgrCallback (CHARMGR.CPP:496)
    void LoadCharacter(u32 type, CharMgrCallback* callback = nullptr);
    // PSX: UnloadCharacter__16CharacterManagerUs (CHARMGR.CPP:693)
    void UnloadCharacter(u32 type);
    // PSX: ReloadCharacter__16CharacterManagerUslP14CharMgrCallback (CHARMGR.CPP:792)
    void ReloadCharacter(u32 type, s32 meshType, CharMgrCallback* callback = nullptr);

    // PSX: LoadCharTexture__16CharacterManagerUs (CHARMGR.CPP:931)
    void LoadCharTexture(u32 type);

    // PSX: IsCharacterLoaded__16CharacterManagerUs (CHARMGR.CPP:1010)
    bool IsCharacterLoaded(u32 type);
    // PSX: GetNumberCharactersLoaded__16CharacterManager (CHARMGR.CPP:1038)
    s32 GetNumberCharactersLoaded();

    // PSX: EnableCache__16CharacterManagerUsi (CHARMGR.CPP:1073)
    void EnableCache(u32 type, s32 enable);

    // PSX: LoadAnimation overloads (CHARMGR.CPP:1309, 1383, 1448)
    void LoadAnimation(u32 type, u32 hash, CharMgrCallback* callback = nullptr);
    void LoadAnimation(u32 type, s32 animEnum, u32 hash, CharMgrCallback* callback = nullptr);
    void LoadAnimationBatch(u32 type, s32 animEnum, CharMgrCallback* callback = nullptr);

    // PSX: UnloadAnimation overloads (CHARMGR.CPP:1740, 1813, 1838)
    void UnloadAnimation(u32 type, u32 hash);
    void UnloadAnimation(u32 type, s32 animEnum, u32 hash);
    void UnloadAnimationBatch(u32 type, s32 animEnum);

    // PSX: GetAnimation__16CharacterManagerUsQ2_2AI9AnimEnums (CHARMGR.CPP:1968)
    void* GetAnimation(u32 type, s32 animEnum);
    // PSX: LookUpAnimation__16CharacterManagerUsPCc (CHARMGR.CPP:2023)
    s32 LookUpAnimation(u32 type, const char* name);

    // PSX: PurgeLevel__16CharacterManager (CHARMGR.CPP:2054)
    void PurgeLevel();

    // PSX: InternalReset/Open/Close (empty stubs)
    void InternalReset() override;
    void InternalOpen() override;
    void InternalClose() override;

private:
    // Find slot index for a type, returns -1 if not found
    s32 FindSlot(u32 type);
    // Find empty slot, returns -1 if none
    s32 FindEmptySlot();
};

// Global singleton (PSX: gp+796)
extern CharacterManager* g_characterManager;

// PSX: FreeAnimMemory (CHARMGR.CPP:201)
void FreeAnimMemory(void* ptr);

// PSX: GetCompositeAnimationNameHash (CHARMGR.CPP:267)
u32 GetCompositeAnimationNameHash(const char* name);

// PSX: GetPlayerMeshType (CHARMGR.CPP:301)
s32* GetPlayerMeshType();

// Global CharFile list head (PSX: gp+800)
extern CharFile* g_charFileList;
