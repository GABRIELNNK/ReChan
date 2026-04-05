// oxscrmgr.cpp - oxScreenManager reversed from PSX OXSCRMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\OXSCRMGR.CPP
#include "fe/oxscrmgr.h"
#include "xclib/xclib.h"
#include "p3d/view.h"

// PSX: view0 global (used for EnterLayer/ExitLayer in Render)
extern tView view0;

// PSX: _._15oxScreenManager (OXSCRMGR.CPP, 0x80040458)
oxScreenManager::~oxScreenManager() {
    MARKFUNCTION(0x80040458);
    if (inited == 0 && sectionMan) {
        // We own the sectionMan; its destructor frees the section
        delete sectionMan;
    } else if (inited == 1) {
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

        // PSX: xcReadFileLow(name, &data, &size)
        FILE* f = FileOpen(name, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            u32 size = (u32)ftell(f);
            fseek(f, 0, SEEK_SET);
            u8* data = new u8[size];
            fread(data, 1, size, f);
            fclose(f);

            // PSX: temporarily swap sectionMan->section so FixInventories
            // can reference the current section during font loading
            xcSection* savedSection = sectionMan->section;
            sectionMan->section = section;

            section->Init(data, size, sectionMan);

            // PSX: restore original section
            sectionMan->section = savedSection;
        }
    } else {
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
    // For PC: the screen stack is not fully implemented yet (no oxScreen objects)
    // The base class Update calls SelfUpdate

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
    } else if (screenOp == 3) {
        screenStackDepth--;
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
            // PSX: push FindScreen result onto stack
            if (screenOp == 2 && screenStackDepth < 4) {
                screenStack[screenStackDepth++] = FindScreen((u32)screenOpArg);
            }
        }
    } else {
        // POP: go to screen referenced by stack entry
        // PSX: FindScreen__9xcSectionUl(section, *(stack_entry + 12))
        // Simplified for PC: go to first screen as fallback
        if (section && section->screens && section->screens->itemCount > 0) {
            const xcInventoryItem* items = section->screens->GetItems();
            xcScreenData* scr = reinterpret_cast<xcScreenData*>(
                section->rawData + items[0].dataOffset);
            section->GotoScreen(scr);
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
