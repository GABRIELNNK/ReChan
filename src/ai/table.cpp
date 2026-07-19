#include "ai/table.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "gen/common.h"
#include "gen/database.h"
#include "gen/levelmgr.h"
#include "gen/colsect.h"
#include "gen/colvol.h"
#include "gen/fxp.h"
#include "gen/geffect.h"
#include "gen/scoremgr.h"
#include "p3d/p3dmath.h"
#include "p3d/hash.h"
#include "snd/snddrct.h"

static constexpr s32 COLLISION_TAG_IMPACT_REGION = static_cast<s32>(0x80000002u);
static constexpr s32 COLLISION_TAG_HIT_TYPE = static_cast<s32>(0x80000003u);
static constexpr s32 COLLISION_TAG_FORCE = static_cast<s32>(0x80000005u);
static constexpr s32 COLLISION_TAG_DAMAGE = static_cast<s32>(0x80000007u);
static constexpr s32 COLLISION_TAG_END = 0;

static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_FIELD = 5461;
static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_VEL_Y_MIN = 21;
static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_FORCE = 10000;
static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_HIT_TYPE = 17;
static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_DAMAGE = 50;
static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_FORCE_SCALE = 1000;
static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_IMPACT_REGION = 4;
static constexpr s32 DYNAMIC_OBSTACLE_INTERACTION_TOP_Y_BIAS = 100;

static s32 Fixed16HighToS32(s32 value) {
    return (s32)(s16)((u32)value >> 16);
}

DynamicObstacle::DynamicObstacle(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80013F24);
    field168 = -10;
    effectHash = 0;
    aliveFlag = 1;
}

DynamicObstacle::~DynamicObstacle() {
    MARKFUNCTION(0x80013FD0);
}

void DynamicObstacle::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80013FFC);

    // Copy orientation from DBRoot (fields 40,44,48)
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    Obstacle::AnalyzeMesh(root);

    // Fill collision box from attrib 5 (geo name)
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);

    maxHealth = 1;

    const DBAttrib* a20 = root->FindAttrib(20);
    if (a20 && a20->strValue) {
        effectHash = p3dHash(a20->strValue);
        effectParam = root->FindAttrib(21) ? 0x80000000u : 0u;
    } else {
        // PSX default: 106729104 = p3dHash of default effect name
        effectHash = 106729104u;
        effectParam = 0x80000000u;
    }
}

void DynamicObstacle::CreateModel(const char* name) {
    MARKFUNCTION(0x80014120);
    Obstacle::CreateModel(name);
}

void DynamicObstacle::DeleteModel() {
    MARKFUNCTION(0x80014140);
    Obstacle::DeleteModel();
}

void DynamicObstacle::Reset() {
    MARKFUNCTION(0x80014148);
    aliveFlag = 1;
    kickFlag = 0;
}

void DynamicObstacle::Think() {
    MARKFUNCTION(0x80014158);
    Move();
    MovePassengersBasic();
}

void DynamicObstacle::Move() {
    MARKFUNCTION(0x80014198);

    LVector clearedForce = {};
    const LVector prevPos = pos;
    const LVector prevPosForCollision = pos;

    // PSX: orientation integrates angular velocity each frame.
    orientation.x += angVelX;
    orientation.y += angVelY;
    orientation.z += angVelZ;

    // PSX: applies per-frame vertical force bias via stateCounter * field168.
    forceY += stateCounter * field168;

    s32 forceDeltaX = 0;
    s32 forceDeltaY = 0;
    s32 forceDeltaZ = 0;

    s32 absForce = forceX;
    if (absForce < 0) {
        absForce = -absForce;
    }
    if (absForce >= 6) {
        forceDeltaX = Fixed16HighToS32(rmDiv16i(forceX, stateCounter));
    }

    absForce = forceY;
    if (absForce < 0) {
        absForce = -absForce;
    }
    if (absForce >= 6) {
        forceDeltaY = Fixed16HighToS32(rmDiv16i(forceY, stateCounter));
    }

    absForce = forceZ;
    if (absForce < 0) {
        absForce = -absForce;
    }
    if (absForce >= 6) {
        forceDeltaZ = Fixed16HighToS32(rmDiv16i(forceZ, stateCounter));
    }

    linVelX += forceDeltaX;
    linVelY += forceDeltaY;
    linVelZ += forceDeltaZ;

    pos.x = prevPos.x + linVelX;
    pos.y = prevPos.y + linVelY;
    pos.z = prevPos.z + linVelZ;

    HandleEnvironmentCollision(prevPosForCollision);

    forceX = clearedForce.x;
    forceY = clearedForce.y;
    forceZ = clearedForce.z;
}

