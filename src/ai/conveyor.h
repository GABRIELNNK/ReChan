#pragma once
#include "ai/obstacle.h"

struct UVPrimData;
class CGenericPersistentSound;

class Conveyor : public Obstacle {
public:
    s32 field116 = 0;
    s32 field120 = 0;
    s32 beltSpeed = 10;
    UVPrimData* field128 = nullptr;
    UVPrimData* field132 = nullptr;
    UVPrimData* field136 = nullptr;
    UVPrimData* field140 = nullptr;
    CGenericPersistentSound* field144 = nullptr;

    Conveyor(const LVector* pos, u16 type);
    ~Conveyor() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};
