// simplbox.h - SimpleBox axis-aligned bounding box
// Reversed from PSX C:\CHAN\GAME\SRC\AI\SIMPLBOX.CPP
#pragma once

#include "core.h"
#include "p3d/lvector.h"

struct DBVolume;

// SimpleBox (24 bytes) - 6 s32 values: minX, minY, minZ, maxX, maxY, maxZ
// PSX source: C:\CHAN\GAME\SRC\AI\SIMPLBOX.CPP
struct SimpleBox {
    s32 minX = 0;
    s32 minY = 0;
    s32 minZ = 0;
    s32 maxX = 0;
    s32 maxY = 0;
    s32 maxZ = 0;

    // PSX: SetBox__9SimpleBoxP8DBVolume (SIMPLBOX.CPP:31)
    // Copies bboxMin/bboxMax from DBVolume (offsets +60..+84 = DWORD[15..20])
    void SetBox(const DBVolume* vol);

    // PSX: IsValid__C9SimpleBox (SIMPLBOX.CPP:44)
    bool IsValid() const { return minX != maxX; }

    // PSX: IsInside__C9SimpleBoxRC10tagLVector (SIMPLBOX.CPP:52)
    bool IsInside(const LVector& pos) const {
        if (minX < pos.x && pos.x < maxX) {
            if (minZ < pos.z && pos.z < maxZ) {
                if (minY < pos.y && pos.y < maxY)
                    return true;
            }
        }
        return false;
    }

    // PSX: IsInside__C9SimpleBoxll (SIMPLBOX.CPP:63)
    bool IsInside(s32 x, s32 z) const {
        return minX < x && x < maxX && minZ < z && z < maxZ;
    }
};