void DynamicObstacle::Draw() {
    MARKFUNCTION(0x80014484);

    if (aliveFlag) {
        Obstacle::Draw();
    }
}

void DynamicObstacle::AddForce(s32 damage, const LVector* matrix) {
    MARKFUNCTION(0x800144B4);

    if (!matrix) {
        return;
    }

    Mat4 rotMatrix;
    // PSX reads these as s16 lanes from a packed RMVECT16 pointer
    // (a3[0], a3[2], a3[4]), i.e. low 16 bits of x/y/z.
    const u16 rotX = static_cast<u16>(matrix->x & 0xFFFF);
    const u16 rotY = static_cast<u16>(matrix->y & 0xFFFF);
    const u16 rotZ = static_cast<u16>(matrix->z & 0xFFFF);
    p3dBuildRotMatrixZYX(rotX, rotY, rotZ, rotMatrix);

    const Vec3 rotated = p3dVecTimesRotMatrix(Vec3(0.0f, 0.0f, (f32)damage), rotMatrix);
    forceX += (s32)rotated.x;
    forceY += (s32)rotated.y;
    forceZ += (s32)rotated.z;
}

void DynamicObstacle::AddMomentVector(const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x8001456C);

    LVector relative = {};
    relative.x = contactPos.x - pos.x;
    relative.y = contactPos.y - pos.y;
    relative.z = contactPos.z - pos.z;

    const s32 absX = (relative.x < 0) ? -relative.x : relative.x;
    const s32 absZ = (relative.z < 0) ? -relative.z : relative.z;

    LVector projected = {};
    if (absX >= absZ) {
        projected.x = (s32)((u32)relative.x << 16);
    }
    else {
        projected.z = (s32)((u32)relative.z << 16);
    }

    momentX = (s32)(((s64)projected.y * (s64)matrix.z - (s64)projected.z * (s64)matrix.y) >> 16);
    momentY = (s32)(((s64)projected.z * (s64)matrix.x - (s64)projected.x * (s64)matrix.z) >> 16);
    momentZ = (s32)(((s64)projected.x * (s64)matrix.y - (s64)projected.y * (s64)matrix.x) >> 16);

    angVelX = (s32)((6553LL * (s64)momentX) >> 16);
    angVelY = (s32)((6553LL * (s64)momentY) >> 16);
    angVelZ = (s32)((6553LL * (s64)momentZ) >> 16);
}

void DynamicObstacle::MovePassengers() {
    MARKFUNCTION(0x80014738);
    MovePassengersBasic();
}

void DynamicObstacle::Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x80014758);

    LVector impulse = {};
    impulse.y += b;

    AddForce(a, &matrix);

    forceX += impulse.x;
    forceY += impulse.y;
    forceZ += impulse.z;

    AddMomentVector(impulse, contactPos);
}

void DynamicObstacle::UpdatePosition() {
    MARKFUNCTION(0x80014818);
}

void DynamicObstacle::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80014820);
}

void DynamicObstacle::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80014828);

    LVector correctedPos = {};
    LVector correctionNormal = {};
    LVector correctionPushedPos = {};

    CorrectThingPositionObstacle(
        pos,
        pos,
        orientation.y,
        orientation.y,
        collBox,
        hum->pos,
        hum->homePos,
        hum->collBboxMin.x,
        hum->collBboxMin.y,
        hum->collBboxMin.z,
        correctedPos,
        correctionNormal,
        correctionPushedPos);

    if (correctionNormal.y > 0 && hum->velocity.y <= 0) {
        hum->SetFloorHeight(pos.y + static_cast<s32>(collBox.maxY));
        AddPassenger(hum);
    }

    hum->homePos = correctedPos;
}

