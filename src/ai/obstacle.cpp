#include "gen/common.h"
#include "ai/obstacle.h"
#include "ai/obstacle_shared.h"
#include "ai/activezn.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "gen/animmat.h"
#include "gen/blockmgr.h"
#include "gen/camera.h"
#include "gen/config.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/director.h"
#include "gen/display.h"
#include "gen/game.h"
#include "gen/geffect.h"
#include "gen/levelmgr.h"
#include "gen/model.h"
#include "gen/path.h"
#include "gen/world.h"
#include "p3d/p3dmath.h"
#include "snd/snddrct.h"

const LVector ZERO_DELTA_VELOCITY = { 0, 0, 0 };

const tagCollisionBox INVALID_COLLISION_BOX = {
    0x7FFF, 0x7FFF, 0x7FFF,
    -0x7FFF, -0x7FFF, -0x7FFF,
    -0x7FFF, 0
};

// PSX: FillCollisionBox__8ObstacleR15tagCollisionBoxRC6DBRootUl (OBSTACLE.CPP:530, 0x8007AF6C)
bool ObstacleFillCollisionBox(tagCollisionBox& box, const DBRoot* root, u32 attribNum) {
    const DBAttrib* attrib = root->FindAttrib(attribNum);
    if (!attrib) {
        return false;
    }
    const char* str = attrib->strValue;
    if (!str) {
        return false;
    }
    s32 hash = (s32)p3dHash(str);
    if (!g_levelManager) {
        return false;
    }
    OriginalBasic* geo = g_levelManager->FindGeo(hash);
    if (!geo) {
        return false;
    }
    OriginalGeo* ogeo = static_cast<OriginalGeo*>(geo);
    FillCollisionBox(box, *ogeo);
    return true;
}

// PSX Door AnalyzeMesh widens Z before SetCollisionBox (DOOR.CPP; lst RAM:8001ABB4 region).
void ApplyDoorStandingZExtent(tagCollisionBox& box) {
    box.minZ = (s16)(box.minZ - 128);
    box.maxZ = (s16)(box.maxZ + 1024);
}

static bool CollisionBoxLooksValid(const tagCollisionBox& box) {
    return box.minX <= box.maxX && box.minY <= box.maxY && box.minZ <= box.maxZ;
}

static constexpr s32 OBSTACLE_LEDGE_MIN_SPAN = 640;
static constexpr s32 OBSTACLE_LEDGE_HAND_Y_TOL = 100;
static constexpr s32 OBSTACLE_LEDGE_FLOOR_MIN_HEIGHT = 1022;
static constexpr s32 OBSTACLE_DETECT_OBSTACLE_OFFSET_XZ = 0x100;
static constexpr s32 OBSTACLE_DETECT_OBSTACLE_OFFSET_Y = 0x100;
static constexpr s32 OBSTACLE_DETECT_OBSTACLE_RADIUS = 0x20;

Obstacle::Obstacle(const LVector* pos, u16 type) : Thing(pos, type) {
    MARKFUNCTION(0x8007CA08);
    collBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    physicalType = 0;
    lightingFlag = 1;
    shadowFlag = 1;
}

Obstacle::~Obstacle() {
    MARKFUNCTION(0x8007CA7C);
}

void Obstacle::Think() {
    MARKFUNCTION(0x8007CDA4);
    Move();
}

void Obstacle::Draw() {
    MARKFUNCTION(0x8007AE04);
    if (model) {
        // PSX: selects render table based on shadowFlag and lightingFlag
        // (litTable, ZSortTable, litFarTable, ZFarTable) - not needed on PC

        // PSX: copies pos and orientation to model fields
        Model* m = static_cast<Model*>(model);
        m->posX = pos.x;
        m->posY = pos.y;
        m->posZ = pos.z;
        m->rotX = (u16)orientation.x;
        m->rotY = (u16)orientation.y;
        m->rotZ = (u16)orientation.z;
        m->Show(0);
    }
}

void Obstacle::Reset() {
    MARKFUNCTION(0x8007CD9C);
}

void Obstacle::Move() {
    MARKFUNCTION(0x8007CDD4);
}

void Obstacle::UpdatePosition() {
    MARKFUNCTION(0x8007CDDC);
}

void Obstacle::CreateModel(const char* name) {
    MARKFUNCTION(0x8007CC64);
    // PSX: empty stub - real work in AllocateAndCreateModel
    // PC: call Thing::CreateModel which creates SModel
    Thing::CreateModel(name);
}

