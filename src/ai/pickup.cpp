#include "ai/pickup.h"

#include "gen/animmat.h"
#include "gen/database.h"
#include "gen/game.h"
#include "gen/levelmgr.h"
#include "gen/model.h"
#include "gen/world.h"
#include "p3d/hash.h"
#include "p3d/p3dmath.h"
#include "snd/sndfact.h"
#include "snd/wpnsnd.h"

static constexpr s32 PICKUP_THRESHOLD = 50;

struct FightingSystemHashEntry {
    u32 hash = 0;
    u32 rootAddress = 0;
};

struct TypeFightingSystemEntry {
    u16 type = 0;
    u32 hash = 0;
};

struct PsxFightingNodeRaw {
    u32 address = 0;
    u32 packedCommand = 0;
    u32 field04 = 0;
    u32 moveData = 0;
    u32 childAddress = 0;
    u32 siblingAddress = 0;
};

struct PsxFightingMoveRaw {
    u32 address = 0;
    u32 firstWord = 0;
    s32 turnDelta = 0;
    u16 anim = 0;
    u16 fightingPoints = 0;
    u8 stylePointsFlag = 0;
    s8 moveWindowStart = 0;
    s8 moveWindowEnd = 0;
    s8 moveDelta = 0;
    s8 combatWindowStart = 0;
    s8 combatWindowEnd = 0;
    u8 weaponBreakOnEmpty = 0;
    u8 fightingType = 0;
    u32 data20 = 0;
    u32 data24 = 0;
    s16 throwVectorX = 0;
    s16 throwVectorY = 0;
    s16 throwVectorZ = 0;
    s16 throwAttachX = 0;
    s16 throwAttachY = 0;
    s16 throwAttachZ = 0;
    u16 throwTargetAnim = 0;
    s8 throwImpactFrame = 0;
    s8 throwAttachFrame = 0;
    s8 throwReleaseFrame = 0;
    s8 throwScoreFrame = 0;
};

struct PsxFightingJointRaw {
    u32 address = 0;
    u32 word0 = 0;
    u32 word1 = 0;
    u32 word2 = 0;
    u32 word3 = 0;
    u32 word4 = 0;
    u32 word5 = 0;

    s8 AttackStartFrame() const { return static_cast<s8>(word4 & 0xFFu); }
};

#include "ai/fightani_data.inl"
#include "ai/fightmove_data.inl"
#include "ai/fightjoint_data.inl"

struct WeaponTypePickupEntry {
    u16 type = 0;
    u16 idleAnim = 0;
    u32 highMove = 0;
    u32 lowMove = 0;
    u32 throwMove = 0;
};

static const s32 pole1Straif[] = {
    189, 0,
    193, 0,
    193, 1,
    194, 0,
    194, 1,
};

static const s32 pole2Straif[] = {
    206, 0,
    210, 0,
    210, 1,
    211, 0,
    211, 1,
};

static const s32 pole3Straif[] = {
    220, 0,
    224, 0,
    224, 1,
    225, 0,
    225, 1,
};

static const s32 drunkenStraif[] = {
    261, 0,
    264, 0,
    264, 1,
    266, 1,
    266, 0,
};

static const TypeFightingSystemEntry kWeaponTypeFightingSystemTable[] = {
    { 305, 0x0AB660C5u },
    { 302, 0x0B821985u },
    { 301, 0x02B224D5u },
    { 303, 0x05B590B5u },
    { 306, 0x0776A3B5u },
    { 307, 0x0D09E3D5u },
    { 308, 0x04DB1075u },
    { 309, 0x072FA3B5u },
    { 310, 0x0C300C25u },
    { 311, 0x0B5923E5u },
    { 312, 0x09C51505u },
    { 313, 0x02B224D5u },
    { 315, 0x0B822985u },
    { 316, 0x02B304D5u },
    { 317, 0x0B821985u },
    { 318, 0x0B82F985u },
    { 319, 0x0B821985u },
    { 320, 0x0D09E3D5u },
    { 321, 0x02B304D5u },
    { 322, 0x0B82F985u },
    { 323, 0x0D09E3D5u },
    { 324, 0x02B214D5u },
    { 325, 0x02B214D5u },
    { 326, 0x0B822985u },
    { 327, 0x02B214D5u },
    { 328, 0x0B5923E5u },
};

