// femenumgr.cpp - feMenuMgr reversed from PSX FEMNUMGR.CPP (Overlay 4)
// PSX source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP
#include "fe/femenumgr.h"
#include "fe/hdmenu.h"
#include "xclib/xclib.h"
#include "gen/game.h"
#include "gen/scoremgr.h"
#include "gen/control.h"

// Global feMenuMgr pointer
feMenuMgr* g_feMenuMgr = nullptr;

// Screen hashes (from decompiled constructor and SelfInit)
static constexpr u32 HASH_MAIN_SCREEN    = 103520738;   // startScreenHashes[0]
static constexpr u32 HASH_LEVEL_SCREEN   = 93891576;    // startScreenHashes[1]
static constexpr u32 HASH_SOUND_MENU     = 102551593;   // Sound menu
static constexpr u32 HASH_CONTROLLER_MENU = 141018276;  // Controller menu
static constexpr u32 HASH_NEWGAME_MENU   = (u32)(-1289713843);  // New game confirm
static constexpr u32 HASH_OPTIONS_MENU   = (u32)(-1066161287);  // Options menu
static constexpr u32 HASH_MEMCARD_MENU   = 448275865;   // Memory card menu

// Menu item callback hashes
static constexpr u32 HASH_ITEM_RESUME    = (u32)(-1778775893);
static constexpr u32 HASH_ITEM_LOAD      = 1837767642;
static constexpr u32 HASH_ITEM_SAVE      = (u32)(-558495447);
static constexpr u32 HASH_ITEM_CREDITS   = (u32)(-372502450);
static constexpr u32 HASH_ITEM_SHOCK     = 1143775739;
static constexpr u32 HASH_ITEM_NEWGAME   = 100369;
static constexpr u32 HASH_ITEM_NEWGAME_RESUME = 2685;
static constexpr u32 HASH_ITEM_CTRL_SEL  = (u32)(-1528003751);

// --- Static menu callbacks (called by hdMenu item selection) ---

// PSX: ResumeGame__9feMenuMgrP10hdMenuItem (Overlay4 0x80010968)
static s32 ResumeGame(hdMenuItem* item) {
    (void)item;
    return 8;  // state=8 = exit menu
}

// PSX: NewGame__9feMenuMgrP10hdMenuItem (Overlay4 0x80010990)
static s32 NewGameCallback(hdMenuItem* item) {
    (void)item;
    // PSX: HandleGameBegin__12ScoreManager(0)
    if (g_scoreManager) {
        g_scoreManager->HandleGameBegin();
    }
    // PSX: UnloadLevel__5World(0)
    // PSX: UnloadLevelPart2__5World(0)
    // PSX: UnloadPermanent__5World()
    // PSX: MEMORY[0x1F4] = 0
    if (g_game) {
        g_game->SetState(GameState::Init);  // state 4
    }
    return 4;  // state=4 = game state change
}

// PSX: LoadGame__9feMenuMgrP10hdMenuItem (Overlay4 0x80010A04)
static s32 LoadGameCallback(hdMenuItem* item) {
    (void)item;
    if (g_feMenuMgr) {
        g_feMenuMgr->PushLoadSaveMenu(1);  // load mode
    }
    return 1;
}

// PSX: SaveGame__9feMenuMgrP10hdMenuItem (Overlay4 0x80010A2C)
static s32 SaveGameCallback(hdMenuItem* item) {
    (void)item;
    if (g_feMenuMgr) {
        g_feMenuMgr->PushLoadSaveMenu(0);  // save mode
    }
    return 1;
}

// PSX: ShowCredits__9feMenuMgrP10hdMenuItem (Overlay4 0x80010A54)
static s32 ShowCredits(hdMenuItem* item) {
    (void)item;
    if (g_game) {
        g_game->SetState(GameState::PlayMovieCredits);  // state 12
    }
    return 4;
}

// PSX: SetControllerShock__FP10hdMenuItem (0x800378D0)
// Free function - sets DualShock vibration on/off based on item value.
static s32 SetControllerShock(hdMenuItem* item) {
    (void)item;
    // PSX: gets item value, calls SetShock, triggers vibration test
    // TODO: implement DualShock control
    return 8;
}

// --- feMenuMgr implementation ---

// PSX: __9feMenuMgr (Overlay4 0x80010EB4)
feMenuMgr::feMenuMgr() {
    MARKFUNCTION(0x80010EB4);
    startScreenHashes[0] = HASH_MAIN_SCREEN;   // 103520738
    startScreenHashes[1] = HASH_LEVEL_SCREEN;   // 93891576
    feMode = 0;
    g_feMenuMgr = this;
}

