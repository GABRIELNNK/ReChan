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

    AI();
    ~AI() override;

    void InternalOpen() override;
    void InternalClose() override;
    void InternalReset() override;
    void AddActiveZone(DBVolume* vol);

    // Creates a Thing from type + parameters and adds to thingList.
    // PSX 6th param is mangled as const LVector* but actually used as const char* model name.
    void AddThingNoTagList(const char* name, u16 type,
                           const LVector* pos, const SVector* orient,
                           const char* modelName, const DBRoot* root);

    // Per-frame: Think → Move → UpdatePosition for all active entities
    void MoveThings();
    void MoveThingsObstacleCollisions();
    void MoveThingsPickupCollisions();
    void MoveCamera();
    void UpdatePositions(ccList& list);
    void KillThings(s32 param);
    void Populate();
    void UnPopulate(s16 blockNum);
    void PopulateActiveZones();
    void PopulateActiveZonesPaths();
    void PopulateActiveZonesSubZones();
    void PopulateBlock();
    void UnpopulateBlock();
    Thing* GetPickupWithinReach(Humanoid* humanoid);
    Thing* FindThing(u32 id);
    void ParseBehaviourAttribScript();
    void privMoveList(ccList& list);
};

// PSX: aiPrivHandler (AI.CPP:257, 0x800540E0) - handler callback
void aiPrivHandler(Handler* h);

// Global AI pointer
extern AI* g_ai;
