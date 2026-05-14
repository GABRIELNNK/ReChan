#include "ai/fevolume.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "fe/femenumgr.h"
#include "fe/hud.h"
#include "gen/config.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/display.h"
#include "gen/camera.h"
#include "gen/game.h"
#include "gen/world.h"

FrontEndVolume::FrontEndVolume(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x8001A758);
    savedPos = {};
    levelCode = 0;
}

void FrontEndVolume::Reset() {
    MARKFUNCTION(0x8001A900);
}

void FrontEndVolume::Think() {
    MARKFUNCTION(0x8001A908);
}

void FrontEndVolume::UpdatePosition() {
    MARKFUNCTION(0x8001A910);
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
    MARKFUNCTION(0x8001A918);
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
    if (a7 && a7->type == 0 && a7->strValue) {
        DBPoint* point = g_database ? g_database->FindPoint(a7->strValue) : nullptr;
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

    World* world = g_game ? g_game->GetWorld() : nullptr;
    if (hum != Player::s_player || !world || world->GetCurLevelID() != 7) {
        return;
    }

    if (levelCode >= 10) {
        g_feMenuMgr->ShowLevel(this, hum);
        return;
    }

    if (g_hud->destSelect.currentLevel != levelCode) {
        g_hud->DisplayTake(Player::s_player->livesLeft, 1);
        g_arrowInside = (u8)(levelCode > 0);
    }

    g_hud->destSelect.ShowLevel(levelCode);
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
