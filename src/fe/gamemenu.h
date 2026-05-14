#pragma once
#include "fe/menumgr.h"

class Game;

// gameMenu (92 bytes on PSX) - inherits MenuMgr (80)
// PSX vtable: 0x800CC0F4
// PSX layout:
//   +0:  MenuMgr base (80 bytes)
//   +80: startScreenHashes[2] (u32[2]) - pause menu screen hashes, indexed by pauseIndex
//   +88: pauseIndex (s16)   - which menu hash to use
//   +90: pad (s16)
class gameMenu : public MenuMgr {
public:
    u32 startScreenHashes[2] = {};  // +80: menu hashes
    s16 pauseIndex = 0;             // +88
    s16 pausePad = 0;               // +90

    gameMenu();
    ~gameMenu() override;

    void Activate() override;
    void SelfInit() override;
    void GotoStartScreen() override;
    void Deactivate() override;
    void InputItemPush() override;
    void PushMenu(hdMenu* menu) override;
    void PopMenu() override;
    void HandleInputChange() override;
    void ShowPauseMenu();
    s32 ShowLoadingScreenText(u32 levelIndex, u32 petalIndex);
    void ResumeGame();
    void ExitGame();
};

// Global gameMenu pointer
extern gameMenu* g_gameMenu;
