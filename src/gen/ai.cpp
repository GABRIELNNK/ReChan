#include "gen/ai.h"
#include "gen/game.h"
#include "gen/database.h"
#include "gen/charmgr.h"
#include "gen/blockmgr.h"
#include "gen/colmgr.h"
#include "ai/colfight.h"
#include "ai/colfight.h"
#include "pc/log.h"
#include "gen/camera.h"
#include "gen/path.h"
#include "p3d/p3dmath.h"
#include "ai/obstacle.h"
#include "ai/player.h"
#include "ai/humanoid.h"
#include "ai/fevolume.h"
#include "ai/activezn.h"
#include "ai/arrow.h"
#include "ai/door.h"
#include "ai/ladder.h"
#include "ai/launcher.h"
#include "ai/trapdoor.h"
#include "ai/platform.h"
#include "ai/teleporter.h"

AI* g_ai = nullptr;

// WorldPoints system - stores NIS reference points parsed from WDB database.
// PSX: WorldPointLists global at gp+FF0 (theWorldPoints).
// Populated from DBPoints with type 110 during AI::Populate.
static ccList g_worldPointList;

// PSX: gp+0x5E0 (0x800DCF2C)
static s32 humanoidVolumeRadius = 0xC0;

static s32 AbsS32(s32 v) {
    if (v < 0) {
        return -v;
    }
    return v;
}

static bool IsHumanoidCollisionSkipStateA(s32 state) {
    if ((u32)(state - 69) < 4u) {
        return true;
    }
    return state == 56;
}

static bool IsHumanoidCollisionSkipStateB(s32 state) {
    return state == 36 || state == 59 || state == 37 || state == 38 ||
        state == 60 || state == 61 || state == 62;
}

void WorldPoints_Reset() {
    while (ccNode* n = static_cast<ccNode*>(g_worldPointList.RemHead())) {
        delete n;
    }
}

void WorldPoints_AddPoint(DBPoint* pt) {
    if (!pt) return;

    if (pt->subType == 0) {
        // Position point (40 bytes on PSX)
        WorldPointNode* node = new WorldPointNode();
        node->pos = pt->pos;
        node->parValue = 9999;
        node->SetName(pt->GetName(), 0);

        const DBAttrib* a15 = pt->FindAttrib(15);
        if (a15) {
            node->parValue = (s32)a15->value;
        }

        g_worldPointList.AddNode(g_worldPointList.tail, node);
    }
    else if (pt->subType == 2) {
        // Par value point (28 bytes on PSX) - stores attrib 4 value
        WorldPointNode* node = new WorldPointNode();
        const DBAttrib* a4 = pt->FindAttrib(4);
        if (a4) {
            node->parValue = (s32)a4->value;
        }
        // PSX: sets name from dword_800DD2E8 (likely "par" or similar)
        node->SetName(pt->GetName(), 0);
        g_worldPointList.AddNode(g_worldPointList.tail, node);
    }
}

WorldPointNode* WorldPoints_GetNISPoint(u32 crc) {
    return static_cast<WorldPointNode*>(g_worldPointList.FindNodeCRC(crc, nullptr));
}

s32 WorldPoints_GetParValue() {
    // PSX: searches by name for the "par" point
    for (ccNode* n = static_cast<ccNode*>(g_worldPointList.head); n; n = static_cast<ccNode*>(n->next)) {
        WorldPointNode* wp = static_cast<WorldPointNode*>(n);
        if (wp->parValue != 9999) {
            return wp->parValue;
        }
    }
    return 0;
}

// PSX Populate__2AI: mesh loop (lst TEXT:80056698) then volume loop (80056748); same nameCRC yields two moveList
// nodes. Skip volume AddThing when a type-6 mesh twin already exists.
static bool SceneVolumeHasMeshTwin(const DBVolume* vol, u16 subType) {
    if (!g_database) {
        return false;
    }
    for (DBMesh* mesh = g_database->GetFirstMesh(); mesh; mesh = static_cast<DBMesh*>(mesh->next)) {
        if (mesh->type == 6 && mesh->subType == subType && mesh->nameCRC == vol->nameCRC) {
            return true;
        }
    }
    return false;
}

