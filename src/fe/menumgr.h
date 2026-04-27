#pragma once
#include "fe/oxscrmgr.h"
#include "fe/hdmenu.h"

class InputManager;
class LineFile;

// MenuMgr (80 bytes on PSX) - inherits oxScreenManager (48)
// PSX vtable: 0x800CD864
// PSX layout:
//   +0:  oxScreenManager base (48 bytes)
//   +48: menuList (ccMinList, 12 bytes) - master list of all hdMenu objects
//   +60: topMenu (hdMenu*)     - top-level/default menu
//   +64: state (s32)           - 1=running, 2=push, 4=game_change, 8=back
//   +68: active (s16)          - 1 when menu system is active
//   +70: soundFlag (s16)       - 1 when menu sounds should play
//   +72: savedControl (s32)    - saves control state during activate
//   +76: curMenu (hdMenu*)     - currently active menu
class MenuMgr : public oxScreenManager {
public:
    ccMinList menuList;             // +48: master list of all hdMenu objects
    hdMenu* topMenu = nullptr;     // +60: top-level/default menu
    s32 state = 1;                 // +64: menu state
    s16 active = 0;                // +68: active flag
    s16 soundFlag = 1;             // +70: play menu sounds
    s32 savedControl = 0;          // +72: saved MEMORY[0x1C]
    hdMenu* curMenu = nullptr;     // +76: currently active menu

    MenuMgr();
    ~MenuMgr() override;

    u32 GetScreenHash(u32 id) override { MARKFUNCTION(0x8005FF38); return id; }
    uintptr_t FindScreen(u32 id) override { MARKFUNCTION(0x8005F5A0); return (uintptr_t)curMenu; }

    void ParseDefFile(const char* filename);
    void ParseMenu();

    // Reads menu items from the current LineFile position into the given hdMenu.
    void ParseMenu(LineFile& lf, hdMenu* menu);

    virtual void InputPadUp();
    virtual void InputPadDown();
    virtual void InputPadRight();
    virtual void InputPadLeft();
    virtual void InputItemPush();
    virtual void InputItemPop();
    virtual void PushMenu(hdMenu* menu);
    virtual s32 Invoke();
    virtual void Activate();
    virtual void Deactivate();
    virtual void QueryInput(bool processInput);
    virtual void PopMenu();
    virtual void HandleInputChange();

    u32 FilterHostMenuButtons(u32 buttons) const;

    void PostFlightDef();

    void SetTopMenu(u32 hash);

    hdMenu* FindMenu(u32 id);
};
