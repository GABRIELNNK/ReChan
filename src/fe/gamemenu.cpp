#include "gen/common.h"
#include "fe/gamemenu.h"
#include "fe/hdmenuitems.h"
#include "gen/game.h"
#include "gen/control.h"
#include "gen/scoremgr.h"
#include "gen/world.h"
#include "fe/hud.h"
#include "snd/fesnd.h"
#include "snd/rsevent.h"
#include "snd/sound.h"
#include "pc/settings.h"
#include "xclib/xclib.h"
#include "fe/fe_sound_menu_helpers.h"

// Global gameMenu pointer
gameMenu* g_gameMenu = nullptr;

// Hash constants for gameMenu
static constexpr u32 HASH_PAUSE_MENU = (u32)(-1033476366);  // 0xC26EF5F2
static constexpr u32 HASH_PAUSE_GOLD_DRAGON_TEXT = (u32)(-1928898378);
static constexpr u32 HASH_ITEM_RESUME_GAME = (u32)(-961823183);  // ResumeGame callback
static constexpr u32 HASH_ITEM_SHOCK_TOGGLE = 810498402;  // SetControllerShock callback
static constexpr u32 HASH_SOUND_MENU = 102551593;     // xcHash("Sound")
static constexpr u32 HASH_SOUND_EFFECT = 459133941;   // xcHash("Sound_Effect")
static constexpr u32 HASH_SOUND_MUSIC = (u32)(-1277551383); // xcHash("Sound_Music")
static constexpr u32 HASH_SOUND_VOICE = (u32)(-1267104802); // xcHash("Sound_Voice")
static constexpr u32 HASH_SOUND_STEREO = 1023610618;  // xcHash("Sound_Stereo")
static constexpr u32 HASH_EXIT_MENU = 2613914;        // xcHash("Exit")
static constexpr u32 HASH_EXIT_NO = 2685;             // xcHash("No")
static constexpr u32 HASH_EXIT_YES = 100369;          // xcHash("Yes")
static constexpr u32 HASH_LOAD_TEXT_OVERLAY = 0x55C01B22;
static constexpr u32 HASH_LOAD_TEXT = 0x1FBB3816;

struct SoundMenuState {
    u16 musicVol = 0;
    u16 dialogVol = 0;
    u16 effectsVol = 0;
    u16 pad = 0;
    u32 stereoEnabled = 1;
};

static SoundMenuState g_soundState;
static s32 s_pauseHudDragonShowState = 0;

// PSX: __8gameMenu (Overlay4 0x80010100)
gameMenu::gameMenu() {
    MARKFUNCTION(0x80010100);
    startScreenHashes[0] = HASH_PAUSE_MENU;
    startScreenHashes[1] = (u32)(-464874722);
    pauseIndex = 0;
    g_gameMenu = this;
}

// PSX: _._8gameMenu (Overlay4 0x80010200)
gameMenu::~gameMenu() {
    MARKFUNCTION(0x80010200);
    if (g_gameMenu == this) g_gameMenu = nullptr;
}

// PSX: Activate__8gameMenu (Overlay4 0x80037A88)
void gameMenu::Activate() {
    MARKFUNCTION(0x80037A88);
    MenuMgr::Activate();

    if (pauseIndex != 0) {
        return;
    }

    xcOverlayData* overlay = FindOverlay(HASH_PAUSE_MENU);
    xcSection* sec = GetSection();
    if (overlay && sec) {
        u8* raw = sec->rawData;
        auto* goldDragonText = reinterpret_cast<xcTextPrim*>(overlay->GetTextObj(HASH_PAUSE_GOLD_DRAGON_TEXT, raw));
        if (goldDragonText) {
            s32 totalGoldDragon = g_scoreManager ? g_scoreManager->GetTotalGoldDragon() : 0;
            if (totalGoldDragon > 99) {
                totalGoldDragon = 99;
            }

            char* goldDragonStr = (char*)sec->FindString(goldDragonText->GetStringHash());
            if (goldDragonStr) {
                std::snprintf(goldDragonStr, 8, "%d", totalGoldDragon);
            }
        }
    }

    hdMenu* pauseMenu = FindMenu(HASH_PAUSE_MENU);
    if (!pauseMenu) {
        return;
    }

    hdMenuItem* shockItem = pauseMenu->FindItem(HASH_ITEM_SHOCK_TOGGLE);
    if (shockItem) {
        shockItem->SetValue(GetShock() ? 1 : 0);
    }
}

