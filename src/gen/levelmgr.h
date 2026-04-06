// levelmgr.h - LevelManager reversed from PSX LEVELMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\LEVELMGR.CPP
// Manages entity/model lists for the current level. Coordinates
// CharacterManager, AnimationManager, and P3D inventory cleanup.
#pragma once

#include "core.h"
#include "gen/manager.h"

// Forward declarations
struct OriginalBasic;

// PSX: ModelListEnum - selects which entity list to search
enum class ModelListEnum : s32 {
    Geo     = 0,
    ETree   = 1,
    STree   = 2,
    Type3   = 3,
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
    // PSX: __12LevelManager (LEVELMGR.CPP, 0x80058AE4)
    LevelManager();
    // PSX: _._12LevelManager (LEVELMGR.CPP, 0x80058BD4)
    ~LevelManager() override;

    // Manager overrides (all stubs on PSX)
    void InternalOpen() override;   // 0x80059388
    void InternalClose() override;  // 0x80059390
    void InternalReset() override;  // 0x80059380

    // PSX: PurgeLevel__12LevelManager (0x80058CC8)
    void PurgeLevel();
    // PSX: PurgePetal__12LevelManager (0x80058DB4)
    void PurgePetal();
    // PSX: LoadPetal__12LevelManager (0x80058E68)
    void LoadPetal();

    // PSX: AddOriginal__12LevelMangerP13OriginalBasicl (0x80058F84)
    void AddOriginal(OriginalBasic* original, s32 param);
    // PSX: DeleteOriginal__12LevelManagerP13OriginalBasic (0x80058FF0)
    void DeleteOriginal(OriginalBasic* original);

    // PSX: FindModel (0x80059268, 0x800592A0)
    OriginalBasic* FindModel(ModelListEnum listType, s32 id);
    OriginalBasic* FindModel(s32 id);
    // PSX: FindSTree__12LevelManagerl (0x80059338)
    OriginalBasic* FindSTree(s32 id);

    // PSX: AddPermMemory__12LevelManagerPcl (0x800590D8)
    void* AddPermMemory(s32 size, s32 id);
    // PSX: DeleteAllPermMem__12LevelManager (0x8005913C)
    void DeleteAllPermMem();
    // PSX: DeletePermMemID__12LevelManagerl (0x800591C8)
    void DeletePermMemID(s32 id);

private:
    // PSX helpers used by PurgeLevel/PurgePetal.
    // C++ signatures preserved as internal helpers.
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
