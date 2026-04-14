#include "ai/obstacle.h"
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
#include "gen/levelmgr.h"
#include "gen/model.h"
#include "gen/world.h"
#include "p3d/p3dmath.h"
#include "snd/snddrct.h"

#include <cstdio>
#include "pc/log.h"

const LVector ZERO_DELTA_VELOCITY = { 0, 0, 0 };

static const tagCollisionBox INVALID_COLLISION_BOX = {
    0x7FFF, 0x7FFF, 0x7FFF,
    -0x7FFF, -0x7FFF, -0x7FFF,
    -0x7FFF, 0
};

// PSX: FillCollisionBox__8ObstacleR15tagCollisionBoxRC6DBRootUl (OBSTACLE.CPP:530, 0x8007AF6C)
static bool ObstacleFillCollisionBox(tagCollisionBox& box, const DBRoot* root, u32 attribNum) {
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
static void ApplyDoorStandingZExtent(tagCollisionBox& box) {
    box.minZ = (s16)(box.minZ - 128);
    box.maxZ = (s16)(box.maxZ + 1024);
}

static bool CollisionBoxLooksValid(const tagCollisionBox& box) {
    return box.minX <= box.maxX && box.minY <= box.maxY && box.minZ <= box.maxZ;
}

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
    return 1;
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

static inline s32 MulShift16(s32 a, s32 b) {
    return (s32)(((s64)a * b) >> 16);
}

static inline s32 Div2TowardZero(s32 value) {
    return (value + (s32)((u32)value >> 31)) >> 1;
}

static constexpr s32 OBSTACLE_CORRECT_BUFFER = 4;
static constexpr s32 OBSTACLE_YMAX_BUFFER = 0x40;
static constexpr s32 CORRECT_THING_OBSTACLE_BOX_Y_MAX_OFFSET = 9;
static s32 CORRECT_THING_POSITION_PUSHABLE_CHEAT = 0;
static constexpr s32 CORRECT_THING_POSITION_EXTRA = 0x20;

// PSX: CorrectThingPosition__8ObstacleRC10tagLVectorT1llRC15tagCollisionBoxT1T1lllR10tagLVectorR9_RMVECT16T11_ (0x8007B398)
static bool CorrectThingPositionObstacle(
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
    s32 x = value << 16;
    s64 prod = (s64)x * 0xB60B60B7LL;
    s32 hi = (s32)(prod >> 32);
    s32 result = (hi + x) >> 8;
    result -= (x >> 31);
    return result;
}

void TrapDoor::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80017054);
    Obstacle::AnalyzeMesh(root);

    field7C.x = root->pos.x;
    field7C.y = root->pos.y;
    field7C.z = root->pos.z;

    while (field7C.y > 0xFF49) {
        field7C.y -= 1;
    }
    while (field7C.y < 0) {
        field7C.y += 0x10000;
    }

    field88 = field7C;
    pos = field7C;

    DBVolume* vol = dynamic_cast<DBVolume*>(root);
    if (vol) {
        FillCollisionBox(field94, *vol);
    }

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
    if (fieldA4 == 1 && source->thingType == 0) {
        fieldB8 = fieldA4;
    }
}

void TrapDoor::Think() {
    MARKFUNCTION(0x80017360);

    if ((u32)(fieldA4 - 2) >= 2) {
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
    s32 savedZ = pos.z;
    pos.z = field88.z;
    Obstacle::Draw();
    pos.z = savedZ;
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

    if (hum->actionState == 0x17) {
        hum->LetGoOfLedge();
    }
}

void TrapDoor::SetupCollisionBox() {
    MARKFUNCTION(0x80017AE4);

    tagCollisionBox box = collBox;

    s32 spanX = (s32)field94.maxX - (s32)field94.minX;
    s32 sinY = rmSin16(field88.z);
    s32 cosY = rmSin16(field88.z + 0x4000);
    s32 maxY = (s32)field94.maxY;

    box.minX = (s16)(-((s32)(((s64)spanX * cosY) >> 16)) - (s32)(((s64)maxY * sinY) >> 16));
    box.minY = (s16)(-((s32)(((s64)spanX * sinY) >> 16)));
    box.maxY = (s16)(((s64)maxY * cosY) >> 16);

    SetCollisionBox(box);
}

s32 TrapDoor::GetFloorMaterial() const {
    MARKFUNCTION(0x80017CC4);
    return 3;
}

Door::Door(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x8001AB0C);
    closedBox = INVALID_COLLISION_BOX;
    doorState = 0;
    targetCRC = 0;
    killThingsCRC = 0;
    tertiaryModelHash = 0;
    currentOpen = 0;
    cutsceneTriggered = 0;
}

