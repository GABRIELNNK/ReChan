#include "ai/humanoid.h"
#include "ai/activezn.h"
#include "ai/humndata.h"
#include "ai/obstacle.h"
#include "ai/player.h"
#include "ai/colfight.h"
#include "gen/common.h"
#include "gen/database.h"
#include "gen/model.h"
#include "gen/ai.h"
#include "gen/animmat.h"
#include "gen/animstruct.h"
#include "gen/director.h"
#include "gen/game.h"
#include "gen/colsect.h"
#include "gen/world.h"
#include "snd/rsevent.h"
#include "snd/hmndsnd.h"
#include "snd/sndfact.h"
#include "p3d/hash.h"
#include "p3d/p3dmath.h"
#include "p3d/skeleton.h"
#include "pc/log.h"

static constexpr s32 HUMANOID_ANIM_RUN = 2;
static constexpr s32 HUMANOID_ANIM_DIVE_ROLL = 90;
static constexpr s32 HUMANOID_ANIM_LEDGE_PULLUP = 30;
static constexpr s32 HUMANOID_ANIM_LEDGE_LATCH = 31;
static constexpr s16 DIVE_ROLL_FORCE_END_FRAME = 14;
static constexpr s32 DIVE_ROLL_FORCE = 0xDAC;
static constexpr s16 DIVE_ROLL_JUMP_PAUSE_FRAME = 0xB;
static constexpr s16 DIVE_ROLL_RUN_STRAFE_FRAME = 0xD;
static constexpr s32 LEDGE_TRACE_DISTANCE = 384;
static constexpr s32 LEDGE_TRACE_MIN_Y = 500;
static constexpr s32 LEDGE_TRACE_MAX_Y = 750;
static constexpr s32 LEDGE_TRACE_CLEARANCE = 1024;
static constexpr s32 LEDGE_FLOOR_MIN_HEIGHT = 1022;

struct FightingComboNode {
    u8 requestedCommand = 0;
    s8 minFrame = 0;
    s8 maxFrame = 0;
    u8 pad03 = 0;
    void* field04 = nullptr;
    void* moveData = nullptr;
    FightingComboNode* child = nullptr;
    FightingComboNode* sibling = nullptr;
};

// PSX: FindSiblingWithRequestedCommand__8HumanoidPC17FightingComboNodel (HUMANOID.CPP:7713)
s32 Humanoid::FindSiblingWithRequestedCommand(const FightingComboNode* root, u32 requestedBits) {
    MARKFUNCTION(0x8006B5A8);

    const FightingComboNode* node = root;
    while (node) {
        if (((requestedBits >> node->requestedCommand) & 1u) != 0) {
            return static_cast<s32>(reinterpret_cast<intptr_t>(node));
        }
        node = node->sibling;
    }
    return 0;
}

// PSX: FindSiblingWithRequestedCommand__8HumanoidPC17FightingComboNodell (HUMANOID.CPP:7741)
s32 Humanoid::FindSiblingWithRequestedCommand(
    const FightingComboNode* root, u32 requestedBits, s32 frame) {
    MARKFUNCTION(0x8006B5EC);

    const FightingComboNode* node = root;
    while (node) {
        if (((requestedBits >> node->requestedCommand) & 1u) != 0
            && frame >= node->minFrame
            && node->maxFrame >= frame) {
            return static_cast<s32>(reinterpret_cast<intptr_t>(node));
        }
        node = node->sibling;
    }
    return 0;
}

// PSX: HandleAnimationControl__8Humanoid (HUMANOID.CPP:1590)
s32 Humanoid::HandleAnimationControl() {
    MARKFUNCTION(0x80064194);

    if ((flags2 & TF2_NIS_ENTER) == 0) {
        return 0;
    }

    if (!model) {
        return 0;
    }

    Model* m = static_cast<Model*>(model);
    if (!m->drawable) {
        return 0;
    }

    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return 0;
    }

    OriginalSTree* source = GetActiveSTree(m->drawable);
    STreeData* skeleton = source ? source->skeleton : nullptr;
    const STreeJoint* joint = skeleton ? skeleton->GetJoint(0) : nullptr;
    if (!joint) {
        return 0;
    }

    LVector nextHome = homePos;
    LVector localOffset = {};
    localOffset.x = joint->translationX;
    localOffset.y = joint->translationY;
    localOffset.z = joint->translationZ;

    LVector worldOffset = {};
    GetObjectToWorldSpaceVector(localOffset, worldOffset);

    const s16 frame = static_cast<s16>((u32)anim->currentFrame >> 16);
    const s32 loopCount = anim->loopCount;
    if (loopCount > 0 && frame == 0) {
        field516 = worldOffset.x;
        field520 = worldOffset.y;
        field524 = worldOffset.z;
    }

    if (((flags2 >> 5) & 1) == 0) {
        nextHome.y += (s32)worldOffset.y - field520;
    }

    if (((flags2 >> 6) & 1) == 0) {
        nextHome.x += (s32)worldOffset.x - field516;
        nextHome.z += (s32)worldOffset.z - field524;
    }

    field516 = worldOffset.x;
    field520 = worldOffset.y;
    field524 = worldOffset.z;

    if (loopCount == 0 && frame == 0) {
        if (actionState == 59) {
            pos.y = nextHome.y;
        }
    }

    homePos = nextHome;

    return 1;
}

// PSX: __8HumanoidPC10tagLVectorUs (HUMANOID.CPP:350)
Humanoid::Humanoid(const LVector* initialPos, u16 type)
    : DynamicThing(initialPos, type) {
    MARKFUNCTION(0x80062A34);

    attackJointIndex = -1;
    prevAttackJointIndex = -1;
    collBboxMin = { 175, 0, 768 };
    collBboxMax = { 175, 0, 768 };
    humanoidSound = nullptr;
    combatFlag = 0;
    turnRate = 2730;
    field344 = 0;
    stateDispatch = SD_STAND;
    field348 = 8;
    distantTargetRange = 16000;
    stateCounter = 100; // PSX: this+52 = 100 for humanoids
    field424 = 0;
    field428 = 0;
    field432 = 0;
    field466 = 0;
    field468 = 0;
    comboCount = 1;
    animControl = 0;
    field528 = 0;
    moveSpeed = 3000; // PSX: set in constructor
    spawnCount = 1;
    field408 = -1;
    field484 = 0;
    field488 = 0;
    field364 = 0;
    field256 = 0;
    field260 = 0;
    rightHandObj = nullptr;
    leftHandObj = nullptr;
    field496 = 0;
    activeZone = nullptr;
    field384 = 0;
    field388 = 0;
    field392 = 0;
    field396 = 0;
    field400 = 0;
    field404 = 0;
    field412 = 0;
    field416 = 0;
    field316 = 0;
    field320 = 0;
    field324 = 0;
    field452 = 0;
    behaviourNameHash = 0;
    field436 = 0;
    characterSubType = 0;
    soundHandle = 0;
    soundParam = 0;
    punchDir = 0;
    kickDir = 0;
    comboDir = 0;
    // PSX: health/maxHealth set to 100 for humanoids
    health = 100;
    maxHealth = 100;

    if (!behaviour) {
        behaviour = new Behaviour(this, thingType, 0);
    }
}

// PSX: _._8Humanoid (HUMANOID.CPP:490)
Humanoid::~Humanoid() {
    MARKFUNCTION(0x80062C58);
    // PSX: KillDialog, DeleteModel, DeleteRightHandObj, DeleteLeftHandObj, etc.
    DeleteRightHandObj();
    DeleteLeftHandObj();
    if (humanoidSound) {
        humanoidSound->Release();
        humanoidSound = nullptr;
    }
    delete behaviour;
    behaviour = nullptr;
    fightingSystem = nullptr;
    defaultFightingSystem = nullptr;
    humanoidData = nullptr;
    trails = nullptr;
}

// PSX: Think__8Humanoid (HUMANOID.CPP:1133)
void Humanoid::Think() {
    MARKFUNCTION(0x80063808);

    // PSX step 1: CHumanoidSound::Think
    if (humanoidSound) {
        humanoidSound->Think();
    }
    // PSX step 2-3: random() + LoadEnemyTaunts (dialog system not yet implemented)
    // PSX step 4: check flags2 bit 7 for dialog state (not yet implemented)

    // PSX step 6: clear flag bits
    flags &= ~TF_BIT1;
    flags2 &= ~TF2_BIT3;

    // PSX step 7: process AI behaviour
    ProcessControl();

    // PSX step 8: delta time computation (fixed-point 16.16 multiply)
    // result = (moveSpeed * deltaTime) >> 16
    s64 dt = (s64)moveSpeed * (s64)deltaTime;
    s32 scaledRange = (s32)((u64)dt >> 16);
    (void)scaledRange; // stored to PSX +212 (animation speed field, not yet wired)
    deltaTime = FIX16_ONE; // reset to 1.0

    // PSX step 9: face player if not player and not in certain states
    // (requires FightingCollision system, simplified for now)

    ProcessAction();

    Move();

    field368 = 0;
    thinkCounter++;
}

// PSX: Draw__8Humanoid (HUMANOID.CPP:1280)
// PSX: swaps animation matrices, sets pos/orient on model, Show(0),
// then updates collision bbox from skeleton joints (debug draw skipped).
void Humanoid::Draw() {
    MARKFUNCTION(0x80063A88);

    LVector drawPos = pos;
    LVector drawOrient = orientation;

    if (model) {
        HumanoidModel* hm = static_cast<HumanoidModel*>(model);
        // PSX: Swap__17AnimationMatrices(v2[24]) - swap double-buffered joint matrices
        if (hm->animMatrices) {
            hm->animMatrices->Swap();
        }
        // PSX: copy pos/orientation to model, then Show(0)
        Model* m = static_cast<Model*>(model);
        m->posX = drawPos.x;
        m->posY = drawPos.y;
        m->posZ = drawPos.z;
        m->rotX = (u16)(drawOrient.x & 0xFFFF);
        m->rotY = (u16)(drawOrient.y & 0xFFFF);
        m->rotZ = (u16)(drawOrient.z & 0xFFFF);
        m->Show(0);

        return;
    }
    // No model: fallback to debug wireframe
    Thing::Draw();
}

// PSX: Reset__8Humanoid (HUMANOID.CPP:513)
void Humanoid::Reset() {
    MARKFUNCTION(0x80062DC0);
    DynamicThing::Reset();

    // PSX: if this != global player, clear target
    if (this != (Thing*)Player::s_player) {
        SetTarget(nullptr);
    }

    turnRate = 2730;
    stateTimer = 0;
    thinkCounter = 0;

    // PSX: flags |= 4 (needs activation), flags &= ~0x100
    flags |= TF_NEEDS_ACTIVATION;
    flags &= ~TF_BIT8;
}

