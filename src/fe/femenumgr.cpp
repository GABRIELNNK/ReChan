#include "fe/femenumgr.h"
#include "fe/hdmenu.h"
#include "fe/hdmenuitems.h"
#include "xclib/xclib.h"
#include "gen/game.h"
#include "gen/ai.h"
#include "gen/world.h"
#include "gen/scoremgr.h"
#include "gen/control.h"
#include "ai/fevolume.h"
#include "ai/obstacle.h"
#include "pc/settings.h"
#include "pc/log.h"
#include "snd/fesnd.h"
#include "snd/rsevent.h"
#include "snd/sound.h"
#include "fe/fe_sound_menu_helpers.h"
#if NEW_CHEATS
#include "extra/cheats.h"
#endif

// Global feMenuMgr pointer
feMenuMgr* g_feMenuMgr = nullptr;

// Screen hashes (from decompiled constructor and SelfInit)
static constexpr u32 HASH_MAIN_SCREEN = 0x062B99E2;      // startScreenHashes[0]
static constexpr u32 HASH_LEVEL_SCREEN = 0x0598ABF8;     // startScreenHashes[1]
static constexpr u32 HASH_SOUND_MENU = 0x061CD029;       // Sound menu
static constexpr u32 HASH_CONTROLLER_MENU = 0x0867C4A4;  // Controller menu
static constexpr u32 HASH_NEWGAME_MENU = 0xB320874D;     // New game confirm
static constexpr u32 HASH_OPTIONS_MENU = 0xC073AB79;     // Options menu
static constexpr u32 HASH_MEMCARD_MENU = 0x1AB82599;     // Memory card menu

struct HubDoorEntry {
    s32 levelID;
    s32 subLevel;
    u32 doorCRC;
};

static const HubDoorEntry HUB_DOOR_TABLE[] = {
    { 1, 1, 0x09F04430 },
    { 1, 2, 0x09F04400 },
    { 1, 2, 0x09F04401 },
    { 2, 0, 0x0D8AC541 },
    { 2, 0, 0x0D8AC542 },
    { 2, 1, 0x0D8AC520 },
    { 2, 1, 0x0D8AC521 },
    { 2, 2, 0x0D8AC510 },
    { 2, 2, 0x0D8AC511 },
    { 3, 0, 0x09CDC541 },
    { 3, 1, 0x09CDC520 },
    { 3, 2, 0x09CDC510 },
    { 4, 0, 0x07965931 },
    { 4, 1, 0x07965950 },
    { 4, 2, 0x07965960 },
    { 5, 0, 0x0AB6F041 },
    { 5, 0, 0x0AB6F042 },
    { 5, 1, 0x0AB6F020 },
    { 5, 2, 0x0AB6F010 },
    { 6, 0, 0x0C472F91 },
    { 6, 0, 0x0C472F92 },
};

struct FeSoundMenuState {
    u16 musicVol = 0;
    u16 dialogVol = 0;
    u16 effectsVol = 0;
    u16 pad = 0;
    u32 stereoEnabled = 1;
};

struct FeControlState {
    u8 playerConfig = 0;
    u8 pad[3] = {};
    s32 shockEnabled = 0;
};

static FeSoundMenuState g_feSoundState;
static FeControlState g_feControlState;

static bool IsRegularPetalUnlocked(s32 levelIndex, s32 subLevel) {
    if (!g_scoreManager) {
        return false;
    }

    if ((u32)levelIndex >= 5 || (u32)subLevel >= 3) {
        return false;
    }

    const PetalStats& ps = g_scoreManager->petalStats[levelIndex * 3 + subLevel];
    return ps.fightScore >= -1;
}

static bool IsRegularLevelComplete(s32 levelIndex) {
    if (!g_scoreManager) {
        return false;
    }

    if ((u32)levelIndex >= 5) {
        return false;
    }

    for (s32 subLevel = 0; subLevel < 3; subLevel++) {
        const PetalStats& ps = g_scoreManager->petalStats[levelIndex * 3 + subLevel];
        if (ps.fightScore < 0) {
            return false;
        }
    }

    return true;
}

