// manager.h - Manager base class reversed from PSX MANAGER.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\MANAGER.CPP
// All game subsystems (World, InputManager, Database, etc.) inherit from Manager.
// Manager extends ccNode for linked-list participation in the Game manager list.
#pragma once

#include "gen/cclist.h"

// Manager - base class for game subsystem managers (28 bytes on PSX)
// PSX layout: +0 ccNode(24), +24 isOpen(s16)
// vtable indices: [2]=dtor, [3]=InternalClose, [4]=InternalOpen, [5]=InternalReset
class Manager : public ccNode {
public:
    s16 isOpen = 0; // +24: 1 = open, 0 = closed

    // PSX: __7Manager (MANAGER.CPP:72, 0x8002ED24)
    Manager() { MARKFUNCTION(0x8002ED24); }

    // PSX: _._7Manager (MANAGER.CPP:77, 0x8002ED5C)
    ~Manager() override { MARKFUNCTION(0x8002ED5C); }

    // PSX: Open__7Manager (MANAGER.CPP:51, 0x8002ECA4)
    void Open() {
        MARKFUNCTION(0x8002ECA4);
        if (isOpen) return;
        InternalOpen();
        isOpen = 1;
    }

    // PSX: Close__7Manager (MANAGER.CPP:34, 0x8002EC58)
    void Close() {
        MARKFUNCTION(0x8002EC58);
        if (!isOpen) return;
        InternalClose();
        isOpen = 0;
    }

    // PSX: Reset__7Manager (MANAGER.CPP:65, 0x8002ECF4)
    void Reset() {
        MARKFUNCTION(0x8002ECF4);
        InternalReset();
    }

    // Virtual overrides for subclasses
    // PSX: InternalOpen__7Manager (MANAGER.CPP:26, 0x8002EC48)
    virtual void InternalOpen() { MARKFUNCTION(0x8002EC48); }
    // PSX: InternalClose__7Manager (MANAGER.CPP:22, 0x8002EC40)
    virtual void InternalClose() { MARKFUNCTION(0x8002EC40); }
    // PSX: InternalReset__7Manager (MANAGER.CPP:30, 0x8002EC50)
    virtual void InternalReset() { MARKFUNCTION(0x8002EC50); }
};