// PSX: _._9feMenuMgr (Overlay4 0x80010F04)
feMenuMgr::~feMenuMgr() {
    MARKFUNCTION(0x80010F04);
    if (g_feMenuMgr == this) g_feMenuMgr = nullptr;
}

// PSX: SelfInit__9feMenuMgr (Overlay4 0x8001103C)
// Parses menu def file, sets up callbacks for all menu items.
void feMenuMgr::SelfInit() {
    MARKFUNCTION(0x8001103C);

    // PSX: ParseDefFile with address 0x80010000 (def file in ROM overlay)
    // PC: load from FE_MNU.TXT in the XC directory
    ParseDefFile("XC/FE_MNU.TXT");

    // PSX: PostFlightDef walks all menus calling PostFlight
    PostFlightDef();

    // Set initial top menu based on feMode
    SetTopMenu(startScreenHashes[feMode]);

    // Set callbacks on main menu: Resume, Load, Save
    hdMenu* mainMenu = FindMenu(HASH_MAIN_SCREEN);
    if (mainMenu) {
        mainMenu->SetCallback(HASH_ITEM_RESUME, (hdMenuItemCallback)ResumeGame);
        mainMenu->SetCallback(HASH_ITEM_LOAD, (hdMenuItemCallback)LoadGameCallback);
        mainMenu->SetCallback(HASH_ITEM_SAVE, (hdMenuItemCallback)SaveGameCallback);
    }

    // Set callback on options menu: Credits
    hdMenu* optionsMenu = FindMenu(HASH_OPTIONS_MENU);
    if (optionsMenu) {
        optionsMenu->SetCallback(HASH_ITEM_CREDITS, (hdMenuItemCallback)ShowCredits);
    }

    // Set callback on controller menu: Shock toggle
    hdMenu* controllerMenu = FindMenu(HASH_CONTROLLER_MENU);
    if (controllerMenu) {
        controllerMenu->SetCallback(HASH_ITEM_SHOCK, (hdMenuItemCallback)SetControllerShock);
    }

    // Set callbacks on new game confirm menu: NewGame + Resume(back)
    hdMenu* newGameMenu = FindMenu(HASH_NEWGAME_MENU);
    if (newGameMenu) {
        newGameMenu->SetCallback(HASH_ITEM_NEWGAME, (hdMenuItemCallback)NewGameCallback);
        newGameMenu->SetCallback(HASH_ITEM_NEWGAME_RESUME, (hdMenuItemCallback)ResumeGame);
    }
}

// PSX: GotoStartScreen__9feMenuMgr (Overlay4 0x800115BC)
// Goes to the screen indexed by feMode from the startScreenHashes array.
void feMenuMgr::GotoStartScreen() {
    MARKFUNCTION(0x800115BC);
    GotoScreen(startScreenHashes[feMode]);
}

// PSX: HandleInputChange__9feMenuMgr (Overlay4 0x80010F2C)
// Checks if on controller menu and handles DualShock enable/disable.
void feMenuMgr::HandleInputChange() {
    MARKFUNCTION(0x80010F2C);
    hdMenu* controllerMenu = FindMenu(HASH_CONTROLLER_MENU);
    if (curMenu == controllerMenu && controllerMenu) {
        hdMenuItem* shockItem = controllerMenu->FindItem(HASH_ITEM_SHOCK);
        if (shockItem) {
            // PSX: IsDualShock check - enable/disable shock item
            // PSX: if DualShock, enable item with normal color
            // PSX: else disable item with dim color, move selection if on it
            // TODO: implement DualShock detection
        }
    }
}

// PSX: InputItemPush__9feMenuMgr (Overlay4 0x80010A7C)
// Before pushing, saves sound/control state if on Sound or Controller menu.
void feMenuMgr::InputItemPush() {
    MARKFUNCTION(0x80010A7C);

    bool isSoundOrController = false;
    if (curMenu == FindMenu(HASH_SOUND_MENU) || curMenu == FindMenu(HASH_CONTROLLER_MENU)) {
        isSoundOrController = true;
    }

    if (!isSoundOrController) {
        MenuMgr::InputItemPush();
        return;
    }

    // PSX: ProcessSoundEvent(0, 14) — confirm sound
    // PSX: Save__14SoundMenuState(gSoundState)
    // PSX: Save__12ControlState(gControlState)
    // Then deactivate (falls through to Deactivate via vtable+80)
    Deactivate();
}

