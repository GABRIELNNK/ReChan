#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/model.h"

void FillCollisionBox(tagCollisionBox& box, const DBVolume& vol) {
    s32 dx = vol.bboxMax.x - vol.bboxMin.x;
    s32 dy = vol.bboxMax.y - vol.bboxMin.y;
    s32 dz = vol.bboxMax.z - vol.bboxMin.z;
    box.minX = (s16)(dx / -2);
    box.minY = (s16)(dy / -2);
    box.minZ = (s16)(dz / -2);
    box.maxX = (s16)(dx / 2);
    box.maxY = (s16)(dy / 2);
    box.maxZ = (s16)(dz / 2);
}

bool FillCollisionBox(tagCollisionBox& box, const OriginalGeo& geo) {
    box.minX = (s16)geo.bboxMin[0];
    box.minY = (s16)geo.bboxMin[1];
    box.minZ = (s16)geo.bboxMin[2];
    box.maxX = (s16)geo.bboxMax[0];
    box.maxY = (s16)geo.bboxMax[1];
    box.maxZ = (s16)geo.bboxMax[2];
    return true;
}

void SetCollisionBoxExtent(tagCollisionBox& box) {
    s16 v = -box.minX;
    if (-box.minX < -box.minZ) {
        v = -box.minZ;
    }
    if (v < box.maxX) {
        v = box.maxX;
    }
    if (v < box.maxZ) {
        v = box.maxZ;
    }
    box.extent = v;
}
