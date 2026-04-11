#include "fe/hdmenu.h"
#include "fe/menumgr.h"
#include "xclib/xclib.h"
#include "xclib/xccolour.h"

// PSX: gp+3832 (0x800E0EF8) - normal/deselected item color
// Initialized by func_8005E354 via Set8(112, 73, 0) = raw 0x812E
static xcColour1555 s_menuItemNormalColor = { 0x812E };

// PSX: PostFlight__6hdMenuP7MenuMgr (0x8005CE44)
void hdMenu::PostFlight(MenuMgr* mgr) {
    MARKFUNCTION(0x8005CE44);
    for (ccMinNode* n = itemList.head; n; n = n->next) {
        static_cast<hdMenuItem*>(n)->PostFlight(mgr);
    }
    curItem = static_cast<hdMenuItem*>(itemList.head);
}

// PSX: UpdateScreen__6hdMenuP15oxScreenManager (0x8005CEB8)
void hdMenu::UpdateScreen(oxScreenManager* mgr) {
    MARKFUNCTION(0x8005CEB8);
    Update();
}

// PSX: AddItem__6hdMenuP10hdMenuItem (0x8005D0BC)
void hdMenu::AddItem(hdMenuItem* item) {
    MARKFUNCTION(0x8005D0BC);
    itemList.AddNode(itemList.tail, item);
}

// PSX: SetItem__6hdMenuP10hdMenuItem (0x8005D15C)
void hdMenu::SetItem(hdMenuItem* item) {
    MARKFUNCTION(0x8005D15C);
    if (curItem) {
        curItem->SetColour(s_menuItemNormalColor, true);
    }
    curItem = item;
}

// PSX: FindItem__6hdMenuUl (0x8005CF2C)
hdMenuItem* hdMenu::FindItem(u32 id) {
    MARKFUNCTION(0x8005CF2C);
    for (ccMinNode* n = itemList.head; n; n = n->next) {
        hdMenuItem* item = static_cast<hdMenuItem*>(n);
        if (item->itemID == id) return item;
    }
    return nullptr;
}

// PSX: ClearItem__6hdMenu (0x8005CFD0)
// Note: PSX only deselects visual color, does NOT clear curItem pointer
void hdMenu::ClearItem() {
    MARKFUNCTION(0x8005CFD0);
    if (curItem) {
        curItem->SetColour(s_menuItemNormalColor, false);
    }
}

// PSX: InputPush__6hdMenuP7MenuMgr (0x8005CF68)
void hdMenu::InputPush(MenuMgr* mgr) {
    MARKFUNCTION(0x8005CF68);
    if (curItem) {
        curItem->SelectItem(mgr);
        curItem->SetColour(s_menuItemNormalColor, false);
    }
}

// PSX: InputNextItem__6hdMenu (0x8005D1B0)
void hdMenu::InputNextItem() {
    MARKFUNCTION(0x8005D1B0);
    if (!curItem) return;
    ccMinNode* n = curItem->next;
    for (;;) {
        if (!n) n = itemList.head;
        if (static_cast<hdMenuItem*>(n)->CanBeSelected()) {
            SetItem(static_cast<hdMenuItem*>(n));
            return;
        }
        n = n->next;
    }
}

// PSX: InputPrevItem__6hdMenu (0x8005D238)
void hdMenu::InputPrevItem() {
    MARKFUNCTION(0x8005D238);
    if (!curItem) return;
    ccMinNode* n = curItem->prev;
    for (;;) {
        if (!n) n = itemList.tail;
        if (static_cast<hdMenuItem*>(n)->CanBeSelected()) {
            SetItem(static_cast<hdMenuItem*>(n));
            return;
        }
        n = n->prev;
    }
}

// PSX: Update__6hdMenu (0x8005D0E8)
void hdMenu::Update() {
    MARKFUNCTION(0x8005D0E8);
    MenuColorNext(menuColor);
    if (!curItem) {
        curItem = static_cast<hdMenuItem*>(itemList.head);
    }
    if (!curItem) return;
    curItem->SetColour(menuColor, false);
}

// PSX: SetID__6hdMenuPCc (0x8005D2CC)
void hdMenu::SetID(const char* id) {
    MARKFUNCTION(0x8005D2CC);
    menuID = xcHash(id);
    screenID = menuID;
}

// PSX: SetCallback__6hdMenuUlPFP10hdMenuItem_i (0x8005CEE8)
void hdMenu::SetCallback(u32 id, hdMenuItemCallback cb) {
    MARKFUNCTION(0x8005CEE8);
    hdMenuItem* item = FindItem(id);
    if (item) {
        item->SetCallback(cb);
    }
}

// PSX: DynSetup__6hdMenu (0x8005D010)
void hdMenu::DynSetup() {
    MARKFUNCTION(0x8005D010);
    ccMinNode* n = itemList.head;
    curItem = static_cast<hdMenuItem*>(n);
    hdMenuItem* firstSelectable = nullptr;
    while (n) {
        hdMenuItem* item = static_cast<hdMenuItem*>(n);
        item->SetColour(s_menuItemNormalColor, true);
        n = n->next;
        if (!firstSelectable && item->CanBeSelected()) {
            firstSelectable = item;
        }
    }
    if (firstSelectable) {
        SetItem(firstSelectable);
    }
}

// PSX: SetColour__10hdMenuItemR12xcColour1555b (0x8005DEA0)
void hdMenuItem::SetColour(xcColour1555& col, bool flag) {
    MARKFUNCTION(0x8005DEA0);
    u8* textObj = static_cast<u8*>(data);
    if (!textObj) return;
    textObj[40] = col.GetRed8();
    textObj[41] = col.GetGreen8();
    textObj[42] = col.GetBlue8();
    textObj[43] = col.GetAlpha8();
}