Door::~Door() {
    MARKFUNCTION(0x8001AB8C);
}

void Door::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001ABB4);

    if (!root) {
        return;
    }

    tagCollisionBox localBox = INVALID_COLLISION_BOX;

    // PSX: copy root orientation to drawRot, then baseRotY = drawRot.y
    drawRot.x = root->field40;
    drawRot.y = root->field44;
    drawRot.z = root->field48;
    baseRotY = drawRot.y;

    // PSX: copy drawRot to thing orientation before Obstacle::AnalyzeMesh
    orientation = drawRot;

    Obstacle::AnalyzeMesh(root);

    // PSX: FillCollisionBox(localBox, root, 5) - OBSTACLE.CPP:530 (0x8007AF6C)
    bool geoFound = ObstacleFillCollisionBox(localBox, root, 5);
    LOG("Door::AnalyzeMesh geoFound=%d box=(%d,%d,%d,%d,%d,%d)",
        geoFound, localBox.minX, localBox.minY, localBox.minZ,
        localBox.maxX, localBox.maxY, localBox.maxZ);

    // PSX: copy localBox to closedBox (always, even if INVALID)
    closedBox = localBox;

    // PSX: attrib 6 → openSpeed (degrees → binary angle per frame)
    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6) {
        openSpeed = ((s32)a6->value << 16) / 360;
    } else {
        openSpeed = 1638;
    }

    // PSX: attrib 7 → direction flag (0=subtract, nonzero=add)
    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7) {
        direction = (s32)a7->value;
    } else {
        direction = 0;
    }

    // PSX: ApplyDoorStandingZExtent then SetCollisionBox
    ApplyDoorStandingZExtent(localBox);
    SetCollisionBox(localBox);

    // PSX: attrib 8 → killThingsCRC (hash of string name)
    const DBAttrib* a8 = root->FindAttrib(8);
    if (a8 && a8->strValue) {
        killThingsCRC = (s32)p3dHash(a8->strValue);
    } else {
        killThingsCRC = 0;
    }

    // PSX: attrib 9 → maxOpenDist (degrees → binary angle)
    const DBAttrib* a9 = root->FindAttrib(9);
    if (a9) {
        maxOpenDist = ((s32)a9->value << 16) / 360;
    } else {
        maxOpenDist = 16384;
    }

    // PSX: attrib 10 → targetCRC (hash of string name)
    const DBAttrib* a10 = root->FindAttrib(10);
    if (a10 && a10->strValue) {
        targetCRC = (u32)p3dHash(a10->strValue);
    } else {
        targetCRC = 0;
    }

    // PSX: attrib 11 → secondaryModelHash (hash of string name)
    const DBAttrib* a11 = root->FindAttrib(11);
    if (a11 && a11->strValue) {
        secondaryModelHash = (s32)p3dHash(a11->strValue);
    } else {
        secondaryModelHash = 0;
    }

    // PSX: attrib 12 → tertiaryModelHash (hash of string name)
    const DBAttrib* a12 = root->FindAttrib(12);
    if (a12 && a12->strValue) {
        tertiaryModelHash = (s32)p3dHash(a12->strValue);
    } else {
        tertiaryModelHash = 0;
    }
}

