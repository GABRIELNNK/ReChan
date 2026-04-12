#include "gen/animmgr.h"

AnimationManager* g_animMgr = nullptr;

// PSX: __16AnimationManager (ANIMMGR.CPP:147, 0x80057174)
AnimationManager::AnimationManager() {
    MARKFUNCTION(0x80057168);
    g_animMgr = this;
}

// PSX: _._16AnimationManager (ANIMMGR.CPP:)
AnimationManager::~AnimationManager() {
    MARKFUNCTION(0x80057174);
    PurgeLevel();
    if (g_animMgr == this) {
        g_animMgr = nullptr;
    }
}

// PSX: InternalOpen__16AnimationManager (ANIMMGR.CPP:152, 0x800571CC)
void AnimationManager::InternalOpen() {
    MARKFUNCTION(0x800571CC);
}

// PSX: InternalClose__16AnimationManager (ANIMMGR.CPP:157, 0x800571D4)
void AnimationManager::InternalClose() {
    MARKFUNCTION(0x800571D4);
    PurgeLevel();
}

// PSX: InternalReset__16AnimationManager (ANIMMGR.CPP:162, 0x80057228)
void AnimationManager::InternalReset() {
    MARKFUNCTION(0x80057228);
}

// PSX: PurgePetal__16AnimationManager (ANIMMGR.CPP:168, 0x80057230)
void AnimationManager::PurgePetal() {
    MARKFUNCTION(0x80057230);
    ccMinNode* node = animList.head;
    while (node) {
        MiscAnimNode* an = static_cast<MiscAnimNode*>(node);
        ccMinNode* next = node->next;
        if (an->type == 2) {
            animList.RemNode(node);
            delete an;
        }
        node = next;
    }
}

// PSX: PurgeLevel__16AnimationManager (ANIMMGR.CPP:185, 0x800572B4)
void AnimationManager::PurgeLevel() {
    MARKFUNCTION(0x800572B4);
    while (ccMinNode* n = animList.RemHead()) {
        delete n;
    }
}

// PSX: GetMiscAnim__16AnimationManagerUl (ANIMMGR.CPP:191, 0x80057308)
MiscAnimNode* AnimationManager::GetMiscAnim(u32 hash) {
    MARKFUNCTION(0x80057308);
    for (ccMinNode* n = animList.head; n; n = n->next) {
        MiscAnimNode* an = static_cast<MiscAnimNode*>(n);
        if (an->hash == hash) {
            return an;
        }
    }
    return nullptr;
}

// PSX: AddNodeTail used from level loading code
void AnimationManager::AddAnim(MiscAnimNode* node) {
    animList.AddNodeTail(node);
}
