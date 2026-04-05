// gamemenu.h - gameMenu reversed from PSX GAMEMENU.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\GAMEMENU.CPP (Overlay 4)
// gameMenu is the in-game pause menu (resume, options, quit).
// Lives in PSX Overlay 4.
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

    // PSX: __8gameMenu (GAMEMENU.CPP, Overlay4 0x80010100)
    gameMenu();

    // PSX: _._8gameMenu (GAMEMENU.CPP, Overlay4 0x80010200)
    ~gameMenu() override;

    // PSX: SelfInit__8gameMenu (GAMEMENU.CPP, Overlay4 0x80010280)
    void SelfInit() override;

    // PSX: SelfUpdate not overridden (vtable entry 8 = oxScreenManager::SelfUpdate no-op)

    // PSX: HandleInputChange__8gameMenu (GAMEMENU.CPP, Overlay4 0x80010400)
    void HandleInputChange() override;

    // PSX: ShowPauseMenu__8gameMenu (GAMEMENU.CPP, Overlay4 0x80010500)
    void ShowPauseMenu();

    // PSX: ResumeGame__8gameMenu (GAMEMENU.CPP, Overlay4 0x80010600)
    void ResumeGame();

    // PSX: ExitGame__8gameMenu (GAMEMENU.CPP, Overlay4 0x80010680)
    void ExitGame();
};

// Global gameMenu pointer
extern gameMenu* g_gameMenu;
