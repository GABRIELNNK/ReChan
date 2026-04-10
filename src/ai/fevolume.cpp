#include "ai/fevolume.h"
#include "ai/humanoid.h"
#include "fe/femenumgr.h"
#include "gen/config.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/display.h"
#include "gen/camera.h"

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

    savedPos = root->pos;

    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7) {
        const char* pointName = a7->strValue;
        DBPoint* point = g_database->FindPoint(pointName);
        if (point) {
            savedPos = root->pos;
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
        // PSX: hdDestSelect path (levelCode < 10)
        // Loads hdDestSelect at global+0x230, checks if already showing this levelCode,
        // calls HUD::DisplayTake, sets g_levelCodeFlag, calls hdDestSelect::ShowLevel.
        // TODO: requires HUD and hdDestSelect classes (not yet reversed)
    }
}

void FrontEndVolume::HandleVolumeExit(Humanoid* hum) {
    MARKFUNCTION(0x8001A9CC);

    LVector delta;
    delta.x = savedPos.x - hum->homePos.x;
    delta.y = savedPos.y - hum->homePos.y;
    delta.z = savedPos.z - hum->homePos.z;

    hum->homePos = savedPos;

    hum->pos.x += delta.x;
    hum->pos.y += delta.y;
    hum->pos.z += delta.z;

    // TODO: hum->SetFloorForced(1, 0, pos, delta) - vtable+232, not yet reversed
    // TODO: FaceThingDesired(hum, nullptr) - not yet reversed
    hum->FaceThing(nullptr, 0);
    if (g_display) {
        g_display->GetCamera()->SetLookAtTarget(hum, 1);
    }
}
