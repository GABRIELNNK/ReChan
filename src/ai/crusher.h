#pragma once
#include "ai/obstacle.h"

class CPlatformSound;

class Crusher : public Obstacle {
public:
    // PSX +116 (s32): actively crushing flag - Move() only runs while set (init 0 in ctor)
    s32 field116 = 0;
    // PSX +120 (s32): set by HandleHumanoidCollision; suppresses the auto-start
    // timer in Think() and, once the head returns to the top, snaps it to
    // field148/clears field116 instead of looping on its own (init 0 in ctor)
    s32 field120 = 0;
    // PSX +124: top limit - the head's spawn-time pos.y (set once in Think())
    s32 field124 = 0;
    // PSX +128: bottom limit - floor height probed below the head minus
    // collBox.minY (set once in Think())
    s32 field128 = 0;
    // PSX +132: attrib 12 - idle delay (frames) before the auto-start timer
    // in Think() flips field116 on
    s32 field132 = 0;
    // PSX +136: attrib 6 (default 5) - field160 snap value on reaching bottom
    s32 field136 = 0;
    // PSX +140: attrib 7 (default 0) - upward speed
    s32 field140 = 0;
    // PSX +144: attrib 8 (default 0) - pause duration at the top
    s32 field144 = 0;
    // PSX +148: attrib 9 (default 5) - field160 snap value on reaching top
    // (only applied when field120 != 0)
    s32 field148 = 0;
    // PSX +152: attrib 10 (default 0) - downward speed
    s32 field152 = 0;
    // PSX +156: attrib 11 (default 0) - pause duration at the bottom
    s32 field156 = 0;
    // PSX +160 (s32): accumulated position offset driving pos.y (init 0 in ctor)
    s32 field160 = 0;
    // PSX +164 (s32): dual-purpose countdown - pre-activation timer, then
    // the pause-at-endpoint timer between direction reversals
    s32 field164 = 0;
    // PSX +168 (s32): direction flag (init 1 in ctor moving down)
    s32 field168 = 1;
    // PSX +172: movement/impact sound, reuses CPlatformSound (init 0 in ctor)
    CPlatformSound* field172 = nullptr;

    Crusher(const LVector* pos, u16 type);
    ~Crusher() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Move() override;
    void UpdatePosition() override;
    void Draw() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
