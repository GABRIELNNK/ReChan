// ai.h - AI entity manager reversed from PSX AI.CPP
// PSX source: C:\CHAN\GAME\SRC\AI\AI.CPP
// AI is the master entity manager: populates, moves, collides, and kills Things.
// Owner of all entity lists.
#pragma once

#include "gen/manager.h"
#include "gen/handler.h"
#include "ai/thing.h"

class Humanoid;
struct DBVolume;

// AI (116 bytes on PSX) - inherits Manager
// PSX layout:
//   +0:  Manager base (28 bytes)
//   +28: thingList (ccList, 12 bytes)   - staging for deletion
//   +40: activeZoneList (ccList, 12 bytes) - ActiveZone objects
//   +52: humanoidList (ccList, 12 bytes) - humanoids (floor, env collisions, Think)
//   +64: pickupList (ccList, 12 bytes)   - active pickups
//   +76: inactivePickupList (ccList, 12 bytes) - deactivated pickups staging
//   +88: moveList (ccList, 12 bytes)     - extra movement list (privMoveList first)
//   +100: behaviourList (ccList, 12 bytes) - BehaviourAttrib entries
//   +112: populateFlags (s32)
class AI : public Manager {
public:
    ccList thingList;           // +28: staging for deletion
    ccList activeZoneList;      // +40: ActiveZone objects
    ccList humanoidList;        // +52: humanoids
    ccList pickupList;          // +64: active pickups
    ccList inactivePickupList;  // +76: deactivated pickups
    ccList moveList;            // +88: movement list
    ccList behaviourList;       // +100: BehaviourAttrib entries

    s32 populateFlags = 0;  // +112

    // PSX: __2AI (AI.CPP:297, 0x80054180)
    AI();

    // PSX: _._2AI (AI.CPP:317, 0x800542C4)
    ~AI() override;

    // PSX: InternalOpen__2AI (AI.CPP:322, 0x8005436C)
    void InternalOpen() override;

    // PSX: InternalClose__2AI (AI.CPP:327, 0x8005438C)
    void InternalClose() override;

    // PSX: InternalReset__2AI (AI.CPP:874, 0x800553A4)
    void InternalReset() override;

    // PSX: AddActiveZone__2AIP8DBVolume (AI.CPP:338, 0x800543AC)
    void AddActiveZone(DBVolume* vol);

    // PSX: AddThingNoTagList (AI.CPP:365, 0x80054404) - enormous 4000-byte function
    // Creates a Thing from type + parameters and adds to thingList.
    // PSX 6th param is mangled as const LVector* but actually used as const char* model name.
    void AddThingNoTagList(const char* name, u16 type,
                           const LVector* pos, const SVector* orient,
                           const char* modelName, const DBRoot* root);

    // PSX: MoveThings__2AI (AI.CPP:1268, 0x80055D10)
    // Per-frame: Think → Move → UpdatePosition for all active entities
    void MoveThings();

    // PSX: MoveThingsObstacleCollisions__2AI (AI.CPP:1407, 0x80055F14)
    void MoveThingsObstacleCollisions();

    // PSX: MoveThingsPickupCollisions__2AI (AI.CPP:1413, 0x80055F44)
    void MoveThingsPickupCollisions();

    // PSX: MoveCamera__2AI (AI.CPP:1442, 0x80055F6C)
    void MoveCamera();

    // PSX: UpdatePositions__2AIR6ccList (AI.CPP:1197, 0x80055BE4)
    void UpdatePositions(ccList& list);

    // PSX: KillThings__2AIl (AI.CPP:1252, 0x80055CB4)
    void KillThings(s32 param);

    // PSX: Populate__2AI (AI.CPP:1575, 0x80056214)
    void Populate();

    // PSX: UnPopulate__2AIs (AI.CPP:1906, 0x800567CC)
    void UnPopulate(s16 blockNum);

    // PSX: PopulateActiveZones__2AI (AI.CPP:1449, 0x80055FC8)
    void PopulateActiveZones();

    // PSX: PopulateActiveZonesPaths__2AI (AI.CPP:1471, 0x80056038)
    void PopulateActiveZonesPaths();

    // PSX: PopulateActiveZonesSubZones__2AI (AI.CPP:1538, 0x80056164)
    void PopulateActiveZonesSubZones();

    // PSX: PopulateBlock__2AI (AI.CPP:1958, 0x80056A34)
    void PopulateBlock();

    // PSX: UnpopulateBlock__2AI (AI.CPP:1979, 0x80056B04)
    void UnpopulateBlock();

    // PSX: GetPickupWithinReach__2AIP8Humanoid (AI.CPP:1999, 0x80056BFC)
    Thing* GetPickupWithinReach(Humanoid* humanoid);

    // PSX: FindThing__2AIUl (AI.CPP:2321, 0x80056DE4)
    Thing* FindThing(u32 id);

    // PSX: ParseBehaviourAttribScript__2AI (AI.CPP:2168, 0x800CA650)
    void ParseBehaviourAttribScript();

    // PSX: privMoveList__2AIR6ccList (AI.CPP:886, 0x800553DC)
    void privMoveList(ccList& list);
};

// PSX: aiPrivHandler (AI.CPP:257, 0x800540E0) - handler callback
void aiPrivHandler(Handler* h);

// Global AI pointer
extern AI* g_ai;
