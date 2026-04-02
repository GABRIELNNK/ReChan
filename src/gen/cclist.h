// cclist.h - Intrusive linked list reversed from PSX CC library
// PSX uses these for callback chains, path lists, entity management, etc.
#pragma once

#include "core.h"

// ccMinNode - minimal intrusive list node (8 bytes on PSX)
// PSX layout: +0 next, +4 prev, (+8 vtable)
struct ccMinNode {
    ccMinNode* next = nullptr;
    ccMinNode* prev = nullptr;

    virtual ~ccMinNode() = default;

    // Unlink from whatever list this node is in
    void Remove() {
        if (prev) prev->next = next;
        if (next) next->prev = prev;
        next = nullptr;
        prev = nullptr;
    }
};

// ccMinList - minimal intrusive linked list (8 bytes on PSX)
// PSX layout: +0 head, +4 tail, (+8 vtable)
struct ccMinList {
    ccMinNode* head = nullptr;
    ccMinNode* tail = nullptr;

    virtual ~ccMinList() {
        // PSX func_8004B624: iterate and delete all nodes
        while (head) {
            ccMinNode* n = head;
            head = n->next;
            delete n;
        }
        tail = nullptr;
    }

    // AddNode - insert 'node' after 'after' in the list
    // PSX: AddNode__9ccMinListP9ccMinNodeT1
    void AddNode(ccMinNode* node, ccMinNode* after) {
        if (!after) {
            // Insert at head
            node->next = head;
            node->prev = nullptr;
            if (head) head->prev = node;
            else tail = node;
            head = node;
        }
        else {
            node->next = after->next;
            node->prev = after;
            if (after->next) after->next->prev = node;
            else tail = node;
            after->next = node;
        }
    }

    // AddNodeTail - append to end of list
    void AddNodeTail(ccMinNode* node) {
        AddNode(node, tail);
    }

    ccMinNode* GetFirst() const {
        return head;
    }
    ccMinNode* GetLast() const {
        return tail;
    }
    bool IsEmpty() const {
        return head == nullptr;
    }
};

// ccNode - extended node with name (24 bytes on PSX)
// PSX layout: +0 ccMinNode base, +8 vtable, +12 name (12 bytes)
struct ccNode : public ccMinNode {
    char name[16] = {};

    ~ccNode() override = default;

    // PSX: SetName__6ccNodePCci
    void SetName(const char* n, s32 /*unused*/) {
        if (n) {
            s32 i = 0;
            while (n[i] && i < 15) { name[i] = n[i]; i++; }
            name[i] = '\0';
        }
    }

    const char* GetName() const { return name; }
};

// ccList - extended list with priority insertion
// PSX: AddNodePri inserts by priority ordering
struct ccList : public ccMinList {
    // AddNodePri - insert ccNode by name order (simplified)
    // PSX does priority-based insertion; we just append for now.
    void AddNodePri(ccNode* node) {
        AddNodeTail(node);
    }
};
