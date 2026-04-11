#include "fe/oxscrmgr.h"
#include "fe/oxscreen.h"
#include "xclib/xclib.h"
#include "p3d/view.h"
#include "xclib/xcfile.h"

// PSX: view0 global (used for EnterLayer/ExitLayer in Render)
extern tView view0;

// PSX: _._15oxScreenManager (OXSCRMGR.CPP, 0x80040458)
oxScreenManager::~oxScreenManager() {
    MARKFUNCTION(0x80040458);
    if (inited == 0 && sectionMan) {
        // We own the sectionMan; its destructor frees the section
        delete sectionMan;
    }
    else if (inited == 1) {
        // Shared sectionMan: we own the section we created, but not the sectionMan
        delete section;
    }
    sectionMan = nullptr;
    section = nullptr;
}

// PSX: Init__15oxScreenManagerPcP15oxScreenManager (OXSCRMGR.CPP:201, 0x800407B4)
void oxScreenManager::Init(const char* name, oxScreenManager* parentMgr) {
    MARKFUNCTION(0x800407B4);

    if (parentMgr) {
        // PSX: share parent's sectionMan, load file into new xcSection
        inited = 1;
        sectionMan = parentMgr->sectionMan;
        section = new xcSection();

        u8* data = nullptr;
        u32 size = 0;
        if (xcReadFileLow(name, &data, &size)) {

            // PSX: temporarily swap sectionMan->section so FixInventories
            // can reference the current section during font loading
            xcSection* savedSection = sectionMan->section;
            sectionMan->section = section;

            section->Init(data, size, sectionMan);

            // PSX: restore original section
            sectionMan->section = savedSection;
        }
    }
    else {
        // PSX: create new xcSectionMan, LoadSection(name, flags=1)
        inited = 0;
        sectionMan = new xcSectionMan();
        sectionMan->LoadSection(name);
        section = sectionMan->section;
    }

    // PSX: screenNames = GetScreenNames() (virtual)
    screenNames = GetScreenNames();

    // PSX: GotoScreen on first screen in screens inventory
    if (section && section->screens && section->screens->itemCount > 0) {
        const xcInventoryItem* items = section->screens->GetItems();
        xcScreenData* firstScreen = reinterpret_cast<xcScreenData*>(
            section->rawData + items[0].dataOffset);
        section->GotoScreen(firstScreen);
    }

    // PSX: SelfInit() (virtual)
    SelfInit();
}

// PSX: Update__15oxScreenManager (OXSCRMGR.CPP:85, 0x80040514)
void oxScreenManager::Update() {
    MARKFUNCTION(0x80040514);
    // PSX: if screenOp pending, process it
    if (screenOp)
        ScreenOperation();

    // PSX: if stack has entries, call current screen's UpdateScreen
    s32 depth = screenStackDepth;
    if (depth > 0) {
        oxScreen* scr = reinterpret_cast<oxScreen*>(screenStack[depth - 1]);
        if (scr) {
            scr->UpdateScreen(this);
        }
    }

    // PSX: virtual SelfUpdate()
    SelfUpdate();
}

// PSX: Render__15oxScreenManager (OXSCRMGR.CPP:98, 0x8004059C)
void oxScreenManager::Render() {
    MARKFUNCTION(0x8004059C);
    // PSX: EnterLayer(&view0, 6); Draw__9xcSection(this->section); ExitLayer(&view0, 6);
    if (section) {
        section->Draw();
    }
}

// PSX: FindScreen__15oxScreenManagerUl (OXSCRMGR.CPP:105, 0x800405F0)
uintptr_t oxScreenManager::FindScreen(u32 id) {
    MARKFUNCTION(0x800405F0);
    return 0;
}

// PSX: GotoScreen__15oxScreenManagerUl (OXSCRMGR.CPP:140, 0x8004063C)
void oxScreenManager::GotoScreen(u32 id) {
    MARKFUNCTION(0x8004063C);
    // PSX: sets pending screen operation to GOTO
    screenOp = 1;
    screenOpArg = (s32)id;
}