static void SaveSoundMenuState(FeSoundMenuState& state) {
    if (!g_sound) {
        return;
    }
    state.musicVol = (u16)g_sound->flag0;
    state.dialogVol = (u16)g_sound->flag1;
    state.effectsVol = (u16)g_sound->flag2;
    state.stereoEnabled = g_sound->activeFlag;
}

static void RestoreSoundMenuState(const FeSoundMenuState& state) {
    if (!g_sound) {
        return;
    }

    g_sound->flag1 = (s16)state.dialogVol;
    rsEvent(RS_SET_DIALOG_VOL, state.dialogVol, 0, 0);

    g_sound->flag2 = (s16)state.effectsVol;
    rsEvent(RS_SET_EFFECTS_VOL, state.effectsVol, 0, 0);
    rsEvent(RS_SET_EFFECTS_VOL_AUX, state.effectsVol, 0, 0);

    g_sound->flag0 = (s16)state.musicVol;
    rsEvent(RS_SET_MUSIC_VOL, state.musicVol, 0, 0);

    g_sound->activeFlag = state.stereoEnabled;
    rsEvent(state.stereoEnabled ? RS_SET_STEREO : RS_SET_MONO, 0, 0, 0);
}

static void SaveControlState(FeControlState& state) {
    if (g_inputManager) {
        state.playerConfig = g_inputManager->GetPlayerConfig();
    }
    state.shockEnabled = GetShock();
}

static void RestoreControlState(const FeControlState& state) {
    if (g_inputManager) {
        g_inputManager->SetPlayerConfig(state.playerConfig);
    }
    SetShock(state.shockEnabled);
}

// Menu item callback hashes
static constexpr u32 HASH_ITEM_RESUME = 0x95FA08AB;
static constexpr u32 HASH_ITEM_LOAD = 0x6D8E23DA;
static constexpr u32 HASH_ITEM_SAVE = 0xDEB60929;
static constexpr u32 HASH_ITEM_CREDITS = 0xE9CC104E;
static constexpr u32 HASH_ITEM_SHOCK = 0x442CA1FB;
static constexpr u32 HASH_ITEM_NEWGAME = 0x00018811;
static constexpr u32 HASH_ITEM_NEWGAME_RESUME = 0x00000A7D;
static constexpr u32 HASH_ITEM_CTRL_SEL = 0xA4EC8359;

// Sound menu item hashes (shared with gameMenu - PSX SOUND.CPP)
static constexpr u32 HASH_SOUND_EFFECT = 0x1B5DD3F5;
static constexpr u32 HASH_SOUND_MUSIC = 0xB3DA1CE9;
static constexpr u32 HASH_SOUND_VOICE = 0xB47983DE;
static constexpr u32 HASH_SOUND_STEREO = 0x3D030EFA;

// PSX: _SetMusicVolume (SOUND.CPP:148, 0x80059594)
static s32 SetMusicVolumeCallback(hdMenuItem* item) {
    return FeSoundMenuSetMusicVolume(item);
}

// PSX: _SetEffectsVolume (SOUND.CPP:159, 0x80059600)
static s32 SetEffectsVolumeCallback(hdMenuItem* item) {
    return FeSoundMenuSetEffectsVolume(item);
}

// PSX: _SetDialogVolume (SOUND.CPP:170, 0x80059698)
static s32 SetDialogVolumeCallback(hdMenuItem* item) {
    return FeSoundMenuSetDialogVolume(item);
}

// PSX: _StereoOnOff (SOUND.CPP:129, 0x8005950C)
static s32 StereoOnOffCallback(hdMenuItem* item) {
    return FeSoundMenuSetStereoOnOff(item);
}

