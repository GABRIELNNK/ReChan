#pragma once
#include "gen/cclist.h"

// Handler = callback node with function pointer, linked into HandlerSet lists.
// Game::ProcessHandlers iterates these each frame (Think handlers, then Draw handlers).

// Handler - callback node (36 bytes on PSX)
// PSX layout: +0 ccNode(24), +24 funcPtr, +28 userData, +32 ownerList
// Constructed inline; no PSX constructor symbol. Dtor at vtable[2].
struct Handler : public ccNode {
    using HandlerFunc = void(*)(Handler*);

    HandlerFunc funcPtr = nullptr;   // +24: callback function
    u32 userData = 0;                // +28: context data (unused in base)
    ccMinList* ownerList = nullptr;  // +32: back-pointer to list this handler is in

    Handler() = default;

    ~Handler() override {
        RemoveFromList();
    }

    void RemoveFromList() {
        MARKFUNCTION(0x8002CDE8);
        if (ownerList) {
            ownerList->RemNode(this);
        }
        ownerList = nullptr;
    }
};

// HandlerSet - embedded linked list of Handler nodes (36 bytes on PSX)
// PSX layout: +0 ccNode(24), +24 handlerList(ccMinList, 12 bytes)
// Game has two HandlerSets: one for think/logic, one for draw/render
struct HandlerSet : public ccNode {
    ccMinList handlerList; // +24: embedded list of Handler nodes

    HandlerSet() = default;

    ~HandlerSet() override {
        PurgeHandlers();
    }

    void PurgeHandlers() {
        MARKFUNCTION(0x8002CBB8);
        while (Handler* h = (Handler*)handlerList.head) {
            if (h->ownerList) {
                h->ownerList->RemNode(h);
                h->ownerList = nullptr;
            }
            else {
                handlerList.RemNode(h);
            }
            delete h;
        }
    }

    // Add a handler with a given function and priority
    Handler* AddHandler(Handler::HandlerFunc func, s8 priority) {
        Handler* h = new Handler();
        h->funcPtr = func;
        h->pri = priority;
        h->ownerList = &handlerList;
        // Insert by priority into the embedded list
        // (Higher pri = earlier execution)
        ccNode* cur = (ccNode*)handlerList.head;
        while (cur) {
            if (h->pri >= cur->pri) {
                handlerList.AddNode(cur->prev, h);
                return h;
            }
            cur = (ccNode*)cur->next;
        }
        handlerList.AddNode(handlerList.tail, h);
        return h;
    }
};