// PSX: KillThingsInList__FR6ccListl (AI.CPP:1252 helper)
// For each Thing in list, if blockNum < param, call Kill().
// When param=0, kills nothing (unsigned comparison).
static void KillThingsInList(ccList& list, s32 param) {
    for (ccMinNode* n = list.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        if (thing == Player::s_player) {
            continue;
        }
        if (thing->blockNum < (u32)param) {
            thing->Kill();
        }
    }
}

// PSX: PopulateBlockHelper__FR6ccList (AI.CPP:1958 helper)
// For each Thing in list, call Activate().
static void PopulateBlockHelper(ccList& list) {
    for (ccMinNode* n = list.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        thing->Activate();
    }
}

// PSX: UnpopulateBlockHelper__FR6ccList (AI.CPP:1979 helper)
// For each Thing in list, call Deactivate().
static void UnpopulateBlockHelper(ccList& list) {
    for (ccMinNode* n = list.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        thing->Deactivate();
    }
}

// PSX: __2AI (AI.CPP:297, 0x80054180)
AI::AI() {
    MARKFUNCTION(0x80054180);
}

// PSX: _._2AI (AI.CPP:317, 0x800542C4)
AI::~AI() {
    MARKFUNCTION(0x800542C4);
    while (thingList.RemHead()) {}
    while (activeZoneList.RemHead()) {}
    while (humanoidList.RemHead()) {}
    while (pickupList.RemHead()) {}
    while (inactivePickupList.RemHead()) {}
    while (moveList.RemHead()) {}
    while (behaviourList.RemHead()) {}
}

// PSX: InternalOpen__2AI (AI.CPP:322, 0x8005436C)
void AI::InternalOpen() {
    MARKFUNCTION(0x8005436C);
}

// PSX: InternalClose__2AI (AI.CPP:327, 0x8005438C)
void AI::InternalClose() {
    MARKFUNCTION(0x8005438C);
    KillThings(0);
}

// PSX: InternalReset__2AI (AI.CPP:874, 0x800553A4)
void AI::InternalReset() {
    MARKFUNCTION(0x800553A4);
    populateFlags = 1;
    UnPopulate(0);
}

// PSX: AddActiveZone__2AIP8DBVolume (AI.CPP:338, 0x800543AC)
void AI::AddActiveZone(DBVolume* vol) {
    MARKFUNCTION(0x800543AC);
    ActiveZone* az = new ActiveZone(vol);
    activeZoneList.AddNodeTail(az);
}

