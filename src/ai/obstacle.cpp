#include "ai/obstacle.h"
#include "ai/humanoid.h"
#include "gen/ai.h"
#include "gen/animmat.h"
#include "gen/config.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/model.h"
#include "p3d/p3dmath.h"

const LVector ZERO_DELTA_VELOCITY = { 0, 0, 0 };

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
        SModel* sm = static_cast<SModel*>(static_cast<Model*>(model));
        sm->posX = pos.x;
        sm->posY = pos.y;
        sm->posZ = pos.z;
        sm->rotX = (u16)orientation.x;
        sm->rotY = (u16)orientation.y;
        sm->rotZ = (u16)orientation.z;
        sm->Show(0);
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
    } else {
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

void Obstacle::HandlePickupCollision(Thing* pickup) {
}

void Obstacle::HandleHumanoidCollision(Humanoid* hum) {
}

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

void Obstacle::HandleAttack(Humanoid* attacker, s32 damageType, s32 damage) {
}

s32 Obstacle::GetFloorMaterial() const {
    MARKFUNCTION(0x8007D0B4);
    return 0;
}

s32 Obstacle::GetObstacleFloorHeight(const LVector& pos) const {
    MARKFUNCTION(0x8007D0CC);
    return 1;
}

s32 Obstacle::GetPhysical() const {
    MARKFUNCTION(0x8007D048);
    // PSX: switch on physicalType
    // 0: return 0  (no floor)
    // 3: return 0  (no floor)
    // default: return non-zero based on type
    if (physicalType == 0 || physicalType == 3) {
        return 0;
    }
    return 1;
}

void Obstacle::SetCollisionBox(const tagCollisionBox& box) {
    MARKFUNCTION(0x8007BE24);
    collBox = box;
    SetCollisionBoxExtent(collBox);
}

// PSX: CheckXZStaticBoxCylinderCollision (OBSTACLE.CPP:193, 0x8007A740)
static bool CheckXZStaticBoxCylinderCollision(
    const LVector& obsPos, const tagCollisionBox& box, s32 orientY,
    const LVector& cylPos, const tagCollisionCylinder& cyl)
{
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

// PSX: CheckStaticBoxCylinderCollision_Obstacle (OBSTACLE.CPP:328, 0x8007AB90)
static bool CheckStaticBoxCylinderCollision_Obstacle(
    const LVector& obsPos, const tagCollisionBox& box, s32 orientY,
    const LVector& cylPos, const tagCollisionCylinder& cyl)
{
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
    const LVector& testPos)
{
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
    } else if (localX > box.maxX) {
        dist = localX - box.maxX;
    }
    if (localZ < box.minZ) {
        dist += box.minZ - localZ;
    } else if (localZ > box.maxZ) {
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
                        dt->homePos, humCyl))
                {
                    // Floor height from physical obstacles
                    if (physical) {
                        if (CheckXZStaticBoxCylinderCollision(
                                obs->pos, obs->collBox, obs->orientation.y,
                                bonePos, footCyl))
                        {
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
                    dt->homePos, humCyl))
            {
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
        } else {
            if (!wasOnObstacle) {
                hum->SetFloorHeight(dt->homePos.y);
            }
            s32 mat = ((Obstacle*)curIssuer)->GetFloorMaterial();
            hum->field436 = mat;
            hum->groundStandHeight = dt->homePos.y;
        }
    } else {
        if (curIssuer) {
            if (!wasOnObstacle) {
                hum->SetFloorHeight(dt->homePos.y);
            }
            s32 mat = ((Obstacle*)curIssuer)->GetFloorMaterial();
            hum->field436 = mat;
            hum->groundStandHeight = dt->homePos.y;
        }
    }
}