void Door::CreateModel(const char* name) {
    MARKFUNCTION(0x8001AE88);
    Thing::CreateModel(name);
}

void Door::DeleteModel() {
    MARKFUNCTION(0x8001AEA8);
    Thing::DeleteModel();
}

void Door::Reset() {
    MARKFUNCTION(0x8001AEC8);

    // PSX: hub level (7) → state 5, else → state 0
    World* world = g_game ? g_game->GetWorld() : nullptr;
    s32 levelID = world ? world->GetCurLevelID() : 0;
    if (levelID == 7) {
        doorState = 5;
        // PSX: if secondaryModelHash, create GEffect (visual lock effect)
        // Not yet implemented on PC
    } else {
        doorState = 0;
    }

    // PSX: reset animation and state fields
    currentOpen = 0;
    cutsceneTriggered = 0;
    deathCountdown = 3;
    drawRot.y = baseRotY;
}

void Door::Think() {
    MARKFUNCTION(0x8001AF5C);

    World* world = g_game ? g_game->GetWorld() : nullptr;
    s32 levelID = world ? world->GetCurLevelID() : 0;

    if (doorState >= 6) {
        return;
    }

    // PSX: switch(doorState) with jump table for states 0-5
    switch (doorState) {
        case 0:
            // State 0: guarded door - monitor kill target
            DeathCheck();
            break;
        case 1:
        case 2:
        case 4:
            // States 1,2,4: animation states - call Move via vtable
            // PSX: also manages secondary model GEffect on non-hub levels
            Move();
            break;
        case 3:
            // State 3: fully open - model management only (no Move)
            // PSX: hides secondary model GEffect on non-hub levels
            break;
        case 5:
            // State 5: hub-closed - idle, waiting for OpenDoors trigger
            break;
    }
}

void Door::UpdatePosition() {
    MARKFUNCTION(0x8001B058);
}

void Door::Draw() {
    MARKFUNCTION(0x8001B060);
    // PSX: custom draw - copies pos and drawRot to model, then calls Show
    if (model) {
        Model* m = static_cast<Model*>(model);
        m->posX = pos.x;
        m->posY = pos.y;
        m->posZ = pos.z;
        m->rotX = (u16)drawRot.x;
        m->rotY = (u16)drawRot.y;
        m->rotZ = (u16)drawRot.z;
        m->Show(0);
    }
}

void Door::Trigger() {
    MARKFUNCTION(0x8001B0D4);

    // PSX: always set state 2 (opening)
    doorState = 2;

    World* world = g_game ? g_game->GetWorld() : nullptr;
    s32 levelID = world ? world->GetCurLevelID() : 0;
    if (levelID == 7) {
        // Hub level: chain trigger to target door/teleporter
        if (targetCRC != 0 && g_ai) {
            ccNode* target = g_ai->moveList.FindNodeCRC(targetCRC);
            if (target) {
                Thing* targetThing = static_cast<Thing*>(target);
                u16 targetType = targetThing->thingType;
                if (targetType == 470) {
                    // Target is a Door - trigger it
                    Obstacle* obs = static_cast<Obstacle*>(targetThing);
                    obs->Trigger();
                } else if (targetType == 201) {
                    // Target is a Teleporter - TriggerByName with g_player
                    Obstacle* obs = static_cast<Obstacle*>(targetThing);
                    obs->TriggerByName(Player::s_player, nullptr, nullptr);
                }
            }
        }
        // PSX: also manages secondary model via FWEffect::Find/Continue
        // Not yet implemented on PC
    } else {
        // Non-hub: play door open sound
        CSoundDirect::PlayTransient(156, static_cast<void*>(&pos), 0, 0);
    }
}

void Door::Open() {
    MARKFUNCTION(0x8001B1D8);
    // PSX Open__4Door: doorState = 2 and transient SFX 156.
    doorState = 2;
    CSoundDirect::PlayTransient(156, static_cast<void*>(&pos), 0, 0);
}