// PSX: AddThingNoTagList (AI.CPP:365, 0x80054404)
// Enormous function (~4000 bytes) that creates entities by type.
// Dispatches based on AI::ThingTypes to create Player, enemies, objects, etc.
// PSX: after creation, calls SetName, AnalyzeMesh(root), Reset(), AddNode(targetList)
void AI::AddThingNoTagList(const char* name, u16 type,
                           const LVector* pos, const SVector* orient,
                           const char* modelName, const DBRoot* root) {
    MARKFUNCTION(0x80054404);

    Thing* thing = nullptr;
    ccList* targetList = &humanoidList; // default: humanoidList (offset +52)

    // PSX: types 1-28 = Humanoid enemies (Thug1-8, bosses, etc.)
    if ((u16)(type - 1) < 28u) {
        Humanoid* h = new Humanoid(pos, type);
        thing = h;
    }
    // PSX: types 301-328 = Pickups (+ type 101 = collectible)
    else if ((u16)(type - 301) < 28u) {
        // Pickup class not yet reversed - skip creation
        targetList = &pickupList;
    }
    // PSX: all other types (0, 101, 201, 402-472, etc.)
    else {
        // Type 0 = Player
        if (type == AITypes::TT_PLAYER) {
            Player* player = new Player(pos);
            thing = player;
            if (orient) {
                thing->orientation.x = orient->x;
                thing->orientation.y = orient->y;
                thing->orientation.z = orient->z;
            }
        }
        // Type 101 = Collectible (Pickup class, goes to pickupList)
        else if (type == AITypes::TT_COLLECTIBLE) {
            // Pickup class not yet reversed - skip creation
            targetList = &pickupList;
        }
        // Type 201 = Platform (goes to moveList)
        // Types 402-472 = Interactive objects (go to moveList)
        else if (type == AITypes::TT_PLATFORM || type >= 402) {
            targetList = &moveList;
            // Type 469 = FrontEndVolume (hub level door triggers)
            if (type == AITypes::TT_BOSS) {
                FrontEndVolume* vol = new FrontEndVolume(pos, type);
                thing = vol;
                LOG("[AI] FrontEndVolume created: name=%s pos=(%d,%d,%d)", name ? name : "null", pos->x, pos->y, pos->z);
            }
            else if (type == AITypes::TT_LAUNCHER) {
                thing = new Launcher(pos, type);
            }
            else if (type == AITypes::TT_DOOR) {
                Door* d = new Door(pos, type);
                thing = d;
            }
            else if (type == AITypes::TT_TELEPORTER) {
                thing = new Teleporter(pos, type);
            }
            else if (type == AITypes::TT_LADDER) {
                thing = new Ladder(pos, type);
            }
            else if (type == AITypes::TT_TRAPDOOR) {
                thing = new TrapDoor(pos, type);
            }
            else if (type == AITypes::TT_PLATFORM) {
                thing = new Platform(pos, type);
                LOG("[AI] Platform created: name=%s pos=(%d,%d,%d)", name ? name : "null", pos->x, pos->y, pos->z);
            }
            // Type 472 = Arrow (hub navigational arrow)
            else if (type == AITypes::TT_ARROW) {
                Arrow* arrow = new Arrow(pos, type);
                thing = arrow;
            }
        }
    }

    if (!thing)
        return;

    if (name) {
        thing->SetName(name, 0);
    }

    if (root) {
        const DBAttrib* a3 = root->FindAttrib(3);
        if (a3) {
            thing->collisionRadius = (u16)a3->value;
        }
    }

    if (root) {
        thing->AnalyzeMesh(const_cast<DBRoot*>(root));
    }

    // PSX: for humanoids (type 1-28), OpenCharacter + LoadCharacter + LoadAnimation(0..123)
    if ((u16)(type - 1) < 28u) {
        if (g_characterManager) {
            g_characterManager->LoadCharacter(type);
            // PSX: LoadAnimation(type, 0, 124, callback) - loads anims 0-123 synchronously
            // AnimCallback chains through all 124 anims via LoadAnimationBatch
            g_characterManager->LoadAnimation(type, 0, 124, nullptr);
        }
    }

    // PSX LABEL_162: CreateModel(modelName) then Reset()
    // PSX: if (!type || GetBlockNumber(pos) != 4096) CreateModel(modelName);
    if (!type || !g_blockManager || g_blockManager->GetBlockNumber(*pos) != 4096) {
        thing->CreateModel(modelName);
    }

    if (type == AITypes::TT_BOSS) {
        LOG("[AI] FrontEndVolume post-create: flags=0x%08X modelHash=%p collRadius=%d",
            thing->flags, thing->modelHash, thing->collisionRadius);
    }

    thing->Reset();

    targetList->AddNodeTail(thing);
}

