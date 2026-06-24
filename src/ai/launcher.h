#pragma once
#include "ai/obstacle.h"

class AnimStructure;
struct MiscAnimNode;

// Launcher (148 bytes on PSX) - trampoline/jump pad obstacle
// Type 402 (TT_LAUNCHER). Launches humanoids upward on contact.
// Inherits Obstacle (116 bytes), adds 32 bytes of Launcher-specific fields.
// PSX NBOL_REL.BIN overlay (region B, overlay ID 6)
// PSX vtable at 0x80025E90
class Launcher : public Obstacle {
public:
    // PSX +116 (s32): launch force X component (DB attrib 6)
    s32 forceX = 0;
    // PSX +120 (s32): launch force Y component (DB attrib 7)
    s32 forceY = 0;
    // PSX +124 (s32): launch force Z component (DB attrib 8)
    s32 forceZ = 0;
    // PSX +128 (ptr): AnimStructure for bounce animation
    AnimStructure* animStruct = nullptr;
    // PSX +132 (ptr): MiscAnimNode from obstacle anim table
    MiscAnimNode* animation = nullptr;
    // PSX +136 (s32): current animation frame counter
    s32 animFrame = 0;
    // PSX +140 (s32): animation playing flag (0 or 1)
    s32 animPlaying = 0;
    // PSX +144 (s32): animation index for GetAnimation lookup (DB attrib 21)
    s32 animIndex = 0;
#if HIGH_FPS_PLAY_PRESENTATION
    // Render-only: frame value at the start of the last logic tick, for Draw()-time interpolation.
    s32 renderPrevFrame = 0;
#endif

    Launcher(const LVector* pos, u16 type);
    ~Launcher() override;

    void Think() override;
    void Draw() override;
    void Reset() override;
    void UpdatePosition() override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void AnalyzeMesh(DBRoot* root) override;
    void HandleHumanoidCollision(Humanoid* hum) override;

private:
    void HandleHumanoidDefaultLaunch(Humanoid* hum);
};
