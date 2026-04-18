#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/model.h"
#include "p3d/p3dmath.h"

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

// PSX: CheckStaticHorizontalBoxPointCollision (COLVOL.CPP:283, 0x800AA0D4)
// Rotates delta (posB - posA) by -rotY, then tests against box XZ bounds.
bool CheckStaticHorizontalBoxPointCollision(
    const LVector& posA, const tagCollisionBox& box, s32 rotY, const LVector& posB) {

    s32 dx = posB.x - posA.x;
    s32 dz = posB.z - posA.z;

    s32 sinY = rmSin16(rotY);
    s32 cosY = rmSin16(rotY + 0x4000);

    // Rotate by -rotY: rotX = cos*dx + (-sin)*dz, rotZ = sin*dx + cos*dz
    s32 rotX = (s32)(((s64)cosY * dx) >> 16) + (s32)(((s64)(-sinY) * dz) >> 16);
    s32 rotZ = (s32)(((s64)sinY * dx) >> 16) + (s32)(((s64)cosY * dz) >> 16);

    if (rotX < box.minX) return false;
    if (rotX > box.maxX) return false;
    if (rotZ < box.minZ) return false;
    if (rotZ > box.maxZ) return false;
    return true;
}