void Door::Move() {
    MARKFUNCTION(0x8001B210);

    // PSX: animate door based on state
    if (doorState == 2) {
        // Opening: increase currentOpen toward maxOpenDist
        currentOpen += openSpeed;
        if (currentOpen >= maxOpenDist) {
            currentOpen = maxOpenDist;
            doorState = 3; // Fully open
        }
    } else if (doorState == 4) {
        // Closing: decrease currentOpen toward 0
        currentOpen -= openSpeed;
        if (currentOpen <= 0) {
            currentOpen = 0;
            doorState = 5; // Closed (hub-idle state)
            // PSX: play close sound on non-hub levels
            World* closeWorld = g_game ? g_game->GetWorld() : nullptr;
            s32 levelID = closeWorld ? closeWorld->GetCurLevelID() : 0;
            if (levelID != 7) {
                CSoundDirect::PlayTransient(157, static_cast<void*>(&pos), 0, 3000);
            }
        }
    }

    // PSX: update drawRotY based on direction flag
    if (direction != 0) {
        drawRot.y = baseRotY + currentOpen;
    } else {
        drawRot.y = baseRotY - currentOpen;
    }
}

void Door::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x8001B43C);
    // PSX: calls Pickup::PlayEffect() then pickup->Kill()
    if (pickup) {
        pickup->Kill();
    }
}

void Door::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8001B47C);

    if (!hum) {
        return;
    }

    // PSX: if cutsceneTriggered == 1, return (don't push during cutscene)
    if (cutsceneTriggered == 1) {
        return;
    }

    // PSX: check if humanoid is a valid player for cutscene triggering
    s32 isValidPlayer = 0;
    if (hum->thingType == 0) {
        // Player type - check actionState (1, 2, 3, 10, 11 are valid)
        s32 as = hum->actionState;
        if (as == 1 || as == 2 || as == 3 || as == 10 || as == 11) {
            // Check combat bits (bit 7 or bit 15 of commandBits)
            s32 bits = hum->commandBits;
            if (((bits >> 7) & 1) || ((bits >> 15) & 1)) {
                isValidPlayer = 1;
            }
        }
    }

    if (isValidPlayer && doorState == 1) {
        // PSX: state 1 + valid player → trigger door cutscene via Director
        if (g_director) {
            // PSX: if killTarget exists, use NISdoor1WithDialog; else NISdoor1
            s32* doorScript = killTarget
                ? Director::GetNISDoor1WithDialogScript()
                : Director::GetNISDoor1Script();
            g_director->SetCodeSnip(doorScript, this);
        }
        cutsceneTriggered = 1;
        return;
    }

    // PSX: push-back via CorrectThingPosition using closedBox and drawRotY
    LVector correctedPos = {};
    LVector correctionNormal = {};
    LVector correctionPushedPos = {};

    CorrectThingPositionObstacle(
        pos,                    // basisA (door pos)
        pos,                    // basisB (door pos)
        drawRot.y,              // rotA (current door rotation)
        drawRot.y,              // rotB (current door rotation)
        closedBox,              // collision box
        hum->pos,               // pointA (humanoid current pos)
        hum->homePos,           // pointB (humanoid home pos)
        hum->collBboxMin.x,    // radius
        hum->collBboxMin.y,    // yMinOffset
        hum->collBboxMin.z,    // yMaxOffset
        correctedPos,           // output: corrected pos
        correctionNormal,       // output: normal
        correctionPushedPos);   // output: pushed pos

    // PSX: copy corrected position to humanoid homePos
    hum->homePos = correctedPos;

    // PSX: if doorState == 1 and hum is the player, set flag bit 6
    if (doorState == 1 && hum == (Humanoid*)Player::s_player) {
        hum->field368 |= 0x40;
    }
}

