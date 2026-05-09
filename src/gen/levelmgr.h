#pragma once
#include "core.h"
#include "gen/manager.h"

// Forward declarations
struct OriginalBasic;

// PSX: ModelListEnum - selects which entity list to search
enum class ModelListEnum : s32 {
    Geo = 0,
    ETree = 1,
    STree = 2,
    Type3 = 3,
};

// PermMemEntry - persistent memory allocation tracked by LevelManager
struct PermMemEntry : public ccMinNode {
    void* data = nullptr;
    s32 size = 0;
    s32 id = 0;

    ~PermMemEntry() override {
        if (data) { std::free(data); data = nullptr; }
    }
};

// LevelManager (136 bytes on PSX) - manages per-level entity lists
// PSX layout:
//   +0:  Manager base (28 bytes)
//   +28: modelLists[4] (4 x ccMinList, 48 bytes)
//   +76: streeList (ccMinList)
//   +88: etreeList (ccMinList)
//   +100: geoList (ccMinList)
//   +112: permMemList (ccMinList)
//   +124: unknownList (ccMinList)
class LevelManager : public Manager {
public:
    LevelManager();
    ~LevelManager() override;

    void InternalOpen() override;   // 0x80059388
    void InternalClose() override;  // 0x80059390
    void InternalReset() override;  // 0x80059380

    void PurgeLevel();
    void PurgePetal();
    void LoadPetal();

    void AddOriginal(OriginalBasic* original, s32 param);
    void DeleteOriginal(OriginalBasic* original);

    OriginalBasic* FindModel(ModelListEnum listType, s32 id);
    OriginalBasic* FindModel(u32 id);
    OriginalBasic* FindETree(s32 id);
    OriginalBasic* FindSTree(s32 id);
    OriginalBasic* FindGeo(s32 id);

    void* AddPermMemory(s32 size, s32 id);
    void DeleteAllPermMem();
    void DeletePermMemID(s32 id);

private:
    void DeleteOriginalModelsByID(s32 id);
    void DeleteInventoryByID(s32 id);

public:
    ccMinList modelLists[4];    // +28: entity lists by type
    ccMinList streeList;        // +76: spatial tree nodes
    ccMinList etreeList;        // +88: export tree nodes
    ccMinList geoList;          // +100: geometry references
    ccMinList permMemList;      // +112: permanent memory allocations
    ccMinList unknownList;      // +124: additional list
};

// PSX: gp+0xEE8, defined in levelmgr.cpp
extern LevelManager* g_levelManager;
