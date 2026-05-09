#pragma once
#include "core.h"
#include "p3d/hash.h"

// ccMinNode - minimal intrusive list node (12 bytes on PSX)
// PSX layout: +0 next, +4 prev, +8 vtable
struct ccMinNode {
    ccMinNode* next = nullptr;  // +0
    ccMinNode* prev = nullptr;  // +4

    ccMinNode() = default;
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

    s32 GetNumElements() const {
        s32 count = 0;
        for (ccMinNode* n = head; n; n = n->next) count++;
        return count;
    }

    // Insert 'newNode' after 'after'. If after==nullptr, insert at head.
    void AddNode(ccMinNode* after, ccMinNode* newNode) {
        if (!after) {
            // Insert at head
            newNode->next = head;
            newNode->prev = nullptr;
            if (head) head->prev = newNode;
            head = newNode;
            if (!tail) tail = newNode;
        }
        else {
            // Insert after 'after'
            newNode->next = after->next;
            newNode->prev = after;
            if (after->next) after->next->prev = newNode;
            after->next = newNode;
            if (tail == after) tail = newNode;
        }
    }

    ccMinNode* RemNode(ccMinNode* node) {
        if (node == head) head = node->next;
        if (node == tail) tail = node->prev;
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        node->next = nullptr;
        node->prev = nullptr;
        return node;
    }

    ccMinNode* RemHead() {
        ccMinNode* h = head;
        if (h) RemNode(h);
        return h;
    }

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

    ccNode() = default;
    ~ccNode() override {
        if (name && (flags & 1)) {
            std::free(name);
        }
        name = nullptr;
        nameCRC = 0;
    }

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
            if (!name)
                return;
            memcpy(name, str, len + 1);
            flags |= 1; // mark as allocated
        }
        else {
            name = const_cast<char*>(str);
        }
        nameCRC = p3dHash(str);
    }

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

    ccNode* FindNode(const char* name, ccNode* startAfter = nullptr) const {
        return FindNodeCRC(p3dHash(name), startAfter);
    }

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

    // PSX: SortPriReverse__6ccList (CCLIST.CPP:580, 0x8003780C)
    // Insertion-sort the list by pri ascending (lowest first = reverse of AddNodePri order).
    // PSX uses CheckPriReverse (0x800377FC) which compares the s8 pri at ccNode+0x12.
    void SortPriReverse() {
        MARKFUNCTION(0x8003780C);
        if (!head || !head->next) return;

        ccMinList sorted;
        while (head) {
            ccNode* node = static_cast<ccNode*>(RemHead());

            ccNode* cur = static_cast<ccNode*>(sorted.head);
            ccMinNode* insertAfter = nullptr;
            while (cur && cur->pri <= node->pri) {
                insertAfter = cur;
                cur = static_cast<ccNode*>(cur->next);
            }
            sorted.AddNode(insertAfter, node);
        }

        head = sorted.head;
        tail = sorted.tail;
        sorted.head = nullptr;
        sorted.tail = nullptr;
    }
};
