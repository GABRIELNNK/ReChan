#pragma once
#include "gen/cclist.h"

// All game subsystems (World, InputManager, Database, etc.) inherit from Manager.
// Manager extends ccNode for linked-list participation in the Game manager list.

// Manager - base class for game subsystem managers (28 bytes on PSX)
// PSX layout: +0 ccNode(24), +24 isOpen(s16)
// vtable indices: [2]=dtor, [3]=InternalClose, [4]=InternalOpen, [5]=InternalReset
class Manager : public ccNode {
public:
    s16 isOpen = 0; // +24: 1 = open, 0 = closed

    Manager() { MARKFUNCTION(0x8002ED24); }
    ~Manager() override { MARKFUNCTION(0x8002ED5C); }
    void Open() {
        MARKFUNCTION(0x8002ECA4);
        if (isOpen) return;
        InternalOpen();
        isOpen = 1;
    }

    void Close() {
        MARKFUNCTION(0x8002EC58);
        if (!isOpen) return;
        InternalClose();
        isOpen = 0;
    }

    void Reset() {
        MARKFUNCTION(0x8002ECF4);
        InternalReset();
    }

    // Virtual overrides for subclasses
    virtual void InternalOpen() { MARKFUNCTION(0x8002EC48); }
    virtual void InternalClose() { MARKFUNCTION(0x8002EC40); }
    virtual void InternalReset() { MARKFUNCTION(0x8002EC50); }
};