static const WeaponTypePickupEntry kWeaponTypePickupSystemTable[] = {
    { 301, 189, 0x800D0FA8u, 0x800D0F8Cu, 0x800D0FE0u },
    { 309, 487, 0x800D12B8u, 0x800D129Cu, 0x800D12D4u },
    { 310, 500, 0x800D1360u, 0x800D1344u, 0x800D137Cu },
    { 311, 254, 0x800D1408u, 0x800D13ECu, 0x800D1424u },
    { 312, 261, 0x800D14E8u, 0x800D14CCu, 0x800D1504u },
    { 313, 189, 0x800D0FA8u, 0x800D0F8Cu, 0x800D0FE0u },
    { 316, 476, 0x800D11D8u, 0x800D11BCu, 0x800D11A0u },
    { 321, 476, 0x800D11D8u, 0x800D11BCu, 0x800D11A0u },
    { 324, 462, 0x800D10DCu, 0x800D10C0u, 0x800D10A4u },
    { 325, 462, 0x800D10DCu, 0x800D10C0u, 0x800D10A4u },
    { 327, 462, 0x800D10DCu, 0x800D10C0u, 0x800D10A4u },
    { 328, 254, 0x800D1408u, 0x800D13ECu, 0x800D1424u },
};

static const s32* GetMoveStruct(u16 type) {
    MARKFUNCTION(0x8006D3F4);

    switch (type) {
    case 301:
    case 313:
        return pole1Straif;
    case 312:
        return drunkenStraif;
    case 316:
    case 321:
        return pole3Straif;
    case 324:
    case 325:
    case 327:
        return pole2Straif;
    default:
        return nullptr;
    }
}

static u32 FindPickupFightingRootAddress(u32 hash) {
    for (const FightingSystemHashEntry& entry : kFightingSystemTable) {
        if (entry.hash == hash) {
            return entry.rootAddress;
        }
    }

    return 0;
}

static u32 GetPickupFighting(u16 type) {
    MARKFUNCTION(0x8007DD5C);

    for (const TypeFightingSystemEntry& entry : kWeaponTypeFightingSystemTable) {
        if (entry.type == type) {
            return FindPickupFightingRootAddress(entry.hash);
        }
    }

    return 0;
}

static const WeaponTypePickupEntry* FindWeaponTypePickupEntry(u16 type) {
    for (const WeaponTypePickupEntry& entry : kWeaponTypePickupSystemTable) {
        if (entry.type == type) {
            return &entry;
        }
    }

    return nullptr;
}

static u32 GetPickupFightingHighPickup(u16 type) {
    MARKFUNCTION(0x8007DD88);

    const WeaponTypePickupEntry* entry = FindWeaponTypePickupEntry(type);
    return entry ? entry->highMove : 0x800D042Cu;
}

static u32 GetPickupFightingLowPickup(u16 type) {
    MARKFUNCTION(0x8007DDDC);

    const WeaponTypePickupEntry* entry = FindWeaponTypePickupEntry(type);
    return entry ? entry->lowMove : 0x800D0410u;
}

static u32 GetPickupFightingThrow(u16 type) {
    MARKFUNCTION(0x8007DE30);

    const WeaponTypePickupEntry* entry = FindWeaponTypePickupEntry(type);
    return entry ? entry->throwMove : 0x800D0448u;
}

static s32 GetPickupFightingIdle(u16 type) {
    MARKFUNCTION(0x8007DE84);

    const WeaponTypePickupEntry* entry = FindWeaponTypePickupEntry(type);
    return entry ? entry->idleAnim : 22;
}

static const PsxFightingMoveRaw* ResolveFightingMoveAddress(u32 address) {
    if (!address) {
        return nullptr;
    }

    for (const PsxFightingMoveRaw& move : kPsxFightingMoveTable) {
        if (move.address == address) {
            return &move;
        }
    }

    return nullptr;
}

static const PsxFightingJointRaw* ResolveFightingJointAddress(u32 address) {
    if (!address) {
        return nullptr;
    }

    for (const PsxFightingJointRaw& joint : kPsxFightingJointTable) {
        if (joint.address == address) {
            return &joint;
        }
    }

    return nullptr;
}

static s32 SetDefaultCollisionPoint(const DBRoot& root, u32 attribNum, LVector& outA, LVector& outB) {
    MARKFUNCTION(0x8006D208);

    const DBAttrib* attrib = root.FindAttrib(attribNum);
    if (!attrib) {
        return 0;
    }

    const char* name = attrib->GetAttribString();
    if (!name || !g_levelManager) {
        return 0;
    }

    OriginalBasic* geo = g_levelManager->FindGeo((s32)p3dHash(name));
    (void)geo;
    outA = {};
    outB = {};
    return 0;
}

