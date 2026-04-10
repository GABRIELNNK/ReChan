#include "gen/colvol.h"
#include "gen/database.h"

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
