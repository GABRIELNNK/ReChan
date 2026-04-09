#pragma once
// Development helper (not PSX code)

#include "core.h"
#include "p3d/lvector.h"

class BlockManager;

namespace CollisionDebug {
    // Toggle collision debug drawing (press key to toggle)
    extern bool enabled;

    // Draw collision walls/floors for all loaded blocks
    // Call after world rendering, before EndRender
    void Draw(BlockManager* blockMgr);

}
