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

    GameOverScreen();
    ~GameOverScreen() override;
    void SelfInit() override;
    void SelfUpdate() override;
    const char** GetScreenNames() override;
};