// PSX: InstallMenu__5Sound (SOUND.CPP:272, 0x80059A4C)
static void InstallSoundMenu(hdMenu* menu) {
    FeInstallSoundMenuCommon(
        menu,
        HASH_SOUND_EFFECT,
        HASH_SOUND_MUSIC,
        HASH_SOUND_VOICE,
        HASH_SOUND_STEREO,
        SetEffectsVolumeCallback,
        SetMusicVolumeCallback,
        SetDialogVolumeCallback,
        StereoOnOffCallback);
}

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
        if (World* world = g_game->GetWorld()) {
            world->Unload();
            world->UnloadLevelPart2();
            world->UnloadPermanent();
        }
    }
    if (g_scoreManager) {
        g_scoreManager->drunkenMasterUnlocked = 0;
    }
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
    if (!item) {
        return 8;
    }

    s32 enabled = item->GetValue() ? 1 : 0;
    SetShock(enabled);
    if (enabled) {
        Shock(SHOCK_0);
    }
    else {
        Shock(SHOCK_CLEAR);
    }
    return 8;
}

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
            hdItemSelection* shockSelection = static_cast<hdItemSelection*>(shockItem);
            if (IsDualShock()) {
                shockSelection->enabled = 1;
                shockItem->SetValue(GetShock() ? 1 : 0);
            }
            else {
                shockSelection->enabled = 0;
                shockItem->SetValue(0);
                if (controllerMenu->curItem == shockItem) {
                    controllerMenu->InputNextItem();
                    if (controllerMenu->curItem == shockItem) {
                        controllerMenu->InputPrevItem();
                    }
                }
            }
        }
    }
}

// PSX: InputItemPush__9feMenuMgr (Overlay4 0x80010A7C)
// On sound/controller menu: save both states, persist to INI, then deactivate.
// PSX calls Deactivate directly (jal Deactivate__9feMenuMgr in overlay).
void feMenuMgr::InputItemPush() {
    MARKFUNCTION(0x80010A7C);

    if (curMenu != FindMenu(HASH_SOUND_MENU) && curMenu != FindMenu(HASH_CONTROLLER_MENU)) {
        MenuMgr::InputItemPush();
        return;
    }

    if (g_frontEndSound) {
        g_frontEndSound->ProcessSoundEvent(FE_SND_MENU_6);
    }

    SaveSoundMenuState(g_feSoundState);
    SaveControlState(g_feControlState);
    g_settings.Save(SETTINGS_PATH);
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
        SaveSoundMenuState(g_feSoundState);
        InstallSoundMenu(menu);
    }
    else if (menu == FindMenu(HASH_CONTROLLER_MENU)) {
        SaveControlState(g_feControlState);
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
        RestoreSoundMenuState(g_feSoundState);
    }
    else if (curMenu == FindMenu(HASH_CONTROLLER_MENU)) {
        RestoreControlState(g_feControlState);
    }

    MenuMgr::PopMenu();
}

