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

    oxScreenManager() { MARKFUNCTION(0x80040420); }

    virtual ~oxScreenManager();
    void Init(const char* name, oxScreenManager* parentMgr);
    void Update();
    void Render();

    virtual void SelfInit() { MARKFUNCTION(0x80040968); }
    virtual void SelfUpdate() { MARKFUNCTION(0x80040910); }
    virtual uintptr_t FindScreen(u32 id);
    virtual const char** GetScreenNames() { MARKFUNCTION(0x80040634); return nullptr; }

    void GotoScreen(u32 id);

    virtual void GotoStartScreen();

    void PushScreen(u32 id);
    void PopScreen();
    void ScreenOperation();
    virtual u32 GetScreenHash(u32 id);

    xcSection* GetSection();

    xcOverlayData* FindOverlay(const char* name);
    xcOverlayData* FindOverlay(u32 hash);
};