// PSX: InputPadUp__9feMenuMgr (Overlay4 0x80010B40)
// Skip if in level mode. Skip if on memcard menu. Otherwise base InputPadUp,
// then if on sound menu, notify sound manager.
void feMenuMgr::InputPadUp() {
    MARKFUNCTION(0x80010B40);
    if (feMode == 1) return;

    hdMenu* memcardMenu = FindMenu(HASH_MEMCARD_MENU);
    if (curMenu == memcardMenu) return;

    MenuMgr::InputPadUp();

    // PSX: if curMenu == sound menu, call OnMenuSelect__5SoundP6hdMenu
    hdMenu* soundMenu = FindMenu(HASH_SOUND_MENU);
    if (curMenu == soundMenu) {
        // PSX: OnMenuSelect__5SoundP6hdMenu(theSoundMgr, curMenu)
    }
}

// PSX: InputPadDown__9feMenuMgr (Overlay4 0x80010BC0)
// Same pattern as InputPadUp.
void feMenuMgr::InputPadDown() {
    MARKFUNCTION(0x80010BC0);
    if (feMode == 1) return;

    hdMenu* memcardMenu = FindMenu(HASH_MEMCARD_MENU);
    if (curMenu == memcardMenu) return;

    MenuMgr::InputPadDown();

    hdMenu* soundMenu = FindMenu(HASH_SOUND_MENU);
    if (curMenu == soundMenu) {
        // PSX: OnMenuSelect__5SoundP6hdMenu(theSoundMgr, curMenu)
    }
}

// PSX: InputPadLeft__9feMenuMgr (Overlay4 0x80010C40)
// If on memcard menu and HasMenu, redirect to InputPadUp; else base InputPadLeft.
void feMenuMgr::InputPadLeft() {
    MARKFUNCTION(0x80010C40);
    hdMenu* memcardMenu = FindMenu(HASH_MEMCARD_MENU);
    if (curMenu != memcardMenu) {
        MenuMgr::InputPadLeft();
        return;
    }
    // PSX: HasMenu__13hdMemCardMenu check
    // PSX: if has menu, redirect to base InputPadUp (navigate vertically)
    MenuMgr::InputPadUp();
}

// PSX: InputPadRight__9feMenuMgr (Overlay4 0x80010CA4)
// If on memcard menu and HasMenu, redirect to InputPadDown; else base InputPadRight.
void feMenuMgr::InputPadRight() {
    MARKFUNCTION(0x80010CA4);
    hdMenu* memcardMenu = FindMenu(HASH_MEMCARD_MENU);
    if (curMenu != memcardMenu) {
        MenuMgr::InputPadRight();
        return;
    }
    // PSX: HasMenu__13hdMemCardMenu check
    // PSX: if has menu, redirect to base InputPadDown (navigate vertically)
    MenuMgr::InputPadDown();
}

// PSX: PushMenu__9feMenuMgrP6hdMenu (Overlay4 0x80010D08)
// Before pushing, saves sound state for Sound menu or control state for Controller menu.
// For Controller menu, also sets up DualShock item enable/description.
void feMenuMgr::PushMenu(hdMenu* menu) {
    MARKFUNCTION(0x80010D08);

    if (menu == FindMenu(HASH_SOUND_MENU)) {
        // PSX: Save__14SoundMenuState(gSoundState)
        // Falls through to base PushMenu
    }
    else if (menu == FindMenu(HASH_CONTROLLER_MENU)) {
        // PSX: Save__12ControlState(gControlState)
        // PSX: Find controller selection item, set value from ShockEnable[2]
        // PSX: SetControlDescription
        // PSX: Find shock toggle item
        // PSX: if GetShock, set value 1; else set value 0
    }

    MenuMgr::PushMenu(menu);
}

// PSX: PopMenu__9feMenuMgr (Overlay4 0x80010E30)
// On pop, restores sound state if leaving Sound menu or control state if leaving Controller menu.
void feMenuMgr::PopMenu() {
    MARKFUNCTION(0x80010E30);

    if (curMenu == FindMenu(HASH_SOUND_MENU)) {
        // PSX: Restore__14SoundMenuState(gSoundState)
    }
    else if (curMenu == FindMenu(HASH_CONTROLLER_MENU)) {
        // PSX: Restore__12ControlState(gControlState)
    }

    MenuMgr::PopMenu();
}