// PSX: QueryInput__9feMenuMgrb (Overlay4 0x80011774)
// Polls input, dispatches to virtual Input* functions.
void feMenuMgr::QueryInput(bool processInput) {
    MARKFUNCTION(0x80011774);
    if (!g_inputManager) {
        return;
    }

    g_inputManager->Step();
    u32 buttons = g_inputManager->GetControlVal(0);
    if (!processInput || buttons == 0) {
        return;
    }

    buttons = FilterHostMenuButtons(buttons);

    if ((buttons & PsxPad::Start) != 0) {
        state = 8;
        if (curMenu) {
            // PSX: calls curMenu vtable+36 (deselect)
        }
    }

    if ((buttons & PsxPad::Up) != 0) {
        InputPadUp();
    }
    if ((buttons & PsxPad::Down) != 0) {
        InputPadDown();
    }
    if ((buttons & PsxPad::Left) != 0) {
        InputPadLeft();
    }
    if ((buttons & PsxPad::Right) != 0) {
        InputPadRight();
    }

    if ((buttons & PsxPad::MenuBack) != 0) {
        InputItemPop();
    }
    if ((buttons & PsxPad::Cross) != 0) {
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
            if (frontEndVolume && humanoid) {
                frontEndVolume->HandleVolumeExit(humanoid);
            }
        }
        else {
            // PSX: save FrontEndVolume return position for next hub load.
            if (frontEndVolume) {
                g_destSelectReturnPos = frontEndVolume->savedPos;
                g_destSelectReturnPosValid = true;
            }
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
// Callback hash for level menu execute
static constexpr u32 HASH_LEVEL_EXECUTE = 0x893DA6B2;

// Overlay and text object hashes for the level select screen
static constexpr u32 HASH_LOCATION_OVERLAY = 42519405;   // Menu_Location overlay
static constexpr u32 HASH_TEXT_LEVELNAME = 0x39AA5899;  // LevelName text
static constexpr u32 HASH_TEXT_DRAGONBAR = 0x3E799456;  // dragon bar text
static constexpr u32 HASH_TEXT_GOLDDRAGON = 0x07C775B9;  // gold dragon text
static constexpr u32 HASH_TEXT_LEVELGRADE = 0x6E7FDFDB;  // LevelGrade text

void feMenuMgr::InitLevelMenu() {
    MARKFUNCTION(0x80011260);
    feMode = 1;
    SetTopMenu(startScreenHashes[1]);

    s32 levelCode = 0;
    if (frontEndVolume) {
        levelCode = frontEndVolume->levelCode;
    }
    s32 levelID = levelCode / 10;
    s32 subLevel = levelCode % 10;
    const bool showDragons = (levelID >= 1 && levelID <= 5);

    World* world = g_game ? g_game->GetWorld() : nullptr;
    s32 levelIndex = 0;
    if (world) {
        levelIndex = world->LevelIDToIndex(levelID);
    }

    xcSection* sec = GetSection();
    xcOverlayData* overlay = FindOverlay(HASH_LOCATION_OVERLAY);
    if (overlay && sec) {
        u8* raw = sec->rawData;

        xcTextPrim* levelNameText = (xcTextPrim*)overlay->GetTextObj(HASH_TEXT_LEVELNAME, raw);
        xcTextPrim* dragonBarText = (xcTextPrim*)overlay->GetTextObj(HASH_TEXT_DRAGONBAR, raw);
        xcTextPrim* goldDragonText = (xcTextPrim*)overlay->GetTextObj(HASH_TEXT_GOLDDRAGON, raw);

        if (levelNameText && dragonBarText && goldDragonText) {
            // Resolve string data pointers for dragon bar and gold dragon
            u32 dragonBarStrHash = dragonBarText->GetStringHash();
            u32 goldDragonStrHash = goldDragonText->GetStringHash();
            char* dragonBarStr = (char*)sec->FindString(dragonBarStrHash);
            char* goldDragonStr = (char*)sec->FindString(goldDragonStrHash);

            if (levelID == 6) {
                // Final level (level 6) - special display
                xcFont* font = sectionMan ? sectionMan->FindFont("Gold_dr") : nullptr;
                levelNameText->paletteIdx = 3;
                dragonBarText->fontHash = font ? xcHash("Gold_dr") : dragonBarText->fontHash;
                dragonBarText->hdr.subtype = 5;
                goldDragonText->fontHash = font ? xcHash("Gold_dr") : goldDragonText->fontHash;
                goldDragonText->hdr.subtype = 5;
            }
            else {
                levelNameText->paletteIdx = (u8)subLevel;
                dragonBarText->hdr.subtype = 4;
                goldDragonText->hdr.subtype = 4;

                if (showDragons) {
                    // Set dragon bar font
                    if (sectionMan) {
                        xcFont* dragonFont = sectionMan->FindFont("Red_dr");
                        if (dragonFont) {
                            dragonBarText->fontHash = xcHash("Red_dr");
                        }
                    }

                    // Fill dragon bar string: 10 dragons, '1'=collected '0'=not
                    // String layout: 5 chars, newline, 5 chars
                    u8 collectCount = 0;
                    if (g_scoreManager) {
                        collectCount = g_scoreManager->petalStats[levelIndex * 3 + subLevel].collectCount;
                    }
                    if (dragonBarStr) {
                        s32 v13 = 0;
                        while (v13 < 10) {
                            s32 v15 = 0;
                            if (v13 == 5) {
                                dragonBarStr[5] = '\n';
                            }
                            if (v13 >= 5) {
                                v15 = 1;
                            }
                            s32 pos = v13 + v15;
                            if (v13 >= collectCount) {
                                dragonBarStr[pos] = '0';
                            }
                            else {
                                dragonBarStr[pos] = '1';
                            }
                            v13++;
                        }
                    }

                    // Set gold dragon font and display
                    if (sectionMan) {
                        xcFont* goldFont = sectionMan->FindFont("Gold_dr");
                        if (goldFont) {
                            goldDragonText->fontHash = xcHash("Gold_dr");
                        }
                    }
                    if (goldDragonStr) {
                        if (g_scoreManager && g_scoreManager->CalcGDrags(collectCount)) {
                            goldDragonStr[0] = '1';
                        }
                        else {
                            goldDragonStr[0] = '0';
                        }
                    }
                }
                else {
                    if (dragonBarStr) {
                        dragonBarStr[0] = '\0';
                    }
                    if (goldDragonStr) {
                        goldDragonStr[0] = '\0';
                    }
                }
            }
        }

        // Grade text
        xcTextPrim* gradeText = (xcTextPrim*)overlay->GetTextObj(HASH_TEXT_LEVELGRADE, raw);
        if (gradeText) {
            u8 gradeIdx = 8;
            if (g_scoreManager) {
                PetalStats* ps = &g_scoreManager->petalStats[levelIndex * 3 + subLevel];
                if (ps->fightScore >= 0) {
                    gradeIdx = ps->grade;
                }
            }
            gradeText->paletteIdx = gradeIdx;
        }
    }

    // Set the LevelMenuExecute callback on the level menu
    hdMenu* levelMenu = FindMenu(startScreenHashes[1]);
    if (levelMenu) {
        levelMenu->SetCallback(HASH_LEVEL_EXECUTE, (hdMenuItemCallback)World::LevelMenuExecute);
        hdMenuItem* execItem = levelMenu->FindItem(HASH_LEVEL_EXECUTE);
        if (execItem) {
            execItem->itemFlags = World::PackLevelName(levelIndex, subLevel);
        }
    }
}

// PSX: OpenDoors__9feMenuMgr (Overlay4 0x80011680)
// Iterates level table, finds scene nodes by CRC, and enables valid doors.
void feMenuMgr::OpenDoors() {
    MARKFUNCTION(0x80011680);

    if (!g_ai) {
        return;
    }

    for (const HubDoorEntry& entry : HUB_DOOR_TABLE) {
        ccNode* node = g_ai->moveList.FindNodeCRC(entry.doorCRC);
        if (!node) {
            LOG("[OpenDoors] door CRC=0x%08X NOT found (level %d/%d)",
                entry.doorCRC, entry.levelID, entry.subLevel);
            continue;
        }

        if (!LevelValid(entry.levelID, entry.subLevel)) {
            LOG("[OpenDoors] door CRC=0x%08X found but level %d/%d not unlocked",
                entry.doorCRC, entry.levelID, entry.subLevel);
            continue;
        }

        Thing* thing = static_cast<Thing*>(node);
        LOG("[OpenDoors] firing Trigger on door CRC=0x%08X (level %d/%d) thingType=%u name=%s",
            entry.doorCRC, entry.levelID, entry.subLevel, thing->thingType,
            node->GetName() ? node->GetName() : "null");
        static_cast<Obstacle*>(thing)->Trigger();
    }
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

#if NEW_CHEATS
    if (IsCheatEnabled(CheatOption::AllLevels)) return 1;
#endif

    if (levelID == 7) {
        return 1;
    }

    World* world = g_game ? g_game->GetWorld() : nullptr;
    if (!world || !g_scoreManager) {
        return (levelID == 1 && subLevel == 0) ? 1 : 0;
    }

    if (levelID == 6) {
        return g_scoreManager->GetTotalGoldDragon() >= 6 ? 1 : 0;
    }

    s32 levelIndex = world->LevelIDToIndex(levelID);
    if ((u32)levelIndex < 5) {
        return IsRegularPetalUnlocked(levelIndex, subLevel) ? 1 : 0;
    }

    switch (levelID) {
        case 11:
            return IsRegularLevelComplete(0) ? 1 : 0;
        case 12:
            return IsRegularLevelComplete(1) ? 1 : 0;
        case 13:
            return IsRegularLevelComplete(2) ? 1 : 0;
        case 14:
            return IsRegularLevelComplete(3) ? 1 : 0;
        case 8:
            return IsRegularLevelComplete(4) ? 1 : 0;
        default:
            return 0;
    }
}
