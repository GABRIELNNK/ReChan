// colmgr.cpp - Collision Manager functions
// Original: C:\CHAN\GAME\SRC\GEN\COLMGR.CPP
#include "gen/colmgr.h"
#include "gen/colsect.h"
#include "gen/colwall.h"
#include "gen/colfloor.h"
#include "gen/cclist.h"
#include "ai/thing.h"
#include "ai/humanoid.h"
#include "p3d/p3dmath.h"

// COLMGR globals (PSX: gp-relative)
Wall* g_wallPtrArray[64] = {};         // gp+2828
s32 g_filledWallCount = 0;           // gp+4172
s32 g_maxWallIterations = 4;         // gp+2836
s32 g_wallHitCounter = 0;            // gp+2840
s32 g_floorSearchMargin = 0x10000;   // gp+2844
s32 g_floorYSearchOffset = 0x40000;  // gp+2852
s32 g_floorStandingTol = 0x1000;     // gp+2856
DynamicThing* g_combatTarget1 = nullptr; // gp+4176
DynamicThing* g_combatTarget2 = nullptr; // gp+4180

// PSX: ExtendRange__FRllT0 (COLMGR.CPP:195) 0x800A7B80
void ExtendRange(s32& rangeMin, s32 value, s32& rangeMax) {
    MARKFUNCTION(0x800A7B80);
    if (value < rangeMin) rangeMin = value;
    if (value > rangeMax) rangeMax = value;
}

// PSX: HTW_FillWallArray__Flll (COLMGR.CPP:240) 0x800A7BBC
// Fills g_wallPtrArray with wall pointers in the entity's vicinity
void HTW_FillWallArray(s32 radius, s32 velX, s32 velZ) {
    MARKFUNCTION(0x800A7BBC);

    CollisionSector* sect0 = &g_collisionSectors[0];

    // Base search range from sector 0 bounds, padded by 0x400
    s32 searchMinX = sect0->boundsMin.x - 0x400;
    s32 searchMinZ = sect0->boundsMin.z - 0x400;
    s32 searchMaxX = sect0->boundsMax.x + 0x400;
    s32 searchMaxZ = sect0->boundsMax.z + 0x400;

    // Extend by velocity direction
    if (velX > 0) {
        ExtendRange(searchMinX, searchMaxX + velX, searchMaxX);
    } else {
        ExtendRange(searchMinX, searchMinX + velX, searchMaxX);
    }
    if (velZ > 0) {
        ExtendRange(searchMinZ, searchMaxZ + velZ, searchMaxZ);
    } else {
        ExtendRange(searchMinZ, searchMinZ + velZ, searchMaxZ);
    }

    LVector searchMin = {searchMinX, 0, searchMinZ};
    LVector searchMax = {searchMaxX, 0, searchMaxZ};

    s32 count = CollisionSector::FillWorldWallArray(searchMin, searchMax, g_wallPtrArray, 64);
    if (count > 64) count = 64;
    g_filledWallCount = count;
}

// PSX: HTW_HandleWallCollisions__FP12DynamicThinglll (COLMGR.CPP:319) 0x800A7E38
// Iterative wall collision correction loop
s32 HTW_HandleWallCollisions(DynamicThing* thing, s32 radius, s32 height, s32 checkHeight) {
    MARKFUNCTION(0x800A7E38);

    s32 wallHitFlags = 0;

    for (s32 iter = 0; iter < g_maxWallIterations; iter++) {
        s32 hit = CollisionSector::CheckArrayWallCollision(
            g_wallPtrArray, g_filledWallCount,
            thing->homePos, thing->homePos,
            radius, height, 0, checkHeight);

        if (!hit) break;

        wallHitFlags |= 0x8000;

        // Apply correction: push homePos along wall normal by collision deficit
        s32 pushDist = radius - g_wallCollisionInfo.collisionRatio + 2;
        thing->homePos.x += fixmul16(g_wallCollisionInfo.wallNormal.x, pushDist);
        thing->homePos.z += fixmul16(g_wallCollisionInfo.wallNormal.z, pushDist);
    }

    return wallHitFlags;
}

// PSX: HTW_HandleHandFootCollisions__FP12DynamicThing (COLMGR.CPP:409) 0x800A7FAC
// Checks wall collisions for attack limb positions (player only)
void HTW_HandleHandFootCollisions(DynamicThing* thing) {
    MARKFUNCTION(0x800A7FAC);
    // NOT YET IMPLEMENTED - requires AnimationMatrices which doesn't exist yet
}

