#pragma once
#include "ai/obstacle.h"

class Humanoid;

// Arrow (844 bytes on PSX) - hub-world navigational arrow
// Points toward the next uncompleted level/petal.
// PSX vtable: 0x80025B94 (overlay 6)
// PSX source: C:\CHAN\GAME\SRC\AI\ARROW.CPP
class Arrow : public Obstacle {
public:
    // +116: oscillation counter, cycles -7 to +7
    s32 arrowState = -7;

    // +120: start positions [5 levels][4 paths] from DB points
    LVector startPos[5][4] = {};

    // +360: end positions [5 levels][4 paths] from DB points
    LVector endPos[5][4] = {};

    // +600: direction/velocity data [5 levels][4 paths] from DB attribs
    struct ArrowDirData {
        s32 dirX = 0;
        s32 dirY = 5;
        s32 dirZ = 0;
    };
    ArrowDirData dirData[5][4] = {};

    // +840: current target level row (-1 = inactive)
    s16 currentRow = -1;
    // +842: current target petal col (-1 = inactive)
    s16 currentCol = -1;

    Arrow(const LVector* pos, u16 type);
    ~Arrow() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void Draw() override;
    void HandleHumanoidCollision(Humanoid* hum) override;
    void HandlePickupCollision(Thing* pickup) override;
};
