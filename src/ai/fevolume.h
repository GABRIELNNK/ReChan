#pragma once
#include "ai/obstacle.h"

class Humanoid;

// FrontEndVolume (132 bytes on PSX) - interactive door trigger in the hub level
// Inherits Obstacle (116 bytes)
// PSX: FEVOLUME.CPP (Overlay 3/4)
// PSX vtable at 0x80025A84
class FrontEndVolume : public Obstacle {
public:
    // PSX +116,+120,+124: saved return position (set from pos in AnalyzeMesh)
    LVector savedPos = {};

    // PSX +128: level code (read from DB attrib 8)
    s32 levelCode = 0;

    FrontEndVolume(const LVector* pos, u16 type);
    ~FrontEndVolume() override = default;

    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void AnalyzeMesh(DBRoot* root) override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    void HandleVolumeExit(Humanoid* hum);
};