// PSX: HandleThingWall__FP12DynamicThinglll (COLMGR.CPP:510) 0x800A8290
// Main wall collision handler for a single DynamicThing
void HandleThingWall(DynamicThing* thing, s32 radius, s32 height, s32 checkHeight) {
    MARKFUNCTION(0x800A8290);

    LVector savedHomePos = thing->homePos;

    // Fill wall array for this entity's region
    HTW_FillWallArray(radius, thing->velocity.x, thing->velocity.z);

    // Run iterative wall correction
    s32 hitFlags = HTW_HandleWallCollisions(thing, radius, height, checkHeight);

    // Wall intersection test for movement direction
    LVector moveDir;
    moveDir.x = thing->homePos.x - thing->pos.x;
    moveDir.y = thing->homePos.y - thing->pos.y;
    moveDir.z = thing->homePos.z - thing->pos.z;

    CollisionSector::CheckArrayWallIntersection(
        g_wallPtrArray, g_filledWallCount,
        thing->homePos, moveDir,
        radius, height, 0, checkHeight);

    // Hand/foot collision for player combat
    HTW_HandleHandFootCollisions(thing);

    // Combat wall hit tracking for humanoids
    if (thing->thingType < 29 && (hitFlags & 0x8000)) {
        Humanoid* hum = (Humanoid*)thing;
        s32 state = hum->actionState;

        // Combat action states that trigger wall-hit effects
        if (state == 36 || state == 59 || state == 37 ||
            state == 38 || state == 60 || state == 61 || state == 62) {
            if (g_combatTarget1 == nullptr) {
                g_combatTarget1 = thing;
            } else {
                g_combatTarget2 = thing;
            }
        }
        g_wallHitCounter++;
    }

    // Transfer wall correction to underlying obstacle if standing on one
    Thing* standingOn = thing->GetTicketIssuer();
    if (standingOn != nullptr && (hitFlags & 0x8000)) {
        // PSX casts the issuer to DynamicThing for homePos access
        DynamicThing* dynStandingOn = (DynamicThing*)standingOn;
        dynStandingOn->homePos.x += (thing->homePos.x - savedHomePos.x);
        dynStandingOn->homePos.z += (thing->homePos.z - savedHomePos.z);
    }
}

// PSX: HandleThingFloor__FP12DynamicThinglll (COLMGR.CPP:795) 0x800A8614
// Main floor collision handler: landing, ceiling, slopes, railings
void HandleThingFloor(DynamicThing* thing, s32 height, s32 radius, s32 checkHeight) {
    MARKFUNCTION(0x800A8614);

    // Build floor search AABB
    s32 deltaX = thing->homePos.x - thing->pos.x;
    s32 deltaZ = thing->homePos.z - thing->pos.z;

    LVector searchMin, searchMax;
    searchMin.x = thing->homePos.x - g_floorSearchMargin;
    searchMin.y = thing->homePos.y - g_floorYSearchOffset;
    searchMin.z = thing->homePos.z - g_floorSearchMargin;
    searchMax.x = thing->homePos.x + g_floorSearchMargin;
    searchMax.y = thing->homePos.y + g_floorYSearchOffset;
    searchMax.z = thing->homePos.z + g_floorSearchMargin;

    // Extend by velocity
    if (deltaX > 0) searchMax.x += deltaX; else searchMin.x += deltaX;
    if (deltaZ > 0) searchMax.z += deltaZ; else searchMin.z += deltaZ;

    // Fill floor array
    Floor* floorArray[64];
    s32 floorCount = CollisionSector::FillWorldFloorArray(searchMin, searchMax, floorArray, 64);
    if (floorCount > 64) floorCount = 64;

    // Pass 1: Floor/ceiling at homePos
    s32 floorH, ceilingH;
    LVector floorNorm, ceilingNorm;
    s32 hasRailing = 0;

    LVector testPos = thing->homePos;
    testPos.y += height;

    CollisionSector::GetArrayFloorAndCeilingHeight(
        floorArray, floorCount,
        floorH, ceilingH, floorNorm, ceilingNorm,
        &hasRailing, &testPos, radius);

    // Ceiling collision
    if (ceilingH != 0x7FFFFFFF) {
        if (thing->homePos.y + height > ceilingH) {
            thing->flags |= 0x40000; // TF_CEILING_HIT
            thing->homePos.y = ceilingH - height;
            if (thing->velocity.y > 0) {
                thing->velocity.y = 0;
            }
        }
    } else {
        thing->flags &= ~0x40000;
    }

    // Floor landing logic
    if (floorH != 0x7FFFFFFF) {
        s32 standDist = thing->homePos.y - floorH;

        if (standDist <= g_floorStandingTol && standDist >= -g_floorStandingTol) {
            // Within landing tolerance
            if (!(thing->flags & TF_ON_GROUND)) {
                thing->Land();
            }
            thing->homePos.y = floorH;
            thing->velocity.y = 0;
        } else if (standDist < -g_floorStandingTol) {
            // Below floor — push up
            thing->homePos.y = floorH;
            thing->velocity.y = 0;
            if (!(thing->flags & TF_ON_GROUND)) {
                thing->Land();
            }
        }
    }

    // Store floor normal into DynamicThing (field148[6..8])
    thing->field148[6] = floorNorm.x;
    thing->field148[7] = floorNorm.y;
    thing->field148[8] = floorNorm.z;

    // Set floor height
    thing->SetFloorHeight(floorH);

    // HandleLand callback
    if (thing->flags & TF_ON_GROUND) {
        thing->HandleLand(floorH);
    }
}