void DynamicObstacle::HandleObjectInterAction(Humanoid* hum) {
    MARKFUNCTION(0x80014934);

    const s32 inField = IsPointInFieldOf(
        pos,
        hum->pos,
        hum->orientation.y,
        DYNAMIC_OBSTACLE_INTERACTION_FIELD,
        DYNAMIC_OBSTACLE_INTERACTION_FIELD);

    s32 absVelY = linVelY;
    if (absVelY < 0) {
        absVelY = -absVelY;
    }

    if (absVelY >= DYNAMIC_OBSTACLE_INTERACTION_VEL_Y_MIN) {
        if (lastHumanoid != hum) {
            hum->HandleCollision(
                this,
                1,
                COLLISION_TAG_IMPACT_REGION,
                DYNAMIC_OBSTACLE_INTERACTION_IMPACT_REGION,
                COLLISION_TAG_FORCE,
                DYNAMIC_OBSTACLE_INTERACTION_FORCE,
                COLLISION_TAG_HIT_TYPE,
                DYNAMIC_OBSTACLE_INTERACTION_HIT_TYPE,
                COLLISION_TAG_DAMAGE,
                DYNAMIC_OBSTACLE_INTERACTION_DAMAGE,
                COLLISION_TAG_END);

            if (hum->thingType != 0 && g_scoreManager) {
                g_scoreManager->AddStylePoints(100);
            }

            hum->contactForce.x += static_cast<s32>((static_cast<s64>(DYNAMIC_OBSTACLE_INTERACTION_FORCE_SCALE) * linVelX) >> 16);
            hum->contactForce.y += static_cast<s32>((static_cast<s64>(DYNAMIC_OBSTACLE_INTERACTION_FORCE_SCALE) * linVelY) >> 16);
            hum->contactForce.z += static_cast<s32>((static_cast<s64>(DYNAMIC_OBSTACLE_INTERACTION_FORCE_SCALE) * linVelZ) >> 16);

            if (kickFlag) {
                Destroy();
                return;
            }
        }
        return;
    }

    s32 canEnterPush = 0;
    if ((((hum->commandBits >> 7) & 1) != 0 || ((hum->commandBits >> 15) & 1) != 0)
        && hum->actionState != AS_PUSH
        && hum->pos.y < pos.y + static_cast<s32>(collBox.maxY) - DYNAMIC_OBSTACLE_INTERACTION_TOP_Y_BIAS) {
        canEnterPush = 1;
    }

    if (canEnterPush != 0 && inField != 0) {
        lastHumanoid = hum;
        hum->field496 = this;
        hum->SetActionState(AS_PUSH, 0);
        kickFlag = 1;
    }
}

void DynamicObstacle::Destroy() {
    MARKFUNCTION(0x80014BF8);

    if (!aliveFlag) {
        return;
    }

    if (effectHash) {
        GEffect_Create(effectHash, &pos, nullptr, nullptr, 0, 0, effectParam);
    }

    CSoundDirect::PlayTransient(177, &pos, 0, 0);

    SetCollisionBox(INVALID_COLLISION_BOX);
    aliveFlag = 0;
    health = 0;
}

void DynamicObstacle::HandleAttack(Humanoid* attacker, s32 damageType, s32 attackMagnitude, s32 damage) {
    MARKFUNCTION(0x80014CA8);
}

void DynamicObstacle::HandleEnvironmentCollision(const LVector& prevPos) {
    MARKFUNCTION(0x80014CB0);

    const s32 sinY = rmSin16((s16)orientation.y);
    const s32 cosY = rmSin16((s16)(orientation.y + 0x4000));

    const s32 centreX = Div2TowardZero((s32)collBox.minX + (s32)collBox.maxX);
    const s32 centreZ = Div2TowardZero((s32)collBox.minZ + (s32)collBox.maxZ);

    const s32 centreOffsetX =
        (s32)(((s64)cosY * (s64)centreX) >> 16) +
        (s32)(((s64)sinY * (s64)centreZ) >> 16);
    const s32 centreOffsetZ =
        (s32)(((s64)(-sinY) * (s64)centreX) >> 16) +
        (s32)(((s64)cosY * (s64)centreZ) >> 16);

    LVector oldFloorProbePos = prevPos;
    oldFloorProbePos.x += centreOffsetX;
    oldFloorProbePos.z += centreOffsetZ;
    oldFloorProbePos.y += (s32)collBox.minY;

    LVector newFloorProbePos = pos;
    newFloorProbePos.x += centreOffsetX;
    newFloorProbePos.z += centreOffsetZ;
    newFloorProbePos.y += (s32)collBox.minY;

    const s32 halfX = Div2TowardZero((s32)collBox.maxX - (s32)collBox.minX);
    const s32 halfZ = Div2TowardZero((s32)collBox.maxZ - (s32)collBox.minZ);

    const s32 radiusAlongX =
        (s32)(((s64)cosY * (s64)halfX) >> 16) +
        (s32)(((s64)sinY * (s64)halfZ) >> 16);
    const s32 radiusAlongZ =
        (s32)(((s64)(-sinY) * (s64)halfX) >> 16) +
        (s32)(((s64)cosY * (s64)halfZ) >> 16);

    s32 moveDeltaX = newFloorProbePos.x - oldFloorProbePos.x;
    s32 moveDeltaZ = newFloorProbePos.z - oldFloorProbePos.z;
    if (moveDeltaX < 0) {
        moveDeltaX = -moveDeltaX;
    }
    if (moveDeltaZ < 0) {
        moveDeltaZ = -moveDeltaZ;
    }

    s32 collisionRadius = (moveDeltaZ < moveDeltaX) ? radiusAlongX : radiusAlongZ;
    if (collisionRadius < 0) {
        collisionRadius = -collisionRadius;
    }
    collisionRadius -= 2;

    s32 collisionRatio = 0;
    LVector wallNormal = {};
    LVector wallHitPos = {};
    s32 wallHorizontal = 0;
    const bool hitWall = CollisionSector::CheckWorldWallCollision(
        oldFloorProbePos,
        newFloorProbePos,
        collisionRadius,
        (s32)collBox.minY,
        (s32)collBox.maxY,
        collisionRatio,
        wallNormal,
        wallHitPos,
        wallHorizontal) != 0;

    LVector resolvedFloorProbePos = wallHitPos;
    if (!hitWall) {
        resolvedFloorProbePos.x = newFloorProbePos.x;
        resolvedFloorProbePos.z = newFloorProbePos.z;
    }

    if (hitWall && kickFlag) {
        Destroy();
    }

    const s32 oldFloorHeight = g_collisionSectors[0].GetWorldFloorHeight(oldFloorProbePos, collisionRadius);
    const s32 newFloorHeight = g_collisionSectors[0].GetWorldFloorHeight(newFloorProbePos, collisionRadius);

    static constexpr s32 INVALID_FLOOR_HEIGHT = (s32)0x80000001;
    const bool steppedOffFloor =
        oldFloorHeight != INVALID_FLOOR_HEIGHT && newFloorHeight < oldFloorHeight;

    if (steppedOffFloor) {
        resolvedFloorProbePos.y = oldFloorHeight;

        linVelX = Div2TowardZero(linVelX);
        linVelY /= -2;
        linVelZ = Div2TowardZero(linVelZ);

        angVelX = 0;
        angVelY = 0;
        angVelZ = 0;

        orientation.x = 0;
        orientation.y = 0;
        orientation.z = 0;

        if (linVelY >= 21 && kickFlag) {
            Destroy();
        }
    }
    else {
        resolvedFloorProbePos.y = newFloorProbePos.y;
    }

    resolvedFloorProbePos.x -= centreOffsetX;
    resolvedFloorProbePos.z -= centreOffsetZ;
    resolvedFloorProbePos.y -= (s32)collBox.minY;

    pos = resolvedFloorProbePos;
}