// PSX: HandleHumanoidHumanoidCollision__FP8HumanoidT0 (AI.CPP:986, 0x80055584)
static void HandleHumanoidHumanoidCollision(Humanoid* a, Humanoid* b) {
    const s32 yTolerance = 0x300;
    const s32 xzRadius = humanoidVolumeRadius;
    const s32 stateA = a->actionState;
    const s32 stateB = b->actionState;

    bool skipStateA = IsHumanoidCollisionSkipStateA(stateA) || IsHumanoidCollisionSkipStateA(stateB);
    bool skipStateB = IsHumanoidCollisionSkipStateB(stateA) && IsHumanoidCollisionSkipStateB(stateB);
    bool skipState63 = stateA == 63 || stateB == 63;
    bool skipState73 = stateA == 73 || stateB == 73;
    bool skipState23 = stateA == 23 || stateB == 23;

    if (skipStateA || skipStateB || skipState63 || skipState73 || skipState23) {
        return;
    }

    s32 dx = b->pos.x - a->pos.x;
    s32 dy = b->pos.y - a->pos.y;
    s32 dz = b->pos.z - a->pos.z;
    s32 twiceRadius = xzRadius + xzRadius;

    if (dx < -twiceRadius || dx > twiceRadius) {
        return;
    }
    if (dy < -yTolerance || dy > yTolerance) {
        return;
    }
    if (dz < -twiceRadius || dz > twiceRadius) {
        return;
    }

    bool aMoved = false;
    if (AbsS32(a->pos.x - a->homePos.x) >= 11 || AbsS32(a->pos.z - a->homePos.z) >= 11) {
        aMoved = true;
    }

    bool bMoved = false;
    if (AbsS32(b->pos.x - b->homePos.x) >= 11 || AbsS32(b->pos.z - b->homePos.z) >= 11) {
        bMoved = true;
    }

    bool lockA = false;
    if (a->velocity.x || a->velocity.z || aMoved) {
        lockA = true;
    }

    bool lockB = false;
    if (b->velocity.x || b->velocity.z || bMoved) {
        lockB = true;
    }

    LVector posA = a->pos;
    LVector posB = b->pos;

    s32 diffX = posB.x - posA.x;
    s32 diffZ = posB.z - posA.z;

    s32 half = rmDiv16i(xzRadius, twiceRadius);

    if (AbsS32(diffZ) >= AbsS32(diffX)) {
        if (lockA) {
            if (!lockB) {
                if (posA.z >= posB.z) {
                    posA.z = posB.z + twiceRadius;
                }
                else {
                    posA.z = posB.z - twiceRadius;
                }
            }
            else {
                s32 split = posA.z + (s32)(((s64)half * (s64)diffZ) >> 16);
                if (posA.z >= split) {
                    posA.z = split + xzRadius;
                    posB.z = split - xzRadius;
                }
                else {
                    posA.z = split - xzRadius;
                    posB.z = split + xzRadius;
                }
            }
        }
        else if (lockB) {
            if (posB.z >= posA.z) {
                posB.z = posA.z + twiceRadius;
            }
            else {
                posB.z = posA.z - twiceRadius;
            }
        }
        else {
            s32 split = posA.z + (s32)(((s64)half * (s64)diffZ) >> 16);
            if (posA.z >= split) {
                posA.z = split + xzRadius;
                posB.z = split - xzRadius;
            }
            else {
                posA.z = split - xzRadius;
                posB.z = split + xzRadius;
            }
        }
    }
    else {
        if (lockA) {
            if (!lockB) {
                if (posA.x >= posB.x) {
                    posA.x = posB.x + twiceRadius;
                }
                else {
                    posA.x = posB.x - twiceRadius;
                }
            }
            else {
                s32 split = posA.x + (s32)(((s64)half * (s64)diffX) >> 16);
                if (posA.x >= split) {
                    posA.x = split + xzRadius;
                    posB.x = split - xzRadius;
                }
                else {
                    posA.x = split - xzRadius;
                    posB.x = split + xzRadius;
                }
            }
        }
        else if (lockB) {
            if (posB.x >= posA.x) {
                posB.x = posA.x + twiceRadius;
            }
            else {
                posB.x = posA.x - twiceRadius;
            }
        }
        else {
            s32 split = posA.x + (s32)(((s64)half * (s64)diffX) >> 16);
            if (posA.x >= split) {
                posA.x = split + xzRadius;
                posB.x = split - xzRadius;
            }
            else {
                posA.x = split - xzRadius;
                posB.x = split + xzRadius;
            }
        }
    }

    a->pos = posA;
    b->pos = posB;
}

// PSX: HandleHumanoidHumanoidCollision__Fv (AI.CPP:951, 0x800554D0)
static void HandleHumanoidHumanoidCollision() {
    MARKFUNCTION(0x800554D0);
    Humanoid** arr = FightingCollision::GetHumanoidArray();
    for (s32 i = 0; i < FIGHTING_COLLISION_MAX; ++i) {
        Humanoid* a = arr[i];
        if (!a) {
            continue;
        }

        for (s32 j = i + 1; j < FIGHTING_COLLISION_MAX; ++j) {
            Humanoid* b = arr[j];
            if (!b) {
                continue;
            }
            HandleHumanoidHumanoidCollision(a, b);
        }
    }
}

