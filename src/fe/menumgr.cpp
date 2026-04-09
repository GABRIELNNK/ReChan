// menumgr.cpp - MenuMgr reversed from PSX MENUMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\MENUMGR.CPP
#include "fe/menumgr.h"
#include "fe/hdmenuitems.h"
#include "gen/linefile.h"
#include "gen/control.h"
#include "pc/inputaction.h"
#include "xclib/xclib.h"

// PSX: __7MenuMgr (0x8005F4A4)
MenuMgr::MenuMgr() {
    MARKFUNCTION(0x8005F4A4);
    topMenu = nullptr;
    state = 1;
    active = 0;
    soundFlag = 1;
    savedControl = 0;
    curMenu = nullptr;
}

// PSX: _._7MenuMgr (0x8005F50C)
MenuMgr::~MenuMgr() {
    MARKFUNCTION(0x8005F50C);
    menuList.Purge();
}

// Opens a LineFile at the given address, parses menu/dynmenu/memcard/dynitem
// definitions, creates hdMenu/hdDynMenu/hdMemCardMenu/hdDynItemMenu objects,
// and calls ParseMenu for each to load items.
void MenuMgr::ParseDefFile(const char* filename) {
    MARKFUNCTION(0x8005F8E0);
    if (!filename) return;

    LineFile lf;
    lf.Open(filename);

    u8* rawData = section ? section->rawData : nullptr;

    while (lf.Next()) {
        const char* keyword = lf.Word(0);

        if (strcmp(keyword, "MENU") == 0) {
            // PSX: new hdMenu (36 bytes)
            hdMenu* menu = new hdMenu();
            ParseMenu(lf, menu);
        }
        else if (strcmp(keyword, "DYNMENU") == 0) {
            // PSX: new hdDynMenu (60 bytes)
            // Word(2) = overlay name, Word(3) = maxItems count
            const char* overlayName = lf.Word(2);
            xcOverlayData* ovl = FindOverlay(overlayName);
            s32 maxCount = atoi(lf.Word(3));
            hdDynMenu* menu = new hdDynMenu(this, ovl, maxCount, rawData);
            ParseMenu(lf, menu);
        }
        else if (strcmp(keyword, "MEMCARD") == 0) {
            // PSX: new hdMemCardMenu (80 bytes)
            // Word(2) = overlay1 name, Word(3) = overlay2 name
            const char* ovlName1 = lf.Word(2);
            const char* ovlName2 = lf.Word(3);
            xcOverlayData* ovl1 = FindOverlay(ovlName1);
            xcOverlayData* ovl2 = FindOverlay(ovlName2);
            hdMemCardMenu* menu = new hdMemCardMenu(this, ovl1, ovl2, rawData);
            ParseMenu(lf, menu);
        }
        else if (strcmp(keyword, "DYNITEMMENU") == 0) {
            // PSX: new hdDynItemMenu (44 bytes)
            // Word(3) = visible count
            s32 count = atoi(lf.Word(3));
            hdDynItemMenu* menu = new hdDynItemMenu(count);
            ParseMenu(lf, menu);
        }
        else if (strcmp(keyword, "ENDTEXT") == 0) {
            break;
        }
    }
}

// PSX: ParseMenu__7MenuMgr
void MenuMgr::ParseMenu() {
}