// PSX: Activate__8Humanoid (HUMANOID.CPP:760)
void Humanoid::Activate() {
    MARKFUNCTION(0x80063210);
    // PSX: save prior activated state, then call base
    bool wasActivated = (flags & TF_ACTIVATED) != 0;
    Thing::Activate();
    // PSX: if newly activated (wasn't before, is now), insert into FightingCollision
    bool isActivated = (flags & TF_ACTIVATED) != 0;
    if (!wasActivated && isActivated) {
        FightingCollision::InsertHumanoid(this);
    }
}

// PSX: Deactivate__8Humanoid (HUMANOID.CPP:776)
void Humanoid::Deactivate() {
    MARKFUNCTION(0x80063270);
    Thing::Deactivate();
    FightingCollision::RemoveHumanoid(this);
}

// PSX: Move__8Humanoid (HUMANOID.CPP:1544)
void Humanoid::Move() {
    MARKFUNCTION(0x80064100);
    DynamicThing::Move();
    HandleAnimationControl();

    World* world = g_game ? g_game->GetWorld() : nullptr;
    if (world) {
        world->CheckThingSwitches(this);
    }
}

// PSX: RestorePositionFromBip01__8Humanoid (HUMANOID.CPP:1681)
s32 Humanoid::RestorePositionFromBip01() {
    MARKFUNCTION(0x800643B8);

    if (!model) {
        return 0;
    }

    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim || !anim->flip) {
        return 0;
    }

    anim->flip->SetFrame(0);
    anim->flip->UpdateJoints();

    if (!m->drawable) {
        return 0;
    }

    OriginalSTree* source = GetActiveSTree(m->drawable);
    STreeData* skeleton = source ? source->skeleton : nullptr;
    const STreeJoint* joint = skeleton ? skeleton->GetJoint(0) : nullptr;
    if (!joint) {
        return 0;
    }

    LVector local = {};
    local.x = joint->translationX;
    local.y = joint->translationY;
    local.z = joint->translationZ;

    LVector worldOffset = {};
    GetObjectToWorldSpaceVector(local, worldOffset);

    s32 nextX = homePos.x;
    s32 nextY = homePos.y - worldOffset.y;
    s32 nextZ = homePos.z;
    if (((flags2 >> 6) & 1) == 0) {
        nextX -= worldOffset.x;
        nextZ -= worldOffset.z;
    }

    pos.x = nextX;
    pos.y = nextY;
    pos.z = nextZ;
    homePos.x = nextX;
    homePos.y = nextY;
    homePos.z = nextZ;

    if (((flags2 >> 6) & 1) != 0) {
        return nextY;
    }
    return nextX;
}

// PSX: CheckForLedges2__8HumanoidR9_RMVECT16R10tagLVectorl (HUMANOID.CPP:6730)
bool Humanoid::CheckForLedges2(LVector& outNormal, LVector& outCorrectionPos, s32 clearance) {
    MARKFUNCTION(0x8006A3B0);

    LVector startPos = pos;
    LVector endPos = startPos;
    endPos.x += (s32)(((s64)384 * rmSin16(orientation.y)) >> 16);
    endPos.z += (s32)(((s64)384 * rmSin16((s16)(orientation.y + 0x4000))) >> 16);

    u16 outMaterial = 0;
    return CollisionSector::LedgePrototype(
        startPos,
        endPos,
        startPos.y + 100,
        startPos.y + 600,
        outNormal,
        outCorrectionPos,
        outMaterial,
        clearance);
}

// PSX: PrepareLedgeLatch__8HumanoidPC10tagLVectorPC9_RMVECT16 (HUMANOID.CPP:6581)
void Humanoid::PrepareLedgeLatch(const LVector& correctionPos, const LVector& normal) {
    MARKFUNCTION(0x80069FEC);

    velocity = {};
    contactForce = {};
    DropPickup(1, 1);

    s32 facingAngle = 0;
    if (normal.x != 0) {
        if (normal.z == 0) {
            facingAngle = (normal.x > 0) ? 0xC000 : 0x4000;
        }
        else {
            facingAngle = (0x4000 - (s32)rmATan216((f32)(-normal.x), (f32)(-normal.z))) & 0xFFFF;
        }
    }
    else {
        facingAngle = (normal.z > 0) ? 0x8000 : 0;
    }

    orientation.y = facingAngle;

    s32 offsetX = (s16)((s64)rmSin16((s16)facingAngle) >> 9);
    s32 offsetZ = (s16)((s64)rmSin16((s16)(facingAngle + 0x4000)) >> 9);

    pos.x = correctionPos.x + offsetX;
    pos.y = correctionPos.y;
    pos.z = correctionPos.z + offsetZ;
    homePos = pos;
}

// PSX: CheckForLedges__8Humanoid (HUMANOID.CPP:6644)
bool Humanoid::CheckForLedges() {
    MARKFUNCTION(0x8006A1D8);

    if (velocity.y > 0) {
        return false;
    }

    LVector startPos = pos;
    LVector endPos = startPos;
    endPos.x += (s32)(((s64)LEDGE_TRACE_DISTANCE * rmSin16(orientation.y)) >> 16);
    endPos.z += (s32)(((s64)LEDGE_TRACE_DISTANCE * rmSin16((s16)(orientation.y + 0x4000))) >> 16);

    LVector ledgeNormal = {};
    LVector ledgePos = {};
    u16 ledgeMaterial = 0;
    if (!CollisionSector::LedgePrototype(
            startPos,
            endPos,
            startPos.y + LEDGE_TRACE_MIN_Y,
            startPos.y + LEDGE_TRACE_MAX_Y,
            ledgeNormal,
            ledgePos,
            ledgeMaterial,
            LEDGE_TRACE_CLEARANCE)) {
        return false;
    }

    if (Obstacle::DetectObstacleAboveLedge(ledgeNormal, ledgePos)) {
        return false;
    }

    s32 floorDelta = 0x2000;
    Model* trackedModel = model ? static_cast<Model*>(model) : nullptr;
    if (trackedModel && trackedModel->field36) {
        const ModelFloorHeightState* floorState =
            static_cast<const ModelFloorHeightState*>(trackedModel->field36);
        if (floorState->current != (s32)0x80000001) {
            floorDelta = ledgePos.y - floorState->current;
        }
    }

    if (floorDelta < LEDGE_FLOOR_MIN_HEIGHT) {
        return false;
    }

    SetActionState(AS_LEDGE_LATCH, 0);
    if (humanoidSound) {
        humanoidSound->Grab((CSoundMaterial)ledgeMaterial);
    }

    Land();
    PrepareLedgeLatch(ledgePos, ledgeNormal);
    return true;
}

// PSX: CheckForPickup__8Humanoid (HUMANOID.CPP:6081, 0x800697C4)
s32 Humanoid::CheckForPickup() {
    MARKFUNCTION(0x800697C4);
    // PSX: only tries to acquire when right hand is free.
    if (rightHandObj) {
        return 0;
    }

    if (!g_ai) {
        return 0;
    }

    Thing* pickup = g_ai->GetPickupWithinReach(this);
    if (!pickup) {
        return 0;
    }

    // PSX removes from pickupList, stores owner pointer in pickup, clears combat flag,
    // plays GrabWeapon SFX, then enters AS_PICKUP.
    g_ai->pickupList.RemNode(static_cast<ccMinNode*>(pickup));
    rightHandObj = pickup;
    combatFlag = 0;

    if (humanoidSound) {
        humanoidSound->GrabWeapon();
    }

    SetActionState(AS_PICKUP, 0);
    return 1;
}

// PSX: CreateModel__8HumanoidPCc (HUMANOID.CPP:795, 0x80063248)
// PSX: creates HumanoidModel if not exists, creates Behaviour, then calls Thing::CreateModel
void Humanoid::CreateModel(const char* name) {
    MARKFUNCTION(0x800632B4);

    // PSX: if model == null, create HumanoidModel(136)
    if (!model) {
        HumanoidModel* hm = new HumanoidModel();
        model = hm;
        hm->backPtr = this;
    }

    // PSX: creates Behaviour if not exists (AI system)
    if (!behaviour) {
        behaviour = new Behaviour(this, thingType, 0);
    }

    if (field452 != 0 && activeZone) {
        activeZone->AddHumanoidToOverlordMembers(this);
    }

    // PSX: calls Thing::CreateModel which does the LevelManager lookup
    Thing::CreateModel(name);

    // PSX: ApplyAnimToModel(thingType, 0, 2, 0, 0) then InitSemiTransMode
    Model* m = static_cast<Model*>(model);
    if (m) {
        m->ApplyAnimToModel(0, 0, 2, 0, 0);
        SModel* sm = static_cast<SModel*>(m);
        sm->scale = GetCharSubTypeScale(characterSubType);
        sm->InitSemiTransMode();
    }

    // PSX: vtable+212 call -> CreateSound
    CreateSound();
}

// PSX: DeleteModel__8Humanoid (HUMANOID.CPP:910)
void Humanoid::DeleteModel() {
    MARKFUNCTION(0x80063514);
    Thing::DeleteModel();

    if (field452 != 0 && activeZone) {
        activeZone->RemoveHumanoidFromOverlordMembers(this);
    }

    ReleaseSound();
}

// PSX: DeleteRightHandObj__8Humanoid (HUMANOID.CPP:6202, 0x8006D070)
void Humanoid::DeleteRightHandObj() {
    if (rightHandObj) {
        delete rightHandObj;
        rightHandObj = nullptr;
        flags2 &= ~1;
    }
}

// PSX: DeleteLeftHandObj__8Humanoid (HUMANOID.CPP:6225, 0x8006D014)
void Humanoid::DeleteLeftHandObj() {
    if (leftHandObj) {
        delete leftHandObj;
        leftHandObj = nullptr;
        flags2 &= ~2;
    }
}

// PSX: DropPickup__8Humanoidii (HUMANOID.CPP:7819, 0x8006B6A0)
// Releases held pickups back to g_ai->pickupList.
// PSX calls Pickup::Release which re-adds to the list. On PC, Pickup class
// not yet reversed, so we detach and add back to pickupList.
void Humanoid::DropPickup(s32 dropRight, s32 dropLeft) {
    MARKFUNCTION(0x8006B6A0);
    if (dropRight) {
        if (rightHandObj) {
            // PSX: checks pickup->field316 == 0 before releasing
            // PSX: Release__6Pickup(rightHandObj, this, &g_ai->pickupList, 0, 0)
            rightHandObj->Remove();
            if (g_ai) {
                g_ai->pickupList.AddNode(nullptr, static_cast<ccMinNode*>(rightHandObj));
            }
            rightHandObj = nullptr;
            flags2 &= ~1u;
        }
    }
    if (dropLeft) {
        if (leftHandObj) {
            leftHandObj->Remove();
            if (g_ai) {
                g_ai->pickupList.AddNode(nullptr, static_cast<ccMinNode*>(leftHandObj));
            }
            leftHandObj = nullptr;
            flags2 &= ~2u;
        }
    }
}