// PSX: _ResumeGame__8gameMenuP10hdMenuItem (0x8003791C)
static s32 ResumeGameCallback(hdMenuItem* item) {
    (void)item;
    return 8;  // state=8 = resume game (exit menu)
}

// PSX: SetControllerShock__FP10hdMenuItem (0x800378D0)
static s32 SetControllerShockCallback(hdMenuItem* item) {
    (void)item;
    // PSX: gets item value, calls SetShock, triggers vibration test
    return 8;
}

static s32 SetMusicVolumeCallback(hdMenuItem* item) {
    return FeSoundMenuSetMusicVolume(item);
}

static s32 SetEffectsVolumeCallback(hdMenuItem* item) {
    return FeSoundMenuSetEffectsVolume(item);
}

static s32 SetDialogVolumeCallback(hdMenuItem* item) {
    return FeSoundMenuSetDialogVolume(item);
}

static s32 StereoOnOffCallback(hdMenuItem* item) {
    return FeSoundMenuSetStereoOnOff(item);
}

static void SaveSoundMenuState(SoundMenuState& state) {
    if (!g_sound) {
        return;
    }
    state.musicVol = (u16)g_sound->flag0;
    state.dialogVol = (u16)g_sound->flag1;
    state.effectsVol = (u16)g_sound->flag2;
    state.stereoEnabled = g_sound->activeFlag;
}

