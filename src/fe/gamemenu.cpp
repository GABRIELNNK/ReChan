#include "fe/gamemenu.h"
#include "gen/game.h"

// Global gameMenu pointer
gameMenu* g_gameMenu = nullptr;

// Hash constants for gameMenu
static constexpr u32 HASH_PAUSE_MENU = (u32)(-1033476366);  // 0xC26EF5F2
static constexpr u32 HASH_ITEM_RESUME_GAME = (u32)(-961823183);  // ResumeGame callback
static constexpr u32 HASH_ITEM_SHOCK_TOGGLE = 810498402;  // SetControllerShock callback

// PSX: __8gameMenu (Overlay4 0x80010100)
gameMenu::gameMenu() {
    MARKFUNCTION(0x80010100);
    startScreenHashes[0] = 0;  // PSX: set by SelfInit
    startScreenHashes[1] = 0;
    pauseIndex = 0;
    g_gameMenu = this;
}

// PSX: _._8gameMenu (Overlay4 0x80010200)
gameMenu::~gameMenu() {
    MARKFUNCTION(0x80010200);
    if (g_gameMenu == this) g_gameMenu = nullptr;
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
