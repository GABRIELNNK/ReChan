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

    TitleScreen();
    ~TitleScreen() override;
    void SelfInit() override;
    void SelfUpdate() override;
    const char** GetScreenNames() override;
};
