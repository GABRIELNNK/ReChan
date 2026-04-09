#include "common.h"
#include "gen/levelmgr.h"
#include "gen/charmgr.h"
#include "gen/model.h"

namespace {
    // OriginalBasic fields accessed via proper struct methods now.
    // PSX access patterns use +16 (u16 list type) and +19 (s8 store ID).
    static u16 GetOriginalListType(const OriginalBasic* original) {
        return original->GetType();
    }

    static s8 GetOriginalStoreID(const OriginalBasic* node) {
        return node->GetStoreID();
    }

}

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

    // PSX: DeleteListID(p3d_inventory, INVMAT, 1/2)
    // TODO: list-ID based P3D inventory purge is not implemented in the PC inventory.
    if (g_characterManager) {
        g_characterManager->PurgeLevel();
    }

    DeleteInventoryByID(1);
    DeleteInventoryByID(2);

    // PSX only clears modelLists[0] here.
    while (ccMinNode* n = modelLists[0].RemHead()) {
        delete n;
    }

    // PSX: EnvironmentManager::Reset()
    DeletePermMemID(1);
    DeletePermMemID(2);

    // PSX: AnimationManager::PurgeLevel()

    LOG("[LevelManager] PurgeLevel");
}

// PSX: PurgePetal__12LevelManager (0x80058DB4)
void LevelManager::PurgePetal() {
    MARKFUNCTION(0x80058DB4);

    // PSX: DeleteListID(p3d_inventory, INVMAT, 2)
    // TODO: list-ID based P3D inventory purge is not implemented in the PC inventory.
    if (g_characterManager) {
        g_characterManager->PurgeLevel();
    }

    DeleteInventoryByID(2);

    // PSX: PurgePetal clears modelLists[0].
    while (ccMinNode* n = modelLists[0].RemHead()) {
        delete n;
    }

    // PSX: EnvironmentManager::Reset()
    // PSX: AnimationManager::PurgePetal()
    // PSX: Obstacle::ClearPetalAnimList()

    LOG("[LevelManager] PurgePetal");
}

// PSX: LoadPetal__12LevelManager (0x80058E68)
void LevelManager::LoadPetal() {
    MARKFUNCTION(0x80058E68);
}

// PSX: DeleteOriginalModelsByID__12LevelManagerl (0x80058E70)
void LevelManager::DeleteOriginalModelsByID(s32 id) {
    MARKFUNCTION(0x80058E70);

    const s8 matchID = static_cast<s8>(id);
    for (s32 i = 0; i < 4; i++) {
        ccMinNode* n = modelLists[i].head;
        while (n) {
            ccMinNode* next = n->next;
            OriginalBasic* ob = static_cast<OriginalBasic*>(static_cast<ccNode*>(n));
            if (GetOriginalStoreID(ob) == matchID) {
                modelLists[i].RemNode(n);
                delete n;
            }
            n = next;
        }
    }
}

// PSX: DeleteInventoryByID__12LevelManagerl (0x80058F30)
void LevelManager::DeleteInventoryByID(s32 id) {
    MARKFUNCTION(0x80058F30);

    // PSX: DeleteAllListsID(p3d_inventory, id)
    // TODO: list-ID based P3D inventory purge is not implemented in the PC inventory.
    DeleteOriginalModelsByID(id);
    DeletePermMemID(id);
}

// PSX: AddOriginal__12LevelManagerP13OriginalBasicl (0x80058F84)
void LevelManager::AddOriginal(OriginalBasic* original, s32 /*param*/) {
    MARKFUNCTION(0x80058F84);

    if (!original) {
        return;
    }

    ccMinList* list = nullptr;
    switch (GetOriginalListType(original)) {
        case 0:
            list = &modelLists[2];
            break;
        case 1:
            list = &streeList;
            break;
        case 2:
            list = &modelLists[3];
            break;
        default:
            return;
    }

    list->AddNode(list->tail, reinterpret_cast<ccMinNode*>(original));
}

// PSX: DeleteOriginal__12LevelManagerP13OriginalBasic (0x80058FF0)
void LevelManager::DeleteOriginal(OriginalBasic* original) {
    MARKFUNCTION(0x80058FF0);

    if (!original) {
        return;
    }

    ccMinList* list = nullptr;
    switch (GetOriginalListType(original)) {
        case 0:
            list = &modelLists[2];
            break;
        case 1:
            list = &streeList;
            break;
        case 2:
            list = &modelLists[3];
            break;
        default:
            return;
    }

    for (ccMinNode* n = list->head; n; n = n->next) {
        if (n == reinterpret_cast<ccMinNode*>(original)) {
            list->RemNode(n);
            delete n;
            return;
        }
    }
}

// PSX: FindModel__12LevelManagerQ212LevelManager13ModelListEnuml (0x80059268)
OriginalBasic* LevelManager::FindModel(ModelListEnum listType, s32 id) {
    MARKFUNCTION(0x80059268);
    s32 idx = static_cast<s32>(listType);
    if (idx < 0 || idx > 3) return nullptr;
    // PSX: FindNodeCRC on the list - search by nameCRC
    u32 crc = static_cast<u32>(id);
    for (ccMinNode* n = modelLists[idx].head; n; n = n->next) {
        OriginalBasic* ob = static_cast<OriginalBasic*>(static_cast<ccNode*>(n));
        if (ob->nameCRC == crc) return ob;
    }
    return nullptr;
}

// PSX: FindModel__12LevelManagerl (0x800592A0)
// PSX searches 4 lists starting at offset +52: modelLists[2], modelLists[3], streeList, etreeList
OriginalBasic* LevelManager::FindModel(s32 id) {
    MARKFUNCTION(0x800592A0);
    u32 crc = static_cast<u32>(id);

    // PSX: iterates from a1+52 with step 12, 4 times
    // That's modelLists[2], modelLists[3], streeList, etreeList
    ccMinList* lists[] = { &modelLists[2], &modelLists[3], &streeList, &etreeList };
    for (s32 i = 0; i < 4; i++) {
        for (ccMinNode* n = lists[i]->head; n; n = n->next) {
            OriginalBasic* ob = static_cast<OriginalBasic*>(static_cast<ccNode*>(n));
            if (ob->nameCRC == crc) return ob;
        }
    }
    return nullptr;
}

// PSX: FindSTree__12LevelManagerl (0x80059338)
OriginalBasic* LevelManager::FindSTree(s32 id) {
    MARKFUNCTION(0x80059338);
    u32 crc = static_cast<u32>(id);
    for (ccMinNode* n = streeList.head; n; n = n->next) {
        OriginalBasic* ob = static_cast<OriginalBasic*>(static_cast<ccNode*>(n));
        if (ob->nameCRC == crc) return ob;
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
