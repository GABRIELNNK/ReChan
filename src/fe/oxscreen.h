// oxscreen.h - oxScreen base class reversed from PSX OXSCREEN.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\OXSCREEN.CPP
// oxScreen is the base node for any screen in the screen manager stack.
#pragma once

#include "gen/cclist.h"

class oxScreenManager;

// oxScreen (16 bytes on PSX) - inherits ccMinNode
// PSX layout: +0 ccMinNode(8), +8 screenID(u32), +12 flags(u32)
class oxScreen : public ccMinNode {
public:
    u32 screenID = 0;   // +8: hashed screen identifier
    u32 screenFlags = 0; // +12

    // PSX: __8oxScreen (OXSCREEN.CPP, 0x80091164)
    oxScreen() { MARKFUNCTION(0x80091164); }

    // PSX: _._8oxScreen (OXSCREEN.CPP, 0x80091198)
    virtual ~oxScreen() { MARKFUNCTION(0x80091198); }

    // PSX: UpdateScreen (virtual) - called per frame by screen manager
    virtual void UpdateScreen(oxScreenManager* mgr) {}
};