// PSX: CreateSound__8Humanoid (HUMANOID.CPP:888, 0x800634C4)
void Humanoid::CreateSound() {
    MARKFUNCTION(0x800634C4);
    if (humanoidSound) {
        return;
    }
    CSound* tmp = nullptr;
    s32 result = CSoundFactory::CreateObject(10060, &tmp, thingType);
    LOG("[Humanoid] CreateSound type=%u result=%d ptr=%p", thingType, result, tmp);
    if (result >= 0) {
        humanoidSound = static_cast<CHumanoidSound*>(tmp);
        humanoidSound->Initialize(&pos, this);
    }
}

// PSX: ReleaseSound__8Humanoid (HUMANOID.CPP:952, 0x80063614)
void Humanoid::ReleaseSound() {
    MARKFUNCTION(0x80063614);
    if (humanoidSound) {
        humanoidSound->Release();
        humanoidSound = nullptr;
    }
}

// PSX: HandleCollision__8HumanoidP5Thingle (HUMANOID.CPP:1997)
// PSX: 904 bytes. Reads tag items for damage/force/impulse from the other
// Thing, applies state-dependent hit reactions, subtracts HP, applies knockback.
// Requires: tag item system, damage types enum, sound system.
void Humanoid::HandleCollision(Thing* other, s32 damage) {
    MARKFUNCTION(0x80064808);
    if (!other) return;
    if (damage <= 0) return;
    health -= damage;
    if (health <= 0) {
        health = 0;
        SetActionState(AS_DEAD, 0);
    }
}

// PSX: HandleCollisionSound__8Humanoidl (HUMANOID.CPP:1978, 0x8006475C)
void Humanoid::HandleCollisionSound(s32 hitType) {
    MARKFUNCTION(0x8006475C);
    if (!humanoidSound) {
        return;
    }
    switch (hitType) {
        case 1:
        case 8:
            humanoidSound->PunchHit();
            break;
        case 2:
        case 3:
            humanoidSound->SuperPunch();
            break;
        case 4:
            humanoidSound->KickHit();
            break;
        case 5:
            humanoidSound->SuperKick();
            break;
        case 18:
            humanoidSound->HitByFireBlast();
            break;
    }
}

// PSX: AnalyzeMesh__8HumanoidP6DBRoot (HUMANOID.CPP:535)
void Humanoid::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80062E54);
    Thing::AnalyzeMesh(root);

    if (!root) {
        return;
    }

    behaviourNameHash = 0;

    for (u32 index = 0; index < root->attribCount; index++) {
        const DBAttrib* attrib = root->GetAttribByIndex(index);
        if (!attrib) {
            continue;
        }

        switch (attrib->id) {
            case 0x0C:
                characterSubType = GetCharSubTypeEnumFromHashID((s32)p3dHash(attrib->GetAttribString()));
                break;
            case 0x0D:
            {
                const u16 hitPoints = (u16)attrib->value;
                maxHealth = hitPoints;
                health = hitPoints;
                break;
            }
            case 0x0E:
                activeZone = g_ai
                    ? static_cast<ActiveZone*>(g_ai->activeZoneList.FindNodeCRC(p3dHash(attrib->GetAttribString())))
                    : nullptr;
                break;
            case 0x10:
                field452 = (s32)attrib->value;
                break;
            case 0x11:
                behaviourNameHash = p3dHash(attrib->GetAttribString());
                break;
            case 0x1D:
                if (p3dHash(attrib->GetAttribString()) == p3dHash("AS_NISMode")) {
                    field364 = 73;
                    SetActionState(73, 0);
                }
                break;
            case 0x1F:
                field384 = GetPreActiveIdle((s32)p3dHash(attrib->GetAttribString()));
                break;
            case 0x20:
                field388 = (s32)p3dHash(attrib->GetAttribString());
                break;
            case 0x21:
                field392 = (s32)p3dHash(attrib->GetAttribString());
                break;
            case 0x22:
                field396 = (s32)p3dHash(attrib->GetAttribString());
                break;
            case 0x23:
                field400 = (s32)p3dHash(attrib->GetAttribString());
                break;
            case 0x24:
                field404 = (s32)p3dHash(attrib->GetAttribString());
                break;
            case 0x25:
                field408 = (s32)attrib->value;
                break;
            case 0x28:
                field412 = 1;
                break;
            case 0x29:
                field416 = 1;
                break;
            default:
                break;
        }
    }

    const s16 subTypeHitPoints = GetCharSubTypeHitPoints(characterSubType);
    if (subTypeHitPoints != 0) {
        maxHealth = (u16)subTypeHitPoints;
        health = (u16)subTypeHitPoints;
    }

    const u32 subTypeBehaviourHash = GetBehaviourNameHash(characterSubType);
    if (subTypeBehaviourHash != 0) {
        behaviourNameHash = subTypeBehaviourHash;
    }
}

// PSX: SetActionState__8HumanoidUll (HUMANOID.CPP:2792)
// PSX: 5580 bytes, 74-case switch. Each case sets up animation, flags, and the
// method thunk (stateDispatch) that ProcessAction uses to call the state handler.
// On PC, we set stateDispatch to the vtable index corresponding to the handler.
void Humanoid::SetActionState(u32 state, s32 param) {
    MARKFUNCTION(0x80065680);

    s32 prevState = actionState;
    (void)prevState;

    // PSX preamble: clear combatFlag, set flags bit 11, end sounds
    combatFlag = 0;
    flags |= TF_DYNAMIC;
    if (humanoidSound) {
        humanoidSound->EndAllSounds();
    }

    if (state >= AS_COUNT) return;

    // Record action state
    actionState = (s32)state;

    // Map state number to handler dispatch index
    // PSX uses a 74-entry jump table; here we map the known cases.
    switch (state) {
        case AS_INACTIVE_IDLE:     stateDispatch = SD_STAND; break;
        case AS_STAND:             stateDispatch = SD_STAND; break;
        case AS_STAND_ANIM:        stateDispatch = SD_STAND; break;
        case AS_DIVE_ROLL:
        {
            stateDispatch = SD_DIVE_ROLL;
            if (model) {
                Model* m = static_cast<Model*>(model);
                s32 animEnum = (field316 != 0) ? field316 : HUMANOID_ANIM_DIVE_ROLL;
                // PSX uses model vtable SetAnim here (not direct ApplyAnimToModel).
                m->SetAnim(animEnum, param, 0, 0);
                AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
                if (anim) {
                    anim->SetLoopType(ANIM_LOOP, 1);
                }
            }
            // PSX LABEL_65 path clears bits 4-6 on dive roll setup.
            flags2 &= ~0x70;
            break;
        }
        case AS_PAUSE:             stateDispatch = SD_PAUSE; break;
        case AS_JUMP:              stateDispatch = SD_JUMP; break;
        case AS_WALL_JUMP:         stateDispatch = SD_WALLJUMP; break;

        case AS_RUN:
            stateDispatch = SD_RUN;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(HUMANOID_ANIM_RUN, param, 0, 0);
            }
            break;
        case AS_BACKFLIP:          stateDispatch = SD_BACKFLIP; break;
        case AS_STRAFE:            stateDispatch = SD_STRAFE; break;
        case AS_LEDGE_LATCH:
            stateDispatch = SD_LEDGE_LATCH;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(HUMANOID_ANIM_LEDGE_LATCH, 0, 0, 0);
            }
            break;
        case AS_LEDGE_PULLUP:
            flags2 &= ~0x70;
            field344 = 0;
            stateDispatch = SD_LEDGE_PULLUP;
            field348 = 8;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(HUMANOID_ANIM_LEDGE_PULLUP, 0, 0, 0);
            }
            break;
        case AS_LADDER_CLIMB_DOWN:
        {
            // PSX case 25 (loc_800665C4): top latch entry, anim 0x122.
            const bool clearLatchBits = ((flags2 & 0x10) == 0) || ((flags2 & 0x20) != 0 && (flags2 & 0x40) != 0);
            if (clearLatchBits) {
                flags2 = (flags2 | 0x10) & ~0x60;
                field516 = 0;
                field520 = 0;
                field524 = 0;
            }
            stateDispatch = SD_LADDER_LATCH_TOP;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(0x122, 0, 0, 0);
            }
            break;
        }
        case AS_LADDER_CLIMB_UP:
            // PSX case 26 (loc_80066644): latch-on state, anim 0x123.
            stateDispatch = SD_LADDER_LATCH;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(0x123, 0, 0, 0);
            }
            break;
        case AS_LADDER_CLIMBING:
        {
            // PSX case 27 (loc_80066668): active climb.
            stateDispatch = SD_CLIMB_LADDER;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(0x123, 0, 0, 0);
            }
            flags &= ~TF_DYNAMIC;
            velocity = {};
            contactForce = {};
            DropPickup(1, 1);
            break;
        }
        case AS_LADDER_DISMOUNT:
            // PSX case 28 (loc_800666E8): dismount, anim 0x126.
            stateDispatch = SD_LADDER_DISMOUNT;
            flags2 &= ~0x70;
            flags |= TF_DYNAMIC;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(0x126, 0, 0, 0);
            }
            break;
        case AS_SLOPE_SLIDE:       stateDispatch = SD_STRAFE; break;
        case AS_PUNCH_ATTACK:      stateDispatch = SD_THROW; break;
        case AS_KICK_ATTACK:       stateDispatch = SD_THROW; break;
        case AS_COMBAT_IDLE:       stateDispatch = SD_STAND; break;
        case AS_PICKUP:
            stateDispatch = SD_PICKUP;
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->SetAnim(44, 0, 0, 0);
            }
            break;
        case AS_THROW_PICKUP:      stateDispatch = SD_THROW; break;
        case AS_STUNNED:
        {
            stateDispatch = SD_STUNNED;
            if (humanoidSound) {
                humanoidSound->BeginStun();
            }
            break;
        }
        case AS_FLYING_BACK_LAND:  stateDispatch = SD_FLYING_BACK; break;
        case AS_BACK_GRAB_RECOVER: stateDispatch = SD_STAND; break;
        case AS_GET_UP:            stateDispatch = SD_STAND; break;
        case AS_FLYING_BACK_CHECK: stateDispatch = SD_FLYING_BACK; break;
        case AS_SPIN_BACK_RECOVER: stateDispatch = SD_STAND; break;
        case AS_DEAD:              stateDispatch = SD_DEAD; break;
        case AS_HOTFOOT:
            field344 = 0;
            stateDispatch = SD_NIS_MODE;
            field348 = 8;
            break;
        case AS_HIT_EXPLOSION:     stateDispatch = SD_GOT_HIT_HIGH; break;
        case AS_HIT_ENVIRONMENT:   stateDispatch = SD_GOT_HIT_HIGH; break;
        default:
            // Many states set up specific animations and dispatch
            // to one of the above handlers. Default to _Stand for safety.
            stateDispatch = SD_STAND;
            break;
    }

    stateTimer = 0;
    (void)param;
}