bool DynamicObstacle::CareAboutAttack() const {
    MARKFUNCTION(0x80015470);
    return false;
}


Table::Table(const LVector* pos, u16 type)
    : DynamicObstacle(pos, type) {
    MARKFUNCTION(0x80015190);
    aliveFlag = 1;
}

Table::~Table() {
    MARKFUNCTION(0x800151C8);
}

void Table::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800151F0);
    DynamicObstacle::AnalyzeMesh(root);
}

void Table::CreateModel(const char* name) {
    MARKFUNCTION(0x80015210);
    DynamicObstacle::CreateModel(name);
}

void Table::DeleteModel() {
    MARKFUNCTION(0x80015230);
    DynamicObstacle::DeleteModel();
}

void Table::Think() {
    MARKFUNCTION(0x80015238);
    DynamicObstacle::Think();
}

void Table::UpdatePosition() {
    MARKFUNCTION(0x80015258);
}

void Table::Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x80015260);
    DynamicObstacle::Throw(a, b, matrix, contactPos);
    CSoundDirect::PlayTransient(211, &pos, 0, 0);
}

void Table::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800152A0);
}

void Table::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800152A8);
    DynamicObstacle::HandleHumanoidCollision(hum);
    HandleObjectInterAction(hum);
}

Chair::Chair(const LVector* pos, u16 type)
    : DynamicObstacle(pos, type) {
    MARKFUNCTION(0x8001531C);
    aliveFlag = 1;
}

Chair::~Chair() {
    MARKFUNCTION(0x80015354);
}

void Chair::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001537C);
    DynamicObstacle::AnalyzeMesh(root);
}

void Chair::CreateModel(const char* name) {
    MARKFUNCTION(0x8001539C);
    DynamicObstacle::CreateModel(name);
}

void Chair::DeleteModel() {
    MARKFUNCTION(0x800153BC);
    DynamicObstacle::DeleteModel();
}

void Chair::Think() {
    MARKFUNCTION(0x800153C4);
    DynamicObstacle::Think();
}

void Chair::UpdatePosition() {
    MARKFUNCTION(0x800153E4);
}

void Chair::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800153EC);
}

void Chair::Throw(s32 a, s32 b, const LVector& matrix, const LVector& contactPos) {
    MARKFUNCTION(0x800153F4);
    DynamicObstacle::Throw(a, b, matrix, contactPos);
}

void Chair::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80015434);
    DynamicObstacle::HandleHumanoidCollision(hum);
    HandleObjectInterAction(hum);
}
