// hdmenu.h - hdMenu and hdMenuItem reversed from PSX HDMENU.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\HDMENU.CPP
#pragma once

#include "fe/oxscreen.h"
#include "gen/cclist.h"
#include "xclib/xccolour.h"

class MenuMgr;
class hdMenuItem;
typedef s32 (*hdMenuItemCallback)(hdMenuItem*);

// hdMenuItem (28 bytes on PSX) - a selectable item in an hdMenu
// PSX vtable at 0x800CD7D0 (MetroWerks: vtable ptr at +8)
// PSX layout: +0 ccMinNode(8), +8 vtable(4), +12 data(ptr),
//             +16 callback(ptr), +20 flags(u32), +24 itemID(u32)
class hdMenuItem : public ccMinNode {
public:
    void* data = nullptr;                // +12: generic data ptr (textObj in subclasses)
    hdMenuItemCallback callback = nullptr; // +16: callback function int(*)(hdMenuItem*)
    u32 itemFlags = 0;                   // +20: flags/color
    u32 itemID = 0;                      // +24: hash identifier

    // PSX: __10hdMenuItem (HDMENU.CPP, 0x8005DE2C)
    hdMenuItem() { MARKFUNCTION(0x8005DE2C); }

    // PSX: _._10hdMenuItem (HDMENU.CPP, 0x8005DE70)
    virtual ~hdMenuItem() { MARKFUNCTION(0x8005DE70); }

    // PSX: PostFlight__10hdMenuItemP7MenuMgr (0x8005DE98)
    virtual void PostFlight(MenuMgr* mgr) { MARKFUNCTION(0x8005DE98); }

    // PSX: SetColour__10hdMenuItemR12xcColour1555b (0x8005DEA0)
    virtual void SetColour(xcColour1555& col, bool flag);

    // PSX: SelectItem__10hdMenuItemP7MenuMgr (0x8005DF10)
    virtual void SelectItem(MenuMgr* mgr) { MARKFUNCTION(0x8005DF10); }

    // PSX: IncItem__10hdMenuItem (0x8005DF18)
    virtual void IncItem() { MARKFUNCTION(0x8005DF18); }

    // PSX: DecItem__10hdMenuItem (0x8005DF20)
    virtual void DecItem() { MARKFUNCTION(0x8005DF20); }

    // PSX: SetValue__10hdMenuItemUl (0x8005DF28) — no-op in base
    virtual void SetValue(u32 val) { MARKFUNCTION(0x8005DF28); }

    // PSX: CanBeSelected__10hdMenuItem (0x8005E564) — returns 1
    virtual s32 CanBeSelected() { MARKFUNCTION(0x8005E564); return 1; }

    // PSX: GetValue__10hdMenuItem (0x8005DF38) — returns 0 in base
    virtual u32 GetValue() { MARKFUNCTION(0x8005DF38); return 0; }

    // PSX: DynSetup__10hdMenuItem (0x8005E55C) — no-op in base
    virtual void DynSetup() { MARKFUNCTION(0x8005E55C); }

    // PSX: SetCallback__10hdMenuItemPFP10hdMenuItem_i (0x8005DF30)
    // Non-virtual on PSX (not in vtable)
    void SetCallback(hdMenuItemCallback cb) { MARKFUNCTION(0x8005DF30); callback = cb; }
};

// hdMenu (36 bytes on PSX) - a menu screen containing hdMenuItems
// Inherits oxScreen (16 bytes)
// PSX layout: +0 oxScreen(12), +8 vtable(4), +12 menuID(4),
//             +16 curItem(4), +20 menuColor(2), +24 itemList(ccMinList, 12) = 36
class hdMenu : public oxScreen {
public:
    ccMinList itemList;             // PSX +24: list of hdMenuItems
    hdMenuItem* curItem = nullptr;  // PSX +16: currently selected item
    xcColour1555 menuColor;         // PSX +20: cycling highlight color
    u32 menuID = 0;                 // PSX +12

    // PSX: __6hdMenu (HDMENU.CPP, 0x8005CD94)
    hdMenu() { MARKFUNCTION(0x8005CD94); MenuColorStart(menuColor); }

    // PSX: _._6hdMenu (HDMENU.CPP, 0x8005CDEC)
    ~hdMenu() override { MARKFUNCTION(0x8005CDEC); }

    // PSX: PostFlight__6hdMenuP7MenuMgr (0x8005CE44)
    virtual void PostFlight(MenuMgr* mgr);

    // PSX: UpdateScreen__6hdMenuP15oxScreenManager (0x8005CEB8)
    void UpdateScreen(oxScreenManager* mgr) override;

    // PSX: AddItem__6hdMenuP10hdMenuItem (0x8005D0BC)
    void AddItem(hdMenuItem* item);

    // PSX: SetItem__6hdMenuP10hdMenuItem (0x8005D1B0)
    void SetItem(hdMenuItem* item);

    // PSX: FindItem__6hdMenuUl (0x8005CF2C)
    hdMenuItem* FindItem(u32 id);

    // PSX: ClearItem__6hdMenu (0x8005CFD0)
    void ClearItem();

    // PSX: InputPush__6hdMenuP7MenuMgr (0x8005CF68)
    void InputPush(MenuMgr* mgr);

    // PSX: InputNextItem__6hdMenu (0x8005D1B0)
    virtual void InputNextItem();

    // PSX: InputPrevItem__6hdMenu (0x8005D238)
    virtual void InputPrevItem();

    // PSX: Update__6hdMenu (0x8005D0E8)
    void Update();

    // PSX: SetID__6hdMenuPCc (0x8005D2CC)
    void SetID(const char* id);

    // PSX: SetCallback__6hdMenuUlPFP10hdMenuItem_i (0x8005CEE8)
    void SetCallback(u32 id, hdMenuItemCallback cb);

    // PSX: DynSetup__6hdMenu (0x8005D010)
    virtual void DynSetup();
};
