#pragma once
#include "gen/cclist.h"

class oxScreenManager;

// oxScreen (16 bytes on PSX) - inherits ccMinNode
// PSX layout: +0 ccMinNode(8), +8 screenID(u32), +12 flags(u32)
class oxScreen : public ccMinNode {
public:
    u32 screenID = 0;   // +8: hashed screen identifier
    u32 screenFlags = 0; // +12

    oxScreen() { MARKFUNCTION(0x80091164); }

    virtual ~oxScreen() { MARKFUNCTION(0x80091198); }
    virtual void UpdateScreen(oxScreenManager* mgr) {}
};