Pickup::Pickup(const LVector* initialPos, u16 type)
    : DynamicThing(initialPos, type) {
    MARKFUNCTION(0x8006D45C);

    moveStruct = GetMoveStruct(type);
    health = 6;
    maxHealth = 6;
    weaponSound = nullptr;
    pickupFlags = 0;
    field328 = 0;
    fightingSystemRoot = GetPickupFighting(type);
    idleAnim = GetPickupFightingIdle(type);
    highPickupMove = GetPickupFightingHighPickup(type);
    lowPickupMove = GetPickupFightingLowPickup(type);
    currentPickupMove = highPickupMove;
    throwMove = GetPickupFightingThrow(type);
    collisionPointCount = 0;
    attachOffset = {};
    damage = 20;
    field308 = 0;
    deactivateFlag = 0;
    weaponField = 0;
    if (type == 314) {
        weaponField = 1;
    }
    field332 = 0;
    maxSpeed = 200;
}

Pickup::~Pickup() {
    MARKFUNCTION(0x8006D5B0);
    if (weaponSound) {
        weaponSound->Release();
        weaponSound = nullptr;
    }
}

void Pickup::Reset() {
    MARKFUNCTION(0x8006D618);

    const LVector savedOrientation = orientation;
    DynamicThing::Reset();
    orientation = savedOrientation;
    attachedOwner = nullptr;
    ignoreCollisionOwner = nullptr;
    maxSpeed = 200;
}

void Pickup::CreateModel(const char* name) {
    MARKFUNCTION(0x8006D688);

    Thing::CreateModel(name);

    if (!weaponSound) {
        CSound* tmp = nullptr;
        if (CSoundFactory::CreateObject(10050, &tmp, thingType) >= 0) {
            weaponSound = static_cast<CWeaponSound*>(tmp);
            weaponSound->Initialize(&pos);
        }
    }
}

void Pickup::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8006D710);

    if (!root) {
        return;
    }

    Thing::AnalyzeMesh(root);

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    collisionPointCount = 0;

    if (const DBAttrib* attrib = root->FindAttrib(9)) {
        collisionPoints[0].x = static_cast<s32>(attrib->value);
        collisionPointCount = 1;
    }
    if (const DBAttrib* attrib = root->FindAttrib(10)) {
        collisionPoints[0].y = static_cast<s32>(attrib->value);
        collisionPointCount = 1;
    }
    if (const DBAttrib* attrib = root->FindAttrib(11)) {
        collisionPoints[0].z = static_cast<s32>(attrib->value);
        collisionPointCount = 1;
    }
    if (const DBAttrib* attrib = root->FindAttrib(12)) {
        collisionPoints[1].x = static_cast<s32>(attrib->value);
        collisionPointCount = 2;
    }
    if (const DBAttrib* attrib = root->FindAttrib(13)) {
        collisionPoints[1].y = static_cast<s32>(attrib->value);
        collisionPointCount = 2;
    }
    if (const DBAttrib* attrib = root->FindAttrib(14)) {
        collisionPoints[1].z = static_cast<s32>(attrib->value);
        collisionPointCount = 2;
    }
    if (root->FindAttrib(22)) {
        field332 = 1;
    }
    if (const DBAttrib* attrib = root->FindAttrib(30)) {
        const char* attribString = attrib->GetAttribString();
        if (attribString) {
            fightingSystemRoot = FindPickupFightingRootAddress((u32)p3dHash(attribString));
        }
    }
    if (root->FindAttrib(31)) {
        deactivateFlag = 1;
    }

    if (collisionPointCount == 0) {
        LVector outA = {};
        LVector outB = {};
        if (SetDefaultCollisionPoint(*root, 5, outA, outB) != 0) {
            collisionPoints[0] = outA;
            collisionPoints[1].x = outA.x / 2;
            collisionPoints[1].y = outA.y / 2;
            collisionPoints[1].z = outA.z / 2;
            collisionPoints[2] = outB;
            collisionPointCount = 3;
        }
        else {
            collisionPointCount = 1;
        }
    }
}

void Pickup::Think() {
    MARKFUNCTION(0x8006D984);

    Move();
    if (g_game) {
        if (World* world = g_game->GetWorld()) {
            world->CheckThingSwitches(this);
        }
    }
}

s32 Pickup::SetupPickup(Thing* owner, u32 joint) {
    MARKFUNCTION(0x8006D9D0);

    deactivateFlag = 0;
    pickupFlags |= 4u;
    if (model) {
        static_cast<Model*>(model)->modelFlags |= 1u;
    }
    attachedOwner = owner;
    attachJoint = static_cast<s32>(joint);
    return 0;
}

void Pickup::UpdatePosition() {
    MARKFUNCTION(0x8006DA00);

    attachOffset = {};

    if ((pickupFlags & 4u) != 0 && attachedOwner && attachedOwner->model) {
        HumanoidModel* modelPtr = static_cast<HumanoidModel*>(attachedOwner->model);
        AnimationMatrices* animMatrices = modelPtr ? modelPtr->animMatrices : nullptr;
        if (animMatrices) {
            const s32* matrix = AnimationMatrices::GetMatrix(animMatrices, static_cast<u32>(attachJoint));
            if (matrix) {
                homePos.x = matrix[5];
                homePos.y = matrix[6];
                homePos.z = matrix[7];
                pos = homePos;
                if (g_blockManager) {
                    blockNum = g_blockManager->GetBlockNumber(homePos);
                }
                return;
            }
        }
    }

    DynamicThing::UpdatePosition();
}