// PSX: MoveThings__2AI (AI.CPP:1268, 0x80055D10)
// Full per-frame pipeline matching PSX exactly.
void AI::MoveThings() {
    MARKFUNCTION(0x80055D10);

    // 1. Drain thingList (death staging) - delete completed deaths
    while (ccMinNode* n = thingList.RemHead()) {
        delete static_cast<Thing*>(n);
    }

    // 2. Clear floor heights for humanoids
    ClearThingFloorHeights(humanoidList);

    // 3. Humanoid vs humanoid collision
    HandleHumanoidHumanoidCollision();

    // 4. Obstacle collisions
    MoveThingsObstacleCollisions();
    HandleHumanoidObstacleCollisions(humanoidList);

    // 5. Pickup collisions
    MoveThingsPickupCollisions();
    HandleHumanoidPickupCollisions(humanoidList, inactivePickupList);

    // 6. Environment collisions
    HandleThingEnvironmentCollisions(humanoidList);
    HandleThingEnvironmentCollisions(pickupList);

    // 7. Clear humanoid command bits
    for (ccMinNode* n = humanoidList.head; n; n = n->next) {
        Humanoid* humanoid = static_cast<Humanoid*>(n);
        humanoid->commandBits = 0;
    }

    // 8. Pickup deactivation: move deactivated pickups from pickupList to inactivePickupList
    // Pickup::PickupDeactivate not yet reversed - skip deactivation loop

    // 9. Update positions
    UpdatePositions(humanoidList);
    UpdatePositions(pickupList);

    // 10. Think pass (privMoveList handles Think + death transfer)
    privMoveList(moveList);
    privMoveList(humanoidList);
    privMoveList(pickupList);

    // 11. Camera
    MoveCamera();

    // 12. Transfer flagged-for-death from inactivePickupList to thingList
    for (ccMinNode* n = inactivePickupList.head; n;) {
        Thing* thing = static_cast<Thing*>(n);
        ccMinNode* next = n->next;
        if (thing->flags & TF_DEAD) {
            inactivePickupList.RemNode(n);
            thingList.AddNodeTail(n);
        }
        n = next;
    }
}

// PSX: privMoveList__2AIR6ccList (AI.CPP:886, 0x800553DC)
void AI::privMoveList(ccList& list) {
    MARKFUNCTION(0x800553DC);
    for (ccMinNode* n = list.head; n;) {
        Thing* thing = static_cast<Thing*>(n);
        ccMinNode* nextNode = n->next;

        u32 fl = thing->flags;
        if (!(fl & TF_DEAD)) {
            if (!(fl & TF_ACTIVATED)) {
                n = nextNode;
                continue;
            }
            thing->Think();
            fl = thing->flags;
            if (!(fl & TF_DEAD) || (fl & 0x0400)) {
                n = nextNode;
                continue;
            }
        }

        // Transfer thing from source list to thingList (death staging)
        if (Player::s_player == thing) {
            g_game->GetCamera().SetLookAtTarget(nullptr, 1);
        }
        list.RemNode(n);
        thingList.AddNodeTail(n);

        n = nextNode;
    }
}

// PSX: UpdatePositions__2AIR6ccList (AI.CPP:1197, 0x80055BE4)
void AI::UpdatePositions(ccList& list) {
    MARKFUNCTION(0x80055BE4);
    for (ccMinNode* n = list.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        thing->UpdatePosition();
    }
}

// PSX: KillThings__2AIl (AI.CPP:1252, 0x80055CB4)
void AI::KillThings(s32 param) {
    MARKFUNCTION(0x80055CB4);
    KillThingsInList(moveList, param);
    KillThingsInList(inactivePickupList, param);
    KillThingsInList(pickupList, param);
    KillThingsInList(humanoidList, param);
}

// PSX: MoveThingsObstacleCollisions__2AI (AI.CPP:1407, 0x80055F14)
void AI::MoveThingsObstacleCollisions() {
    MARKFUNCTION(0x80055F14);
    HandlePickupObstacleCollisions(pickupList);
    HandlePickupObstacleCollisions(inactivePickupList);
}

