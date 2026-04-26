#pragma once

#include "gen/model.h"

// PSX source mapping: C:\CHAN\GAME\SRC\GEN\MPLAYER.CPP/.HPP
// Player-specific model behavior and NIS animation loading.
class PlayerModel : public HumanoidModel {
public:
    PlayerModel();
    ~PlayerModel() override;

    void ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) override;
    void SetupModelCallbacks() override;
    void SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) override;
    s32 MirrorTree() override;
    void HandleLoop(AnimStructure* anim) override;
    void HandleRunToLast(AnimStructure* anim) override;
    void HandleIncFrame(AnimStructure* anim) override;
    s32 LoadNIS(u32 argc, const char** argv, s32 a4, s32 a5);
};
