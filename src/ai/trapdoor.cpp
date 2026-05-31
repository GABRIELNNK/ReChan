#include "gen/common.h"
#include "ai/trapdoor.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "gen/ai.h"
#include "gen/database.h"
#include "p3d/p3dmath.h"
#include "snd/snddrct.h"

TrapDoor::TrapDoor(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x80016FB8);
    field94 = INVALID_COLLISION_BOX;
    fieldA4 = 1;
    fieldB4 = 0;
    fieldB8 = 0;
}

TrapDoor::~TrapDoor() {
    MARKFUNCTION(0x8001702C);
}

static s32 TrapDoorAttribToAngle(s32 value) {
    return (value << 16) / 360;
}

void TrapDoor::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80017054);
    Obstacle::AnalyzeMesh(root);

    field7C.x = root->field40;
    field7C.y = root->field44;
    field7C.z = root->field48;

    while (field7C.y > 0xFF49) {
        field7C.y -= 0x10000;
    }
    while (field7C.y < 0) {
        field7C.y += 0x10000;
    }

    field88 = field7C;
    orientation = field7C;

    ObstacleFillCollisionBox(field94, root, 5);

    SetCollisionBox(field94);
    SetupCollisionBox();

    const DBAttrib* a6 = root->FindAttrib(6);
    field74 = a6 ? TrapDoorAttribToAngle((s32)a6->value) : 0x4000;

    const DBAttrib* a9 = root->FindAttrib(9);
    field78 = a9 ? TrapDoorAttribToAngle((s32)a9->value) : 0xB6;

    const DBAttrib* a10 = root->FindAttrib(10);
    fieldAC = a10 ? (s32)a10->value : 0;

    const DBAttrib* a11 = root->FindAttrib(11);
    fieldA8 = a11 ? (s32)a11->value : 0;

    const DBAttrib* a12 = root->FindAttrib(12);
    fieldB8 = a12 ? (s32)a12->value : 0;

    const DBAttrib* a13 = root->FindAttrib(13);
    fieldB0 = a13 ? (s32)a13->value : 0;

    const DBAttrib* a14 = root->FindAttrib(14);
    if (a14 && a14->strValue) {
        fieldBC = (s32)p3dHash(a14->strValue);
    }
    else {
        fieldBC = 0;
    }
}

void TrapDoor::CreateModel(const char* name) {
    MARKFUNCTION(0x800172EC);
    Thing::CreateModel(name);
}

void TrapDoor::DeleteModel() {
    MARKFUNCTION(0x8001730C);
    Thing::DeleteModel();
}

void TrapDoor::Reset() {
    MARKFUNCTION(0x8001732C);
}

void TrapDoor::TriggerByName(Thing* source, const char* /*name*/, const char* /*param*/) {
    MARKFUNCTION(0x80017334);
    if (fieldA4 == 1 && source && source->thingType == 0) {
        fieldB8 = fieldA4;
    }
}

void TrapDoor::Think() {
    MARKFUNCTION(0x80017360);

    if ((u32)(fieldA4 - 2) < 2) {
        Move();
        return;
    }

    if (!fieldB8) {
        return;
    }

    s32 timer = fieldB4 + 1;
    fieldB4 = timer;

    if (fieldA4 == 0) {
        if (fieldA8 > 0 && fieldA8 < timer) {
            fieldA4 = 3;
        }
        return;
    }

    if (fieldA4 == 1) {
        if (fieldAC > 0 && fieldAC < timer) {
            fieldA4 = 2;
            CSoundDirect::PlayTransient(159, &pos, 0, 0);

            if (fieldBC != 0 && g_ai) {
                ccNode* target = g_ai->moveList.FindNodeCRC((u32)fieldBC);
                if (target) {
                    static_cast<Obstacle*>(static_cast<Thing*>(target))->TriggerByName(
                        nullptr, nullptr, reinterpret_cast<const char*>(this));
                }
            }
        }
    }
}

void TrapDoor::UpdatePosition() {
    MARKFUNCTION(0x80017484);
}

void TrapDoor::Draw() {
    MARKFUNCTION(0x8001748C);
    s32 savedZ = orientation.z;
    orientation.z = field88.z;
    Obstacle::Draw();
    orientation.z = savedZ;
}

