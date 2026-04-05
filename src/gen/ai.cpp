// ai.cpp - AI entity manager reversed from PSX AI.CPP
// PSX source: C:\CHAN\GAME\SRC\AI\AI.CPP
#include "gen/ai.h"
#include "gen/game.h"
#include "gen/database.h"
#include "gen/charmgr.h"
#include "gen/blockmgr.h"
#include "ai/player.h"
#include "ai/humanoid.h"

extern CharacterManager* g_characterManager;

AI* g_ai = nullptr;

// PSX: __2AI (AI.CPP:297, 0x80054180)
AI::AI() {
    MARKFUNCTION(0x80054180);
    field88 = 0;
    field92 = 0;
    field96 = 0;
    populateFlags = 0;
    field104 = 0;
    field108 = 0;
    field112 = 0;
}

// PSX: _._2AI (AI.CPP:317, 0x800542C4)
AI::~AI() {
    MARKFUNCTION(0x800542C4);
    // Detach all nodes from lists without deleting - Things may be shared
    // across multiple AI lists. InternalClose/KillThings handles actual cleanup.
    while (thingList.RemHead()) {}
    while (activeList.RemHead()) {}
    while (moveList.RemHead()) {}
    while (killList.RemHead()) {}
    while (activeZoneList.RemHead()) {}
}

// PSX: InternalOpen__2AI (AI.CPP:322, 0x8005436C)
void AI::InternalOpen() {
    MARKFUNCTION(0x8005436C);
}

// PSX: InternalClose__2AI (AI.CPP:327, 0x8005438C)
void AI::InternalClose() {
    MARKFUNCTION(0x8005438C);
    // PSX: kills all things, clears lists
    KillThings(0);
}

// PSX: InternalReset__2AI (AI.CPP:874, 0x800553A4)
void AI::InternalReset() {
    MARKFUNCTION(0x800553A4);
    KillThings(0);
    populateFlags = 0;
}

// PSX: AddActiveZone__2AIP8DBVolume (AI.CPP:338, 0x800543AC)
void AI::AddActiveZone(DBVolume* vol) {
    MARKFUNCTION(0x800543AC);
    // TODO: Add DBVolume to activeZoneList
}

// PSX: AddThingNoTagList (AI.CPP:365, 0x80054404)
// Enormous function (~4000 bytes) that creates entities by type.
// Dispatches based on AI::ThingTypes to create Player, enemies, objects, etc.
// PSX: after creation, calls SetName, AnalyzeMesh(root), Reset(), AddNode(targetList)
void AI::AddThingNoTagList(const char* name, u16 type,
                           const LVector* pos, const SVector* orient,
                           const LVector* pos2, const DBRoot* root) {
    MARKFUNCTION(0x80054404);

    Thing* thing = nullptr;
    ccList* targetList = &moveList; // default: moveList (offset +52)

    // PSX: type 0 = Player
    if (type == AITypes::TT_PLAYER) {
        // PSX: allocates 764-byte Player, constructs with pos
        Player* player = new Player(pos);
        thing = player;
        // PSX: copies orient to thing+40 (orientation) if provided
        if (orient) {
            thing->orientation.x = orient->x;
            thing->orientation.y = orient->y;
            thing->orientation.z = orient->z;
        }
    }
    // PSX: types 1-28 = Humanoid enemies (Thug1-8, bosses, etc.)
    else if (type >= AITypes::TT_THUG1 && type <= AITypes::TT_THUG8 + 20) {
        Humanoid* h = new Humanoid(pos, type);
        thing = h;
    }
    // Other entity types not yet reversed

    if (!thing)
        return;

    // PSX: SetName (for debug identification)
    if (name) {
        thing->SetName(name, 0);
    }

    // PSX: FindAttrib(root, 3) -> collisionRadius
    if (root) {
        const DBAttrib* a3 = root->FindAttrib(3);
        if (a3) {
            thing->collisionRadius = (u16)a3->value;
        }
    }

    // PSX: for humanoids, check attrib 31 (pre-active idle animation)
    // Animation system not yet reversed - skip

    // PSX: AnalyzeMesh (attrib 5 = mesh name hash, attrib 15 = block number)
    if (root) {
        thing->AnalyzeMesh(const_cast<DBRoot*>(root));
    }

    // PSX: for humanoids (type 1-28), OpenCharacter + LoadCharacter
    if (type >= AITypes::TT_THUG1 && type <= AITypes::TT_THUG8 + 20) {
        if (g_characterManager) {
            g_characterManager->LoadCharacter(type);
        }
    }

    // PSX: if block valid, call Reset via vtable
    thing->Reset();

    // PSX: AddNode to target list (moveList for player, moveList for humanoids)
    targetList->AddNodeTail(thing);
}