// PSX: ParseMenu__7MenuMgrR8LineFileP6hdMenu (0x8005FFD0)
// Reads the menu definition line (ID, screen, overlay) then reads item lines
// until "END MENU" is encountered. Creates hdMenuItem subclass for each item.
void MenuMgr::ParseMenu(LineFile& lf, hdMenu* menu) {
    MARKFUNCTION(0x8005FFD0);

    // Current line is the MENU/DYNMENU/MEMCARD/DYNITEMMENU line
    // Word(1) = menu ID name
    const char* menuIdName = lf.Word(1);
    menu->SetID(menuIdName);

    // Add menu to master list
    menuList.AddNode(menuList.tail, menu);

    // Word(2) = screen name - find and goto the screen
    const char* screenName = lf.Word(2);
    if (section && section->screens) {
        u32 screenHash = xcHash(screenName);
        const xcInventoryItem* item = section->screens->FindItem(screenHash);
        if (item) {
            xcScreenData* scr = reinterpret_cast<xcScreenData*>(
                section->rawData + item->dataOffset);
            section->GotoScreen(scr);
        }
    }

    // Word(2) also names the overlay containing item text objects
    xcOverlayData* overlay = FindOverlay(screenName);
    u8* rawData = section ? section->rawData : nullptr;

    // Parse items until END MENU
    while (lf.Next()) {
        const char* type = lf.Word(0);

        if (strcmp(type, "END") == 0) {
            // PSX: check Word(1) == "MENU"
            if (strcmp(lf.Word(1), "MENU") == 0)
                return;
            continue;
        }

        hdMenuItem* newItem = nullptr;

        if (strcmp(type, "GOTO") == 0) {
            // GOTO <textObjName> <targetMenuName>
            const char* textName = lf.Word(1);
            const char* targetName = lf.Word(2);
            u8* textObj = nullptr;
            if (overlay) textObj = overlay->GetTextObj(xcHash(textName), rawData);
            hdItemGoto* item = new hdItemGoto(textObj, targetName);
            item->itemID = xcHash(textName);
            newItem = item;
        }
        else if (strcmp(type, "BUTTON") == 0) {
            // BUTTON <textObjName>
            const char* textName = lf.Word(1);
            u8* textObj = nullptr;
            if (overlay) textObj = overlay->GetTextObj(xcHash(textName), rawData);
            hdItemButton* item = new hdItemButton(textObj, textName);
            newItem = item;
        }
        else if (strcmp(type, "SELECT") == 0) {
            // SELECT <overlayName>
            const char* ovlName = lf.Word(1);
            xcOverlayData* itemOvl = FindOverlay(ovlName);
            hdItemSelection* item = new hdItemSelection(itemOvl, ovlName, rawData);
            newItem = item;
        }
        else if (strcmp(type, "SHKSELECT") == 0) {
            // SHKSELECT <overlayName>
            const char* ovlName = lf.Word(1);
            xcOverlayData* itemOvl = FindOverlay(ovlName);
            hdShockSelection* item = new hdShockSelection(itemOvl, ovlName, rawData);
            newItem = item;
        }
        else if (strcmp(type, "SNDSELECT") == 0) {
            // SNDSELECT <overlayName> <steps> <maxValue>
            const char* ovlName = lf.Word(1);
            s32 steps = atoi(lf.Word(2));
            u32 maxVal = (u32)atoi(lf.Word(3));
            xcOverlayData* itemOvl = FindOverlay(ovlName);
            hdSndItemSelection* item = new hdSndItemSelection(itemOvl, ovlName, rawData, steps, maxVal);
            newItem = item;
        }
        else if (strcmp(type, "CTLSELECT") == 0) {
            // CTLSELECT <overlayName> <descOverlayName>
            const char* ovlName = lf.Word(1);
            const char* descName = lf.Word(2);
            xcOverlayData* itemOvl = FindOverlay(ovlName);
            xcOverlayData* descOvl = FindOverlay(descName);
            hdControllerSelection* item = new hdControllerSelection(itemOvl, ovlName, rawData, descOvl);
            newItem = item;
        }
        else if (strcmp(type, "ALPHASELECT") == 0) {
            // ALPHASELECT <overlayName> <min> <max>
            const char* ovlName = lf.Word(1);
            s32 minVal = atoi(lf.Word(2));
            s32 maxVal = atoi(lf.Word(3));
            xcOverlayData* itemOvl = FindOverlay(ovlName);
            hdAlphaSelection* item = new hdAlphaSelection(itemOvl, ovlName, rawData, minVal, maxVal);
            newItem = item;
        }
        else if (strcmp(type, "DYNSELECT") == 0) {
            // DYNSELECT <overlayName>
            const char* ovlName = lf.Word(1);
            xcOverlayData* itemOvl = overlay; // PSX uses parent menu's overlay
            hdDynItemSelection* item = new hdDynItemSelection(itemOvl, ovlName, rawData);
            newItem = item;
        }
        else if (strcmp(type, "NUMSELECT") == 0) {
            // NUMSELECT <overlayName> <min> <max>
            const char* ovlName = lf.Word(1);
            s32 minVal = atoi(lf.Word(2));
            s32 maxVal = atoi(lf.Word(3));
            xcOverlayData* itemOvl = FindOverlay(ovlName);
            hdNumericSelection* item = new hdNumericSelection(itemOvl, ovlName, rawData, minVal, maxVal);
            newItem = item;
        }

        if (newItem) {
            menu->AddItem(newItem);
        }
    }
}