void Obstacle::DeleteModel() {
    MARKFUNCTION(0x8007CD94);
    Thing::DeleteModel();
}

void Obstacle::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8007CBC4);
    Thing::AnalyzeMesh(root);

    const DBAttrib* a = root->FindAttrib(50);
    if (a) {
        physicalType = (u8)a->value;
    }

    if (thingType == 436) {
        lightingFlag = 0;
    }
    else {
        const DBAttrib* a51 = root->FindAttrib(51);
        if (a51 && a51->value == 2) {
            lightingFlag = 0;
        }
    }

    a = root->FindAttrib(52);
    if (a) {
        shadowFlag = 0;
    }
}

void Obstacle::FillSphere(tSphere& sphere) const {
    MARKFUNCTION(0x8007CE08);
}

void Obstacle::HandlePickupCollision(Thing* pickup) {}

void Obstacle::HandleHumanoidCollision(Humanoid* hum) {}

void Obstacle::Trigger() {
    MARKFUNCTION(0x8007CE40);
}

void Obstacle::TriggerByName(Thing* source, const char* name, const char* param) {
    MARKFUNCTION(0x8007CE94);
}

void Obstacle::ExplosiveTrigger(s32 damage, const char* name) {
    MARKFUNCTION(0x8007CE60);
}

const LVector* Obstacle::GetDeltaVelocity() const {
    MARKFUNCTION(0x8007CE70);
    return &ZERO_DELTA_VELOCITY;
}

bool Obstacle::CareAboutAttack() const {
    return false;
}

void Obstacle::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {}

s32 Obstacle::GetFloorMaterial() const {
    MARKFUNCTION(0x8007D0B4);
    return 0;
}

s32 Obstacle::GetObstacleFloorHeight(const LVector& pos) const {
    MARKFUNCTION(0x8007D0CC);
    // PSX: return this->pos.y + (s32)(s16)collBox.maxY
    return this->pos.y + (s32)collBox.maxY;
}

bool Obstacle::LedgeCheck(const tagCollisionBox& box, const LVector& normal, const LVector& correctionPos, Humanoid* hum) const {
    MARKFUNCTION(0x8007BE84);

    if (!hum) {
        return false;
    }

    const s32 actionState = hum->actionState;
    if (actionState == 32 || actionState == 34 || actionState == 35) {
        return false;
    }

    bool hasWideTop = false;
    if ((s32)box.maxX - (s32)box.minX >= OBSTACLE_LEDGE_MIN_SPAN) {
        hasWideTop = ((s32)box.maxZ - (s32)box.minZ) >= OBSTACLE_LEDGE_MIN_SPAN;
    }

    LVector facing = {};
    facing.x = rmSin16((s16)hum->orientation.y);
    facing.z = rmSin16((s16)(hum->orientation.y + 0x4000));

    const bool facingObstacleFront = rmV3Dot(&normal, &facing) < 0;

    HumanoidModel* model = hum->model ? static_cast<HumanoidModel*>(hum->model) : nullptr;
    if (!model || !model->animMatrices) {
        return false;
    }

    s32* leftHandMatrix = AnimationMatrices::GetMatrix(model->animMatrices, 1);
    s32* rightHandMatrix = AnimationMatrices::GetMatrix(model->animMatrices, 2);
    if (!leftHandMatrix || !rightHandMatrix) {
        return false;
    }

    const s32 avgHandY = (leftHandMatrix[6] + rightHandMatrix[6]) / 2;
    const bool nearHands = correctionPos.y >= avgHandY - OBSTACLE_LEDGE_HAND_Y_TOL
        && correctionPos.y <= avgHandY + OBSTACLE_LEDGE_HAND_Y_TOL;

    ModelFloorHeightState* floorState = model->field36
        ? static_cast<ModelFloorHeightState*>(model->field36)
        : nullptr;
    if (!floorState || floorState->current == (s32)0x80000001) {
        return false;
    }

    if (!hasWideTop || !facingObstacleFront || !nearHands) {
        return false;
    }

    return correctionPos.y >= floorState->current + OBSTACLE_LEDGE_FLOOR_MIN_HEIGHT;
}

