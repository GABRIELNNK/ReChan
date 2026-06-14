#include "gen/common.h"
#include "ai/teleporter.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/blockmgr.h"
#include "gen/camera.h"
#include "gen/database.h"
#include "gen/display.h"

Teleporter::Teleporter(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x800AA48C);
    targetPos = this->pos;
    targetAngle = 0;
    killThings = 0;
}

Teleporter::~Teleporter() {
    MARKFUNCTION(0x800AA4D8);
}

void Teleporter::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800AA500);
    Obstacle::AnalyzeMesh(root);

    if (!root) {
        return;
    }

    targetPos = pos;
    targetAngle = orientation.y;
    killThings = 0;

    DBVolume* vol = dynamic_cast<DBVolume*>(root);
    if (vol) {
        tagCollisionBox box = {};
        FillCollisionBox(box, *vol);
        SetCollisionBox(box);
    }

    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6 && g_database) {
        DBPoint* point = g_database->FindPoint(a6->strValue);
        if (point) {
            targetPos = point->pos;
            targetAngle = point->field44;
        }
    }

    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7) {
        killThings = 1;
    }

    LOG("[Teleporter] setup name=%s pos=(%d,%d,%d) target=(%d,%d,%d) angle=%d kill=%d",
        root->GetName() ? root->GetName() : "<unnamed>",
        pos.x, pos.y, pos.z,
        targetPos.x, targetPos.y, targetPos.z,
        targetAngle,
        killThings);
}

void Teleporter::CreateModel(const char* name) {
    MARKFUNCTION(0x800AA618);
    (void)name;
    flags |= TF_MODEL_CREATED;
}

void Teleporter::DeleteModel() {
    MARKFUNCTION(0x800AA62C);
    flags &= ~TF_MODEL_CREATED;
}

void Teleporter::Reset() {
    MARKFUNCTION(0x800AA640);
}

void Teleporter::Think() {
    MARKFUNCTION(0x800AA648);
}

void Teleporter::UpdatePosition() {
    MARKFUNCTION(0x800AA650);
}

void Teleporter::HandlePickupCollision(Thing* /*pickup*/) {
    MARKFUNCTION(0x800AA658);
}

void Teleporter::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800AA660);
    
    if (hum != Player::s_player) {
        return;
    }

    if (g_blockManager && g_blockManager->GetBlockNumber(targetPos) == BLOCK_UNASSIGNED) {
        LOG("[Teleporter::HandleHumanoidCollision] targetPos block UNASSIGNED, skipping teleport from (%d,%d,%d) to (%d,%d,%d)",
            hum->pos.x, hum->pos.y, hum->pos.z,
            targetPos.x, targetPos.y, targetPos.z);
        return;
    }

    LOG("[Teleporter::HandleHumanoidCollision] teleporting to (%d,%d,%d) angle=%d",
        targetPos.x, targetPos.y, targetPos.z, targetAngle);

    hum->DeleteRightHandObj();
    hum->DeleteLeftHandObj();

    LVector delta = {};
    delta.x = targetPos.x - hum->homePos.x;
    delta.y = targetPos.y - hum->homePos.y;
    delta.z = targetPos.z - hum->homePos.z;

    hum->homePos = targetPos;

    hum->pos.x += delta.x;
    hum->pos.y += delta.y;
    hum->pos.z += delta.z;

    hum->ClearFloorHeight();
    hum->SetDesiredMoveDirection(targetAngle);
    hum->FaceAngleY(targetAngle, 0);
    hum->velocity.y = 0;

    // PSX: adjusts thePlayer height-tracking fields by deltaY
    Player* player = Player::s_player;
    player->jumpReturnHeight += delta.y;
    player->groundStandHeight += delta.y;

    if (killThings && g_ai) {
        g_ai->KillThings(blockNum);
    }

    // PSX: theCamera->lookAtMode = 1 (direct write, not SetLookAtTarget)
    if (g_display) {
        Camera* cam = g_display->GetCamera();
        if (cam) {
            cam->SetLookAtMode(1);
        }
    }
}

