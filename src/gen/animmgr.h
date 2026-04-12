#pragma once
#include "gen/cclist.h"
#include "gen/manager.h"
#include "p3d/flip.h"

// MiscAnimNode - node stored in AnimationManager's anim list (PSX: 28 bytes)
// PSX layout: ccMinNode(12) + field12(u32) + field16(s16) + field18(u8) + type(u8) + hash(u32) + anim(ptr)
struct MiscAnimNode : public ccMinNode {
    u32 field12 = 0;       // +12
    s16 field16 = 0;       // +16
    u8 field18 = 0;        // +18
    u8 type = 0;           // +19: 1=level, 2=petal
    u32 hash = 0;          // +20
    TransformAnim* anim = nullptr; // +24

    MiscAnimNode() = default;
    ~MiscAnimNode() override = default;
};

// AnimationManager (PSX: 40 bytes) - PSX GAME.CPP manager #15
// Stores a list of misc TransformAnim objects for level/petal prop animations.
// PSX ANIMMGR.CPP: 0x80057168
// Singleton stored at gp+3760 (0x800DD7FC)
class AnimationManager : public Manager {
public:
    ccMinList animList; // +28: list of MiscAnimNode*

    AnimationManager();
    ~AnimationManager() override;

    void InternalOpen() override;
    void InternalClose() override;
    void InternalReset() override;

    // PSX: PurgePetal__16AnimationManager (0x80057230)
    // Removes and deletes all petal-type (type==2) anim nodes.
    void PurgePetal();

    // PSX: PurgeLevel__16AnimationManager (0x800572B4)
    // Removes and deletes all anim nodes.
    void PurgeLevel();

    // PSX: GetMiscAnim__16AnimationManagerUl (0x80057308)
    // Returns the first MiscAnimNode whose hash matches, or nullptr.
    MiscAnimNode* GetMiscAnim(u32 hash);

    // Add a node to the animation list (tail insertion).
    void AddAnim(MiscAnimNode* node);
};

extern AnimationManager* g_animMgr;
