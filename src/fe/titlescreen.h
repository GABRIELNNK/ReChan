// titlescreen.h - TitleScreen reversed from PSX FEMNUMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP (Overlay 4)
// TitleScreen is the "press start" title screen shown after intro.
// Loads XC/TITLE.1 through oxScreenManager::Init which uses xclib.
#pragma once

#include "fe/oxscrmgr.h"
#include "xclib/xccolour.h"
#include "xclib/xclib.h"

// TitleScreen (56 bytes on PSX) - inherits oxScreenManager (48)
// PSX constructor: 0x800118F0 (Overlay 4)
// PSX layout:
//   +0..43: oxScreenManager base
//   +44: vtable
//   +48: pressStartText (u8*) - pointer to xcTextObj prim data
//   +52: menuColor (xcColour1555) - cycling color for "PRESS START" text
class TitleScreen : public oxScreenManager {
public:
    xcTextPrim* pressStartText = nullptr; // +48: xcTextObj prim for "PRESS START"
    xcColour1555 menuColor;         // +52: cycling text color

    // PSX: __11TitleScreen (FEMNUMGR.CPP:726, Overlay4 0x800118F0)
    TitleScreen();

    // PSX: _._11TitleScreen (FEMNUMGR.H:118)
    ~TitleScreen() override;

    // PSX: SelfInit__11TitleScreen (FEMNUMGR.CPP:738, Overlay4 0x800119A8)
    void SelfInit() override;

    // PSX: SelfUpdate__11TitleScreen (FEMNUMGR.CPP:731, Overlay4 0x8001192C)
    void SelfUpdate() override;

    // PSX: GetScreenNames__11TitleScreen (FEMNUMGR.CPP:750, Overlay4 0x80011A1C)
    const char** GetScreenNames() override;
};
