#pragma once
#include "core.h"
#include "gen/manager.h"
#include "ai/thing.h"

// Manages character slot loading, model lifecycle, and animation handles.
// PC port: .RR files loaded from disk (assets/RCHARS/) via standard file I/O.

// Forward declarations
class CharacterManager;
struct CharFile;
class ccFile;

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
struct CharFile {
    void* dataBuffer = nullptr;       // PSX +0: raw .RR data section (resource 1)
    s32 dataSize = 0;                 // PSX +4: size in WORDS (bytes/4)
    ccFile* fileHandle = nullptr;     // PSX +8: file handle
    RREntry* rrHeader = nullptr;      // PSX +12: rrLoadHeaderOnly result
    s32 rrHeaderEntries = 0;          // PC: number of entries in header
    CharFile* next = nullptr;         // PSX +20: linked list next
    s32 refCount = 0;                 // PSX +24: reference count
    u32 thingType = 0;               // PSX +28: AI::ThingTypes

    void* wholeFileBuf = nullptr;

    CharFile(u32 type);
    ~CharFile();

    void AddRef();
    void DeleteRef();
    static CharFile* Find(u32 type);
    s32 FindAnim(u32 hash);
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

    AnimCallback(u32 type, s32 animEnum, u32 count, CharMgrCallback* cb);
    ~AnimCallback() override;

    void Callback() override;
};

// PSX: CharacterManager (~3004 bytes)
// Singleton manager for character slots, model loading, animation handles.
// Global: gp+796
static constexpr s32 CHAR_MAX_SLOTS = 4;
static constexpr s32 CHAR_MAX_ANIMS = 255;

class CharacterManager : public Manager {
public:
    CharSlot slots[CHAR_MAX_SLOTS];        // PSX +28: 4 slots of 424 bytes each
    void* animPtrs[CHAR_MAX_ANIMS] = {};   // PSX +1724: animation pointer array
    s32 animCount = 0;                     // PSX +2740
    void* freeListHead = nullptr;          // PSX +2744: free list into animPtrs
    u8 animRefCounts[CHAR_MAX_ANIMS] = {}; // PSX +2748: ref counts (bit 7 = cached)

    CharacterManager();
    ~CharacterManager() override;

    void OpenCharacter(u32 type);
    void CloseCharacter(u32 type);

    void LoadCharacter(u32 type, CharMgrCallback* callback = nullptr);
    void UnloadCharacter(u32 type);
    void ReloadCharacter(u32 type, s32 meshType, CharMgrCallback* callback = nullptr);
    void LoadCharTexture(u32 type);

    bool IsCharacterLoaded(u32 type);
    s32 GetNumberCharactersLoaded();
    void EnableCache(u32 type, s32 enable);

    void LoadAnimation(u32 type, u32 hash, CharMgrCallback* callback = nullptr);
    void LoadAnimation(u32 type, s32 animEnum, u32 count, CharMgrCallback* callback = nullptr);
    void LoadAnimationBatch(u32 type, s32 animEnum, CharMgrCallback* callback = nullptr);

    void UnloadAnimation(u32 type, u32 hash);
    void UnloadAnimation(u32 type, s32 animEnum, u32 count);
    void UnloadAnimationBatch(u32 type, s32 animEnum);

    void* GetAnimation(u32 type, s32 animEnum);
    s32 LookUpAnimation(u32 type, const char* name);
    void PurgeLevel();

    // PC: flush deferred TransformAnim deletions queued by FreeAnimMemory.
    // Called from EndFrameHandler after AnimateEverythingHandler completes.
    void FlushPendingFree();

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

void FreeAnimMemory(void* ptr);
u32 GetCompositeAnimationNameHash(const char* name);
s32* GetPlayerMeshType();

// Global CharFile list head (PSX: gp+800)
extern CharFile* g_charFileList;