// PSX: Activate__7MenuMgr (0x8005FBA4)
// Sets active flag, plays menu open sound, sets control mode to menu mode,
// saves current control state, activates topMenu, goes to start screen.
void MenuMgr::Activate() {
    MARKFUNCTION(0x8005FBA4);
    active = 1;
    // PSX: rsEvent(10, 0, 0, 0) — menu open sound
    // PSX: ProcessSoundEvent(0, 10)
    // PSX: if soundFlag, play ambient music (rsEvent(30, ...))
    state = 1;
    // PSX: save MEMORY[0x1C] into savedControl
    if (topMenu) {
        topMenu->DynSetup();
    }
    curMenu = topMenu;
    GotoStartScreen();
    Update();
    Update();
    Render();
}

// PSX: Deactivate__7MenuMgr (0x8005FD30)
// Clears active flag, restores control mode, restores saved state.
void MenuMgr::Deactivate() {
    MARKFUNCTION(0x8005FD30);
    active = 0;
    // PSX: ProcessSoundEvent(0, 11) — menu close sound
    // PSX: rsEvent(11, 0, 0, 0)
    screenStackDepth = 0;
    // PSX: MEMORY[0x1C] = savedControl
}

// PSX: InputPadUp__7MenuMgr (0x8005F660)
void MenuMgr::InputPadUp() {
    MARKFUNCTION(0x8005F660);
    // PSX: ProcessSoundEvent(0, 15) — nav sound
    if (curMenu) {
        curMenu->InputPrevItem();
    }
}

// PSX: InputPadDown__7MenuMgr (0x8005F740)
void MenuMgr::InputPadDown() {
    MARKFUNCTION(0x8005F740);
    // PSX: ProcessSoundEvent(0, 15) — nav sound
    if (curMenu) {
        curMenu->InputNextItem();
    }
}

// PSX: InputPadRight__7MenuMgr (0x8005F6B0)
void MenuMgr::InputPadRight() {
    MARKFUNCTION(0x8005F6B0);
    hdMenuItem* item = curMenu->curItem;
    if (item) {
        item->IncItem();
    }
}

// PSX: InputPadLeft__7MenuMgr (0x8005F6F8)
void MenuMgr::InputPadLeft() {
    MARKFUNCTION(0x8005F6F8);
    hdMenuItem* item = curMenu->curItem;
    if (item) {
        item->DecItem();
    }
}

// PSX: InputItemPush__7MenuMgr (0x8005F790)
void MenuMgr::InputItemPush() {
    MARKFUNCTION(0x8005F790);
    // PSX: if state != 8, play confirm sound
    if (curMenu) {
        curMenu->InputPush(this);
    }
}

// PSX: InputItemPop__7MenuMgr (0x8005F5AC)
void MenuMgr::InputItemPop() {
    MARKFUNCTION(0x8005F5AC);
    if (curMenu) {
        curMenu->ClearItem();
        if (curMenu == topMenu) {
            state = 8;
        } else {
            // PSX: calls curMenu vtable+32 (DeselectItem) then vtable+36 (???)
            // PSX: ProcessSoundEvent(0, 12) — back sound
            Deactivate();
        }
    }
}

// PSX: PushMenu__7MenuMgrP6hdMenu (0x8005F830)
void MenuMgr::PushMenu(hdMenu* menu) {
    MARKFUNCTION(0x8005F830);
    curMenu = menu;
    if (menu) {
        PushScreen(menu->menuID);
        Update();
        Update();
        // PSX: calls menu vtable+28 (DynSetup/Select)
    }
}