void Door::TeleportPlayer() {
    MARKFUNCTION(0x8001B624);

    // PSX: find target by targetCRC in g_ai->moveList
    // Then call target->HandleHumanoidCollision(g_player)
    if (targetCRC != 0 && g_ai) {
        ccNode* target = g_ai->moveList.FindNodeCRC(targetCRC);
        if (target) {
            Obstacle* obs = static_cast<Obstacle*>(static_cast<Thing*>(target));
            obs->HandleHumanoidCollision(Player::s_player);
        }
    }

    // PSX: clear cutsceneTriggered flag
    cutsceneTriggered = 0;
}

void Door::DeathCheck() {
    MARKFUNCTION(0x8001B2FC);

    s32 isDead = 0;

    // PSX: if killTarget is null, find it by killThingsCRC in g_ai->activeZoneList
    if (!killTarget) {
        if (killThingsCRC != 0 && g_ai) {
            killTarget = static_cast<Thing*>(g_ai->activeZoneList.FindNodeCRC((u32)killThingsCRC));
        }
        if (!killTarget) {
            isDead = 1; // Target not found → consider dead
        }
    } else {
        // PSX: check if ActiveZone still has thinking (alive) members
        // 0x800A6E30 = ActiveZone::GetNumberOfThinkingMembers
        ActiveZone* az = static_cast<ActiveZone*>(static_cast<ccNode*>(killTarget));
        s32 thinkingCount = 0;
        for (s32 i = 0; i < az->memberCount; i++) {
            if (az->members[i] != nullptr) {
                thinkingCount++;
            }
        }

        if (thinkingCount > 0) {
            // Still alive - reset countdown
            deathCountdown = 3;
        } else {
            // No thinking members - decrement countdown
            deathCountdown--;
            if (deathCountdown <= 0) {
                isDead = 1;
            }
        }
    }

    if (isDead) {
        // PSX: if level != 6, set state 1 + SFX 158; if level 6, call Open
        World* deathWorld = g_game ? g_game->GetWorld() : nullptr;
        s32 levelID = deathWorld ? deathWorld->GetCurLevelID() : 0;
        if (levelID == 6) {
            Open();
        } else {
            doorState = 1;
            CSoundDirect::PlayTransient(158, static_cast<void*>(&pos), 0, 0);
        }
        // PSX: hide secondary and tertiary model GEffects
        // Not yet implemented on PC
    }
}

Ladder::Ladder(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x80089CA4);
    ladderFaceAngle = 0;
    hatchEnabled = 1;
    deathCountdown = 3;
    state = 0;
    cutscenePending = 0;
    hatchTriggerCRC = 0;
    hatchCloseCRC = 0;
    teleportTargetCRC = 0;
    deathCheckCRC = 0;
    hatchThing = nullptr;
    deathThing = nullptr;
    hatchYTrigger = 0x7FFFFFFF;
}

Ladder::~Ladder() {
    MARKFUNCTION(0x80089D18);
}

void Ladder::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80089D40);
    Obstacle::AnalyzeMesh(root);

    if (!root) {
        return;
    }

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    DBVolume* vol = dynamic_cast<DBVolume*>(root);
    if (!vol && g_database) {
        for (DBVolume* dbv = g_database->GetFirstVolume(); dbv; dbv = static_cast<DBVolume*>(dbv->next)) {
            if (dbv->nameCRC == root->nameCRC) {
                vol = dbv;
                break;
            }
        }
    }

    if (vol) {
        tagCollisionBox box = {};
        FillCollisionBox(box, *vol);
        box.maxY = (s16)(box.maxY + 256);
        box.maxZ = (s16)(box.maxZ + 384);
        SetCollisionBox(box);
    }

    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6) {
        ladderFaceAngle = ((s32)a6->value << 16) / 360 + 0x8000;
    }

    hatchEnabled = root->FindAttrib(7) ? 1 : 0;

    char pointNameBuf[32] = {};

    const DBAttrib* a8 = root->FindAttrib(8);
    hatchTriggerCRC = 0;
    if (a8) {
        hatchTriggerCRC = (s32)p3dHash(a8->strValue);
    }

    const DBAttrib* a9 = root->FindAttrib(9);
    teleportTargetCRC = 0;
    if (a9) {
        teleportTargetCRC = (s32)p3dHash(a9->strValue);
    }

    const DBAttrib* a10 = root->FindAttrib(10);
    hatchCloseCRC = 0;
    if (a10) {
        hatchCloseCRC = (s32)p3dHash(a10->strValue);
    }

    const DBAttrib* a11 = root->FindAttrib(11);
    deathCheckCRC = 0;
    if (a11) {
        deathCheckCRC = (s32)p3dHash(a11->strValue);
    }

    const DBAttrib* a12 = root->FindAttrib(12);
    hatchYTrigger = a12 ? (s32)a12->value : 0x7FFFFFFF;
}

