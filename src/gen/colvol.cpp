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

// PSX: FillCollisionBox__FR15tagCollisionBoxRC10tagLVectorPC10tagLVectorUl (COLVOL.CPP:200, 0x800A9D8C)
bool FillCollisionBox(tagCollisionBox& box, const LVector& origin, const LVector* points, u32 pointCount) {
    MARKFUNCTION(0x800A9D8C);

    const s32 firstX = points[0].x - origin.x;
    const s32 firstY = points[0].y - origin.y;
    const s32 firstZ = points[0].z - origin.z;

    box.minX = (s16)firstX;
    box.minY = (s16)firstY;
    box.minZ = (s16)firstZ;
    box.maxX = (s16)firstX;
    box.maxY = (s16)firstY;
    box.maxZ = (s16)firstZ;

    for (u32 i = 1; i < pointCount; i++) {
        const s32 dx = points[i].x - origin.x;
        const s32 dy = points[i].y - origin.y;
        const s32 dz = points[i].z - origin.z;

        if (dx >= (s32)box.minX) {
            if ((s32)box.maxX < dx) {
                box.maxX = (s16)dx;
            }
        }
        else {
            box.minX = (s16)dx;
        }

        if (dy >= (s32)box.minY) {
            if ((s32)box.maxY < dy) {
                box.maxY = (s16)dy;
            }
        }
        else {
            box.minY = (s16)dy;
        }

        if (dz >= (s32)box.minZ) {
            if ((s32)box.maxZ < dz) {
                box.maxZ = (s16)dz;
            }
        }
        else {
            box.minZ = (s16)dz;
        }
    }

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

// PSX: CheckStaticBoxSphereCollision (COLVOL.CPP:461, 0x800AA22C)
bool CheckStaticBoxSphereCollision(
    const LVector& boxPos,
    const tagCollisionBox& box,
    s32 rotY,
    const LVector& spherePos,
    const tagCollisionSphere& sphere) {
    MARKFUNCTION(0x800AA22C);

    const s32 minX = (s32)box.minX - sphere.radius;
    const s32 maxX = (s32)box.maxX + sphere.radius;
    const s32 minY = (s32)box.minY - sphere.radius;
    const s32 maxY = (s32)box.maxY + sphere.radius;
    const s32 minZ = (s32)box.minZ - sphere.radius;
    const s32 maxZ = (s32)box.maxZ + sphere.radius;

    const s32 dx = spherePos.x - boxPos.x;
    const s32 dy = spherePos.y - boxPos.y;
    const s32 dz = spherePos.z - boxPos.z;

    const s32 sinY = rmSin16(rotY);
    const s32 cosY = rmSin16(rotY + 0x4000);

    const s32 localX = static_cast<s32>((static_cast<s64>(cosY) * dx) >> 16)
        + static_cast<s32>((static_cast<s64>(-sinY) * dz) >> 16);
    const s32 localZ = static_cast<s32>((static_cast<s64>(sinY) * dx) >> 16)
        + static_cast<s32>((static_cast<s64>(cosY) * dz) >> 16);

    if (localX < minX || localX > maxX) {
        return false;
    }
    if (dy < minY || dy > maxY) {
        return false;
    }
    if (localZ < minZ || localZ > maxZ) {
        return false;
    }

    return true;
}

bool CheckStaticCylinderSphereCollision(
    const LVector& cylPos,
    const tagCollisionCylinder& cyl,
    const LVector& spherePos,
    const tagCollisionSphere& sphere) {
    MARKFUNCTION(0x800AA3E0);

    const s32 radius = cyl.radius + sphere.radius;
    const s32 minY = cyl.lowerY - sphere.radius;
    const s32 maxY = cyl.upperY + sphere.radius;
    const s32 relY = spherePos.y - cylPos.y;

    if (relY < minY || relY > maxY) {
        return false;
    }

    return rmMag2(
        static_cast<f32>(spherePos.x - cylPos.x),
        static_cast<f32>(spherePos.z - cylPos.z)) <= radius;
}
