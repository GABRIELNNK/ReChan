#pragma once
#include "gen/manager.h"
#include "gen/handler.h"
#include "ai/thing.h"

class Humanoid;
class Obstacle;
struct DBVolume;
struct FightingCollisionAttackType;

// PSX: BehaviourAttrib (56 bytes) - parsed from scr\behave.txt and stored in AI::behaviourList.
struct BehaviourAttrib : public ccNode {
    u16 attackFreq = 50;      // +24 / +0x18
    u16 aggression = 75;      // +26 / +0x1A
    u16 distancing = 80;      // +28 / +0x1C
    u16 comboCount = 0;       // +30 / +0x1E
    u16* comboChances = nullptr; // +32 / +0x20
    char** comboStrings = nullptr; // +36 / +0x24
    u16 minThinkFreq = 0;     // +40 / +0x28
    u16 maxThinkFreq = 5;     // +42 / +0x2A
    u16 runningSpeed = 4000;  // +44 / +0x2C
    u16 strafingSpeed = 1250; // +46 / +0x2E
    u16 punchPerc = 60;       // +48 / +0x30
    u16 kickPerc = 40;        // +50 / +0x32
    u16 throwPerc = 0;        // +52 / +0x34
    u16 circling = 50;        // +54 / +0x36

    BehaviourAttrib();
    ~BehaviourAttrib() override;
};

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
    // Returns the created object pointer (null if nothing was created).
    // PSX 6th param is mangled as const LVector* but actually used as const char* model name.
    Thing* AddThingNoTagList(const char* name, u16 type,
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
    s32 CheckObstacleAttack(
        Obstacle** outObstacles,
        s32 maxCount,
        const Humanoid* humanoid,
        const FightingCollisionAttackType* attackType);
    Thing* FindThing(u32 id);
    void ParseBehaviourAttribScript();
    void privMoveList(ccList& list);
};

// PSX: aiPrivHandler (AI.CPP:257, 0x800540E0) - handler callback
void aiPrivHandler(Handler* h);

// Global AI pointer
extern AI* g_ai;

// WorldPoints - NIS reference points populated from DB type 110 points
struct WorldPointNode : public ccNode {
    LVector pos = {};
    s32 parValue = 9999;
    ~WorldPointNode() override = default;
};
void WorldPoints_Reset();
void WorldPoints_AddPoint(struct DBPoint* pt);
WorldPointNode* WorldPoints_GetNISPoint(u32 crc);
s32 WorldPoints_GetParValue();