s32 Pickup::Release(Thing* owner, ccList* list, const SVector* forceDir, s32 forceMag) {
    MARKFUNCTION(0x8006DBC0);

    if (list) {
        list->AddNode(nullptr, static_cast<ccMinNode*>(this));
    }

    flags &= ~TF_ON_GROUND;
    pickupFlags &= ~4u;
    if (model) {
        static_cast<Model*>(model)->modelFlags &= ~1u;
    }
    ignoreCollisionOwner = owner;

    if (forceDir && forceMag != 0) {
        AddForce(forceMag, forceDir);
    }

    attachedOwner = nullptr;
    return 0;
}

void Pickup::Move() {
    MARKFUNCTION(0x8006DD00);

    if (deactivateFlag == 0) {
        DynamicThing::Move();
    }
}

void Pickup::HandleCollision(Thing* other, s32 damage, ...) {
    MARKFUNCTION(0x8006DD30);
    Thing::HandleCollision(other, damage);
}

void Pickup::DamageExtra() {
    MARKFUNCTION(0x8006DD5C);
}

s32 Pickup::PlayEffect() {
    MARKFUNCTION(0x8006DD64);

    if (weaponSound) {
        weaponSound->Explode();
    }
    ignoreCollisionOwner = nullptr;
    return 0;
}

bool Pickup::PickupDeactivate() const {
    MARKFUNCTION(0x8006DDDC);

    const bool stopped = velocity.x == 0 && velocity.y == 0 && velocity.z == 0;
    return ((attachedOwner == nullptr && (flags & TF_ON_GROUND) != 0 && stopped) || deactivateFlag != 0);
}

s32 Pickup::GetCollisionYMin() const {
    MARKFUNCTION(0x8006DEDC);

    s32 yMin = 0;
    Mat4 rotMatrix;
    p3dBuildRotMatrixZYX((u16)orientation.x, (u16)orientation.y, (u16)orientation.z, rotMatrix);

    for (s32 index = 0; index < collisionPointCount; index++) {
        const LVector& point = collisionPoints[index];
        Vec3 rotated = p3dVecTimesRotMatrix(Vec3((f32)point.x, (f32)point.y, (f32)point.z), rotMatrix);
        const s32 rotatedY = static_cast<s32>(rotated.y);
        if (rotatedY < yMin) {
            yMin = rotatedY;
        }
    }

    return yMin;
}

CWeaponSound* Pickup::GetWeaponSoundPtr() {
    MARKFUNCTION(0x8006DF9C);
    return weaponSound;
}

const CWeaponSound* Pickup::GetWeaponSoundPtr() const {
    MARKFUNCTION(0x8006DF9C);
    return weaponSound;
}

s32 Pickup::SetPickupMove(s32 compareY) {
    MARKFUNCTION(0x8006DFA8);

    currentPickupMove = (PICKUP_THRESHOLD >= compareY - pos.y) ? highPickupMove : lowPickupMove;
    return static_cast<s32>(currentPickupMove);
}

u16 Pickup::GetPickupMove() const {
    MARKFUNCTION(0x8006DFD8);

    const PsxFightingMoveRaw* move = ResolveFightingMoveAddress(currentPickupMove);
    return move ? move->anim : 0;
}

s8 Pickup::GetPickupMoveGrabFrame() const {
    MARKFUNCTION(0x8006DFEC);

    const PsxFightingMoveRaw* move = ResolveFightingMoveAddress(currentPickupMove);
    if (!move) {
        return 0;
    }

    const PsxFightingJointRaw* joint = ResolveFightingJointAddress(move->data20);
    return joint ? joint->AttackStartFrame() : 0;
}

u16 Pickup::GetThrowMove() const {
    MARKFUNCTION(0x8006E008);

    const PsxFightingMoveRaw* move = ResolveFightingMoveAddress(throwMove);
    return move ? move->anim : 0;
}

s8 Pickup::GetThrowMoveThrowFrame() const {
    MARKFUNCTION(0x8006E01C);

    const PsxFightingMoveRaw* move = ResolveFightingMoveAddress(throwMove);
    if (!move) {
        return 0;
    }

    const PsxFightingJointRaw* joint = ResolveFightingJointAddress(move->data20);
    return joint ? joint->AttackStartFrame() : 0;
}