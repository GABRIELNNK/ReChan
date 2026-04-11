#include "ai/fevolume.h"
#include "ai/humanoid.h"
#include "fe/femenumgr.h"
#include "gen/config.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/display.h"
#include "gen/camera.h"
#include "gen/game.h"
#include "pc/log.h"

#include <cstdio>

static const char* GetAttribStringCompat(const DBAttrib* attrib, char* tempBuf, size_t tempBufSize) {
    if (!attrib) {
        return nullptr;
    }

    if (attrib->type == 0) {
        return attrib->strValue;
    }

    if (!tempBuf || tempBufSize == 0) {
        return nullptr;
    }

    // PSX GetAttribString returns decimal text for numeric attributes.
    snprintf(tempBuf, tempBufSize, "%d", (s32)attrib->value);
    return tempBuf;
}

FrontEndVolume::FrontEndVolume(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x8001A758);
    savedPos = {};
    levelCode = 0;
}

void FrontEndVolume::Reset() {
}

void FrontEndVolume::Think() {
}

void FrontEndVolume::UpdatePosition() {
}

void FrontEndVolume::CreateModel(const char* name) {
    MARKFUNCTION(0x8001A8D8);
    flags |= TF_MODEL_CREATED;
}

void FrontEndVolume::DeleteModel() {
    MARKFUNCTION(0x8001A8EC);
    flags &= ~TF_MODEL_CREATED;
}

void FrontEndVolume::HandlePickupCollision(Thing* pickup) {
}

void FrontEndVolume::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001A7C4);
    Obstacle::AnalyzeMesh(root);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    tagCollisionBox localBox = {};
    DBVolume* vol = static_cast<DBVolume*>(root);
    FillCollisionBox(localBox, *vol);
    SetCollisionBox(localBox);

    // PSX copies this object's position after AnalyzeMesh setup.
    savedPos = pos;

    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7) {
        char pointNameBuf[32] = {};
        const char* pointName = GetAttribStringCompat(a7, pointNameBuf, sizeof(pointNameBuf));
        DBPoint* point = (g_database && pointName) ? g_database->FindPoint(pointName) : nullptr;
        if (point) {
            savedPos = point->pos;
        }
    }

    const DBAttrib* a8 = root->FindAttrib(8);
    if (a8) {
        levelCode = (s32)a8->value;
    }
}

void FrontEndVolume::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001A920);

    if (levelCode >= 10) {
        if (g_feMenuMgr) {
            g_feMenuMgr->ShowLevel(this, hum);
        }
    } else {
        // PSX path for levelCode < 10 routes through hdDestSelect/HUD.
        // That subsystem is not active in the current PC runtime.
    }
}

void FrontEndVolume::HandleVolumeExit(Humanoid* hum) {
    MARKFUNCTION(0x8001A9CC);

    if (!hum) {
        return;
    }

    LVector delta;
    delta.x = savedPos.x - hum->homePos.x;
    delta.y = savedPos.y - hum->homePos.y;
    delta.z = savedPos.z - hum->homePos.z;

    hum->homePos = savedPos;

    hum->pos.x += delta.x;
    hum->pos.y += delta.y;
    hum->pos.z += delta.z;

    // PSX vtable+232 call from HandleVolumeExit maps to SetActionState(1, 0).
    hum->SetActionState(AS_STAND, 0);

    if (g_display) {
        Camera* cam = g_display->GetCamera();
        if (cam) {
            const LVector& camPos = cam->GetPosition();
            hum->FacePointDesired(camPos);
            hum->FacePoint(camPos, 0);
            cam->SetLookAtTarget(hum, 1);
        }
    }
}
