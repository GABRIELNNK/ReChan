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

    hdMenuItem() { MARKFUNCTION(0x8005DE2C); }
    virtual ~hdMenuItem() { MARKFUNCTION(0x8005DE70); }
    virtual void PostFlight(MenuMgr* mgr) { MARKFUNCTION(0x8005DE98); }
    virtual void SetColour(xcColour1555& col, bool flag);
    virtual void SelectItem(MenuMgr* mgr) { MARKFUNCTION(0x8005DF10); }
    virtual void IncItem() { MARKFUNCTION(0x8005DF18); }
    virtual void DecItem() { MARKFUNCTION(0x8005DF20); }
    virtual void SetValue(u32 val) { MARKFUNCTION(0x8005DF28); }
    virtual s32 CanBeSelected() { MARKFUNCTION(0x8005E564); return 1; }
    virtual u32 GetValue() { MARKFUNCTION(0x8005DF38); return 0; }
    virtual void DynSetup() { MARKFUNCTION(0x8005E55C); }

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

    hdMenu() { MARKFUNCTION(0x8005CD94); MenuColorStart(menuColor); }
    ~hdMenu() override { MARKFUNCTION(0x8005CDEC); }

    virtual void PostFlight(MenuMgr* mgr);

    void UpdateScreen(oxScreenManager* mgr) override;
    void AddItem(hdMenuItem* item);
    void SetItem(hdMenuItem* item);
    hdMenuItem* FindItem(u32 id);
    void ClearItem();
    void InputPush(MenuMgr* mgr);

    virtual void InputNextItem();
    virtual void InputPrevItem();

    virtual void Update();

    void SetID(const char* id);
    void SetCallback(u32 id, hdMenuItemCallback cb);

    virtual void DynSetup();
    virtual s32 CanAbortNow() { MARKFUNCTION(0); return 1; }
    virtual void Cleanup() { MARKFUNCTION(0); }
};
