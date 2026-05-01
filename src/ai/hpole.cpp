#include "gen/common.h"
#include "ai/hpole.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "p3d/p3dmath.h"

HorizontalPole::HorizontalPole(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x80015478);

    pointA = {};
    pointB = {};
    field140 = 0;
    field144 = 0;
    field148 = 0;
    field152 = 0;
}

HorizontalPole::~HorizontalPole() {
    MARKFUNCTION(0x800154D4);
}

void HorizontalPole::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800154FC);

    Obstacle::AnalyzeMesh(root);

    FillVectorArray(&pointA, *static_cast<DBLine*>(root));

    const LVector centre = {
        Div2TowardZero(pointA.x + pointB.x),
        Div2TowardZero(pointA.y + pointB.y),
        Div2TowardZero(pointA.z + pointB.z),
    };

    pos = centre;
    orientation = {};

    tagCollisionBox box = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    FillCollisionBox(box, centre, &pointA, 2);

    box.minX = (s16)(box.minX - 256);
    box.minY = (s16)(box.minY - 256);
    box.minZ = (s16)(box.minZ - 256);
    box.maxX = (s16)(box.maxX + 256);
    box.maxY = (s16)(box.maxY + 256);
    box.maxZ = (s16)(box.maxZ + 256);

    SetCollisionBox(box);

    const s32 dx = pointB.x - pointA.x;
    const s32 dz = pointB.z - pointA.z;
    const s32 mag = (s32)rmMag2((f32)dx, (f32)dz);

    field140 = rmDiv16i(-dz, mag);
    field144 = rmDiv16i(dx, mag);
    field148 = -(MulShift16(field140, pointA.x) + MulShift16(field144, pointA.z));

    field152 = -(s32)rmATan216((f32)field144, (f32)-field140);
    if (field152 < 0) {
        field152 += PSX_ANGLE_360;
    }
    else if (field152 > 0xFFFF) {
        field152 -= PSX_ANGLE_360;
    }
}

void HorizontalPole::CreateModel(const char* /*name*/) {
    MARKFUNCTION(0x80015750);
    flags |= TF_MODEL_CREATED;
}

void HorizontalPole::DeleteModel() {
    MARKFUNCTION(0x80015764);
    flags &= ~TF_MODEL_CREATED;
}

void HorizontalPole::Reset() {
    MARKFUNCTION(0x8001576C);
}

void HorizontalPole::Think() {
    MARKFUNCTION(0x80015774);
}

void HorizontalPole::UpdatePosition() {
    MARKFUNCTION(0x8001577C);
}

void HorizontalPole::HandlePickupCollision(Thing* /*pickup*/) {
    MARKFUNCTION(0x80015784);
}

void HorizontalPole::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001578C);

    if (!hum) {
        return;
    }

    if (hum->thingType != AITypes::TT_PLAYER) {
        return;
    }

    const s32 lineOffset = MulShift16(field140, hum->pos.x)
        + MulShift16(field144, hum->pos.z)
        + field148;

    s32 absOffset = lineOffset;
    if (absOffset < 0) {
        absOffset = -absOffset;
    }

    if (absOffset >= 512) {
        return;
    }

    const LVector attachPos = {
        hum->pos.x - MulShift16(field140, lineOffset),
        pointA.y,
        hum->pos.z - MulShift16(field144, lineOffset),
    };

    if (pointA.y < hum->pos.y) {
        return;
    }

    if (hum->pos.y + 1536 < pointA.y) {
        return;
    }

    if (hum->field420 == this) {
        return;
    }

    hum->homePos = attachPos;
    hum->collBboxMax.y = attachPos.x;
    hum->collBboxMax.z = attachPos.y;
    hum->field304 = attachPos.z;

    const s32 oldOrientX = hum->orientation.x;
    const s32 oldOrientZ = hum->orientation.z;

    s32 yaw = hum->orientation.y;
    while (yaw < 0) {
        yaw += PSX_ANGLE_360;
    }
    while (yaw > 0xFFFF) {
        yaw -= PSX_ANGLE_360;
    }

    if (field152 == 0) {
        if ((u32)(yaw - 0x4000) > 0x8000u) {
            yaw = 0;
        }
        else {
            yaw = 0x8000;
        }
    }
    else {
        const s32 delta = yaw - field152;
        if (delta >= 0) {
            if (delta < 0x4000) {
                yaw = field152;
            }
            else {
                yaw = field152 + 0x8000;
            }
        }
        else {
            if ((field152 - yaw) < 0x4000) {
                yaw = field152;
            }
            else {
                yaw = field152 + 0x8000;
            }
        }
    }

    if (yaw > 0xFFFF) {
        yaw -= PSX_ANGLE_360;
    }

    hum->orientation.x = oldOrientX;
    hum->orientation.y = yaw;
    hum->orientation.z = oldOrientZ;

    hum->SetDesiredMoveDirection(yaw);

    if (hum->actionState != AS_POLE_IDLE) {
        hum->SetActionState(AS_POLE_IDLE, 0);
    }

    hum->field420 = this;
}
