#pragma once
#include "core.h"
#include "p3d/lvector.h"

struct DBVolume;

// SimpleBox (24 bytes) - 6 s32 values: minX, minY, minZ, maxX, maxY, maxZ
struct SimpleBox {
    s32 minX = 0;
    s32 minY = 0;
    s32 minZ = 0;
    s32 maxX = 0;
    s32 maxY = 0;
    s32 maxZ = 0;

    void SetBox(const DBVolume* vol);

    bool IsValid() const { return minX != maxX; }

    bool IsInside(const LVector& pos) const {
        if (minX < pos.x && pos.x < maxX) {
            if (minZ < pos.z && pos.z < maxZ) {
                if (minY < pos.y && pos.y < maxY)
                    return true;
            }
        }
        return false;
    }

    bool IsInside(s32 x, s32 z) const {
        return minX < x && x < maxX && minZ < z && z < maxZ;
    }
};
