// levelmgr.cpp - LevelManager reversed from PSX LEVELMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\LEVELMGR.CPP
#include "gen/levelmgr.h"

// PSX: gp+0xEE8
LevelManager* g_levelManager = nullptr;

// PSX: __12LevelManager (0x80058AE4)
LevelManager::LevelManager() {
    MARKFUNCTION(0x80058AE4);
    g_levelManager = this;
    LOG("[LevelManager] Created");
}

// PSX: _._12LevelManager (0x80058BD4)
LevelManager::~LevelManager() {
    MARKFUNCTION(0x80058BD4);
    DeleteAllPermMem();
    g_levelManager = nullptr;
}

// PSX: InternalOpen__12LevelManager (0x80059388) - NOP stub
void LevelManager::InternalOpen() {
    MARKFUNCTION(0x80059388);
}

// PSX: InternalClose__12LevelManager (0x80059390) - NOP stub
void LevelManager::InternalClose() {
    MARKFUNCTION(0x80059390);
}

// PSX: InternalReset__12LevelManager (0x80059380) - NOP stub
void LevelManager::InternalReset() {
    MARKFUNCTION(0x80059380);
}

// PSX: PurgeLevel__12LevelManager (0x80058CC8)
void LevelManager::PurgeLevel() {
    MARKFUNCTION(0x80058CC8);

    // PSX: DeleteListID(p3d_inventory, 1), DeleteListID(p3d_inventory, 2)
    // PSX: CharacterManager::PurgeLevel()
    // PSX: DeleteInventoryByID(this, 1), DeleteInventoryByID(this, 2)
    // PSX: clear modelLists[0]
    // PSX: EnvironmentManager::Reset()
    // PSX: DeletePermMemID(this, 1), DeletePermMemID(this, 2)
    // PSX: AnimationManager::PurgeLevel()

    // Clear all model lists
    for (s32 i = 0; i < 4; i++) {
        while (ccMinNode* n = modelLists[i].RemHead()) {
            delete n;
        }
    }

    // Clear tree/geo lists
    while (ccMinNode* n = streeList.RemHead()) { delete n; }
    while (ccMinNode* n = etreeList.RemHead()) { delete n; }
    while (ccMinNode* n = geoList.RemHead()) { delete n; }

    DeletePermMemID(1);
    DeletePermMemID(2);

    LOG("[LevelManager] PurgeLevel");
}

// PSX: PurgePetal__12LevelManager (0x80058DB4)
void LevelManager::PurgePetal() {
    MARKFUNCTION(0x80058DB4);
    // PSX: lighter cleanup than PurgeLevel - clears petal section only
    // TODO: implement petal-specific purge when petal loading is wired
    LOG("[LevelManager] PurgePetal");
}

// PSX: AddOriginal__12LevelManagerP13OriginalBasicl (0x80058F84)
void LevelManager::AddOriginal(OriginalBasic* original, s32 /*param*/) {
    MARKFUNCTION(0x80058F84);
    // PSX routes to different modelLists based on OriginalBasic::field_16
    // For now add to default list
    modelLists[0].AddNode(modelLists[0].tail, (ccMinNode*)original);
}

// PSX: DeleteOriginal__12LevelManagerP13OriginalBasic (0x80058FF0)
void LevelManager::DeleteOriginal(OriginalBasic* original) {
    MARKFUNCTION(0x80058FF0);
    // Try to find and remove from all lists
    for (s32 i = 0; i < 4; i++) {
        for (ccMinNode* n = modelLists[i].head; n; n = n->next) {
            if (n == (ccMinNode*)original) {
                modelLists[i].RemNode(n);
                delete n;
                return;
            }
        }
    }
}

// PSX: FindModel__12LevelManagerQ212LevelManager13ModelListEnuml (0x80059268)
void* LevelManager::FindModel(ModelListEnum listType, s32 id) {
    MARKFUNCTION(0x80059268);
    s32 idx = static_cast<s32>(listType);
    if (idx < 0 || idx > 3) return nullptr;
    // PSX uses FindNodeCRC on the list
    // TODO: implement CRC-based lookup when OriginalBasic has nameCRC
    (void)id;
    return nullptr;
}

// PSX: FindModel__12LevelManagerl (0x800592A0)
void* LevelManager::FindModel(s32 id) {
    MARKFUNCTION(0x800592A0);
    // Search all 4 lists
    for (s32 i = 0; i < 4; i++) {
        void* result = FindModel(static_cast<ModelListEnum>(i), id);
        if (result) return result;
    }
    return nullptr;
}

// PSX: AddPermMemory__12LevelManagerPcl (0x800590D8)
void* LevelManager::AddPermMemory(s32 size, s32 id) {
    MARKFUNCTION(0x800590D8);
    PermMemEntry* entry = new PermMemEntry();
    entry->data = std::malloc(size);
    entry->size = size;
    entry->id = id;
    permMemList.AddNode(permMemList.tail, entry);
    return entry->data;
}

// PSX: DeleteAllPermMem__12LevelManager (0x8005913C)
void LevelManager::DeleteAllPermMem() {
    MARKFUNCTION(0x8005913C);
    while (ccMinNode* n = permMemList.RemHead()) {
        delete n;
    }
}

// PSX: DeletePermMemID__12LevelManagerl (0x800591C8)
void LevelManager::DeletePermMemID(s32 id) {
    MARKFUNCTION(0x800591C8);
    ccMinNode* n = permMemList.head;
    while (n) {
        ccMinNode* next = n->next;
        PermMemEntry* entry = static_cast<PermMemEntry*>(n);
        if (entry->id == id) {
            permMemList.RemNode(n);
            delete n;
        }
        n = next;
    }
}