// PSX: QueryInput__9feMenuMgrb (Overlay4 0x80011774)
// Polls input, dispatches buttons to virtual Input* functions.
// Uses menu-remapped button constants (set by Activate's control mode).
void feMenuMgr::QueryInput(bool processInput) {
    MARKFUNCTION(0x80011774);
    if (!g_inputManager) return;

    g_inputManager->Step();
    u32 buttons = g_inputManager->GetControlVal(0);

    if (!processInput) return;
    if (!buttons) return;

    // 0x800 = Back/Triangle (remapped)
    if (buttons & 0x800) {
        // If not on the level select screen, set state=8 (exit) and deselect
        hdMenu* levelMenu = FindMenu(HASH_LEVEL_SCREEN);
        if (curMenu != levelMenu) {
            state = 8;
            if (curMenu) {
                // PSX: calls curMenu vtable+36 (deselect)
            }
        }
    }

    // 0x1000 = Up
    if (buttons & 0x1000) {
        InputPadUp();
    }
    // 0x4000 = Down
    if (buttons & 0x4000) {
        InputPadDown();
    }
    // 0x8000 = Left
    if (buttons & 0x8000) {
        InputPadLeft();
    }
    // 0x2000 = Right
    if (buttons & 0x2000) {
        InputPadRight();
    }
    // 0x10 = Cancel/Pop (L1 remapped)
    if (buttons & 0x10) {
        InputItemPop();
    }
    // 0x40 = Confirm/Push (R1 remapped)
    if (buttons & 0x40) {
        InputItemPush();
    }
}

// PSX: Deactivate__9feMenuMgr (Overlay4 0x80011540)
// Calls base Deactivate, then handles level mode exit.
void feMenuMgr::Deactivate() {
    MARKFUNCTION(0x80011540);
    MenuMgr::Deactivate();

    if (feMode == 1) {
        if (state == 8) {
            // PSX: HandleVolumeExit__14FrontEndVolumeP8Humanoid(frontEndVolume, humanoid)
            // Returns to 3D hub from level select
        } else {
            // PSX: saves frontEndVolume position [29],[30],[31] into globals
            // gDestSelectReturnPos = frontEndVolume[29]
        }
    }
}

// PSX: ShowNewGameMenu__9feMenuMgr (Overlay4 0x800115F0)
// Resets to main menu mode.
void feMenuMgr::ShowNewGameMenu() {
    MARKFUNCTION(0x800115F0);
    feMode = 0;
    soundFlag = 0;
    SetTopMenu(startScreenHashes[0]);
}

// PSX: ShowLevel__9feMenuMgrP14FrontEndVolumeP8Humanoid (Overlay4 0x80011218)
// Switches to level select mode from the 3D hub.
void feMenuMgr::ShowLevel(FrontEndVolume* vol, Humanoid* hum) {
    MARKFUNCTION(0x80011218);
    frontEndVolume = vol;
    soundFlag = 1;
    humanoid = hum;
    if (g_game) {
        g_game->SetState(GameState::LocationMenu);  // state 18
    }
    InitLevelMenu();
}

// PSX: InitLevelMenu__9feMenuMgr (Overlay4 0x80011260)
// Sets up the level select screen with score data and unlock state.
void feMenuMgr::InitLevelMenu() {
    MARKFUNCTION(0x80011260);
    feMode = 1;
    SetTopMenu(startScreenHashes[1]);

    // PSX: reads frontEndVolume+128 for level code
    // PSX: finds overlay 42519405, gets text objects for score display
    // PSX: complex level/score display setup using World, ScoreManager, oxFontFile
    // PSX: sets callback HASH -1992448334 on level menu for LevelMenuExecute
    // TODO: implement full level menu setup
}

// PSX: OpenDoors__9feMenuMgr (Overlay4 0x80011680)
// Iterates level table, finds scene nodes by CRC, and enables valid doors.
void feMenuMgr::OpenDoors() {
    MARKFUNCTION(0x80011680);
    // PSX: copies 256-byte table from ROM (level ID, sublevel, node CRC triples)
    // PSX: for each entry: FindNodeCRC, check LevelValid, enable door
    // TODO: implement door opening (needs ccList, World level table)
}

// PSX: PushLoadSaveMenu__9feMenuMgri (Overlay4 0x80011618)
// Finds the memcard menu, starts its state machine, and pushes it.
void feMenuMgr::PushLoadSaveMenu(s32 mode) {
    MARKFUNCTION(0x80011618);
    hdMenu* memcardMenu = FindMenu(HASH_MEMCARD_MENU);
    if (memcardMenu) {
        // PSX: StateStart__13hdMemCardMenui(memcardMenu, mode)
        PushMenu(memcardMenu);
    }
}

// PSX: LevelValid__9feMenuMgril (Overlay4 0x80011188)
// Checks if a level/sublevel combination is unlocked.
s32 feMenuMgr::LevelValid(s32 levelID, s32 subLevel) {
    MARKFUNCTION(0x80011188);
    // PSX: if levelID==6, check GetTotalGoldDragon >= 6
    // PSX: else LevelIDToIndex, check score data at 48*index + 0x1C + 16*subLevel
    // TODO: implement (needs ScoreManager, World)
    return 0;
}
