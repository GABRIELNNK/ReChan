// menumgr.h - MenuMgr reversed from PSX MENUMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\MENUMGR.CPP
// MenuMgr manages menus with input handling. Owns a master list of hdMenu
// objects parsed from a .DEF file, and tracks the current/top menu.
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

    // PSX: __7MenuMgr (MENUMGR.CPP, 0x8005F4A4)
    MenuMgr();

    // PSX: _._7MenuMgr (MENUMGR.CPP, 0x8005F50C)
    ~MenuMgr() override;

    // PSX: GetScreenHash__7MenuMgrUl (MENUMGR.CPP:373, 0x8005FF38)
    // Override: MenuMgr uses hash-based navigation, just returns id as-is.
    u32 GetScreenHash(u32 id) override { MARKFUNCTION(0x8005FF38); return id; }

    // PSX: FindScreen__7MenuMgrUl (MENUMGR.CPP:86, 0x8005F5A0)
    // Override: returns curMenu as the current screen
    uintptr_t FindScreen(u32 id) override { MARKFUNCTION(0x8005F5A0); return (uintptr_t)curMenu; }

    // PSX: ParseDefFile__7MenuMgrPc (MENUMGR.CPP, 0x8005F8E0)
    void ParseDefFile(const char* filename);

    // PSX: ParseMenu__7MenuMgr (MENUMGR.CPP)
    void ParseMenu();

    // PSX: ParseMenu__7MenuMgrR8LineFileP6hdMenu (MENUMGR.CPP:416, 0x8005FFD0)
    // Reads menu items from the current LineFile position into the given hdMenu.
    void ParseMenu(LineFile& lf, hdMenu* menu);

    // PSX: Activate__7MenuMgr (MENUMGR.CPP, 0x8005FBA4)
    virtual void Activate();

    // PSX: Deactivate__7MenuMgr (MENUMGR.CPP, 0x8005FD30)
    virtual void Deactivate();

    // PSX: InputPadUp__7MenuMgr (MENUMGR.CPP, 0x8005F660)
    virtual void InputPadUp();

    // PSX: InputPadDown__7MenuMgr (MENUMGR.CPP, 0x8005F740)
    virtual void InputPadDown();

    // PSX: InputPadRight__7MenuMgr (MENUMGR.CPP, 0x8005F6B0)
    virtual void InputPadRight();

    // PSX: InputPadLeft__7MenuMgr (MENUMGR.CPP, 0x8005F6F8)
    virtual void InputPadLeft();

    // PSX: InputItemPush__7MenuMgr (MENUMGR.CPP, 0x8005F790)
    virtual void InputItemPush();

    // PSX: InputItemPop__7MenuMgr (MENUMGR.CPP, 0x8005F5AC)
    virtual void InputItemPop();

    // PSX: PushMenu__7MenuMgrP6hdMenu (MENUMGR.CPP, 0x8005F830)
    virtual void PushMenu(hdMenu* menu);

    // PSX: PopMenu__7MenuMgr (MENUMGR.CPP:386, 0x8005FF4C)
    virtual void PopMenu();

    // PSX: QueryInput__7MenuMgrb (MENUMGR.CPP, 0x8005FDF4)
    virtual void QueryInput(bool processInput);

    // PSX: HandleInputChange__7MenuMgr (MENUMGR.CPP)
    virtual void HandleInputChange();

    // PSX: Invoke__7MenuMgr (MENUMGR.CPP, 0x8005FB00)
    virtual s32 Invoke();

    // PSX: PostFlightDef__7MenuMgr (MENUMGR.CPP, 0x8005F894)
    void PostFlightDef();

    // PSX: SetTopMenu__7MenuMgrUl (MENUMGR.CPP, 0x8005F7DC)
    void SetTopMenu(u32 hash);

    // PSX: FindMenu__7MenuMgrUl (MENUMGR.CPP, 0x8005F564)
    hdMenu* FindMenu(u32 id);
};