// PSX: ProcessAction__8Humanoid (HUMANOID.CPP:2659)
// PSX uses a method thunk at fields +344/+346/+348 to dispatch to the current
// state handler. On PC, we dispatch via stateDispatch (the vtable index).
void Humanoid::ProcessAction() {
    MARKFUNCTION(0x8006538C);
    if (stateDispatch == SD_NONE) return;

    switch (stateDispatch) {
        case SD_STAND:        _Stand(); break;
        case SD_RUN:          _Run(); break;
        case SD_JUMP:         _Jump(); break;
        case SD_FALL:         _Fall(); break;
        case SD_STRAFE:       _Straif(); break;
        case SD_DIVE_ROLL:    _DiveRoll(); break;
        case SD_BACKFLIP:     _Straif(); break;
        case SD_PAUSE:        _Pause(); break;
        case SD_GOT_HIT_HIGH: _GotHitHigh(); break;
        case SD_GOT_HIT_MED:  _GotHitMed(); break;
        case SD_GOT_HIT_LOW:  _GotHitLow(); break;
        case SD_COLLAPSE:     _Collapse(); break;
        case SD_DEAD:         _Dead(); break;
        case SD_SPIN_BACK:    _SpinBack(); break;
        case SD_FLYING_BACK:  _FlyingBack(); break;
        case SD_STUNNED:      _Stunned(); break;
        case SD_THROW:        _Throw(); break;
        case SD_PICKUP:       _Pickup(); break;
        case SD_LEDGE_LATCH:  _LedgeLatch(); break;
        case SD_LEDGE_PULLUP: _LedgePullup(); break;
        case SD_LADDER_LATCH_TOP: _LadderLatchTop(); break;
        case SD_LADDER_LATCH: _LadderLatch(); break;
        case SD_CLIMB_LADDER: _ClimbLadder(); break;
        case SD_LADDER_DISMOUNT: _LadderDismount(); break;
        case SD_NIS_MODE:     _NISMode(); break;
        default: break;
    }
}

// PSX: ProcessControl__8Humanoid (HUMANOID.CPP:961)
void Humanoid::ProcessControl() {
    MARKFUNCTION(0x80063660);
    commandBits = 0;
    if (behaviour) {
        behaviour->Process();
    }
}

void Humanoid::RequestAction(u32 actionID) {
    MARKFUNCTION(0x8006CFFC);
    commandBits |= (1 << actionID);
}

// PSX: FaceThing__8HumanoidP5Thingi (HUMANOID.CPP:2252)
void Humanoid::FaceThing(Thing* target, s32 immediate) {
    MARKFUNCTION(0x80064B98);
    if (!target) return;
    LVector point = target->pos;
    FacePoint(point, immediate);
}

// PSX: FacePoint__8HumanoidRC10tagLVectori (HUMANOID.CPP:2260)
// Computes the angle from this->pos to point, then either snaps or gradually
// turns orientation.y towards it, limited by turnRate.
void Humanoid::FacePoint(const LVector& point, s32 immediate) {
    MARKFUNCTION(0x80064BD0);

    s32 dx = point.x - pos.x;
    s32 dz = point.z - pos.z;

    // Compute target angle using atan2(dx, dz) in PSX binary angle units (0-65535)
    // PSX convention: 0 = +Z, 0x4000 = +X, 0x8000 = -Z, 0xC000 = -X
    f32 rad = atan2((f32)dx, (f32)dz);
    s32 targetAngle = RAD2ANGLE(rad) & 0xFFFF;

    if (immediate == 0) {
        // Snap directly to target angle
        orientation.y = targetAngle;
        return;
    }

    // Gradually turn towards target, limited by turnRate
    s32 diff = targetAngle - orientation.y;

    // Wrap difference to -32768..32767
    if (diff > PSX_ANGLE_180) diff -= PSX_ANGLE_360;
    if (diff < -PSX_ANGLE_180) diff += PSX_ANGLE_360;

    s32 absDiff = (diff >= 0) ? diff : -diff;

    if (absDiff < (s32)turnRate) {
        // Close enough, snap to target
        orientation.y = targetAngle;
    }
    else if (diff >= 0) {
        orientation.y += turnRate;
    }
    else {
        orientation.y -= turnRate;
    }
}

// PSX: FaceThingDesired__8HumanoidP5Thing (HUMANOID.CPP:2333)
bool Humanoid::FaceThingDesired(Thing* target) {
    MARKFUNCTION(0x80064D7C);
    if (!target) {
        return orientation.y == faceAngle;
    }
    return FacePointDesired(target->pos);
}

// PSX: FacePointDesired__8HumanoidRC10tagLVector (HUMANOID.CPP:2352)
// Computes desired facing angle only (faceAngle), without changing orientation.
bool Humanoid::FacePointDesired(const LVector& point) {
    MARKFUNCTION(0x80064DB4);

    s32 dx = point.x - pos.x;
    s32 dz = point.z - pos.z;

    s32 desired = 0;
    u32 quadrant = (u32)dx >> 31;
    if (dz < 0) {
        quadrant += 2;
    }

    if (quadrant == 1) {
        desired = (s32)rmATan216((f32)-dx, (f32)dz) + 0xC000;
    }
    else if (quadrant >= 2) {
        if (quadrant < 4) {
            desired = (s32)rmATan216((f32)-dz, (f32)-dx) + 0x8000;
        }
    }
    else {
        desired = (s32)rmATan216((f32)-dx, (f32)dz) - 0x4000;
    }

    faceAngle = desired;
    return orientation.y == faceAngle;
}

// PSX: SetIdleAnimation__8Humanoidli (HUMANOID.CPP:2717, 0x800654C4)
// Plays idle animation via model->SetAnim. If no weapon, plays anim 22.
// If weapon, plays weapon idle with optional transition.
void Humanoid::SetIdleAnimation(s32 loopType, s32 doTransition) {
    MARKFUNCTION(0x800654C4);

    if (!model) {
        return;
    }

    // PSX: checks rightHandObj (pickup pointer) for weapon idle
    if (!rightHandObj) {
        // No weapon: play standard idle (anim 22) via model->SetAnim
        Model* m = static_cast<Model*>(model);
        m->SetAnim(22, loopType, 0, 0);
        return;
    }

    // TODO: weapon idle system (GetWeaponTransitionIdle, SetTransitionAnim)
    // For now, fall back to standard idle
    Model* m = static_cast<Model*>(model);
    m->SetAnim(22, loopType, 0, 0);
}

// PSX: TestIdleAnimation__8Humanoid (HUMANOID.CPP:2763, 0x80065618)
// Returns true if currently playing the idle animation (22 or weapon idle).
bool Humanoid::TestIdleAnimation() {
    MARKFUNCTION(0x80065618);

    if (!model) {
        return false;
    }

    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return false;
    }

    s32 curAnim = anim->animEnum;

    // PSX: if has weapon (rightHandObj), check weapon idle anim ID at +216
    // PSX: if has leftHandObj, check that anim ID
    // Otherwise: check against 22 (standard idle)
    // TODO: weapon idle anim check
    return curAnim == 22;
}

// PSX: FindFoe__8HumanoidUlli (HUMANOID.CPP:2446)
// Searches nearby humanoids within range for a combat target.
// Searches nearby humanoids within range for a combat target.
Humanoid* Humanoid::FindFoe(u32 range, s32 param, s32 immediate) {
    MARKFUNCTION(0x80064F94);

    Humanoid** arr = FightingCollision::GetHumanoidArray();
    Humanoid* best = nullptr;
    s32 bestDist = (s32)range;

    for (s32 i = 0; i < FIGHTING_COLLISION_MAX; ++i) {
        Humanoid* h = arr[i];
        if (!h || h == this) continue;
        if (!(h->flags & TF_ACTIVATED)) continue;

        // PSX: distance check on XZ plane
        s32 dx = h->pos.x - pos.x;
        s32 dz = h->pos.z - pos.z;
        if (dx < 0) dx = -dx;
        if (dz < 0) dz = -dz;
        s32 dist = dx + dz; // Manhattan distance (PSX approx)
        if (dist < bestDist) {
            bestDist = dist;
            best = h;
        }
    }
    return best;
}

// PSX: SetTarget__8HumanoidP8Humanoid (HUMANOID.CPP:2502)
void Humanoid::SetTarget(Humanoid* target) {
    MARKFUNCTION(0x8006511C);
    if (target == (Humanoid*)this) return;
    // PSX: sets field384 = target, increments target's refcount
    field384 = (s32)(intptr_t)target;
}

// PSX: SetHumanoidTarget__8HumanoidP8Humanoid (HUMANOID.CPP:2535, 0x800651C0)
// Releases old target, sets new one with refcount.
void Humanoid::SetHumanoidTarget(Humanoid* target) {
    ReleaseTarget();
    if (target) {
        target->field260++;
    }
    field256 = (s32)(intptr_t)target;
}

// PSX: ReleaseTarget__8Humanoid (HUMANOID.CPP:2553)
void Humanoid::ReleaseTarget() {
    MARKFUNCTION(0x80065200);
    s32 targetAddr = field256;
    if (targetAddr) {
        Humanoid* t = (Humanoid*)(intptr_t)targetAddr;
        if (t->field260 > 0) {
            t->field260--;
        }
        field256 = 0;
    }
}

// PSX: IsInActiveZone__8Humanoid (HUMANOID.CPP:2612, 0x80065230)
bool Humanoid::IsInActiveZone() const {
    MARKFUNCTION(0x80065230);

    return activeZone != nullptr
        && activeZone->box.IsValid()
        && activeZone->box.IsInside(pos);
}

// PSX: IsTargetInActiveZone__8Humanoid (HUMANOID.CPP:2630, 0x80065290)
bool Humanoid::IsTargetInActiveZone() const {
    MARKFUNCTION(0x80065290);

    Humanoid* target = reinterpret_cast<Humanoid*>(static_cast<intptr_t>(field256));
    return target != nullptr
        && activeZone != nullptr
        && activeZone->box.IsValid()
        && activeZone->box.IsInside(target->pos);
}

// PSX: FaceAngleY__8Humanoidli (HUMANOID.CPP:2402)
// Turns orientation.y toward the given angle, limited by turnRate.
// If immediate == 0: snap directly. Otherwise: gradual turn.
void Humanoid::FaceAngleY(s32 angle, s32 immediate) {
    MARKFUNCTION(0x80064EB0);

    if (immediate == 0) {
        orientation.y = angle;
        return;
    }

    s32 diff = angle - orientation.y;

    // Wrap difference to -32768..32767
    if (diff > PSX_ANGLE_180) diff -= PSX_ANGLE_360;
    if (diff < -PSX_ANGLE_180) diff += PSX_ANGLE_360;

    s32 absDiff = (diff >= 0) ? diff : -diff;

    if (absDiff < (s32)turnRate) {
        orientation.y = angle;
    }
    else if (diff < 0) {
        orientation.y -= turnRate;
    }
    else {
        orientation.y += turnRate;
    }
}