void Ladder::CreateModel(const char* /*name*/) {
    MARKFUNCTION(0x80089F48);
    flags |= TF_MODEL_CREATED;
}

void Ladder::DeleteModel() {
    MARKFUNCTION(0x80089F5C);
    flags &= ~TF_MODEL_CREATED;
}

void Ladder::Reset() {
    MARKFUNCTION(0x80089F70);
    World* world = g_game ? g_game->GetWorld() : nullptr;
    s32 levelID = world ? world->GetCurLevelID() : 0;
    state = (teleportTargetCRC != 0 && levelID == 7) ? 1 : 0;
    cutscenePending = 0;
    deathCountdown = 3;
}

void Ladder::Think() {
    MARKFUNCTION(0x80089FD4);
    if (state == 0) {
        DeathCheck();
    }
}

void Ladder::Move() {
    MARKFUNCTION(0x8008A068);
}

void Ladder::UpdatePosition() {
    MARKFUNCTION(0x8008A10C);
}

void Ladder::Draw() {
    MARKFUNCTION(0x8008A114);
}

void Ladder::Trigger() {
    MARKFUNCTION(0x8008A11C);

    hatchThing = nullptr;
    if (hatchTriggerCRC != 0 && g_ai) {
        ccNode* n = g_ai->moveList.FindNodeCRC((u32)hatchTriggerCRC);
        if (n) {
            hatchThing = static_cast<Thing*>(n);
        }
    }

    if (hatchThing) {
        static_cast<Obstacle*>(hatchThing)->TriggerByName(nullptr, nullptr, nullptr);
        state = 3;
    }
    else {
        state = 2;
    }
}

void Ladder::TeleportPlayer() {
    MARKFUNCTION(0x8008A19C);

    if (teleportTargetCRC == 0 || !g_ai) {
        return;
    }

    ccNode* n = g_ai->moveList.FindNodeCRC((u32)teleportTargetCRC);
    if (n) {
        static_cast<Obstacle*>(static_cast<Thing*>(n))->Trigger();
    }
}

void Ladder::CloseHatch() {
    MARKFUNCTION(0x8008A1F4);

    if (hatchCloseCRC != 0 && g_ai) {
        ccNode* n = g_ai->moveList.FindNodeCRC((u32)hatchCloseCRC);
        if (n) {
            static_cast<Obstacle*>(static_cast<Thing*>(n))->TriggerByName(nullptr, nullptr, nullptr);
        }
    }

    cutscenePending = 0;
}

s32 Ladder::DeathCheck() {
    MARKFUNCTION(0x8008A070);

    if (!deathThing && deathCheckCRC != 0 && g_ai) {
        ccNode* n = g_ai->moveList.FindNodeCRC((u32)deathCheckCRC);
        if (n) {
            deathThing = static_cast<Thing*>(n);
        }
    }

    if (!deathThing || !(deathThing->flags & TF_ACTIVATED)) {
        deathCountdown--;
        if (deathCountdown <= 0) {
            Kill();
        }
    }
    else {
        deathCountdown = 3;
    }

    return deathCountdown;
}