// PSX: GotoStartScreen__15oxScreenManager (OXSCRMGR.CPP:248, 0x80040948)
void oxScreenManager::GotoStartScreen() {
    MARKFUNCTION(0x80040948);
    GotoScreen(0);
}

// PSX: PushScreen__15oxScreenManagerUl (OXSCRMGR.CPP:262, 0x80040970)
void oxScreenManager::PushScreen(u32 id) {
    MARKFUNCTION(0x80040970);
    screenOp = 2;
    screenOpArg = (s32)id;
}

// PSX: PopScreen__15oxScreenManager (OXSCRMGR.CPP:271, 0x80040980)
void oxScreenManager::PopScreen() {
    MARKFUNCTION(0x80040980);
    screenOp = 3;
}

// PSX: ScreenOperation__15oxScreenManager (OXSCRMGR.CPP:148, 0x8004064C)
void oxScreenManager::ScreenOperation() {
    MARKFUNCTION(0x8004064C);

    // PSX: handle goto (clear stack) and pop (decrement stack)
    if (screenOp == 1) {
        if (screenStackDepth > 0)
            screenStackDepth = 0;
    }
    else if (screenOp == 3) {
        if (screenStackDepth > 0) {
            screenStackDepth--;
        }
    }

    if (screenOp == 1 || screenOp == 2) {
        // GOTO or PUSH: find screen by hash from screenNames table
        if (section && section->screens) {
            u32 hash = GetScreenHash((u32)screenOpArg);
            const xcInventoryItem* item = section->screens->FindItem(hash);
            if (item) {
                xcScreenData* scr = reinterpret_cast<xcScreenData*>(
                    section->rawData + item->dataOffset);
                section->GotoScreen(scr);
            }
            // PSX: push FindScreen result onto stack (for BOTH goto and push)
            if (screenStackDepth < 4) {
                screenStack[screenStackDepth++] = FindScreen((u32)screenOpArg);
            }
        }
    }
    else {
        // POP: go to screen referenced by previous stack entry.
        // PSX reads id/hash from the stacked screen/menu object and resolves that screen.
        if (section && section->screens && screenStackDepth > 0) {
            uintptr_t stackEntry = screenStack[screenStackDepth - 1];
            if (stackEntry) {
                oxScreen* stacked = reinterpret_cast<oxScreen*>(stackEntry);
                u32 hash = GetScreenHash(stacked->screenID);
                const xcInventoryItem* item = section->screens->FindItem(hash);
                if (item) {
                    xcScreenData* scr = reinterpret_cast<xcScreenData*>(
                        section->rawData + item->dataOffset);
                    section->GotoScreen(scr);
                }
            }
        }
    }

    // PSX: clear pending operation
    screenOp = 0;
    screenOpArg = 255;
}

// PSX: GetScreenHash__15oxScreenManagerUl (OXSCRMGR.CPP:243, 0x80040918)
u32 oxScreenManager::GetScreenHash(u32 id) {
    MARKFUNCTION(0x80040918);
    // PSX: xcHash(screenNames[id])
    if (screenNames && screenNames[id])
        return xcHash(screenNames[id]);
    return id;
}

// PSX: GetSection__15oxScreenManager (OXSCRMGR.CPP:279, 0x8004098C)
xcSection* oxScreenManager::GetSection() {
    MARKFUNCTION(0x8004098C);
    return section;
}

// PSX: FindOverlay__15oxScreenManagerPc (OXSCRMGR.CPP:111, 0x800405F8)
xcOverlayData* oxScreenManager::FindOverlay(const char* name) {
    MARKFUNCTION(0x800405F8);
    if (!section) return nullptr;
    u32 hash = xcHash(name);
    return section->FindOverlay(hash);
}

// PSX: FindOverlay__15oxScreenManagerUl (OXSCRMGR.CPP:192, 0x80040784)
xcOverlayData* oxScreenManager::FindOverlay(u32 hash) {
    MARKFUNCTION(0x80040784);
    if (!section) return nullptr;
    return section->FindOverlay(hash);
}
