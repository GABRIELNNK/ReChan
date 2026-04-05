// gameoverscreen.h - GameOverScreen reversed from PSX FEMNUMGR.CPP (Overlay 4)
// PSX source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP
// GameOverScreen is the "GAME OVER" / "CONTINUE?" screen.
// Same pattern as TitleScreen: color-cycling text on xclib overlays.
#pragma once

#include "fe/oxscrmgr.h"
#include "xclib/xccolour.h"
#include "xclib/xclib.h"

// GameOverScreen (56 bytes on PSX) - inherits oxScreenManager (48)
// PSX constructor: 0x80011A28 (Overlay 4)
// PSX layout:
//   +0..43: oxScreenManager base
//   +44: vtable
//   +48: continueText (u8*) - pointer to xcTextObj prim data
//   +52: menuColor (xcColour1555) - cycling color for text
class GameOverScreen : public oxScreenManager {
public:
    xcTextPrim* continueText = nullptr; // +48: xcTextObj prim for continue text
    xcColour1555 menuColor;        // +52: cycling text color

    // PSX: __14GameOverScreen (FEMNUMGR.CPP:758, Overlay4 0x80011A28)
    GameOverScreen();

    // PSX: _._14GameOverScreen (FEMNUMGR.H:132)
    ~GameOverScreen() override;

    // PSX: SelfInit__14GameOverScreen (FEMNUMGR.CPP:770, Overlay4 0x80011AE0)
    void SelfInit() override;

    // PSX: SelfUpdate__14GameOverScreen (FEMNUMGR.CPP:763, Overlay4 0x80011A64)
    void SelfUpdate() override;

    // PSX: GetScreenNames__14GameOverScreen (FEMNUMGR.CPP:782, Overlay4 0x80011B54)
    const char** GetScreenNames() override;
};