// PSX: PopMenu__7MenuMgr (0x8005FF4C)
// Pops back to the previous menu on the screen stack.
// Walks menuList to find the menu stored at screenStack[depth-2],
// calls DynSetup on it, then pops the screen stack entry.
void MenuMgr::PopMenu() {
    MARKFUNCTION(0x8005FF4C);
    // PSX: target = screenStack[screenStackDepth - 2]
    uintptr_t target = screenStack[screenStackDepth - 2];
    curMenu = nullptr;

    // PSX: walk menuList to find menu matching the stored pointer
    for (ccMinNode* n = menuList.head; n; n = n->next) {
        if ((uintptr_t)n == target) {
            curMenu = static_cast<hdMenu*>(n);
            break;
        }
    }

    // PSX: curMenu->DynSetup() (vtable+28)
    curMenu->DynSetup();

    // PSX: PopScreen()
    PopScreen();
}

// PSX: QueryInput__7MenuMgrb (0x8005FDF4)
// Polls input and dispatches to virtual Input* functions.
// processInput: when true, process buttons; false = poll only.
void MenuMgr::QueryInput(bool processInput) {
    MARKFUNCTION(0x8005FDF4);
    if (!g_actionInput) return;
    if (!processInput) return;

    // Back/Start
    if (g_actionInput->JustPressed(ACTION_START)) {
        state = 8;
    }
    // D-pad navigation
    if (g_actionInput->JustPressed(ACTION_MENU_UP)) {
        InputPadUp();
    }
    if (g_actionInput->JustPressed(ACTION_MENU_DOWN)) {
        InputPadDown();
    }
    if (g_actionInput->JustPressed(ACTION_MENU_LEFT)) {
        InputPadLeft();
    }
    if (g_actionInput->JustPressed(ACTION_MENU_RIGHT)) {
        InputPadRight();
    }
    // Cancel/Pop
    if (g_actionInput->JustPressed(ACTION_MENU_BACK)) {
        InputItemPop();
    }
    // Confirm/Push
    if (g_actionInput->JustPressed(ACTION_MENU_CONFIRM)) {
        InputItemPush();
    }
}

// PSX: HandleInputChange__7MenuMgr
void MenuMgr::HandleInputChange() {
}

// PSX: Invoke__7MenuMgr (0x8005FB00)
// Main per-frame entry point for menu system. Called by Game during FE state.
s32 MenuMgr::Invoke() {
    MARKFUNCTION(0x8005FB00);
    if (!active) {
        Activate();
    }
    QueryInput(true);
    Update();
    if (state == 2 || state == 8 || state == 4) {
        Deactivate();
    }
    return state;
}

// PSX: PostFlightDef__7MenuMgr (0x8005F894)
// After parsing def file, calls PostFlight on every menu in the list.
void MenuMgr::PostFlightDef() {
    MARKFUNCTION(0x8005F894);
    for (ccMinNode* n = menuList.head; n; n = n->next) {
        hdMenu* menu = static_cast<hdMenu*>(n);
        menu->PostFlight(this);
    }
}

// PSX: SetTopMenu__7MenuMgrUl (0x8005F7DC)
// Sets top menu and current menu to the menu with the given hash,
// then goes to the corresponding screen.
void MenuMgr::SetTopMenu(u32 hash) {
    MARKFUNCTION(0x8005F7DC);
    hdMenu* menu = FindMenu(hash);
    topMenu = menu;
    curMenu = menu;
    GotoScreen(hash);
    Update();
    Update();
}

// PSX: FindMenu__7MenuMgrUl (0x8005F564)
// Walks menuList looking for a menu with matching hash at +12 (menuID).
hdMenu* MenuMgr::FindMenu(u32 id) {
    MARKFUNCTION(0x8005F564);
    for (ccMinNode* n = menuList.head; n; n = n->next) {
        hdMenu* menu = static_cast<hdMenu*>(n);
        if (menu->menuID == id) return menu;
    }
    return nullptr;
}