// PSX: DetectObstacleAboveLedge__8ObstacleRC9_RMVECT16RC10tagLVector (OBSTACLE.CPP:1576)
bool Obstacle::DetectObstacleAboveLedge(const LVector& normal, const LVector& ledgePos) {
    MARKFUNCTION(0x8007C6DC);

    const s32 offsetX = MulShift16(normal.x, OBSTACLE_DETECT_OBSTACLE_OFFSET_XZ);
    const s32 offsetZ = MulShift16(normal.z, OBSTACLE_DETECT_OBSTACLE_OFFSET_XZ);

    LVector startPos = {};
    startPos.x = ledgePos.x + offsetX;
    startPos.y = ledgePos.y + OBSTACLE_DETECT_OBSTACLE_OFFSET_Y;
    startPos.z = ledgePos.z + offsetZ;

    LVector endPos = {};
    endPos.x = ledgePos.x - offsetX;
    endPos.y = ledgePos.y + OBSTACLE_DETECT_OBSTACLE_OFFSET_Y;
    endPos.z = ledgePos.z - offsetZ;

    return DetectObstacle(startPos, endPos, OBSTACLE_DETECT_OBSTACLE_RADIUS);
}

// PSX: DetectObstacle__8ObstacleRC10tagLVectorT1l (OBSTACLE.CPP:1611)
bool Obstacle::DetectObstacle(const LVector& startPos, const LVector& endPos, s32 radius) {
    MARKFUNCTION(0x8007C7B0);

    if (!g_ai) {
        return false;
    }

    const s32 midX = (startPos.x + endPos.x) / 2;
    const s32 midY = (startPos.y + endPos.y) / 2;
    const s32 midZ = (startPos.z + endPos.z) / 2;
    const u32 searchExtent = rmMag2(endPos.x - startPos.x, endPos.z - startPos.z);

    for (ccMinNode* node = g_ai->moveList.head; node; node = node->next) {
        Obstacle* obs = static_cast<Obstacle*>(static_cast<Thing*>(node));
        if ((obs->flags & TF_MODEL_CREATED) == 0) {
            continue;
        }
        if (!obs->GetPhysical()) {
            continue;
        }

        s32 threshold = (s32)obs->collBox.extent + (s32)searchExtent + radius;

        s32 deltaX = midX - obs->pos.x;
        if (deltaX < 0) {
            deltaX = -deltaX;
        }
        if (deltaX >= threshold) {
            continue;
        }

        s32 deltaZ = midZ - obs->pos.z;
        if (deltaZ < 0) {
            deltaZ = -deltaZ;
        }
        if (deltaZ >= threshold) {
            continue;
        }

        s32 deltaY = endPos.y - startPos.y;
        if (deltaY < 0) {
            deltaY = -deltaY;
        }

        if (midY + deltaY + radius < obs->pos.y + (s32)obs->collBox.minY) {
            continue;
        }
        if (obs->pos.y + (s32)obs->collBox.maxY < midY - deltaY - radius) {
            continue;
        }

        LVector outPos = {};
        LVector outNormal = {};
        LVector outPushedPos = {};
        if (CorrectThingPositionObstacle(
                obs->pos,
                obs->pos,
                obs->orientation.y,
                obs->orientation.y,
                obs->collBox,
                startPos,
                endPos,
                radius,
                -radius,
                radius,
                outPos,
                outNormal,
                outPushedPos)) {
            return true;
        }
    }

    return false;
}

s32 Obstacle::GetPhysical() const {
    MARKFUNCTION(0x8007D034);
    // PSX: switch on thingType (OBSTACLE.CPP:2152, 0x8007D034)
    switch (thingType) {
        case 404:  // Conveyor
        case 407:  // HorizontalPole
        case 435:  // Untouchable
        case 436:  // Collectible
        case 451:  // TriggerThing
        case 459:  // Blast
        case 463:  // Door
        case 464:  // Teleporter
        case 467:  // TrapDoor
        case 469:  // FrontEndVolume
        case 470:  // Ladder
            return 0;
        default:
            return 1;
    }
}

void Obstacle::SetCollisionBox(const tagCollisionBox& box) {
    MARKFUNCTION(0x8007BE24);
    collBox = box;
    SetCollisionBoxExtent(collBox);
}