// PSX: MoveThings__2AI (AI.CPP:1268, 0x80055D10)
// Per-frame loop: Think â†’ handle collisions â†’ Move â†’ UpdatePosition
void AI::MoveThings() {
    MARKFUNCTION(0x80055D10);

    // PSX: iterate activeList calling privMoveList
    privMoveList(activeList);

    // PSX: MoveThingsObstacleCollisions
    MoveThingsObstacleCollisions();

    // PSX: MoveThingsPickupCollisions
    MoveThingsPickupCollisions();

    // PSX: MoveCamera
    MoveCamera();
}

// PSX: privMoveList__2AIR6ccList (AI.CPP:886, 0x800553DC)
void AI::privMoveList(ccList& list) {
    MARKFUNCTION(0x800553DC);
    // PSX: for each Thing in list: Think(), Move(), UpdatePosition()
    for (ccMinNode* n = list.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        thing->Think();
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
    // TODO: iterate killList, destroy marked things
}

// PSX: MoveThingsObstacleCollisions__2AI (AI.CPP:1407, 0x80055F14)
void AI::MoveThingsObstacleCollisions() {
    MARKFUNCTION(0x80055F14);
    // TODO: obstacle collision detection
}

// PSX: MoveThingsPickupCollisions__2AI (AI.CPP:1413, 0x80055F44)
void AI::MoveThingsPickupCollisions() {
    MARKFUNCTION(0x80055F44);
    // TODO: pickup collision detection
}

// PSX: MoveCamera__2AI (AI.CPP:1442, 0x80055F6C)
void AI::MoveCamera() {
    MARKFUNCTION(0x80055F6C);
    // PSX MoveCamera dispatches camera virtuals from theCamera.
    // Keep camera stepping in AI handler order (after things move, before draw).
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

    // PSX: iterate points list - type 6 = spawnable entity
    for (DBPoint* pt = g_database->GetFirstPoint(); pt; pt = static_cast<DBPoint*>(pt->next)) {
        if (pt->type != 6)
            continue;

        u16 subType = pt->subType;

        if (subType != 0) {
            // Non-player entity: find mesh name attrib and spawn
            const char* meshName = nullptr;
            const DBAttrib* a5 = pt->FindAttrib(5);
            if (a5 && a5->strValue) {
                meshName = a5->strValue;
            }
            AddThingNoTagList(pt->GetName(), subType, &pt->pos,
                              (const SVector*)&pt->field40, nullptr, pt);
        } else {
            // Player spawn point (type 6, subType 0)
            // PSX: creates player if needed, then configures with WDB position
            if (!Player::s_player) {
                // PSX: player creation happens via a gp-relative call before Populate
                // configures it. Create via AddThingNoTagList (adds to moveList).
                AddThingNoTagList(pt->GetName(), AITypes::TT_PLAYER, &pt->pos,
                                  (const SVector*)&pt->field40, nullptr, pt);
            }

            Player* player = Player::s_player;
            if (player) {
                // PSX: ClearFloorHeight
                player->ClearFloorHeight();

                player->homePos = pt->pos;
                player->pos = pt->pos;

                // PSX: SetDesiredMoveDirection + FaceAngleY from WDB field44
                s32 yRot = pt->field44;
                player->faceAngle = yRot;
                player->FaceAngleY(yRot, 0);

                // PSX: reads attrib 15 for block number
                const DBAttrib* a15 = pt->FindAttrib(15);
                if (a15) {
                    player->blockNum = (u16)a15->value;
                }

                // PSX: calls Activate
                player->Activate();

                // PSX: move player to activeList for Think() ticking
                player->Remove();
                activeList.AddNodeTail(player);

                player->flags |= TF_ACTIVATED;
                player->jumpReturnHeight = player->homePos.y;

                // PSX: CharacterManager::LoadCharacter(0)
                if (g_characterManager) {
                    g_characterManager->LoadCharacter(0);
                }
            }
        }
    }

    // PSX: iterate meshes list
    for (DBMesh* mesh = g_database->GetFirstMesh(); mesh; mesh = static_cast<DBMesh*>(mesh->next)) {
        if (mesh->type != 6)
            continue;
        AddThingNoTagList(mesh->GetName(), mesh->subType, &mesh->pos,
                          nullptr, nullptr, mesh);
    }

    // PSX: iterate lines list
    for (DBLine* line = g_database->GetFirstLine(); line; line = static_cast<DBLine*>(line->next)) {
        if (line->type != 6)
            continue;
        AddThingNoTagList(line->GetName(), line->subType, &line->pos,
                          nullptr, nullptr, line);
    }

    // PSX: iterate volumes list
    for (DBVolume* vol = g_database->GetFirstVolume(); vol; vol = static_cast<DBVolume*>(vol->next)) {
        if (vol->type != 6)
            continue;
        AddThingNoTagList(vol->GetName(), vol->subType, &vol->pos,
                          nullptr, nullptr, vol);
    }
}

// PSX: UnPopulate__2AIs (AI.CPP:1906, 0x800567CC)
void AI::UnPopulate(s16 blockNum) {
    MARKFUNCTION(0x800567CC);
    // TODO: deactivate entities in block
}

// PSX: PopulateActiveZones__2AI (AI.CPP:1449, 0x80055FC8)
void AI::PopulateActiveZones() {
    MARKFUNCTION(0x80055FC8);
    // TODO: activate entities within active zone volumes
}

// PSX: PopulateActiveZonesPaths__2AI (AI.CPP:1471, 0x80056038)
void AI::PopulateActiveZonesPaths() {
    MARKFUNCTION(0x80056038);
    // TODO: activate entities along path splines
}

// PSX: PopulateActiveZonesSubZones__2AI (AI.CPP:1538, 0x80056164)
void AI::PopulateActiveZonesSubZones() {
    MARKFUNCTION(0x80056164);
    // TODO: activate sub-zone entities
}

// PSX: PopulateBlock__2AI (AI.CPP:1958, 0x80056A34)
void AI::PopulateBlock() {
    MARKFUNCTION(0x80056A34);
}

// PSX: UnpopulateBlock__2AI (AI.CPP:1979, 0x80056B04)
void AI::UnpopulateBlock() {
    MARKFUNCTION(0x80056B04);
}

// PSX: GetPickupWithinReach__2AIP8Humanoid (AI.CPP:1999, 0x80056BFC)
Thing* AI::GetPickupWithinReach(Humanoid* humanoid) {
    MARKFUNCTION(0x80056BFC);
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
void AI::ParseBehaviourAttribScript() {
    MARKFUNCTION(0x800CA650);
    // TODO: parse behaviour attribute data from level
}

// PSX: aiPrivHandler (AI.CPP:257, 0x800540E0) - handler callback
// Called from handlerSet1 each frame. Runs the AI::MoveThings pipeline.
void aiPrivHandler(Handler* h) {
    MARKFUNCTION(0x800540E0);
    if (g_ai) {
        g_ai->MoveThings();
    }
}
