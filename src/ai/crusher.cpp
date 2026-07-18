#include "ai/crusher.h"
#include "ai/humanoid.h"
#include "ai/pickup.h"
#include "ai/player.h"
#include "gen/common.h"
#include "gen/database.h"
#include "gen/colsect.h"
#include "ai/obstacle_shared.h"
#include "snd/platsnd.h"

// PSX: gp+3108 - small extra depth added to field128 once a humanoid is
// actually caught, so the head visibly presses a bit further than the
// floor-probed bottom limit. Exact PSX constant not resolved; tuned value.
static constexpr s32 CRUSH_EXTRA_DEPTH = 16;

Crusher::Crusher(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x8001F6A4);
    field116 = 0;
    field120 = 0;
    field160 = 0;
    field168 = 1;
    field172 = nullptr;
    aliveFlag = 0;
}

Crusher::~Crusher() {
    MARKFUNCTION(0x8001F6F8);
}

void Crusher::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001F760);

    Obstacle::AnalyzeMesh(root);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);

    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6) {
        field136 = (s32)a6->value;
    }
    else {
        field136 = 5;
    }

    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7) {
        field140 = (s32)a7->value;
    }
    else {
        field140 = 0;
    }

    const DBAttrib* a8 = root->FindAttrib(8);
    if (a8) {
        field144 = (s32)a8->value;
    }
    else {
        field144 = 0;
    }

    const DBAttrib* a9 = root->FindAttrib(9);
    if (a9) {
        field148 = (s32)a9->value;
    }
    else {
        field148 = 5;
    }

    const DBAttrib* a10 = root->FindAttrib(10);
    if (a10) {
        field152 = (s32)a10->value;
    }
    else {
        field152 = 0;
    }

    const DBAttrib* a11 = root->FindAttrib(11);
    if (a11) {
        field156 = (s32)a11->value;
    }
    else {
        field156 = 0;
    }

    const DBAttrib* a12 = root->FindAttrib(12);
    if (a12) {
        field132 = (s32)a12->value;
    }
    else {
        field132 = 0;
    }

    field160 = field148;
}

void Crusher::CreateModel(const char* name) {
    MARKFUNCTION(0x8001F924);
    Obstacle::CreateModel(name);
}

void Crusher::DeleteModel() {
    MARKFUNCTION(0x8001F980);
    Obstacle::DeleteModel();
}

void Crusher::Reset() {
    MARKFUNCTION(0x8001F9D0);
    field168 = 1;
    field116 = 0;
    aliveFlag = 0;
    field120 = 0;
    field164 = 0;
    field160 = 0;
}

void Crusher::Think() {
    MARKFUNCTION(0x8001F9F0);

    if (aliveFlag != 0) {
        if (field116 != 0) {
            Move();
            return;
        }
        if (field120 != 0) {
            return;
        }
        field164 += 1;
        if (field132 < field164) {
            field116 = 1;
            field164 = 0;
        }
        return;
    }

    // PSX: first Think() after spawn only - record the spawn height as the
    // top limit and probe straight down from the head's underside to find
    // the floor, establishing how far down it should crush.
    LVector probePos = pos;
    probePos.y += (s32)collBox.minY;

    field124 = pos.y;
    aliveFlag = 1;

    // PSX: GetWorldFloorHeight__15CollisionSectorRC10tagLVectorl (0x800417B8)
    s32 floorHeight = (s32)0x80000001;
    s32 ceilingHeight = 0;
    LVector floorNormal = {};
    LVector ceilingNormal = {};
    CollisionSector::GetWorldFloorAndCeilingHeight(
        floorHeight, ceilingHeight, floorNormal, ceilingNormal, probePos, 16);
    field128 = floorHeight - (s32)collBox.minY;
}

void Crusher::UpdatePosition() {
    MARKFUNCTION(0x8001FAE0);
}

void Crusher::Draw() {
    MARKFUNCTION(0x8001FAE8);
    Obstacle::Draw();
}

void Crusher::Move() {
    MARKFUNCTION(0x8001FB08);

    field164 -= 1;
    if (field164 > 0) {
        return;
    }

    bool reversed = false;
    LVector newPos = pos;

    if (field168 != 0) {
        // Moving down.
        field160 += field152;
        s32 candidateY = pos.y - field160;
        if (candidateY < field128) {
            // Reached the bottom.
            candidateY = field128;
            field168 = 0;
            field164 = field156;
            field160 = field136;
            reversed = true;
        }
        newPos.y = candidateY;
    }
    else {
        // Moving up.
        field160 += field140;
        s32 candidateY = pos.y + field160;
        if (field124 < candidateY) {
            // Reached the top.
            candidateY = field124;
            field168 = 1;
            field164 = field144;
            if (field120 != 0) {
                field160 = field148;
                field116 = 0;
            }
            reversed = true;
        }
        newPos.y = candidateY;
    }

    if (reversed && field172) {
        field172->EndMove();
        field172->BeginMove();
    }

    pos = newPos;
}

void Crusher::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001FC58);

    if (!pickup) {
        return;
    }

    static_cast<Pickup*>(pickup)->PlayEffect();
    pickup->Kill();
}

void Crusher::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001FC98);

    if (!hum) {
        return;
    }

    // No physical push-out on PSX - this only fires the crush/kill reaction
    // once the head is actively descending near its bottom limit while
    // overlapping the humanoid; it never corrects/repositions them.
    const bool caughtHumanoid =
        field120 == 0 && field168 != 0 &&
        (pos.y - field128) < (hum->collBboxMin.z - hum->collBboxMin.y);

    if (!caughtHumanoid) {
        return;
    }

    if (hum->actionState != AS_GOT_HIT_CRUSHER && hum->actionState != AS_DEAD) {
        hum->SetActionState(AS_GOT_HIT_CRUSHER, 0);
    }

    if (field172) {
        field172->HitHumanoid();
    }

    field128 += CRUSH_EXTRA_DEPTH;

    if (hum == Player::s_player) {
        field120 = 1;
    }
    else {
        field168 = 0;
    }
}