// PSX: CheckXZStaticBoxCylinderCollision (OBSTACLE.CPP:193, 0x8007A740)
static bool CheckXZStaticBoxCylinderCollision(
    const LVector& obsPos, const tagCollisionBox& box, s32 orientY,
    const LVector& cylPos, const tagCollisionCylinder& cyl) {
    s32 minX = box.minX - cyl.radius;
    s32 maxX = box.maxX + cyl.radius;
    s32 maxZ = box.maxZ + cyl.radius;
    s32 minZ = box.minZ - cyl.radius;

    LVector delta;
    delta.x = cylPos.x - obsPos.x;
    delta.y = cylPos.y - obsPos.y;
    delta.z = cylPos.z - obsPos.z;

    s32 sinV = rmSin16(orientY);
    s32 cosV = rmSin16(orientY + 0x4000);

    s32 localX = (s32)(((s64)cosV * delta.x) >> 16) + (s32)((-(s64)sinV * delta.z) >> 16);
    s32 localZ = (s32)(((s64)sinV * delta.x) >> 16) + (s32)(((s64)cosV * delta.z) >> 16);

    if (localX >= minX && maxX >= localX && localZ >= minZ && maxZ >= localZ) {
        return true;
    }
    return false;
}

inline s32 MulShift16(s32 a, s32 b) {
    return (s32)(((s64)a * b) >> 16);
}

inline s32 Div2TowardZero(s32 value) {
    return (value + (s32)((u32)value >> 31)) >> 1;
}

static constexpr s32 OBSTACLE_CORRECT_BUFFER = 4;
static constexpr s32 OBSTACLE_YMAX_BUFFER = 0x40;
static constexpr s32 CORRECT_THING_OBSTACLE_BOX_Y_MAX_OFFSET = 9;
static s32 CORRECT_THING_POSITION_PUSHABLE_CHEAT = 0;
static constexpr s32 CORRECT_THING_POSITION_EXTRA = 0x20;

// DEBUG: set by Platform::HandleHumanoidCollision to enable logging inside CorrectThingPositionObstacle
static bool s_debugCTP = false;

void SetCorrectThingPositionDebug(bool enabled) {
    s_debugCTP = enabled;
}

