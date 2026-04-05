// ai.cpp - AI entity manager reversed from PSX AI.CPP
// PSX source: C:\CHAN\GAME\SRC\AI\AI.CPP
#include "gen/ai.h"
#include "gen/game.h"

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
void AI::AddThingNoTagList(const char* name, u16 type,
                           const LVector* pos, const SVector* orient,
                           const LVector* pos2, const DBRoot* root) {
    MARKFUNCTION(0x80054404);
    // TODO: entity factory dispatch by type
    LOG("[AI] AddThingNoTagList: type=%d name=%s", type, name ? name : "null");
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
    // PSX: calls gameCamera.Think() and gameCamera.Update()
    // PC: Camera is ticked from gsPlayState directly for now
}

// PSX: Populate__2AI (AI.CPP:1575, 0x80056214)
void AI::Populate() {
    MARKFUNCTION(0x80056214);
    PopulateActiveZones();
    PopulateActiveZonesPaths();
    PopulateActiveZonesSubZones();
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
