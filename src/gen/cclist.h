// cclist.h - Intrusive linked list reversed from PSX CC library
// PSX source: C:\CHAN\GAME\SRC\GEN\CCLIST.CPP / CCLIST.HPP
// PSX uses these for callback chains, path lists, entity management, etc.
#pragma once

#include "common.h"
#include "p3d/hash.h"

// ccMinNode - minimal intrusive list node (12 bytes on PSX)
// PSX layout: +0 next, +4 prev, +8 vtable
struct ccMinNode {
    ccMinNode* next = nullptr;  // +0
    ccMinNode* prev = nullptr;  // +4

    // PSX: __9ccMinNode (CCLIST.CPP:245, 0x80037310)
    ccMinNode() = default;
    // PSX: _._9ccMinNode (CCLIST.CPP:253, 0x80037324)
    virtual ~ccMinNode() = default;

    // Unlink from whatever list this node is in
    void Remove() {
        if (prev) prev->next = next;
        if (next) next->prev = prev;
        next = nullptr;
        prev = nullptr;
    }
};

// ccMinList - minimal intrusive linked list (12 bytes on PSX)
// PSX layout: +0 head, +4 tail, +8 vtable
struct ccMinList {
    ccMinNode* head = nullptr;  // +0
    ccMinNode* tail = nullptr;  // +4

    virtual ~ccMinList() { Purge(); }

    // PSX: GetNumElements__9ccMinList (CCLIST.CPP:369, 0x800374EC)
    s32 GetNumElements() const {
        s32 count = 0;
        for (ccMinNode* n = head; n; n = n->next) count++;
        return count;
    }

    // PSX: AddNode__9ccMinListP9ccMinNodeT1 (CCLIST.CPP:389, 0x80037510)
    // Insert 'newNode' after 'after'. If after==nullptr, insert at head.
    void AddNode(ccMinNode* after, ccMinNode* newNode) {
        if (!after) {
            // Insert at head
            newNode->next = head;
            newNode->prev = nullptr;
            if (head) head->prev = newNode;
            head = newNode;
            if (!tail) tail = newNode;
        } else {
            // Insert after 'after'
            newNode->next = after->next;
            newNode->prev = after;
            if (after->next) after->next->prev = newNode;
            after->next = newNode;
            if (tail == after) tail = newNode;
        }
    }

    // PSX: RemNode__9ccMinList (CCLIST.CPP:447, 0x80037570)
    ccMinNode* RemNode(ccMinNode* node) {
        if (node == head) head = node->next;
        if (node == tail) tail = node->prev;
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        node->next = nullptr;
        node->prev = nullptr;
        return node;
    }

    // PSX: RemHead__9ccMinList (CCLIST.CPP:486, 0x800375E8)
    ccMinNode* RemHead() {
        ccMinNode* h = head;
        if (h) RemNode(h);
        return h;
    }

    // PSX: Purge__9ccMinList (CCLIST.CPP, 0x8002CD44)
    // Removes and deletes all nodes via virtual destructor
    void Purge() {
        while (ccMinNode* n = RemHead()) {
            delete n;
        }
    }

    void AddNodeTail(ccMinNode* node) {
        AddNode(tail, node);
    }

    ccMinNode* GetFirst() const { return head; }
    ccMinNode* GetLast() const { return tail; }
    bool IsEmpty() const { return head == nullptr; }
};

// ccNode - extended node with name, priority, CRC (24 bytes on PSX)
// PSX layout: +0 ccMinNode(12), +12 name(char*), +16 flags(s16), +18 pri(s8), +20 nameCRC(u32)
struct ccNode : public ccMinNode {
    char* name = nullptr;    // +12: allocated name string (or borrowed pointer)
    s16 flags = 0;           // +16: node flags
    s8 pri = 0;              // +18: priority for sorted insertion (signed)
    u32 nameCRC = 0;         // +20: hash of name via p3dHash

    // PSX: __6ccNode (CCLIST.CPP:259, 0x80037358)
    ccNode() = default;

    // PSX: _._6ccNode (CCLIST.CPP:276, 0x8003739C)
    ~ccNode() override {
        if (name && (flags & 1)) {
            std::free(name);
        }
        name = nullptr;
        nameCRC = 0;
    }

    // PSX: SetName__6ccNodePCci (CCLIST.CPP:296, 0x800373F0)
    // If alloc!=0, allocates a copy. Otherwise just stores pointer and CRC.
    void SetName(const char* str, s32 alloc) {
        if (name && (flags & 1)) {
            std::free(name);
            name = nullptr;
            nameCRC = 0;
            flags &= ~1;
        }
        if (!str) return;
        if (alloc) {
            s32 len = (s32)strlen(str);
            name = (char*)std::malloc(len + 1);
            memcpy(name, str, len + 1);
            flags |= 1; // mark as allocated
        } else {
            name = const_cast<char*>(str);
        }
        nameCRC = p3dHash(str);
    }

    // PSX: SetNameNoAlloc__6ccNodePCc (CCLIST.CPP:341, 0x80037494)
    void SetNameNoAlloc(const char* str) {
        if (name && (flags & 1)) {
            std::free(name);
            flags &= ~1;
        }
        name = nullptr;
        nameCRC = 0;
        if (str) {
            name = const_cast<char*>(str);
            nameCRC = p3dHash(str);
        }
    }

    const char* GetName() const { return name ? name : ""; }
    u32 GetNameCRC() const { return nameCRC; }
};

// ccList - extended list with priority insertion and named lookup
// PSX source: CCLIST.CPP
struct ccList : public ccMinList {
    // PSX: FindNodeCRC__6ccListUlP6ccNode (CCLIST.CPP:565, 0x80037664)
    ccNode* FindNodeCRC(u32 crc, ccNode* startAfter = nullptr) const {
        ccNode* n = startAfter ? startAfter : (ccNode*)head;
        while (n) {
            if (n->nameCRC == crc) return n;
            n = (ccNode*)n->next;
        }
        return nullptr;
    }

    // PSX: FindNode__6ccListPCcP6ccNode (CCLIST.CPP:559, 0x80037620)
    ccNode* FindNode(const char* name, ccNode* startAfter = nullptr) const {
        return FindNodeCRC(p3dHash(name), startAfter);
    }

    // PSX: AddNodePri__6ccListP6ccNode (CCLIST.CPP:764, 0x80037830)
    // Insert in priority-descending order (highest pri first)
    void AddNodePri(ccNode* newNode) {
        ccNode* cur = (ccNode*)head;
        while (cur) {
            if (newNode->pri >= cur->pri) {
                // Insert before cur = after cur->prev
                AddNode(cur->prev, newNode);
                return;
            }
            cur = (ccNode*)cur->next;
        }
        // Reached end - insert after tail
        AddNode(tail, newNode);
    }

    // PSX: SortPriReverse__6ccList (CCLIST.CPP:740, 0x8003780C)
    void SortPriReverse();
};