void TrapDoor::Move() {
    MARKFUNCTION(0x800174C8);

    if (fieldA4 == 2) {
        s32 next = field88.z + field78;
        s32 base = field7C.z;
        s32 openAbs = field74;
        if (openAbs < 0) {
            openAbs = -openAbs;
        }

        field88.z = next;

        s32 delta = next - base;
        bool past = false;
        if (delta >= 0) {
            past = openAbs < delta;
        }
        else {
            past = openAbs < (base - next);
        }

        if (past) {
            fieldA4 = 0;
            fieldB4 = 0;
            field88.z = field7C.z + field74;
            if (fieldA8 <= 0) {
                fieldB8 = 0;
            }
        }
    }
    else if (fieldA4 == 3) {
        s32 next = field88.z - field78;
        s32 base = field7C.z;
        s32 stepAbs = field78;
        if (stepAbs < 0) {
            stepAbs = -stepAbs;
        }

        field88.z = next;

        s32 delta = next - base;
        bool done = false;
        if (delta >= 0) {
            done = delta < stepAbs;
        }
        else {
            done = (base - next) < stepAbs;
        }

        if (done) {
            fieldA4 = 1;
            CSoundDirect::PlayTransient(160, &pos, 0, 0);
            field88.z = field7C.z;
            fieldB4 = 0;
            if (fieldB0 == 0) {
                fieldB8 = 0;
            }
        }
    }

    SetupCollisionBox();
}

void TrapDoor::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800175F4);
    pickup->Kill();
}

void TrapDoor::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80017628);

    LVector savedHomePos = hum->homePos;
    LVector savedVelocity = hum->velocity;

    s32 addVelX = 0;
    s32 addVelY = 0;
    s32 addVelZ = 0;

    s32 floorY = pos.y + (s32)collBox.maxY;

    switch (field88.y) {
        case 0:
        case 0x8000: {
            s32 slope = rmDiv16i(rmSin16(field88.z), rmSin16(field88.z + 0x4000));
            s32 delta = savedHomePos.x - pos.x;
            s32 term = (s32)(((s64)slope * delta) >> 16);
            floorY -= (term < 0) ? -term : term;
            break;
        }
        case 0x4000:
        case 0xC000:
        case 0x10000: {
            s32 slope = rmDiv16i(rmSin16(field88.z), rmSin16(field88.z + 0x4000));
            s32 delta = savedHomePos.z - pos.z;
            s32 term = (s32)(((s64)slope * delta) >> 16);
            floorY -= (term < 0) ? -term : term;
            break;
        }
        default:
            break;
    }

    if (field88.z >= 0x2000) {
        switch (field88.y) {
            case 0:
            case 0x10000:
                addVelX = -0xFA0;
                break;
            case 0x8000:
                addVelX = 0xFA0;
                break;
            case 0x4000:
                addVelZ = 0xFA0;
                break;
            case 0xC000:
                addVelZ = -0xFA0;
                break;
            default:
                break;
        }
    }

    if (floorY + savedVelocity.y - 0x80 < hum->pos.y) {
        hum->SetFloorHeight(floorY + (s32)collBox.maxY);

        if (savedVelocity.y <= 0) {
            if (!(floorY + 0x80 < savedHomePos.y)) {
                savedHomePos.y = floorY;
                hum->velocity.y = 0;
                hum->homePos = savedHomePos;
                AddPassenger(hum);
                hum->velocity.x += addVelX;
                hum->velocity.y += addVelY;
                hum->velocity.z += addVelZ;
            }
        }
    }
    else {
        LVector correctionNormal = { 0, 0, 0 };
        LVector correctionPushedPos = { 0, 0, 0 };

        CorrectThingPositionObstacle(
            pos,
            pos,
            field88.y,
            field88.y,
            collBox,
            savedHomePos,
            savedHomePos,
            hum->collBboxMin.x,
            hum->collBboxMin.y,
            hum->collBboxMin.z,
            savedHomePos,
            correctionNormal,
            correctionPushedPos);

        hum->homePos = savedHomePos;
    }

    if (hum->actionState == (s32)AS_LEDGE_LATCH) {
        hum->LetGoOfLedge();
    }
}

void TrapDoor::SetupCollisionBox() {
    MARKFUNCTION(0x80017AE4);

    tagCollisionBox box = collBox;

    s32 spanX = (s32)field94.maxX - (s32)field94.minX;
    s32 sinY = rmSin16(field88.z);
    s32 cosY = rmSin16(field88.z + 0x4000);
    s32 maxY = (s32)(u16)field94.maxY;

    box.minX = (s16)(-((s32)(((s64)spanX * cosY) >> 16)) - (s32)(((s64)maxY * sinY) >> 16));
    box.minY = (s16)(-((s32)(((s64)spanX * sinY) >> 16)));
    box.maxY = (s16)(((s64)maxY * cosY) >> 16);

    SetCollisionBox(box);
}

s32 TrapDoor::GetFloorMaterial() const {
    MARKFUNCTION(0x80017CC4);
    return 3;
}