// PSX: CorrectThingPosition__8ObstacleRC10tagLVectorT1llRC15tagCollisionBoxT1T1lllR10tagLVectorR9_RMVECT16T11_ (0x8007B398)
bool CorrectThingPositionObstacle(
    const LVector& basisA,
    const LVector& basisB,
    s32 rotA,
    s32 rotB,
    const tagCollisionBox& box,
    const LVector& pointA,
    const LVector& pointB,
    s32 radius,
    s32 yMinOffset,
    s32 yMaxOffset,
    LVector& outPos,
    LVector& outNormal,
    LVector& outPushedPos) {
    const s32 sinA = rmSin16(rotA);
    const s32 cosA = rmSin16(rotA + 0x4000);
    const s32 sinB = rmSin16(rotB);
    const s32 cosB = rmSin16(rotB + 0x4000);

    LVector localA = {
        pointA.x - basisA.x,
        pointA.y - basisA.y,
        pointA.z - basisA.z
    };
    LVector localB = {
        pointB.x - basisB.x,
        pointB.y - basisB.y,
        pointB.z - basisB.z
    };

    const s32 localAX = localA.x;
    localA.x = MulShift16(cosA, localA.x) + MulShift16(-sinA, localA.z);
    localA.z = MulShift16(sinA, localAX) + MulShift16(cosA, localA.z);

    const s32 localBX = localB.x;
    localB.x = MulShift16(cosB, localB.x) + MulShift16(-sinB, localB.z);
    localB.z = MulShift16(sinB, localBX) + MulShift16(cosB, localB.z);

    s32 minX = 0;
    s32 maxX = 0;
    if (localA.x >= localB.x) {
        minX = localB.x - radius;
        maxX = localA.x + radius;
    }
    else {
        minX = localA.x - radius;
        maxX = localB.x + radius;
    }

    s32 minY = 0;
    s32 maxY = 0;
    if (localA.y >= localB.y) {
        minY = localB.y + yMinOffset;
        maxY = localA.y + yMaxOffset;
    }
    else {
        minY = localA.y + yMinOffset;
        maxY = localB.y + yMaxOffset;
    }

    s32 minZ = 0;
    s32 maxZ = 0;
    if (localA.z >= localB.z) {
        minZ = localB.z - radius;
        maxZ = localA.z + radius;
    }
    else {
        minZ = localA.z - radius;
        maxZ = localB.z + radius;
    }

    bool overlapXZ = false;
    if (maxX >= (s32)box.minX && (s32)box.maxX >= minX &&
        maxZ >= (s32)box.minZ && (s32)box.maxZ >= minZ) {
        overlapXZ = true;
    }

    bool overlapY = false;
    if (maxY >= (s32)box.minY) {
        overlapY = ((s32)box.maxY - CORRECT_THING_OBSTACLE_BOX_Y_MAX_OFFSET) >= minY;
    }

    s32 correctedX = localB.x;
    s32 correctedY = localB.y;
    s32 correctedZ = localB.z;
    s32 normalX = 0;
    s32 normalY = 0;
    s32 normalZ = 0;

    const s32 centreX = Div2TowardZero((s32)box.minX + (s32)box.maxX);
    const s32 centreZ = Div2TowardZero((s32)box.minZ + (s32)box.maxZ);

    bool corrected = false;

    if (overlapXZ && overlapY) {
        s32 pushX = 0;
        s32 pushY = 0;
        s32 pushZ = 0;
        s32 distX = -0xFFFF;
        s32 distY = -0xFFFF;
        s32 distZ = -0xFFFF;
        s32 targetX = 0;
        s32 targetY = 0;
        s32 targetZ = 0;

        if (localA.x < centreX) {
            if (((s32)box.minX - radius - CORRECT_THING_POSITION_EXTRA) < localB.x) {
                pushX = -1;
                distX = (s32)box.minX - localA.x;
                targetX = (s32)box.minX - radius - OBSTACLE_CORRECT_BUFFER;
            }
        }
        else if (centreX < localA.x) {
            if (localB.x < ((s32)box.maxX + radius + CORRECT_THING_POSITION_EXTRA)) {
                pushX = 1;
                distX = localA.x - (s32)box.maxX;
                targetX = (s32)box.maxX + radius + OBSTACLE_CORRECT_BUFFER;
            }
        }

        if (localB.x + CORRECT_THING_POSITION_PUSHABLE_CHEAT >= (s32)box.minX &&
            (s32)box.maxX >= localB.x - CORRECT_THING_POSITION_PUSHABLE_CHEAT &&
            localB.z + CORRECT_THING_POSITION_PUSHABLE_CHEAT >= (s32)box.minZ &&
            (s32)box.maxZ >= localB.z - CORRECT_THING_POSITION_PUSHABLE_CHEAT) {
            if (localA.y < (s32)box.minY) {
                pushY = -1;
                distY = (s32)box.minY - (localA.y + yMaxOffset);
                targetY = (s32)box.minY - yMaxOffset;
            }
            else if (((s32)box.maxY - OBSTACLE_YMAX_BUFFER) < localA.y) {
                pushY = 1;
                distY = 0xFFFF;
                targetY = (s32)box.maxY - yMinOffset;
            }
        }

        if (localA.z < centreZ) {
            if (((s32)box.minZ - radius - CORRECT_THING_POSITION_EXTRA) < localB.z) {
                pushZ = -1;
                distZ = (s32)box.minZ - localA.z;
                targetZ = (s32)box.minZ - radius - OBSTACLE_CORRECT_BUFFER;
            }
        }
        else if (centreZ < localA.z) {
            if (localB.z < ((s32)box.maxZ + radius + CORRECT_THING_POSITION_EXTRA)) {
                pushZ = 1;
                distZ = localA.z - (s32)box.maxZ;
                targetZ = (s32)box.maxZ + radius + OBSTACLE_CORRECT_BUFFER;
            }
        }

        if (pushX != 0 || pushY != 0 || pushZ != 0) {
            corrected = true;

            if (s_debugCTP) {
                bool pushYGuard = (localB.x + CORRECT_THING_POSITION_PUSHABLE_CHEAT >= (s32)box.minX &&
                    (s32)box.maxX >= localB.x - CORRECT_THING_POSITION_PUSHABLE_CHEAT &&
                    localB.z + CORRECT_THING_POSITION_PUSHABLE_CHEAT >= (s32)box.minZ &&
                    (s32)box.maxZ >= localB.z - CORRECT_THING_POSITION_PUSHABLE_CHEAT);
                LOG("[CTP] localA=(%d,%d,%d) localB=(%d,%d,%d) box=(%d,%d,%d,%d,%d,%d)",
                    localA.x, localA.y, localA.z, localB.x, localB.y, localB.z,
                    box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
                LOG("[CTP] push=(%d,%d,%d) dist=(%d,%d,%d) centre=(%d,%d) pushYGuard=%d radius=%d",
                    pushX, pushY, pushZ, distX, distY, distZ, centreX, centreZ, pushYGuard, radius);
                const char* winner = "none";
                if (distX < distZ) {
                    winner = (distZ >= distY) ? "Z" : "Y";
                } else {
                    winner = (distX >= distY) ? "X" : "Y";
                }
                LOG("[CTP] winner=%s target=(%d,%d,%d)", winner, targetX, targetY, targetZ);
            }

            if (distX < distZ) {
                if (distZ >= distY) {
                    correctedZ = targetZ;
                    normalZ = pushZ * 0x10000;
                }
                else {
                    correctedY = targetY;
                    normalY = pushY * 0x10000;
                }
            }
            else if (distX >= distY) {
                correctedX = targetX;
                normalX = pushX * 0x10000;
            }
            else {
                correctedY = targetY;
                normalY = pushY * 0x10000;
            }
        }
    }

    const s32 worldX = MulShift16(cosB, correctedX) + MulShift16(sinB, correctedZ);
    const s32 worldZ = MulShift16(-sinB, correctedX) + MulShift16(cosB, correctedZ);

    outPos.x = basisB.x + worldX;
    outPos.y = basisB.y + correctedY;
    outPos.z = basisB.z + worldZ;

    outNormal.x = MulShift16(cosB, normalX) + MulShift16(sinB, normalZ);
    outNormal.y = normalY;
    outNormal.z = MulShift16(-sinB, normalX) + MulShift16(cosB, normalZ);

    outPushedPos.x = outPos.x - MulShift16(radius, outNormal.x);
    outPushedPos.y = basisB.y + (s32)box.maxY;
    outPushedPos.z = outPos.z - MulShift16(radius, outNormal.z);

    CORRECT_THING_POSITION_PUSHABLE_CHEAT = 0;
    return corrected;
}

// PSX: CheckStaticBoxCylinderCollision_Obstacle (OBSTACLE.CPP:328, 0x8007AB90)
static bool CheckStaticBoxCylinderCollision_Obstacle(
    const LVector& obsPos, const tagCollisionBox& box, s32 orientY,
    const LVector& cylPos, const tagCollisionCylinder& cyl) {
    s32 minX = box.minX - cyl.radius;
    s32 maxX = box.maxX + cyl.radius;
    s32 minY = box.minY - cyl.upperY;
    s32 maxY = box.maxY - cyl.lowerY;
    s32 maxZ = box.maxZ + cyl.radius;
    s32 minZ = box.minZ - cyl.radius;

    LVector delta;
    delta.x = cylPos.x - obsPos.x;
    delta.y = cylPos.y - obsPos.y;
    delta.z = cylPos.z - obsPos.z;

    s32 sinV = rmSin16(orientY);
    s32 cosV = rmSin16(orientY + 0x4000);

    s32 localX = (s32)(((s64)cosV * delta.x) >> 16) + (s32)((-(s64)sinV * delta.z) >> 16);
    s32 localZ = (s32)(((s64)sinV * delta.x) >> 16) + (s32)(((s64)cosV * delta.z) >> 16);

    if (localX >= minX && maxX >= localX &&
        delta.y >= minY && maxY >= delta.y &&
        localZ >= minZ && maxZ >= localZ) {
        return true;
    }
    return false;
}

// PSX: GetXZStaticBoxCylinderCollisionSortDistance (OBSTACLE.CPP:241, 0x8007A970)
static s32 GetXZStaticBoxCylinderCollisionSortDistance(
    const LVector& obsPos, const tagCollisionBox& box, s32 orientY,
    const LVector& testPos) {
    LVector delta;
    delta.x = testPos.x - obsPos.x;
    delta.y = testPos.y - obsPos.y;
    delta.z = testPos.z - obsPos.z;

    s32 sinV = rmSin16(orientY);
    s32 cosV = rmSin16(orientY + 0x4000);

    s32 localX = (s32)(((s64)cosV * delta.x) >> 16) + (s32)((-(s64)sinV * delta.z) >> 16);
    s32 localZ = (s32)(((s64)sinV * delta.x) >> 16) + (s32)(((s64)cosV * delta.z) >> 16);

    s32 dist = 0;
    if (localX < box.minX) {
        dist = box.minX - localX;
    }
    else if (localX > box.maxX) {
        dist = localX - box.maxX;
    }
    if (localZ < box.minZ) {
        dist += box.minZ - localZ;
    }
    else if (localZ > box.maxZ) {
        dist += localZ - box.maxZ;
    }
    return dist;
}

struct ObstaclePair {
    s32 distance;
    Obstacle* obstacle;
};

static ObstaclePair pairArray[8];

// PSX: Obstacle::HandleHumanoidObstacleCollision (OBSTACLE.CPP:1301, 0x8007C178)
void Obstacle::HandleHumanoidObstacleCollision(Humanoid* hum) {
    MARKFUNCTION(0x8007C178);

    if (!g_ai) {
        return;
    }

    DynamicThing* dt = (DynamicThing*)hum;

    s32 wasOnObstacle = (hum->flags >> 12) & 1;
    s32 bestFloorHeight = (s32)0x80000001;

    Thing* prevIssuer = dt->GetTicketIssuer();

    // Get bone 5 world position from animation matrices
    LVector bonePos = {};
    HumanoidModel* hmodel = (HumanoidModel*)hum->model;
    if (hmodel && hmodel->animMatrices) {
        s32* mat = AnimationMatrices::GetMatrix(hmodel->animMatrices, 5);
        if (mat) {
            bonePos.x = mat[5];
            bonePos.y = mat[6];
            bonePos.z = mat[7];
        }
    }

    // Foot-level cylinder (radius=0, with humanoid Y extents)
    tagCollisionCylinder footCyl;
    footCyl.radius = 0;
    footCyl.lowerY = hum->collBboxMin.y;
    footCyl.upperY = hum->collBboxMin.z;

    // Humanoid collision cylinder
    tagCollisionCylinder humCyl;
    humCyl.radius = hum->collBboxMin.x;
    humCyl.lowerY = hum->collBboxMin.y;
    humCyl.upperY = hum->collBboxMin.z;

    // Disembark if currently on something and not in an exempt state
    if (prevIssuer) {
        s32 exempt = 0;
        s32 state = hum->actionState;
        if (state == 36 || state == 59 || state == 37 ||
            state == 38 || state == 60 || state == 61 || state == 62) {
            exempt = 1;
        }
        if (!exempt) {
            dt->Disembark();
        }
    }

    s32 pairCount = 0;

    for (ccMinNode* node = g_ai->moveList.head; node != nullptr;) {
        ccMinNode* next = node->next;
        Obstacle* obs = static_cast<Obstacle*>((Thing*)node);

        if (obs->flags & TF_MODEL_CREATED) {
            // Broad-phase: extent + cylinder radius
            s32 threshold = (s32)obs->collBox.extent + humCyl.radius;

            s32 dx = dt->homePos.x - obs->pos.x;
            if (dx < 0) dx = -dx;

            s32 dz = dt->homePos.z - obs->pos.z;
            if (dz < 0) dz = -dz;

            if (dx < threshold && dz < threshold) {
                s32 physical = obs->GetPhysical();

                // Y range check
                s32 dy = dt->homePos.y - obs->pos.y;
                s32 yLow = (s32)obs->collBox.minY - humCyl.upperY;
                s32 yHigh = (s32)obs->collBox.maxY - humCyl.lowerY;
                bool yPass = (dy >= yLow && yHigh >= dy);

                if (CheckXZStaticBoxCylinderCollision(
                    obs->pos, obs->collBox, obs->orientation.y,
                    dt->homePos, humCyl)) {
                    // Floor height from physical obstacles
                    if (physical) {
                        if (CheckXZStaticBoxCylinderCollision(
                            obs->pos, obs->collBox, obs->orientation.y,
                            bonePos, footCyl)) {
                            s32 floorH = obs->GetObstacleFloorHeight(bonePos);
                            if (bestFloorHeight < floorH &&
                                floorH < dt->homePos.y + humCyl.upperY) {
                                bestFloorHeight = floorH;
                            }
                        }
                    }

                    // Distance-sorted pair insertion for HandleHumanoidCollision
                    if (yPass) {
                        s32 sortDist = GetXZStaticBoxCylinderCollisionSortDistance(
                            obs->pos, obs->collBox, obs->orientation.y,
                            hum->pos);

                        if (pairCount < 8) {
                            s32 insertIdx = pairCount;
                            if (pairCount > 0) {
                                s32 j = pairCount;
                                while (j > 0) {
                                    if (sortDist >= pairArray[j - 1].distance) {
                                        break;
                                    }
                                    pairArray[j] = pairArray[j - 1];
                                    j--;
                                }
                                insertIdx = j;
                            }
                            pairArray[insertIdx].distance = sortDist;
                            pairArray[insertIdx].obstacle = obs;
                            pairCount++;
                        }
                    }
                }
            }
        }

        node = next;
    }

    // Process closest obstacle first, then re-check rest
    if (pairCount > 0) {
        pairArray[0].obstacle->HandleHumanoidCollision(hum);

        for (s32 i = 1; i < pairCount; i++) {
            Obstacle* obs = pairArray[i].obstacle;
            if (CheckStaticBoxCylinderCollision_Obstacle(
                obs->pos, obs->collBox, obs->orientation.y,
                dt->homePos, humCyl)) {
                obs->HandleHumanoidCollision(hum);
            }
        }
    }

    hum->SetFloorHeight(bestFloorHeight);

    Thing* curIssuer = dt->GetTicketIssuer();
    if (prevIssuer) {
        if (!curIssuer) {
            const LVector* obsPos = ((Obstacle*)prevIssuer)->GetDeltaVelocity();
            dt->DisembarkObstacle(*obsPos);
            hum->LetGoOfLedge();
        }
        else {
            // PSX: vtable+80 = HandleLand(homePos.y) when transitioning to obstacle
            if (!wasOnObstacle) {
                hum->HandleLand(dt->homePos.y);
            }
            s32 mat = ((Obstacle*)curIssuer)->GetFloorMaterial();
            hum->field436 = mat;
            hum->groundStandHeight = dt->homePos.y;
        }
    }
    else {
        if (curIssuer) {
            // PSX: vtable+80 = HandleLand(homePos.y) when transitioning to obstacle
            if (!wasOnObstacle) {
                hum->HandleLand(dt->homePos.y);
            }
            s32 mat = ((Obstacle*)curIssuer)->GetFloorMaterial();
            hum->field436 = mat;
            hum->groundStandHeight = dt->homePos.y;
        }
    }
}

// Obstacle animation table (PSX: g_animTable at 0x800E0E58, g_animCount at GP+0xF38)
#include "gen/animmgr.h"

static constexpr s32 MAX_OBSTACLE_ANIMS = 32;
static MiscAnimNode* g_obstacleAnimTable[MAX_OBSTACLE_ANIMS] = {};
static s32 g_obstacleAnimCount = 0;

static u32 ObstacleReadU32LE(const u8* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

// PSX: GetAnimation__8Obstaclelllll → 0x8007CB88 (OBSTACLE.CPP:1803)
MiscAnimNode* Obstacle_GetAnimation(s32 animIndex) {
    MARKFUNCTION(0x8007CB88);
    if (animIndex < 0 || g_obstacleAnimCount <= animIndex) {
        return nullptr;
    }
    return g_obstacleAnimTable[animIndex];
}

// PSX: ClearPetalAnimList__8Obstacle → 0x8007CB5C (OBSTACLE.CPP:1797)
void Obstacle_ClearPetalAnimList() {
    MARKFUNCTION(0x8007CB5C);
    for (s32 i = MAX_OBSTACLE_ANIMS - 1; i >= 0; i--) {
        g_obstacleAnimTable[i] = nullptr;
    }
    g_obstacleAnimCount = 0;
}

// PSX: AddAnimation portion of Obstacle::Load (0x8007CAA4)
void Obstacle_AddAnimation(MiscAnimNode* node) {
    if (g_obstacleAnimCount >= MAX_OBSTACLE_ANIMS) {
        return;
    }
    g_obstacleAnimTable[g_obstacleAnimCount] = node;
    g_obstacleAnimCount++;
}

// PSX: Load__8ObstacleR10tReadChunkPPv (0x8007CAA4)
// Chunk payload is two u32 values read from tFile::GetLong(); first is lookup hash.
void Obstacle_LoadAnimChunk(const u8* body, u32 bodySize) {
    MARKFUNCTION(0x8007CAA4);

    if (!body || bodySize < 8) {
        return;
    }

    u32 animHash = ObstacleReadU32LE(body + 0);
    (void)ObstacleReadU32LE(body + 4); // PSX reads and ignores second long here.

    MiscAnimNode* misc = nullptr;
    if (g_animMgr) {
        misc = g_animMgr->GetMiscAnim(animHash);
    }

    if (misc) {
        Obstacle_AddAnimation(misc);
        return;
    }

    MiscAnimNode* effectAnim = nullptr;
    if (GEffect_FindEffectAnim(animHash, &effectAnim)) {
        Obstacle_AddAnimation(effectAnim);
    }
}