// PSX: MoveThingsPickupCollisions__2AI (AI.CPP:1413, 0x80055F44)
void AI::MoveThingsPickupCollisions() {
    MARKFUNCTION(0x80055F44);
    HandleHumanoidPickupCollisions(humanoidList, pickupList);
}

// PSX: MoveCamera__2AI (AI.CPP:1442, 0x80055F6C)
void AI::MoveCamera() {
    MARKFUNCTION(0x80055F6C);
    if (g_game) {
        g_game->GetCamera().Think();
        g_game->GetCamera().Update();
    }
}

// PSX: Populate__2AI (AI.CPP:1575, 0x80056214)
// Iterates DB points, meshes, lines, volumes and spawns entities.
// Special case: type==6/subType==0 = player spawn point (sets position).
void AI::Populate() {
    MARKFUNCTION(0x80056214);
    PopulateActiveZones();
    PopulateActiveZonesPaths();
    PopulateActiveZonesSubZones();

    if (!g_database)
        return;

    // PSX: iterate points list - type 6 = spawnable entity, type 110 = NIS world points
    for (DBPoint* pt = g_database->GetFirstPoint(); pt; pt = static_cast<DBPoint*>(pt->next)) {
        if (pt->type == 110) {
            WorldPoints_AddPoint(pt);
            continue;
        }
        if (pt->type != 6) {
            LOG("[AI::Populate] point: type=%u subType=%u name=%s crc=0x%08X (skipped, type!=6)",
                pt->type, pt->subType, pt->GetName() ? pt->GetName() : "null", pt->GetNameCRC());
            continue;
        }

        u16 subType = pt->subType;
        LOG("[AI::Populate] point: type=%u subType=%u name=%s crc=0x%08X pos=(%d,%d,%d)",
            pt->type, subType, pt->GetName() ? pt->GetName() : "null", pt->GetNameCRC(),
            pt->pos.x, pt->pos.y, pt->pos.z);

        if (subType != 0) {
            // PSX: attrib 5 = model name string for entity creation
            const char* modelName = nullptr;
            const DBAttrib* a5 = pt->FindAttrib(5);
            if (a5) {
                modelName = a5->strValue;
            }
            AddThingNoTagList(pt->GetName(), subType, &pt->pos,
                              (const SVector*)&pt->field40, modelName, pt);
        }
        else {
            // Player spawn point (type 6, subType 0)
            // PSX: player must already exist; Populate configures position/orientation.
            if (!Player::s_player) {
                AddThingNoTagList(pt->GetName(), AITypes::TT_PLAYER, &pt->pos,
                                  (const SVector*)&pt->field40, nullptr, pt);
            }

            Player* player = Player::s_player;
            if (player) {
                player->ClearFloorHeight();
                player->homePos = pt->pos;
                player->pos = pt->pos;

                s32 yRot = pt->field44;
                player->faceAngle = yRot;
                player->FaceAngleY(yRot, 0);

                const DBAttrib* a15 = pt->FindAttrib(15);
                if (a15) {
                    player->blockNum = (u16)a15->value;
                }

                // PSX: calls Reset via vtable, then sets TF_ACTIVATED
                player->Reset();
                player->flags |= TF_ACTIVATED;

                if (g_characterManager) {
                    g_characterManager->LoadCharacter(0);
                }
            }
        }
    }

    // PSX: iterate meshes list - passes mesh->fileName (+60) as modelName
    for (DBMesh* mesh = g_database->GetFirstMesh(); mesh; mesh = static_cast<DBMesh*>(mesh->next)) {
        LOG("[AI::Populate] mesh: type=%u subType=%u name=%s crc=0x%08X pos=(%d,%d,%d) model=%s",
            mesh->type, mesh->subType, mesh->GetName() ? mesh->GetName() : "null",
            mesh->GetNameCRC(), mesh->pos.x, mesh->pos.y, mesh->pos.z,
            mesh->fileName ? mesh->fileName : "null");
        if (mesh->type != 6)
            continue;
        AddThingNoTagList(mesh->GetName(), mesh->subType, &mesh->pos,
                          (const SVector*)&mesh->field40, mesh->fileName, mesh);
    }

    // PSX: iterate lines list
    for (DBLine* line = g_database->GetFirstLine(); line; line = static_cast<DBLine*>(line->next)) {
        if (line->type != 6)
            continue;
        AddThingNoTagList(line->GetName(), line->subType, &line->pos,
                          nullptr, nullptr, line);
    }

    // PSX: iterate volumes list
    s32 volCount = 0;
    for (DBVolume* vol = g_database->GetFirstVolume(); vol; vol = static_cast<DBVolume*>(vol->next)) {
        LOG("[AI::Populate] volume: type=%u subType=%u name=%s pos=(%d,%d,%d)",
            vol->type, vol->subType, vol->GetName() ? vol->GetName() : "null",
            vol->pos.x, vol->pos.y, vol->pos.z);
        volCount++;
        if (vol->type != 6)
            continue;
        if ((vol->subType == AITypes::TT_DOOR || vol->subType == AITypes::TT_TELEPORTER) &&
            SceneVolumeHasMeshTwin(vol, vol->subType)) {
            continue;
        }
        AddThingNoTagList(vol->GetName(), vol->subType, &vol->pos,
                          nullptr, nullptr, vol);
    }
    LOG("[AI::Populate] total volumes: %d", volCount);
}