// PSX: ReturnMostSignificant32BitNumber__FUl (HUMANOID.CPP:3826)
// Returns the 1-based index of the highest set bit, or 0 if input is 0.
// PSX uses binary search: test top 16 bits, then 8, then 4, etc.
static s32 ReturnMostSignificant32BitNumber(u32 value) {
    MARKFUNCTION(0x80066C4C);
    if (value == 0) return 0;
    s32 result = 0;
    s32 shift = 16;
    while (shift > 0) {
        u32 upper = value >> shift;
        if (upper != 0) {
            result += shift;
            value = upper;
        }
        shift >>= 1;
    }
    return result;
}

// PSX: _Stand__8Humanoid (HUMANOID.CPP:3859)
// Dispatches input commands from commandBits via highest-bit priority.
void Humanoid::_Stand() {
    MARKFUNCTION(0x80066CA0);

    s32 cmd = ReturnMostSignificant32BitNumber((u32)commandBits);
    flags2 |= 0x0008; // ground sticking

    LOG("_Stand: commandBits=0x%X cmd=%d actionState=%d stateDispatch=%d", commandBits, cmd, actionState, stateDispatch);

    if (cmd < 1 || cmd > 31) return;

    SVector dir;
    dir.x = (s16)orientation.x;
    dir.y = 0;
    dir.z = (s16)orientation.y;
    dir.pad = 0;

    switch (cmd) {
        case 1: // guard/face
            FaceAngleY(faceAngle, 0);
            return;
        case 2: // kick -> run
            SetActionState(AS_RUN, runSpeed);
            return;
        case 3: // punch -> jump
            SetActionState(AS_JUMP, 0);
            return;
        case 4: // facing -> dive roll
            SetActionState(AS_DIVE_ROLL, 0);
            return;
        case 5: // roll to stand -> backflip
            SetActionState(AS_BACKFLIP, 0);
            return;
        case 6: // backflip -> pickup/throw or combat idle
            if (rightHandObj != 0 || leftHandObj != 0) {
                if (rightHandObj != 0 && field316 != 0) {
                    SetActionState(AS_COMBAT_IDLE, 0);
                }
                else {
                    SetActionState(AS_THROW_PICKUP, 0);
                }
            }
            else {
                SetActionState(AS_COMBAT_IDLE, 0);
            }
            return;
        case 7: // PSX case 7: grab/combat - CheckForPickup then combat idle
            if (rightHandObj != 0 || leftHandObj != 0) {
                if (rightHandObj != 0 && field316 != 0) {
                    SetActionState(AS_COMBAT_IDLE, 0);
                }
                else {
                    SetActionState(AS_THROW_PICKUP, 0);
                }
            }
            else {
                if (CheckForPickup() == 1) {
                    return;
                }
                SetActionState(AS_COMBAT_IDLE, 0);
            }
            return;
        case 8: // dodge -> combat idle
            SetActionState(AS_COMBAT_IDLE, 0);
            return;
        case 9: // face + stand
            FaceAngleY(faceAngle, 0);
            SetActionState(AS_STAND, 0);
            return;
        case 10: // run -> strafe
            FaceAngleY(faceAngle, 0);
            SetActionState(AS_STRAFE, 0);
            return;
        case 11: // hit explosion
            SetActionState(AS_HIT_ENVIRONMENT, 0);
            return;
        case 12: // hit environment
            SetActionState(AS_HIT_EXPLOSION, 0);
            return;
        default:
            return;
    }
}