static void RestoreSoundMenuState(const SoundMenuState& state) {
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

// PSX: ExitGame__8gameMenuP10hdMenuItem (0x80037924)
static s32 ExitGameCallback(hdMenuItem* item) {
    if (item->itemFlags) {
        LOG("[GameMenu] ExitGameCallback choice=yes");
        if (g_scoreManager) {
            g_scoreManager->HandleLevelAbort();
        }

        if (g_game) {
            g_game->SetState(GameState::OpenLocationMenu);
        }
        return 4;
    }

    LOG("[GameMenu] ExitGameCallback choice=no");
    return 8;
}

// PSX: SelfInit__8gameMenu (Overlay4 0x80010280)
void gameMenu::SelfInit() {
    MARKFUNCTION(0x80010280);
    // PSX: ParseDefFile, PostFlightDef, SetTopMenu
    ParseDefFile("XC/GAME_MNU.TXT");
    PostFlightDef();
    SetTopMenu(startScreenHashes[pauseIndex]);

    // PSX: Set callbacks on pause menu
    hdMenu* pauseMenu = FindMenu(HASH_PAUSE_MENU);
    if (pauseMenu) {
        pauseMenu->SetCallback(HASH_ITEM_RESUME_GAME, (hdMenuItemCallback)ResumeGameCallback);
        pauseMenu->SetCallback(HASH_ITEM_SHOCK_TOGGLE, (hdMenuItemCallback)SetControllerShockCallback);
    }
}

// PSX: GotoStartScreen__8gameMenu (Overlay4 0x80037DA0)
void gameMenu::GotoStartScreen() {
    MARKFUNCTION(0x80037DA0);
    GotoScreen(startScreenHashes[pauseIndex]);

    if (g_hud) {
        s_pauseHudDragonShowState = g_hud->dragonShowState;
        g_hud->EnableInput(0);
        if (!s_pauseHudDragonShowState) {
            g_hud->ToggleShowAll();
        }
    }
}

// PSX: Deactivate__8gameMenu (Overlay4 0x80037E14)
void gameMenu::Deactivate() {
    MARKFUNCTION(0x80037E14);
    MenuMgr::Deactivate();

    if (g_hud) {
        g_hud->EnableInput(1);
        if (!s_pauseHudDragonShowState) {
            g_hud->ToggleShowAll();
        }
    }
}

// PSX: InputItemPush__8gameMenu (Overlay4 0x80037B6C)
void gameMenu::InputItemPush() {
    MARKFUNCTION(0x80037B6C);

    if (curMenu != FindMenu(HASH_SOUND_MENU)) {
        MenuMgr::InputItemPush();
        return;
    }

    if (g_frontEndSound) {
        g_frontEndSound->ProcessSoundEvent(FE_SND_MENU_6);
    }

    SaveSoundMenuState(g_soundState);
    g_settings.Save(SETTINGS_PATH);
    Deactivate();
}

// PSX: PushMenu__8gameMenuP6hdMenu (0x80037970)
void gameMenu::PushMenu(hdMenu* menu) {
    MARKFUNCTION(0x80037970);
    if (menu == FindMenu(HASH_SOUND_MENU)) {
        SaveSoundMenuState(g_soundState);
        InstallSoundMenu(menu);
    }
    else if (menu == FindMenu(HASH_EXIT_MENU)) {
        menu->SetCallback(HASH_EXIT_NO, (hdMenuItemCallback)ExitGameCallback);
        hdMenuItem* noItem = menu->FindItem(HASH_EXIT_NO);
        if (noItem) {
            noItem->itemFlags = 0;
        }
        menu->SetCallback(HASH_EXIT_YES, (hdMenuItemCallback)ExitGameCallback);
        hdMenuItem* yesItem = menu->FindItem(HASH_EXIT_YES);
        if (yesItem) {
            yesItem->itemFlags = 1;
        }
    }
    MenuMgr::PushMenu(menu);
}

// PSX: PopMenu__8gameMenu (Overlay4 0x80037A3C)
void gameMenu::PopMenu() {
    MARKFUNCTION(0x80037A3C);
    if (curMenu == FindMenu(HASH_SOUND_MENU)) {
        RestoreSoundMenuState(g_soundState);
    }
    MenuMgr::PopMenu();
}

// PSX: HandleInputChange__8gameMenu (Overlay4 0x80010400)
// On the pause menu, enables/disables the shock toggle item based on DualShock presence.
void gameMenu::HandleInputChange() {
    MARKFUNCTION(0x80010400);
    hdMenu* pauseMenu = FindMenu(HASH_PAUSE_MENU);
    if (curMenu == pauseMenu && pauseMenu) {
        hdMenuItem* shockItem = pauseMenu->FindItem(HASH_ITEM_SHOCK_TOGGLE);
        if (shockItem) {
            // PSX: if IsDualShock, enable item with normal color
            // PSX: else disable with dim color, move selection if on it
            // PC: DualShock detection not implemented
        }
    }
}

// PSX: ShowPauseMenu__8gameMenu (Overlay4 0x80010500)
// Sets up the pause menu screen (Activate called separately by Game).
void gameMenu::ShowPauseMenu() {
    MARKFUNCTION(0x80010500);
    pauseIndex = 0;
    SetTopMenu(startScreenHashes[0]);
}

// PSX: ShowLoadingScreenText__8gameMenuUlUl (Overlay4 0x80037E88)
s32 gameMenu::ShowLoadingScreenText(u32 levelIndex, u32 petalIndex) {
    MARKFUNCTION(0x80037E88);

    World* world = g_game ? g_game->GetWorld() : nullptr;
    if (!world) {
        return 0;
    }

    u32 levelID = static_cast<u32>(world->GetLevelIDFromIndex(levelIndex));
    u8 textIndex = 0;
    s32 showText = 1;

    switch (levelID) {
    case 8u:
        petalIndex = 3;
        levelID = 5;
        break;
    case 11u:
        petalIndex = 3;
        levelID = 1;
        break;
    case 12u:
        petalIndex = 3;
        levelID = 2;
        break;
    case 13u:
        petalIndex = 3;
        levelID = 3;
        break;
    case 14u:
        petalIndex = 3;
        levelID = 4;
        break;
    default:
        break;
    }

    s32 result = (levelID < 7u) ? 1 : 0;
    if (levelID < 7u && levelID != 0u) {
        result = static_cast<s32>((levelID - 1u) * 4u);
        textIndex = static_cast<u8>(result + petalIndex);
    }
    else {
        showText = 0;
    }

    if (showText) {
        xcOverlayData* overlay = FindOverlay(HASH_LOAD_TEXT_OVERLAY);
        xcSection* sec = GetSection();
        auto* loadText = reinterpret_cast<xcTextPrim*>(overlay->GetTextObj(HASH_LOAD_TEXT, sec->rawData));
        loadText->paletteIdx = textIndex;

        GotoScreen(startScreenHashes[1]);
        g_display->BeginFrame();
        Update();
        Render();
        g_display->EndFrame();
        return 1;
    }

    return result;
}

// PSX: ResumeGame__8gameMenu (Overlay4 0x80010600)
// The ResumeGame callback returns 8 (handled by Invoke->Deactivate).
// This method is likely never called directly; the static callback is used instead.
void gameMenu::ResumeGame() {
    MARKFUNCTION(0x80010600);
    Deactivate();
}

// PSX: ExitGame__8gameMenu (Overlay4 0x80010680)
void gameMenu::ExitGame() {
    MARKFUNCTION(0x80010680);
    Deactivate();
    if (g_game) {
        g_game->SetState(GameState::Title);
    }
}