// PSX: UnPopulate__2AIs (AI.CPP:1906, 0x800567CC)
// PSX preserves the player when blockNum == 0 (normal level transition).
// When blockNum != 0 (UnloadPermanent), the player is deleted.
void AI::UnPopulate(s16 blockNum) {
    MARKFUNCTION(0x800567CC);

    // PSX: remove player from humanoidList before clearing
    Player* player = Player::s_player;
    if (player) {
        humanoidList.RemNode(static_cast<ccMinNode*>(player));
    }

    // PSX: clear all entity lists
    ccMinNode* n;
    while ((n = activeZoneList.RemHead()) != nullptr) { delete n; }
    while ((n = humanoidList.RemHead()) != nullptr) { delete n; }
    while ((n = inactivePickupList.RemHead()) != nullptr) { delete n; }
    while ((n = pickupList.RemHead()) != nullptr) { delete n; }
    while ((n = moveList.RemHead()) != nullptr) { delete n; }

    // PSX: handle player after list cleanup
    if (player) {
        if (blockNum != 0) {
            // UnloadPermanent path: delete the player
            delete player;
            // ~Player sets s_player = nullptr
        }
        else {
            // Normal level transition: keep player, re-add to humanoidList
            player->DeleteRightHandObj();
            player->DeleteLeftHandObj();
            humanoidList.AddNode(nullptr, static_cast<ccMinNode*>(player));
        }
    }

    // PSX: clear thingList (death staging)
    while ((n = thingList.RemHead()) != nullptr) { delete n; }

    // PSX: clear world points (NIS reference points)
    WorldPoints_Reset();
}

// PSX: PopulateActiveZones__2AI (AI.CPP:1449, 0x80055FC8)
void AI::PopulateActiveZones() {
    MARKFUNCTION(0x80055FC8);
    if (!g_database)
        return;
    for (DBVolume* vol = g_database->GetFirstVolume(); vol; vol = static_cast<DBVolume*>(vol->next)) {
        if (vol->subType == 300) {
            AddActiveZone(vol);
        }
    }
}

// PSX: PopulateActiveZonesPaths__2AI (AI.CPP:1471, 0x80056038)
// Iterates DB paths, finds parent ActiveZone via attrib 7 name hash,
// creates LinearPath, adds to matching ActiveZone.
void AI::PopulateActiveZonesPaths() {
    MARKFUNCTION(0x80056038);
    if (!g_database)
        return;
    for (DBPath* path = g_database->GetFirstPath(); path; path = static_cast<DBPath*>(path->next)) {
        // PSX: check path.points.head exists, then
        // check the first point's type==33 and subType==50
        DBPoint* firstPt = static_cast<DBPoint*>(path->points.head);
        if (!firstPt)
            continue;
        if (firstPt->type != 33 || firstPt->subType != 50)
            continue;

        // PSX: attrib 7 on the first point = parent zone name hash
        const DBAttrib* a7 = firstPt->FindAttrib(7);
        if (!a7)
            continue;
        const char* zoneName = a7->strValue;
        if (!zoneName)
            continue;
        u32 crc = p3dHash(zoneName);

        ActiveZone* az = static_cast<ActiveZone*>(activeZoneList.FindNodeCRC(crc));
        if (!az)
            continue;

        LinearPath* lp = new LinearPath();
        lp->Init(path);
        az->AddLinearPath(lp);
    }
}