void Ladder::HandlePickupCollision(Thing* /*pickup*/) {
    MARKFUNCTION(0x8008A258);
}

void Ladder::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x8008A260);

    if (!hum) return;

    if (state != 2) return;
    if (cutscenePending == 1) return;

    LVector savedHomePos = hum->homePos;

    s32 topY = pos.y + (s32)collBox.minY;
    s32 bottomY = pos.y + (s32)collBox.maxY;
    s32 maxZ = pos.z + (s32)collBox.maxZ;

    s32 as = hum->actionState;

    // PSX 0x8008A2E4-0x8008A338: states 25/26 just set flag and return
    if (as == AS_LADDER_CLIMB_UP || as == AS_LADDER_CLIMB_DOWN) {
        hum->field368 |= 2;
        return;
    }

    // PSX 0x8008A33C: state 27 (climbing)
    if (as == AS_LADDER_CLIMBING) {
        hum->field368 |= 2;
        hum->groundStandHeight = hum->pos.y;

        if (teleportTargetCRC != 0) {
            if (savedHomePos.y >= hatchYTrigger) {
                if (g_director) {
                    g_director->SetCodeSnip(Director::GetNISLadder1Script(), this);
                }
                cutscenePending = 1;
            }
        }

        // PSX 0x8008A390: if above top, clamp Y and restore homePos
        if (savedHomePos.y < topY) {
            savedHomePos.y = topY;
            hum->homePos = savedHomePos;
            PutHumanoidOnLadder(hum);
            return;
        }

        // PSX 0x8008A3AC: if near bottom (savedHomePos.y > bottomY)
        if (bottomY < savedHomePos.y) {
            if ((hum->commandBits >> 2) & 1) {
                savedHomePos.y = bottomY;
                if (hatchThing) {
                    // PSX calls Ladder::CheckForLedges here (not implemented on PC)
                    // Without it, fall through to restore path
                    hum->homePos = savedHomePos;
                    PutHumanoidOnLadder(hum);
                    return;
                }
            }
        }

        // PSX: middle section or near-bottom without ledge — no homePos restore
        PutHumanoidOnLadder(hum);
        return;
    }

    // PSX state 73 (0x49): return
    if (as == 73) return;

    // Default: player approaching ladder — check for grab input
    hum->field368 |= 2;

    s32 grabButton = 0;
    s32 bits = hum->commandBits;
    if (((bits >> 7) & 1) || ((bits >> 15) & 1)) {
        grabButton = 1;
    }
    if (!grabButton) return;

    // PSX 0x8008A490: top approach check uses pos.y
    if (hum->pos.y > bottomY - 256) {
        if (hum != (Humanoid*)Player::s_player) return;
        // PSX calls Ladder::CheckForLedges here (not implemented on PC)
        // Without it, top-approach climb-down is disabled
        return;
    }

    // PSX 0x8008A50C: bottom approach uses pos.y
    if (hum->pos.y < topY) return;
    if (savedHomePos.z > maxZ - 384) return;

    PutHumanoidOnLadder(hum);
    hum->SetActionState(AS_LADDER_CLIMB_UP, 0);
}

// PSX: PutHumanoidOnLadder__6LadderP8Humanoid (0x8008A570)
// Aligns humanoid X/Z to ladder, sets facing angle
void Ladder::PutHumanoidOnLadder(Humanoid* hum) {
    // PSX: hum->orientation.y = ladder->ladderFaceAngle
    hum->orientation.y = ladderFaceAngle;

    // PSX: set hum X/Z to ladder X/Z, keep hum Y
    hum->homePos.x = pos.x;
    hum->homePos.z = pos.z;
}

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
}

void Teleporter::CreateModel(const char* name) {
    MARKFUNCTION(0x800AA618);
    Thing::CreateModel(name);
}

void Teleporter::DeleteModel() {
    MARKFUNCTION(0x800AA62C);
    Thing::DeleteModel();
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
        return;
    }

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