// PSX: _DiveRoll__8Humanoid (HUMANOID.CPP:3977)
// Frame-based dive roll: force on early frames, then command-gated exits.
void Humanoid::_DiveRoll() {
    MARKFUNCTION(0x80066E3C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX reads currentFrame high word (+62), not low word.
    s16 frame = (s16)((u32)anim->currentFrame >> 16);
    u32 cb = (u32)commandBits;

    if (anim->loopCount > 0) {
        if ((cb >> 3) & 1) {
            SetActionState(AS_JUMP, 0);
            return;
        }
        if ((cb >> 4) & 1) {
            SetActionState(AS_PAUSE, 0);
            return;
        }
        if ((cb >> 5) & 1) {
            SetActionState(AS_DIVE_ROLL, 0);
            return;
        }
        if ((cb >> 2) & 1) {
            SetActionState(AS_RUN, 0);
            m->SetAnim(HUMANOID_ANIM_RUN, 0, 0, 0);
            return;
        }
        SetActionState(AS_STAND, 0);
        return;
    }

    if (frame < DIVE_ROLL_FORCE_END_FRAME) {
        SVector dir;
        dir.x = (s16)orientation.x;
        dir.y = 0;
        dir.z = (s16)orientation.y;
        dir.pad = 0;
        AddForce(DIVE_ROLL_FORCE, &dir);
    }

    if (frame >= DIVE_ROLL_JUMP_PAUSE_FRAME) {
        if ((cb >> 3) & 1) {
            SetActionState(AS_JUMP, 0);
        }
        else if ((cb >> 4) & 1) {
            SetActionState(AS_PAUSE, 0);
        }
    }

    if (frame >= DIVE_ROLL_RUN_STRAFE_FRAME) {
        if ((cb >> 2) & 1) {
            SetActionState(AS_RUN, 0);
            m->SetAnim(HUMANOID_ANIM_RUN, 0, 0, 0);
        }
        else if ((cb >> 5) & 1) {
            SetActionState(AS_DIVE_ROLL, 0);
        }
    }

    // PSX also drives a dive-roll kick combo subtree here via fighting nodes.
}

// PSX: _Taunt__8Humanoid (HUMANOID.CPP:4069)
// Wait for animation to complete, then dispatch commands.
void Humanoid::_Taunt() {
    MARKFUNCTION(0x8006710C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: wait for animation to complete (loopCount > 0)
    if (anim->loopCount == 0) {
        return;
    }

    s32 cmd = ReturnMostSignificant32BitNumber((u32)commandBits);
    if (cmd < 2 || cmd > 31) {
        SetActionState(AS_STAND, 0);
        return;
    }

    switch (cmd) {
        case 2:
            SetActionState(AS_RUN, runSpeed);
            return;
        case 6:
            SetActionState(AS_BACKFLIP, 0);
            return;
        case 7:
        case 15:
        case 16:
            if (rightHandObj != 0 || leftHandObj != 0) {
                s32 weaponField = 0;
                if (rightHandObj != 0) {
                    // PSX reads pickup->field316 - Pickup class not reversed yet
                    Humanoid* pickup = static_cast<Humanoid*>(rightHandObj);
                    weaponField = pickup->field316;
                }
                if (weaponField != 0) {
                    SetActionState(AS_COMBAT_IDLE, 0);
                }
                else {
                    SetActionState(AS_THROW_PICKUP, 0);
                }
            }
            else {
                if (CheckForPickup() == 1) {
                    return;
                }
                SetActionState(AS_COMBAT_IDLE, 0);
            }
            return;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 20:
            SetActionState(AS_PUNCH_ATTACK, 0);
            return;
        case 19:
            SetActionState(AS_COMBAT_IDLE, 0);
            return;
        case 21:
            FaceAngleY(faceAngle, 0);
            return;
        case 30:
            SetActionState(AS_HIT_EXPLOSION, 0);
            return;
        case 31:
            SetActionState(AS_HIT_ENVIRONMENT, 0);
            return;
        default:
            SetActionState(AS_STAND, 0);
            return;
    }
}

// PSX: _Pause__8Humanoid (HUMANOID.CPP:4153)
// Simple counter decrement, then return to stand.
void Humanoid::_Pause() {
    MARKFUNCTION(0x80067288);

    FaceAngleY(faceAngle, 0);

    if (field324 != 0) {
        field324--;
    }
    else {
        SetActionState(AS_STAND, 0);
    }
}

// PSX: _Run__8Humanoid (HUMANOID.CPP:4172)
// Extensive bit dispatch for attack/move transitions.
void Humanoid::_Run() {
    MARKFUNCTION(0x800672EC);

    flags2 |= 0x0008; // ground sticking
    s32 sd = commandBits;

    // Backflip (bit 6)
    if (sd & 0x0040) {
        SetActionState(AS_BACKFLIP, 0);
        return;
    }

    // Attack punch group (bits 8,10,12,20)
    if ((sd >> 8) & 1 || (sd >> 10) & 1 || (sd >> 12) & 1 || (sd >> 20) & 1) {
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // Attack kick group (bits 9,11,13,14)
    if ((sd >> 9) & 1 || (sd >> 11) & 1 || (sd >> 13) & 1 || (sd >> 14) & 1) {
        SetActionState(AS_KICK_ATTACK, 0);
        return;
    }

    // Multi-hit combat (bits 7,19,15,16,17,18) -> pickup/throw or combat idle
    if ((sd >> 7) & 1 || (sd >> 19) & 1 || (sd >> 15) & 1
        || (sd >> 16) & 1 || (sd >> 17) & 1 || (sd >> 18) & 1) {
        if (rightHandObj != 0 || leftHandObj != 0) {
            if (rightHandObj != 0 && field316 != 0) {
                SetActionState(AS_COMBAT_IDLE, 0);
            }
            else {
                SetActionState(AS_THROW_PICKUP, 0);
            }
        }
        else {
            SetActionState(AS_COMBAT_IDLE, 0);
        }
        return;
    }

    // Taunt (bit 4) -> pause
    if (sd & 0x0010) {
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // Punch (bit 3) -> jump
    if (sd & 0x0008) {
        SetActionState(AS_JUMP, 0);
        return;
    }

    // Dive roll (bit 21)
    if (sd & 0x200000) {
        SetActionState(AS_DIVE_ROLL, 0);
        return;
    }

    // Guard (bit 1) -> stand
    if (sd & 0x0002) {
        SetActionState(AS_STAND, 0);
        return;
    }

    // Kick (bit 2) -> face + run forward
    if (sd & 0x0004) {
        FaceAngleY(faceAngle, 0);
        SVector dir;
        dir.x = (s16)orientation.x;
        dir.y = 0;
        dir.z = (s16)orientation.y;
        dir.pad = 0;
        AddForce(runSpeed, &dir);
        return;
    }

    // Explosion (bit 31)
    if (sd < 0) {
        SetActionState(AS_HIT_ENVIRONMENT, 0);
        return;
    }

    // Env hit (bit 30)
    if ((sd >> 30) & 1) {
        SetActionState(AS_HIT_EXPLOSION, 0);
        return;
    }

    // Strafe (bit 5) -> face and strafe
    if (sd & 0x0020) {
        FaceAngleY(faceAngle, 0);
        SetActionState(AS_STRAFE, 0);
    }
}

// PSX: _Straif__8Humanoid (HUMANOID.CPP:4307)
void Humanoid::_Straif() {
    MARKFUNCTION(0x80067610);

    // PSX: capture orientation.y and faceAngle at function entry.
    // FaceThingDesired/FaceAngleY modify these fields later, but the captured
    // values are used for movement direction and animation selection.
    s32 savedOrientY = orientation.y;
    s32 savedFaceAngle = faceAngle;

    // PSX: SVector direction = {0, 0, savedFaceAngle, 0} for AddForce
    SVector dir;
    dir.x = 0;
    dir.y = 0;
    dir.z = (s16)(savedFaceAngle & 0xFFFF);
    dir.pad = (s16)((u32)savedFaceAngle >> 16);

    // PSX: flags bit 17 -> slope slide
    if ((flags >> 17) & 1) {
        SetActionState(AS_SLOPE_SLIDE, 0);
        return;
    }

    s32 sd = commandBits;

    // Attack punch group (bits 8,10,12,20)
    if ((sd >> 8) & 1 || (sd >> 10) & 1 || (sd >> 12) & 1 || (sd >> 20) & 1) {
        // PSX: remap back-punch (bit 10) to punch (bit 8)
        if ((sd >> 10) & 1) {
            commandBits = (sd & 0xFFFFFAFF) | 0x100;
        }
        faceAngle = savedOrientY;
        ReleaseTarget();
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // Attack kick group (bits 9,11,13,14)
    if ((sd >> 9) & 1 || (sd >> 11) & 1 || (sd >> 13) & 1 || (sd >> 14) & 1) {
        // PSX: remap back-kick (bit 11) to kick (bit 9)
        if ((sd >> 11) & 1) {
            commandBits = (sd & 0xFFFFF5FF) | 0x200;
        }
        faceAngle = savedOrientY;
        ReleaseTarget();
        SetActionState(AS_KICK_ATTACK, 0);
        return;
    }

    // Grab/combat (bits 7,19,15,16,17,18)
    if ((sd >> 7) & 1 || (sd >> 19) & 1 || (sd >> 15) & 1
        || (sd >> 16) & 1 || (sd >> 17) & 1 || (sd >> 18) & 1) {
        if (rightHandObj || leftHandObj) {
            if (!((s32*)rightHandObj)[79]) {
                ReleaseTarget();
                SetActionState(AS_THROW_PICKUP, 0);
                return;
            }
        }
        else if (CheckForPickup() == 1) {
            ReleaseTarget();
            return;
        }
        ReleaseTarget();
        SetActionState(AS_COMBAT_IDLE, 0);
        return;
    }

    // Explosion (bit 31)
    if (sd < 0) {
        ReleaseTarget();
        SetActionState(AS_HIT_ENVIRONMENT, 0);
        return;
    }

    // Env hit (bit 30)
    if ((sd >> 30) & 1) {
        ReleaseTarget();
        SetActionState(AS_HIT_EXPLOSION, 0);
        return;
    }

    // Guard release (bit 1)
    if (sd & 0x0002) {
        faceAngle = savedOrientY;
        ReleaseTarget();
        SetActionState(AS_STAND, 0);
        SetIdleAnimation(0, 0);
        return;
    }

    // Dive roll (bit 21)
    if (sd & 0x200000) {
        ReleaseTarget();
        SetActionState(AS_DIVE_ROLL, 0);
        return;
    }

    // Taunt (bit 4) -> pause
    if (sd & 0x0010) {
        ReleaseTarget();
        SetActionState(AS_PAUSE, 0);
        return;
    }

    // Punch (bit 3) -> jump
    if (sd & 0x0008) {
        ReleaseTarget();
        SetActionState(AS_JUMP, 0);
        return;
    }

    // Kick (bit 2) -> face + run
    if (sd & 0x0004) {
        faceAngle = savedOrientY;
        ReleaseTarget();
        SetActionState(AS_RUN, 0);
        return;
    }

    // Strafe (bit 5) -> face + strafe
    if (sd & 0x0020) {
        FaceAngleY(faceAngle, 0);
        ReleaseTarget();
        SetActionState(AS_STRAFE, 0);
        return;
    }

    // PSX: resolve faceAngleData - use rightHandObj's if available (weapon strafe anims)
    s32* animArray;
    if (!rightHandObj || !(animArray = (s32*)(*(void**)((u8*)rightHandObj + 220)))) {
        animArray = (s32*)faceAngleData;
    }

    // PSX: face target if present
    Humanoid* target = (Humanoid*)(intptr_t)field256;
    if (target) {
        FaceThingDesired(target);
        FaceAngleY(faceAngle, 1);
        if (target->actionState == AS_DEAD) {
            ReleaseTarget();
        }
    }

    // PSX: model and animStructure (loaded unconditionally on PSX)
    Model* m = static_cast<Model*>(model);
    AnimStructure* animStruct = m ? (AnimStructure*)m->animStructure : nullptr;

    if (moveSpeed != 0) {
        // PSX: movement force uses captured faceAngle direction
        AddForce(moveSpeed, &dir);

        // PSX: ClipAngle360 = mask to 16-bit unsigned [0, 65535]
        s32 angleDiff = (s32)((u32)(savedOrientY - savedFaceAngle) & 0xFFFFu);

        // PSX: select strafe animation based on clipped angle ranges
        s32 animIndex;
        s32 loopType;

        if (angleDiff >= 24577 && angleDiff <= 40959) {
            // 135-225 degrees: back-side strafe
            animIndex = animArray[4];
            loopType = animArray[5];
        }
        else if (angleDiff >= 8193 && angleDiff < 24576) {
            // 45-135 degrees: side strafe
            animIndex = animArray[8];
            loopType = animArray[9];
        }
        else if (angleDiff > 40960 && angleDiff <= 57343) {
            // 225-315 degrees: backward strafe
            animIndex = animArray[6];
            loopType = animArray[7];
        }
        else {
            // 0-45 degrees or 315-360 degrees: forward strafe
            animIndex = animArray[2];
            loopType = animArray[3];
        }

        if (m) {
            m->SetAnim(animIndex, 0, 0, 0);
        }
        if (animStruct) {
            animStruct->SetLoopType(loopType, 0);
        }
    }
    else {
        // PSX: no movement - play idle when strafe anim loops back to start
        if (animStruct) {
            // PSX: *(__int16*)(animStruct + 62) == *(__int16*)(animStruct + 66)
            // = currentFrame.hi == startFrame.hi (animation looped back to start)
            bool looped = (s16)((u32)animStruct->currentFrame >> 16) == (s16)((u32)animStruct->startFrame >> 16);
            if (animStruct->animEnum != animArray[0] && looped) {
                if (m) {
                    m->SetAnim(animArray[0], 3, 0, 0);
                }
            }
        }
    }
}

// PSX: _Jump__8Humanoid (HUMANOID.CPP:4569)
// Apply forces, check air attack, call HandleLand.
void Humanoid::_Jump() {
    MARKFUNCTION(0x80067DBC);

    flags2 |= 0x0008; // ground sticking

    // If kick bit set (bit 2), apply directional jump
    if (commandBits & 0x0004) {
        FaceAngleY(faceAngle, 1);
        SVector dir;
        dir.x = (s16)orientation.x;
        dir.y = 0;
        dir.z = (s16)orientation.y;
        dir.pad = 0;
        AddForce(runSpeed, &dir);
    }

    // PSX: check animation frame > threshold for air attack
    // Check attack bits (8,9,14)
    if ((commandBits >> 8) & 1 || (commandBits >> 9) & 1 || (commandBits >> 14) & 1) {
        commandBits = (commandBits | 0x4000) & ~0x0100 & ~0x0200;
        SetActionState(AS_PUNCH_ATTACK, 0);
        return;
    }

    // PSX: call HandleLand (checks if landed on ground)
    HandleLand(0);
}

// PSX: _Fall__8Humanoid (HUMANOID.CPP:4620)
// Empty function on PSX (8 bytes, just jr $ra + nop)
void Humanoid::_Fall() {
    MARKFUNCTION(0x80067F2C);
}

// PSX: _DoStand__8Humanoid (HUMANOID.CPP:6274, 0x80069A04)
// Callback target used by AnimStructure::ProcessHumanoidCB selector 61.
void Humanoid::_DoStand() {
    MARKFUNCTION(0x80069A04);
    SetActionState(AS_STAND, 0);
}

// PSX: _DoRun__8Humanoid (HUMANOID.CPP:6280, 0x80069A34)
// Callback target used by AnimStructure::ProcessHumanoidCB selector 62.
void Humanoid::_DoRun() {
    MARKFUNCTION(0x80069A34);
    if (!model) {
        return;
    }

    Model* m = static_cast<Model*>(model);
    m->SetAnim(HUMANOID_ANIM_RUN, ANIM_BLEND, 0, 0);
}

// PSX: _Pickup__8Humanoid (HUMANOID.CPP:4959)
// Grab item at animation frame threshold.
void Humanoid::_Pickup() {
    MARKFUNCTION(0x80068508);

    // PSX: checks model->animStruct frame >= grabFrame, then sets up pickup
    // Without animation system, use stateTimer as proxy
    stateTimer++;

    if (rightHandObj != 0 && stateTimer > 10) {
        flags2 |= 0x0001; // carrying flag
    }

    // PSX: if animation complete, return to stand
    if (stateTimer > 20) {
        SetActionState(AS_STAND, 0);
    }
}

// PSX: Hotfoot__8Humanoid (HUMANOID.CPP:4644, 0x80067F54)
void Humanoid::_Hotfoot() {
    MARKFUNCTION(0x80067F54);

    const s32 faceImmediate = ((flags2 & TF2_NIS_ENTER) != 0) ? 0 : 1;
    FaceAngleY(faceAngle, faceImmediate);

    SVector dir = {};
    dir.x = (s16)orientation.x;
    dir.y = 0;
    dir.z = (s16)orientation.y;
    dir.pad = 0;

    if (((u32)commandBits >> 2) & 1u) {
        AddForce(runSpeed, &dir);
    }

    if (((u32)commandBits >> 6) & 1u) {
        AddForce(runSpeed, &dir);
    }

    if (((u32)commandBits >> 4) & 1u) {
        SetActionState(AS_PAUSE, 0);
    }

    if (((u32)commandBits >> 3) & 1u) {
        SetActionState(AS_JUMP, 0);
    }

    const bool nisActive = (flags2 & TF2_NIS_MASK) != 0;
    if (!nisActive && (((u32)field368 >> 3) & 1u) == 0) {
        SetActionState(AS_RUN, 0);
    }

    if (model) {
        Model* m = static_cast<Model*>(model);
        AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
        if (anim && anim->loopCount > 0) {
            SetActionState(AS_STAND, 0);
        }
    }

    if (health == 0) {
        SetActionState(AS_DEAD, 0);
    }
}

// PSX: _LadderLatchTop__8Humanoid (HUMANOID.CPP:6362, 0x80069B94)
void Humanoid::_LadderLatchTop() {
    MARKFUNCTION(0x80069B94);

    velocity = {};
    contactForce = {};
    maxFallDivisor = 0;

    if (!model) {
        return;
    }

    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (anim && anim->loopCount > 0) {
        flags2 &= ~0x70;
        SetActionState(AS_LADDER_CLIMBING, 0);
        RestorePositionFromBip01();
    }
}

// PSX: _LadderLatch__8Humanoid (HUMANOID.CPP:6393, 0x80069C2C)
void Humanoid::_LadderLatch() {
    MARKFUNCTION(0x80069C2C);

    const u32 f368 = static_cast<u32>(field368);
    velocity = {};
    contactForce = {};
    maxFallDivisor = 0;

    if (((f368 >> 1) & 1u) == 0) {
        SetActionState(AS_LADDER_DISMOUNT, 0);
    }

    if (!model) {
        return;
    }

    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (anim && anim->loopCount > 0) {
        SetActionState(AS_LADDER_CLIMBING, 0);
    }
}

// PSX: LadderDismount__8Humanoid (HUMANOID.CPP:6426, 0x80069CC8)
void Humanoid::_LadderDismount() {
    MARKFUNCTION(0x80069CC8);
    _Jump();
}

// PSX: ClimbLadder__8Humanoid (HUMANOID.CPP:6448, 0x80069CF8)
void Humanoid::_ClimbLadder() {
    MARKFUNCTION(0x80069CF8);

    s32 climbUp = 0;
    s32 slideDown = 0;

    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);

    // PSX: check commandBits bit 2 (directional input active)
    if ((commandBits >> 2) & 1) {
        // wrap (orientation.y - faceAngle) into 0..0xFFFF range
        s32 angleDiff = orientation.y - faceAngle;
        while (angleDiff > 0xFFFF) {
            angleDiff -= 0xFFFF;
        }
        while (angleDiff < 0) {
            angleDiff += 0xFFFF;
        }

        climbUp = (u32)(angleDiff - 0x2000) > 0xC000;
        slideDown = (u32)(angleDiff - 0x6001) < 0x3FFF;
    }

    // clear velocity, contactForce, maxFallDivisor
    velocity = {};
    contactForce = {};
    maxFallDivisor = 0;

    // PSX: NIS override check - if player and director is running NIS ladder script,
    // force climbing behavior (skip dismount check)
    s32 nisOverride = 0;
    if (this == (Humanoid*)Player::s_player) {
        if (g_director && g_director->scriptState != 0) {
            if (g_director->codeSnipPtr == Director::GetNISLadder1Script()) {
                nisOverride = 1;
            }
        }
    }

    if (!nisOverride) {
        // dismount check: if not holding ladder input, jump off
        s32 shouldDismount = 0;
        if (!(field368 & 2) || (commandBits & 8) || (commandBits & 16)) {
            shouldDismount = 1;
        }
        if (shouldDismount) {
            SetActionState(AS_LADDER_DISMOUNT, 0);
            return;
        }
    }

    if (climbUp) {
        // end any slide sound
        if (humanoidSound) {
            humanoidSound->EndSlideDownLadder();
        }

        // switch to climb-up anim (292) if not already playing
        if (anim->animEnum != 292) {
            m->SetAnim(292, 0, 0, 0);
            anim->SetLoopType(ANIM_BLEND, 1);
            flags2 = (flags2 & ~0x70) | 0x50;
            field516 = 0;
            field520 = 0;
            field524 = 0;
        }

        anim->IncFrame();

        // play footstep at frames 10 and 2
        s16 frame = (s16)((u32)anim->currentFrame >> 16);
        if (frame == 10 || frame == 2) {
            if (humanoidSound) {
                humanoidSound->Footstep(CSoundMaterial(2));
            }
        }

    }
    else if (slideDown) {
        // begin slide sound
        if (humanoidSound) {
            humanoidSound->BeginSlideDownLadder();
        }

        // switch to slide-down anim (293) if not already playing
        if (anim->animEnum != 293) {
            m->SetAnim(293, 0, 0, 0);
            RestorePositionFromBip01();
            // clear bits 4,5,6
            flags2 &= ~0x70;
        }

        // slide down: decrease homePos.y
        homePos.y -= 64;
    }
}

// PSX: _LedgeLatch__8Humanoid (HUMANOID.CPP:6753, 0x8006A4C4)
void Humanoid::_LedgeLatch() {
    MARKFUNCTION(0x8006A4C4);

    if (!model) {
        return;
    }

    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    s32 shouldPullUp = 0;
    if (anim && anim->loopCount > 0) {
        shouldPullUp = 1;
    }
    else if ((commandBits & (1 << GA_MOVE)) != 0) {
        shouldPullUp = 1;
    }

    if (shouldPullUp) {
        SetActionState(AS_LEDGE_PULLUP, 0);
    }
}

// PSX: _LedgePullup__8Humanoid (HUMANOID.CPP:6764, 0x8006A538)
void Humanoid::_LedgePullup() {
    MARKFUNCTION(0x8006A538);

    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = m ? static_cast<AnimStructure*>(m->animStructure) : nullptr;
    if (anim && anim->loopCount > 0) {
        SetActionState(AS_STAND, 0);
    }

    if (collBboxMin.y < 0) {
        collBboxMin.y = 0;
    }
}

// PSX: TestAndSetRisingAttack__8Humanoid (HUMANOID.CPP:5438, 0x80068D38)
// Checks command bit 9 (rising attack), sets combat flags, finds combo node.
// Combat system not fully reversed - stub for now.
s32 Humanoid::TestAndSetRisingAttack() {
    MARKFUNCTION(0x80068D38);
    if (field488 != 0) {
        return field488;
    }

    const u32 requested = static_cast<u32>(commandBits);
    if (((requested >> GA_KICK) & 1u) != 0) {
        u32 remapped = requested;
        remapped &= ~(1u << GA_PUNCH);         // clear bit 8
        remapped &= ~(1u << GA_KICK);          // clear bit 9
        remapped &= ~(1u << GA_GRAB);          // clear bit 7
        remapped &= ~(1u << GA_GRAB_FORWARD);  // clear bit 15
        remapped &= ~(1u << 17);               // clear combo bit 17
        remapped &= ~(1u << 18);               // clear combo bit 18
        remapped |= (1u << 23);                // set rising-attack request bit

        commandBits = static_cast<s32>(remapped);
        field488 = FindSiblingWithRequestedCommand(
            static_cast<const FightingComboNode*>(defaultFightingSystem), remapped);
    }

    return field488;
}

// PSX: LetGoOfLedge__8Humanoid (HUMANOID.CPP:8735, 0x8006C478)
// Releases from ledge: repositions away from ledge face and transitions to fall.
s32 Humanoid::LetGoOfLedge() {
    MARKFUNCTION(0x8006C478);

    // PSX: check actionState == 23 and animEnum == 31 (LEDGE_LATCH)
    s32 ok = 0;
    if (actionState == (s32)AS_LEDGE_LATCH) {
        if (model) {
            Model* m = static_cast<Model*>(model);
            AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
            if (anim && anim->animEnum == 31) {
                ok = 1;
            }
        }
    }

    if (ok) {
        SetActionState(AS_FALL, 0);

        // PSX: reposition away from ledge face by -300 units along orientation
        s32 hx = homePos.x;
        s32 hy = homePos.y;
        s32 hz = homePos.z;
        s32 sinY = rmSin16(orientation.y);
        s32 cosY = rmSin16((s16)(orientation.y + 0x4000));
        s32 newX = hx + (s32)((-300LL * sinY) >> 16);
        s32 newY = hy - 850;
        s32 newZ = hz + (s32)((-300LL * cosY) >> 16);
        pos.x = newX;
        pos.y = newY;
        pos.z = newZ;
        homePos.x = newX;
        homePos.y = newY;
        homePos.z = newZ;
    }

    return ok;
}

// PSX: _NISMode__8Humanoid (HUMANOID.CPP:8770, 0x8006C564)
void Humanoid::_NISMode() {
    MARKFUNCTION(0x8006C564);

    flags &= ~TF_DYNAMIC;
    velocity = {};
    contactForce = {};
    maxFallDivisor = 0;

    if (model) {
        HumanoidModel* humanoidModel = static_cast<HumanoidModel*>(model);
        humanoidModel->field116 = 0;
    }
}

// PSX: _Throw__8Humanoid (HUMANOID.CPP:4998, 0x800685A8)
// Face target during early frames, release thrown object at animation
// frame threshold, transition to stand when animation completes.
void Humanoid::_Throw() {
    MARKFUNCTION(0x800685A8);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    s16 frame = (s16)((u32)anim->currentFrame >> 16);

    // PSX: face target during first 6 frames
    if (frame < 6 && field256 != 0) {
        Thing* target = (Thing*)(intptr_t)field256;
        FaceThing(target, 1);
    }

    // PSX: if pickup object exists and past throw frame, release it
    if (rightHandObj != 0) {
        // PSX: GetThrowMoveThrowFrame (not reversed) - use frame 8 as proxy
        if (frame >= 8) {
            if (flags2 & 0x0001) {
                // PSX: carrying flag set - release with direction
                if (this == (Humanoid*)Player::s_player) {
                    PlayDialog(84, 10);
                }
                rightHandObj = nullptr;
                flags2 &= ~0x0001;
            }
            else {
                // PSX: release without direction
                rightHandObj = nullptr;
            }
        }
    }

    // PSX: if animation completed (loopCount > 0)
    if (anim->loopCount > 0) {
        ReleaseTarget();
        SetActionState(AS_STAND, 0);
    }
}

// PSX: _GotHitHigh__8Humanoid (HUMANOID.CPP:5114, 0x8006882C)
// First frame: force animation to specific global frame. Adjusts speed
// based on field466 knockback. Sets death state if health gone.
void Humanoid::_GotHitHigh() {
    MARKFUNCTION(0x8006882C);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: on first frame (walkCycleFlag == 46), force specific global frame
    if (walkCycleFlag == 46) {
        // PSX: ForceFrame(gp+1856) - global value, use 0 as default
        anim->ForceFrame(0);
        walkCycleFlag = 1;
    }

    // PSX: if field466 (knockback speed) nonzero, adjust animation speed
    if (field466 != 0) {
        // PSX: speed = rmDiv16i(endFrame, field466 << 16)
        if (anim->endFrame != 0) {
            anim->speed = rmDiv16i(anim->endFrame, (s32)field466 << 16);
        }
    }

    // PSX: if health == 0, set walkCycleFlag to AS_DEAD (72)
    if (health <= 0) {
        walkCycleFlag = (s32)AS_DEAD;
    }
}

// PSX: _GotHitMed__8Humanoid (HUMANOID.CPP:5161, 0x800688B4)
// Adjusts animation speed from knockback. Sets death state if HP gone.
void Humanoid::_GotHitMed() {
    MARKFUNCTION(0x800688B4);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: if field466 nonzero, adjust animation speed
    if (field466 != 0) {
        if (anim->endFrame != 0) {
            anim->speed = rmDiv16i(anim->endFrame, (s32)field466 << 16);
        }
    }

    // PSX: if health == 0, set walkCycleFlag to AS_DEAD
    if (health <= 0) {
        walkCycleFlag = (s32)AS_DEAD;
    }
}

// PSX: _GotHitLow__8Humanoid (HUMANOID.CPP:5232, 0x800689B4)
// Identical logic to _GotHitMed: speed adjust + death check.
void Humanoid::_GotHitLow() {
    MARKFUNCTION(0x800689B4);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    if (field466 != 0) {
        if (anim->endFrame != 0) {
            anim->speed = rmDiv16i(anim->endFrame, (s32)field466 << 16);
        }
    }

    if (health <= 0) {
        walkCycleFlag = (s32)AS_DEAD;
    }
}

// PSX: _Stunned__8Humanoid (HUMANOID.CPP:5333, 0x80068AB4)
// Countdown stun timer (field468). On expire, clean up animControl
// and return to stand. On health depletion, go dead.
void Humanoid::_Stunned() {
    MARKFUNCTION(0x80068AB4);

    if ((s16)field468 > 0) {
        // PSX: decrement stun timer by rate (field468 - comboCount)
        field468 = (u16)((u16)field468 - comboCount);
    }
    else {
        // Stun expired
        field468 = 0;

        // PSX: if animControl target exists, signal and clear
        if (animControl != 0) {
            // PSX: *(animControl + 108) = 1 — signal stun target complete
            animControl = 0;
        }

        SetActionState(AS_STAND, 0);
    }

    // PSX: death check (health == 0)
    if (health <= 0) {
        if (animControl != 0) {
            animControl = 0;
        }
        SetActionState(AS_DEAD, 0);
    }
}

// PSX: _SpinBack__8Humanoid (HUMANOID.CPP:5373, 0x80068B78)
// Wait for spin-back animation to complete (loopCount > 0),
// then transition to recovery state.
void Humanoid::_SpinBack() {
    MARKFUNCTION(0x80068B78);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: if loopCount > 0, transition to spin-back recovery
    if (anim->loopCount > 0) {
        SetActionState(AS_SPIN_BACK_RECOVER, 0);
    }
}

// PSX: _FlyingBack__8Humanoid (HUMANOID.CPP:5397, 0x80068BC8)
// Scale velocity by global knockback factor, check animation complete
// for landing transition, check ground for ground-check transition.
void Humanoid::_FlyingBack() {
    MARKFUNCTION(0x80068BC8);

    // PSX: velocity.x *= gp+1860 (knockback damping factor)
    // PSX: maxFallDivisor = 18 / gp+1764
    // Global values not reversed - use defaults
    maxFallDivisor = 18;

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    // PSX: if animation complete (loopCount > 0), transition to landing
    if (anim->loopCount > 0) {
        SetActionState(AS_FLYING_BACK_LAND, 0);
    }

    // PSX: if on ground (flags bit 12), transition to ground check
    if (flags & TF_ON_GROUND) {
        SetActionState(AS_FLYING_BACK_CHECK, 0);
    }
}

// PSX: _Collapse__8Humanoid (HUMANOID.CPP:5476, 0x80068DD4)
// Play collapse groan dialog, call ProcessControl, check animation
// complete + on-ground for get-up/death transition.
void Humanoid::_Collapse() {
    MARKFUNCTION(0x80068DD4);

    if (!model) {
        return;
    }
    Model* m = static_cast<Model*>(model);
    AnimStructure* anim = static_cast<AnimStructure*>(m->animStructure);
    if (!anim) {
        return;
    }

    LoadDialog(1, 50);

    // PSX: vtable+260 = TestAndSetRisingAttack
    TestAndSetRisingAttack();

    // PSX: check loopCount > 0 AND on-ground
    if (anim->loopCount <= 0) {
        return;
    }
    if (!(flags & TF_ON_GROUND)) {
        return;
    }

    // PSX: if health == 0, die
    if (health <= 0) {
        SetActionState(AS_DEAD, 0);
        return;
    }

    // PSX: check stateTimer against humanoidDataID threshold
    if ((s16)humanoidDataID < (s16)stateTimer) {
        // PSX: if not this player AND model has bit 4 flag, signal get-up
        if (this != (Humanoid*)Player::s_player) {
            // PSX: check model->modelFlags bit 4
            if (m->modelFlags & 0x10) {
                Player::s_player->SignalEnemyGetUp();
            }
        }
        SetActionState(AS_GET_UP, 0);
    }
    else {
        stateTimer++;
    }
}

// PSX: _Dead__8Humanoid (HUMANOID.CPP:5723, 0x800691DC)
// Complex death handler: type-specific checks, signal player,
// toggle flags, cleanup, remove from fighting system.
void Humanoid::_Dead() {
    MARKFUNCTION(0x800691DC);

    // PSX: type check for respawn eligibility
    // Types 10, 12, 13, 15, 17 are boss types that don't signal player
    bool isBossType = false;
    switch (thingType) {
        case AITypes::TT_GRONTAR:
        case AITypes::TT_PAUL:
        case AITypes::TT_OSCAR:
        case AITypes::TT_DANTE:
        case AITypes::TT_BUTCH:
            isBossType = true;
            break;
    }

    if (!isBossType) {
        // PSX: SignalEnemyDead(thePlayer, this)
        if (Player::s_player) {
            Player::s_player->SignalEnemyDead(this);
        }

        // PSX: toggle flags bit 8 based on thinkCounter state
        if ((thinkCounter & 0x03) == 2) {
            if (flags & TF_BIT8) {
                flags &= ~TF_BIT8;
            }
            else {
                flags |= TF_BIT8;
            }
        }

        // PSX: check if death animation complete + enough time elapsed
        if (!model) {
            goto cleanup;
        }
        {
            Model* m = static_cast<Model*>(model);
            if (m->modelFlags & 0x10) {
                // Animation still playing
                return;
            }
            if (thinkCounter < 41) {
                return;
            }
        }

cleanup:
        FightingCollision::RemoveHumanoid(this);
        ReleaseTarget();
        flags &= ~0x0080; // clear bit 7

        if (field260 != 0) {
            // PSX: set model flag for fade-out
            if (model) {
                Model* m = static_cast<Model*>(model);
                m->modelFlags |= 0x20;
            }
        }
        else {
            // PSX: call Kill virtual to deactivate
            Kill();
        }
    }

    KillDialog(0, 0, 512);
}

// PSX: LoadDialog__8HumanoidUll (HUMANOID.CPP, 0x8006CB54)
s32 Humanoid::LoadDialog(u32 dialogID, s32 priority) {
    MARKFUNCTION(0x8006CB54);
    s32 handle = rsEvent(RS_LOAD_DIALOG, (s32)thingType, (s32)dialogID, priority);
    if (handle) {
        soundHandle = handle;
        soundParam = (s32)dialogID;
    }
    return 1;
}

// PSX: PlayDialog__8HumanoidUlUl (HUMANOID.CPP, 0x8006CBA0)
s32 Humanoid::PlayDialog(u32 dialogID, s32 priority) {
    MARKFUNCTION(0x8006CBA0);
    if (soundParam != (s32)dialogID) {
        return 0;
    }
    if (soundHandle && jcsValidateHandle(soundHandle)) {
        if (rsEvent(RS_PLAY_DIALOG, soundHandle, (s32)(intptr_t)&pos, priority) != 0) {
            return 1;
        }
        rsEvent(RS_KILL_DIALOG, soundHandle, 0, 0);
    }
    soundHandle = 0;
    soundParam = 0;
    return 1;
}

// PSX: PlayDialogBasedOnPriority__8Humanoidll (HUMANOID.CPP, 0x8006CC38)
s32 Humanoid::PlayDialogBasedOnPriority(s32 minPriority, s32 maxPriority) {
    MARKFUNCTION(0x8006CC38);

    if (!soundHandle) {
        soundHandle = 0;
        soundParam = 0;
        return 0;
    }

    if (!jcsValidateHandle(soundHandle)) {
        soundHandle = 0;
        soundParam = 0;
        return 0;
    }

    s32 dialogPriority = jcsQueryDialogPriority(soundHandle);
    if (dialogPriority >= minPriority) {
        if (maxPriority >= dialogPriority) {
            if (rsEvent(RS_PLAY_DIALOG, soundHandle, (s32)(intptr_t)&pos, 30) != 0) {
                return 1;
            }
            rsEvent(RS_KILL_DIALOG, soundHandle, 0, 0);
            soundHandle = 0;
            soundParam = 0;
            return 0;
        }
    }

    return 0;
}

// PSX: KillDialog__8Humanoidill (HUMANOID.CPP, 0x8006CCF8)
s32 Humanoid::KillDialog(s32 force, s32 minPriority, s32 maxPriority) {
    MARKFUNCTION(0x8006CCF8);

    if (!soundHandle) {
        soundHandle = 0;
        soundParam = 0;
        return 1;
    }

    if (!jcsValidateHandle(soundHandle)) {
        soundHandle = 0;
        soundParam = 0;
        return 1;
    }

    s32 dialogPriority = jcsQueryDialogPriority(soundHandle);
    if (dialogPriority >= minPriority && maxPriority >= dialogPriority) {
        if (!jcsIsPlaying(soundHandle) || force) {
            rsEvent(RS_KILL_DIALOG, soundHandle, 0, 0);
            soundHandle = 0;
            soundParam = 0;
            return 1;
        }
    }

    return 0;
}

// PSX: EnterCombatCombo__8Humanoid (HUMANOID.CPP, 0x80065ECC)
// Searches the fighting combo tree for a matching node based on the current
// request (commandBits) and fighting system. Returns 1 if a combo was entered.
// Depends on FightingComboNode tree, FightTargetAndThrowLatch, etc.
s32 Humanoid::EnterCombatCombo() {
    MARKFUNCTION(0x80065ECC);

    // PSX: if (!TestAndSetWeaponKungFU(this)) defaultFightingSystem = fightingSystem
    // TestAndSetWeaponKungFU checks weapon state and remaps fightingSystem.
    // Without weapon system, always copies fightingSystem to defaultFightingSystem.
    defaultFightingSystem = fightingSystem;

    // PSX: TestWallContextFightingRequestRemap(this) - adjusts commandBits for wall context
    // Without wall collision data, this is a no-op.

    // PSX: if (vtable[64](this) == 1) return 1
    // This checks a virtual function related to special attack states.
    // For base Humanoid, this is not triggered.

    // PSX: FindSiblingWithRequestedCommand(this, defaultFightingSystem, commandBits)
    // defaultFightingSystem (+480) is the combo tree root node.
    // If null, FindSiblingWithRequestedCommand returns 0.
    s32 foundNode = FindSiblingWithRequestedCommand(
        static_cast<const FightingComboNode*>(defaultFightingSystem),
        static_cast<u32>(commandBits));
    field488 = foundNode;

    if (foundNode) {
        // PSX: extract fighting type from node, check type, target, enter combat
        // Not reachable without fighting system data loaded
        return 1;
    }

    field488 = 0;
    return 0;
}