// PSX: PopulateActiveZonesSubZones__2AI (AI.CPP:1538, 0x80056164)
// Iterates DB volumes with subType 301, creates SubZoneVolume, adds to parent ActiveZone.
void AI::PopulateActiveZonesSubZones() {
    MARKFUNCTION(0x80056164);
    if (!g_database)
        return;
    for (DBVolume* vol = g_database->GetFirstVolume(); vol; vol = static_cast<DBVolume*>(vol->next)) {
        if (vol->subType != 301)
            continue;

        // PSX: attrib 4 = parent zone name hash
        const DBAttrib* a4 = vol->FindAttrib(4);
        if (!a4)
            continue;
        const char* zoneName = a4->strValue;
        if (!zoneName)
            continue;
        u32 crc = p3dHash(zoneName);

        ActiveZone* az = static_cast<ActiveZone*>(activeZoneList.FindNodeCRC(crc));
        if (!az)
            continue;

        SubZoneVolume* szv = new SubZoneVolume(vol);
        az->AddSubZoneVolume(szv);
    }
}

// PSX: PopulateBlock__2AI (AI.CPP:1958, 0x80056A34)
void AI::PopulateBlock() {
    MARKFUNCTION(0x80056A34);
    PopulateBlockHelper(humanoidList);
    PopulateBlockHelper(pickupList);
    PopulateBlockHelper(inactivePickupList);
    PopulateBlockHelper(moveList);
}

// PSX: UnpopulateBlock__2AI (AI.CPP:1979, 0x80056B04)
void AI::UnpopulateBlock() {
    MARKFUNCTION(0x80056B04);
    UnpopulateBlockHelper(humanoidList);
    UnpopulateBlockHelper(pickupList);
    UnpopulateBlockHelper(inactivePickupList);
    UnpopulateBlockHelper(moveList);
}

// PSX: GetPickupWithinReach__2AIP8Humanoid (AI.CPP:1999, 0x80056BFC)
Thing* AI::GetPickupWithinReach(Humanoid* humanoid) {
    MARKFUNCTION(0x80056BFC);
    if (!humanoid)
        return nullptr;

    LVector reachPt;
    reachPt.x = humanoid->pos.x + (s32)((300LL * (s32)sinf(ANGLE2RAD(humanoid->orientation.y))) >> 0);
    reachPt.y = humanoid->pos.y + 300;
    reachPt.z = humanoid->pos.z + (s32)((300LL * (s32)sinf(ANGLE2RAD(humanoid->orientation.y + 0x4000))) >> 0);

    for (ccMinNode* n = pickupList.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        s32 dx = thing->pos.x - reachPt.x;
        s32 dy = thing->pos.y - reachPt.y;
        s32 dz = thing->pos.z - reachPt.z;
        s32 dist = (s32)rmMag3((f32)dx, (f32)dy, (f32)dz);
        if (dist < 550)
            return thing;
    }
    return nullptr;
}

// PSX: FindThing__2AIUl (AI.CPP:2321, 0x80056DE4)
Thing* AI::FindThing(u32 id) {
    MARKFUNCTION(0x80056DE4);
    for (ccMinNode* n = thingList.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        if (thing->uniqueID == (u16)id) return thing;
    }
    return nullptr;
}

// PSX: ParseBehaviourAttribScript__2AI (AI.CPP:2168, 0x800CA650)
// Opens scr\behave.txt via ccFile, tokenizes by keyword hash,
// builds BehaviourAttrib objects, adds to behaviourList (+100).
// Requires ccFile + BehaviourAttrib class reversal.
void AI::ParseBehaviourAttribScript() {
    MARKFUNCTION(0x800CA650);
}

// PSX: aiPrivHandler (AI.CPP:257, 0x800540E0) - handler callback
void aiPrivHandler(Handler* h) {
    MARKFUNCTION(0x800540E0);
    if (g_ai) {
        g_ai->MoveThings();
    }
}