// PSX: ClearThingFloorHeights__FR6ccList (COLMGR.CPP:1352) 0x800A9284
void ClearThingFloorHeights(ccList& list) {
    MARKFUNCTION(0x800A9284);
    for (ccMinNode* node = list.head; node != nullptr; node = node->next) {
        ((Thing*)node)->ClearFloorHeight();
    }
}

// PSX: HandleThingEnvironmentCollisions__FR6ccList (COLMGR.CPP:1395) 0x800A92C4
// Main collision dispatcher - iterates entity list, dispatches wall+floor handlers
void HandleThingEnvironmentCollisions(ccList& thingList) {
    MARKFUNCTION(0x800A92C4);

    g_combatTarget1 = nullptr;
    g_combatTarget2 = nullptr;

    for (ccMinNode* node = thingList.head; node != nullptr; node = node->next) {
        DynamicThing* thing = (DynamicThing*)node;

        if (!(thing->flags & TF_MODEL_CREATED)) continue;

        s32 radius, height, ckHeight;
        bool doWall = true;
        bool doFloor = true;

        if (thing->thingType < 29) {
            // Humanoid
            Humanoid* hum = (Humanoid*)thing;
            s32 state = hum->actionState;

            // States that skip wall collision: climbing (23), hanging (24), ledge (56)
            if (state == 23 || state == 24 || state == 56) {
                doWall = false;
            }

            radius = 0x10000;     // default humanoid walk radius
            height = 0x18000;     // default humanoid height
            ckHeight = height;
        } else if (thing->thingType == 101) {
            // Pickup
            radius = 0;
            height = 0x8000;
            ckHeight = 0;
        } else if (thing->thingType >= 301 && thing->thingType <= 328) {
            // Obstacle
            radius = 0x10000;
            height = 0x20000;
            ckHeight = height;
        } else {
            // Default
            radius = 0x10000;
            height = 0x10000;
            ckHeight = height;
        }

        if (doWall) {
            HandleThingWall(thing, radius, height, ckHeight);
        }
        if (doFloor) {
            HandleThingFloor(thing, height, radius, ckHeight);
        }
    }

    // Post-loop: re-process combat targets
    if (g_combatTarget1 != nullptr) {
        HandleThingWall(g_combatTarget1, 0x10000, 0x20000, 0x20000);
    }
    if (g_combatTarget2 != nullptr) {
        HandleThingWall(g_combatTarget2, 0x10000, 0x20000, 0x20000);
    }
}

// PSX: HandleHumanoidObstacleCollisions__FR6ccList (COLMGR.CPP:1667) 0x800A96EC
void HandleHumanoidObstacleCollisions(ccList& obstacleList) {
    MARKFUNCTION(0x800A96EC);
    // NOT YET IMPLEMENTED - requires Obstacle class
}

// PSX: HandlePickupObstacleCollisions__FR6ccList (COLMGR.CPP:1699) 0x800A9740
void HandlePickupObstacleCollisions(ccList& obstacleList) {
    MARKFUNCTION(0x800A9740);
    // NOT YET IMPLEMENTED - requires Obstacle class
}

// PSX: HandleHumanoidPickupCollisions__FR6ccListT0 (COLMGR.CPP:1732) 0x800A9794
void HandleHumanoidPickupCollisions(ccList& humanoidList, ccList& pickupList) {
    MARKFUNCTION(0x800A9794);
    // NOT YET IMPLEMENTED - requires Pickup + CollisionVolume classes
}
