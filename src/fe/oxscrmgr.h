// oxscrmgr.h - oxScreenManager reversed from PSX OXSCRMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\OXSCRMGR.CPP
// Screen stack manager - pushes/pops screens, handles Init/Update/Render.
#pragma once

#include "core.h"

struct xcSection;
struct xcSectionMan;
struct xcScreenData;
struct xcOverlayData;

// oxScreenManager (48 bytes on PSX)
// PSX layout (MetroWerks: vtable at end, +44):
//   +0:  screenOp (s32)     - pending operation: 0=none, 1=goto, 2=push, 3=pop
//   +4:  screenOpArg (s32)  - screen index for pending goto/push
//   +8:  screenStack[4] (u32 x4) - stack of screen data pointers
//   +24: screenStackDepth (s32)
//   +28: sectionMan (xcSectionMan*)
//   +32: section (xcSection*)
//   +36: screenNames (char**) - returned by virtual GetScreenNames
//   +40: inited (s32)
//   +44: vtable (MetroWerks)
class oxScreenManager {
public:
    s32 screenOp = 0;                   // +0: pending screen operation
    s32 screenOpArg = 0;                // +4: arg for pending operation
    uintptr_t screenStack[4] = {};      // +8: screen stack entries (PSX: u32[4])
    s32 screenStackDepth = 0;           // +24: current stack depth
    xcSectionMan* sectionMan = nullptr; // +28: section manager (owns the section)
    xcSection* section = nullptr;       // +32: active xcSection
    const char** screenNames = nullptr; // +36: screen name table from GetScreenNames
    s32 inited = 0;                     // +40: 0=owns sectionMan, 1=shared from parent

    // PSX: __15oxScreenManager (OXSCRMGR.CPP, 0x80040420)
    oxScreenManager() { MARKFUNCTION(0x80040420); }

    // PSX: _._15oxScreenManager (OXSCRMGR.CPP, 0x80040458)
    virtual ~oxScreenManager();

    // PSX: Init__15oxScreenManagerPcP15oxScreenManager (OXSCRMGR.CPP:201, 0x800407B4)
    // Non-virtual on PSX (not in vtable 0x800CC744)
    void Init(const char* name, oxScreenManager* parentMgr);

    // PSX: Update__15oxScreenManager (OXSCRMGR.CPP:85, 0x80040514)
    // Non-virtual on PSX
    void Update();

    // PSX: Render__15oxScreenManager (OXSCRMGR.CPP:98, 0x8004059C)
    // Non-virtual on PSX
    void Render();

    // PSX: SelfInit__15oxScreenManager (OXSCRMGR.CPP:255, 0x80040968)
    virtual void SelfInit() { MARKFUNCTION(0x80040968); }

    // PSX: SelfUpdate__15oxScreenManager (OXSCRMGR.CPP:238, 0x80040910)
    virtual void SelfUpdate() { MARKFUNCTION(0x80040910); }

    // PSX: FindScreen__15oxScreenManagerUl (OXSCRMGR.CPP:105, 0x800405F0)
    virtual uintptr_t FindScreen(u32 id);

    // PSX: GetScreenNames__15oxScreenManager (OXSCRMGR.CPP:132, 0x80040634)
    virtual const char** GetScreenNames() { MARKFUNCTION(0x80040634); return nullptr; }

    // PSX: GotoScreen__15oxScreenManagerUl (OXSCRMGR.CPP:140, 0x8004063C)
    // Non-virtual on PSX
    void GotoScreen(u32 id);

    // PSX: GotoStartScreen__15oxScreenManager (OXSCRMGR.CPP:248, 0x80040948)
    // Virtual: vtable[4] = 0x80040948
    virtual void GotoStartScreen();

    // PSX: PushScreen__15oxScreenManagerUl (OXSCRMGR.CPP:262, 0x80040970)
    // Non-virtual on PSX
    void PushScreen(u32 id);

    // PSX: PopScreen__15oxScreenManager (OXSCRMGR.CPP:271, 0x80040980)
    // Non-virtual on PSX
    void PopScreen();

    // PSX: ScreenOperation__15oxScreenManager (OXSCRMGR.CPP:148, 0x8004064C)
    void ScreenOperation();

    // PSX: GetScreenHash__15oxScreenManagerUl (OXSCRMGR.CPP:243, 0x80040918)
    // Virtual: vtable[3] = 0x80040918. MenuMgr overrides to pass-through.
    virtual u32 GetScreenHash(u32 id);

    // PSX: GetSection__15oxScreenManager (OXSCRMGR.CPP:279, 0x8004098C)
    xcSection* GetSection();

    // PSX: FindOverlay__15oxScreenManagerPc (OXSCRMGR.CPP:111, 0x800405F8)
    xcOverlayData* FindOverlay(const char* name);

    // PSX: FindOverlay__15oxScreenManagerUl (OXSCRMGR.CPP:192, 0x80040784)
    xcOverlayData* FindOverlay(u32 hash);
};
