// colmgr.cpp - Collision Manager functions
// Original: C:\CHAN\GAME\SRC\GEN\COLMGR.CPP
#include "gen/colmgr.h"
#include "gen/colsect.h"
#include "gen/colwall.h"
#include "gen/colfloor.h"
#include "gen/cclist.h"
#include "ai/thing.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "p3d/p3dmath.h"
#include "gen/model.h"
#include "gen/animmat.h"
#include "pc/log.h"

static s32 g_floorDebugCounter = 0;

// COLMGR globals (PSX: gp-relative)
Wall* g_wallPtrArray[64] = {};         // gp+2828
s32 g_filledWallCount = 0;           // gp+4172
s32 g_maxWallIterations = 4;         // gp+2836
s32 g_wallHitCounter = 0;            // gp+2840
s32 g_floorSearchMargin = 500;       // gp+2844: floor AABB search expansion
s32 g_floorYSearchOffset = 1024;     // gp+2852: vertical offset for floor search
s32 g_floorStandingTol = 64;         // gp+2856: landing detection tolerance
s32 g_colDefaultHeight = 768;        // gp+2904: default humanoid collision height
s32 g_colMaxRadius = 400;            // gp+2908: max collision radius
s32 g_colDefaultRadius = 175;        // gp+2912: default collision radius
s32 g_colSlopeTol = 0x8000;          // gp+2884: slope tolerance
s32 g_colSlopeForce = 0x4000;        // gp+2888: slope sliding force
s32 g_colFallVelDivisor = 2;         // gp+2900: fall velocity divisor (s16)
s32 g_colFallVelThreshold = 0;       // gp+2902: fall velocity threshold (s16)
Floor* g_floorPtrArrayGlobal[64] = {};   // gp+2828+256 (global floor scratch, 64 ptrs)
s32 g_climbSearchRadius = 512;       // gp+2848: climbing state search radius
s32 g_boneFloorRadius = 2;           // gp+2860: bone floor search radius
s32 g_camLookAheadDist = 1024;       // gp+2868: camera look-ahead distance
s32 g_camEdgeThreshold = 256;        // gp+2872: camera edge height threshold
s32 g_camEdgeCounter = 0;            // gp+2876: camera edge frame counter
s32 g_camEdgeMaxCount = 3;           // gp+2880: camera edge max frames
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
// PSX params: (thing, searchRadius, yMinOffset, checkHeight)
// Reversed from PSX decompile at line 100880-101330
void HandleThingFloor(DynamicThing* thing, s32 radius, s32 yMinOffset, s32 checkHeight) {
    MARKFUNCTION(0x800A8614);

    s32 halfRadius = radius / 2;

    // Work on local copies (PSX writes back only if TF_DYNAMIC at end)
    s32 localHomePosX = thing->homePos.x;
    s32 localHomePosY = thing->homePos.y;
    s32 localHomePosZ = thing->homePos.z;
    s32 localVelX = thing->velocity.x;
    s32 localVelY = thing->velocity.y;
    s32 localVelZ = thing->velocity.z;

    // Compute displacement from pos to homePos
    s32 dispX = localHomePosX - thing->pos.x;
    s32 dispY = localHomePosY - thing->pos.y;
    s32 dispZ = localHomePosZ - thing->pos.z;

    // Build search AABB from min/max of pos and homePos, expanded by halfRadius + searchMargin
    LVector searchMin, searchMax;
    searchMin.x = thing->pos.x;
    searchMin.y = thing->pos.y;
    searchMin.z = thing->pos.z;
    searchMax.x = thing->pos.x;
    searchMax.y = thing->pos.y;
    searchMax.z = thing->pos.z;
    ExtendRange(searchMin.x, localHomePosX, searchMax.x);
    ExtendRange(searchMin.y, localHomePosY, searchMax.y);
    ExtendRange(searchMin.z, localHomePosZ, searchMax.z);

    searchMin.x -= halfRadius + g_floorSearchMargin;
    searchMax.x += halfRadius + g_floorSearchMargin;
    searchMin.z -= halfRadius + g_floorSearchMargin;
    searchMax.z += halfRadius + g_floorSearchMargin;

    // Fill floor array
    Floor* floorArray[64];
    s32 floorCount = CollisionSector::FillWorldFloorArray(searchMin, searchMax, floorArray, 64);
    if (floorCount > 64) floorCount = 64;

    static s32 floorDbgCount = 0;
    if (floorDbgCount < 3) {
        LOG("[ColMgr] HandleThingFloor: pos=(%d,%d,%d) home=(%d,%d,%d) search=(%d,%d,%d)-(%d,%d,%d) floors=%d",
            thing->pos.x, thing->pos.y, thing->pos.z,
            localHomePosX, localHomePosY, localHomePosZ,
            searchMin.x, searchMin.y, searchMin.z,
            searchMax.x, searchMax.y, searchMax.z,
            floorCount);
        floorDbgCount++;
    }

    // PSX: climbing offset (v83, v85)
    s32 climbOffX = 0;
    s32 climbOffY = 0;
    s32 climbOffZ = 0;

    // PSX: detect humanoid for special handling (thingType < 29)
    Humanoid* humanoid = nullptr;
    if (thing->thingType < 29) {
        humanoid = (Humanoid*)thing;
    }

    // PSX: climbing state 23 search offset
    s32 isClimbing = 0;
    if ((!humanoid && Player::s_player && Player::s_player->actionState == 23) ||
        (humanoid && humanoid->actionState == 23)) {
        isClimbing = 1;
    }
    if (isClimbing) {
        s32 sinVal = rmSin16(thing->orientation.y);
        climbOffX = -(s32)(((s64)sinVal * (s64)g_climbSearchRadius) >> 16);
        s32 cosVal = rmSin16((s16)(thing->orientation.y + 0x4000));
        climbOffZ = -(s32)(((s64)cosVal * (s64)g_climbSearchRadius) >> 16);
    }

    // Pass 1: floor/ceiling at OLD position (pos + climbOffset + ySearchOffset)
    LVector testPosOld;
    testPosOld.x = thing->pos.x + climbOffX;
    testPosOld.y = thing->pos.y + g_floorYSearchOffset;
    testPosOld.z = thing->pos.z + climbOffZ;

    s32 floorHOld, ceilingHOld;
    LVector floorNormOld = {};
    LVector ceilingNormOld = {};
    s32 hasRailing = 0;
    LVector railCorrection = {};

    CollisionSector::GetArrayFloorAndCeilingHeight(
        floorArray, floorCount,
        floorHOld, ceilingHOld, floorNormOld, ceilingNormOld,
        &hasRailing, &railCorrection, &testPosOld, 2);

    // Pass 2: floor/ceiling at NEW position (homePos + climbOffset + ySearchOffset)
    LVector testPosNew;
    testPosNew.x = localHomePosX + climbOffX;
    testPosNew.y = thing->pos.y + g_floorYSearchOffset;
    testPosNew.z = localHomePosZ + climbOffZ;

    s32 floorHNew, ceilingHNew;
    LVector floorNormNew = {};
    LVector ceilingNormNew = {};
    s32 hasRailingNew = 0;
    LVector railCorrectionNew = {};

    CollisionSector::GetArrayFloorAndCeilingHeight(
        floorArray, floorCount,
        floorHNew, ceilingHNew, floorNormNew, ceilingNormNew,
        &hasRailingNew, &railCorrectionNew, &testPosNew, 2);

    // Apply railing correction
    if (hasRailingNew) {
        localHomePosX += railCorrectionNew.x;
        localHomePosZ += railCorrectionNew.z;
    }

    s32 bestFloorH = floorHNew;

    // Pass 3: bone-based floor query for humanoids
    if (humanoid) {
        HumanoidModel* hmodel = (HumanoidModel*)humanoid->model;
        if (hmodel && hmodel->animMatrices) {
            s32* boneMatrix = AnimationMatrices::GetMatrix(hmodel->animMatrices, 5);
            if (boneMatrix) {
                LVector bonePos;
                bonePos.x = boneMatrix[5];
                bonePos.y = boneMatrix[6];
                bonePos.z = boneMatrix[7];

                s32 boneFloorH, boneCeilH;
                LVector boneFloorNorm = {};
                LVector boneCeilNorm = {};
                s32 boneRailing = 0;
                LVector boneRailCorr = {};

                CollisionSector::GetArrayFloorAndCeilingHeight(
                    floorArray, floorCount,
                    boneFloorH, boneCeilH, boneFloorNorm, boneCeilNorm,
                    &boneRailing, &boneRailCorr, &bonePos, g_boneFloorRadius);

                bestFloorH = boneFloorH;
            }
        }
    } else {
        // Non-humanoid path: use global player's bone matrix for floor query
        // Then do camera look-ahead edge detection
        Player* player = Player::s_player;
        if (player) {
            HumanoidModel* pmodel = (HumanoidModel*)player->model;
            if (pmodel && pmodel->animMatrices) {
                s32* boneMatrix = AnimationMatrices::GetMatrix(pmodel->animMatrices, 5);
                if (boneMatrix) {
                    LVector bonePos;
                    bonePos.x = boneMatrix[5];
                    bonePos.y = boneMatrix[6];
                    bonePos.z = boneMatrix[7];

                    s32 boneFloorH, boneCeilH;
                    LVector boneFloorNorm = {};
                    LVector boneCeilNorm = {};
                    s32 boneRailing = 0;
                    LVector boneRailCorr = {};

                    CollisionSector::GetArrayFloorAndCeilingHeight(
                        floorArray, floorCount,
                        boneFloorH, boneCeilH, boneFloorNorm, boneCeilNorm,
                        &boneRailing, &boneRailCorr, &bonePos, g_boneFloorRadius);

                    bestFloorH = boneFloorH;
                }
            }

            // Camera look-ahead edge detection
            s32 sinVal = rmSin16(player->orientation.y);
            s32 cosVal = rmSin16((s16)(player->orientation.y + 0x4000));

            LVector lookAheadPos;
            lookAheadPos.x = testPosOld.x + (s32)(((s64)g_camLookAheadDist * (s64)sinVal) >> 16);
            lookAheadPos.y = testPosOld.y + checkHeight;
            lookAheadPos.z = testPosOld.z + (s32)(((s64)g_camLookAheadDist * (s64)cosVal) >> 16);

            s32 lookFloorH, lookCeilH;
            LVector lookFloorNorm = {};
            LVector lookCeilNorm = {};
            s32 lookRailing = 0;
            LVector lookRailCorr = {};

            CollisionSector::GetArrayFloorAndCeilingHeight(
                floorArray, floorCount,
                lookFloorH, lookCeilH, lookFloorNorm, lookCeilNorm,
                &lookRailing, &lookRailCorr, &lookAheadPos, 16);

            if (lookFloorH + g_camEdgeThreshold >= floorHNew) {
                g_camEdgeCounter = 0;
            } else {
                g_camEdgeCounter++;
            }
            if (g_camEdgeMaxCount >= g_camEdgeCounter) {
                thing->flags &= ~0x40000;
            } else {
                thing->flags |= 0x40000;
            }
        }
    }

    // Slope correction from floor normals
    s32 slopeCorr = 0;
    if (floorHOld > (s32)0x80000001) {
        s32 slopeDot = (s32)(((s64)floorNormOld.z * (s64)dispZ) >> 16)
                     + (s32)(((s64)floorNormOld.x * (s64)dispX) >> 16);
        if (floorNormOld.y != 0) {
            slopeCorr = rmDiv16i(slopeDot, floorNormOld.y) + 2;
        }
    }
    s32 slopeCorr2 = 0;
    if (floorHNew > (s32)0x80000001) {
        s32 slopeDot = (s32)(((s64)floorNormNew.z * (s64)dispZ) >> 16)
                     + (s32)(((s64)floorNormNew.x * (s64)dispX) >> 16);
        if (floorNormNew.y != 0) {
            slopeCorr2 = rmDiv16i(slopeDot, floorNormNew.y) + 2;
        }
    }
    if (slopeCorr2 < slopeCorr) slopeCorr2 = slopeCorr;
    if (slopeCorr2 < 0) slopeCorr2 = 0;

    // Ceiling collision
    if (ceilingHNew != 0x7FFFFFFF && ceilingHNew - checkHeight < localHomePosY) {
        localHomePosY = ceilingHNew - checkHeight - 1;
        if (localVelY > 0) localVelY = 0;
    }

    // Clear ground/slope/ceiling bits, preserve others
    bool wasOnGround = (thing->flags >> 12) & 1;
    thing->flags &= ~(TF_ON_GROUND | 0x10000 | 0x20000);

    // Floor normal output
    LVector outFloorNorm = {};
    outFloorNorm.y = FIX16_ONE;

    // Landing logic
    if (floorHNew > (s32)0x80000001) {
        s32 landingLevel = floorHNew - yMinOffset;

        if (thing->pos.y >= landingLevel - g_floorStandingTol && localHomePosY < landingLevel + slopeCorr2) {

            // PSX: falling velocity division for actionState 63 (flying back)
            if (!wasOnGround) {
                s32 absVelY = localVelY < 0 ? -localVelY : localVelY;
                s32 threshold = g_colFallVelThreshold < 0 ? -g_colFallVelThreshold : g_colFallVelThreshold;
                if (absVelY >= threshold && thing->thingType < 29) {
                    Humanoid* hum = (Humanoid*)thing;
                    if (hum->actionState == 63 && g_colFallVelDivisor != 0) {
                        localVelY = -(localVelY / g_colFallVelDivisor);
                    }
                }
            }

            // Set floor-contact flag
            thing->flags |= 0x10000;

            // Land
            thing->Land();
            localHomePosY = landingLevel + 1;
            if (localVelY < 0) localVelY = 0;

            // Slope sliding force
            if (g_colSlopeTol >= floorNormNew.y) {
                s64 slopeScale = (s64)g_colSlopeForce * (s64)thing->stateCounter;
                LVector slopeForce;
                LVector slopeDir;
                slopeDir.x = (s32)(((s64)floorNormNew.x * (s64)floorNormNew.y) >> 16);
                slopeDir.y = (s32)(((s64)floorNormNew.y * (s64)floorNormNew.y) >> 16) - 0x10000;
                slopeDir.z = (s32)(((s64)floorNormNew.z * (s64)floorNormNew.y) >> 16);
                rmV3Scale(&slopeForce, &slopeDir, (s32)(slopeScale >> 16));
                slopeForce.y = 0;
                thing->contactForce.x += slopeForce.x;
                thing->contactForce.y += slopeForce.y;
                thing->contactForce.z += slopeForce.z;
                thing->flags |= 0x20000;
                outFloorNorm = floorNormNew;
            }

            // PSX: virtual HandleLand call if not already on ground
            if (!wasOnGround) {
                thing->HandleLand(localHomePosY);
            }

            // PSX: store ceiling norm into humanoid field436
            if (humanoid) {
                humanoid->field436 = ceilingNormNew.x;
            }

            // PSX: pickup/obstacle effect after landing (not yet reversed)
            // Checks thing+308 != 0, then calls PlayEffect + Kill
        }
    }

    // Write back local variables if TF_DYNAMIC
    if (thing->flags & TF_DYNAMIC) {
        thing->homePos.x = localHomePosX;
        thing->homePos.y = localHomePosY;
        thing->homePos.z = localHomePosZ;
        thing->velocity.x = localVelX;
        thing->velocity.y = localVelY;
        thing->velocity.z = localVelZ;
    }

    // Store floor normal
    thing->field148[6] = outFloorNorm.x;
    thing->field148[7] = outFloorNorm.y;
    thing->field148[8] = outFloorNorm.z;

    // Set floor height on model
    thing->SetFloorHeight(bestFloorH);

    // Store ground height for standing-on detection
    if (thing->flags & TF_ON_GROUND) {
        thing->groundStandHeight = localHomePosY;
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

        // PSX params: radius, yMinOffset (CollisionYMin), checkHeight
        s32 radius, yMinOffset, ckHeight;
        bool doWall = true;

        if (thing->thingType < 29) {
            // Humanoid - use collision bbox values
            Humanoid* hum = (Humanoid*)thing;
            s32 state = hum->actionState;

            // States that skip wall collision: climbing (23), hanging (24)
            if (state == 23 || state == 24) {
                doWall = false;
            }

            // PSX: v7 = gp+2912 (default collision radius), v10 = 768 (default height)
            // PSX: if collBboxMax.z < gp+2904: v7 = gp+2908, v10 = gp+2904
            // Use humanoid bbox values as approximation of PSX globals
            radius = g_colDefaultRadius;
            yMinOffset = 0;
            ckHeight = hum->collBboxMax.z;
            if (ckHeight == 0) ckHeight = 768;
        } else if (thing->thingType == 101) {
            // Pickup
            radius = 0;
            yMinOffset = 0;
            ckHeight = 0;
        } else if (thing->thingType >= 301 && thing->thingType <= 328) {
            // Obstacle
            radius = g_colDefaultRadius;
            yMinOffset = 0;
            ckHeight = 768;
        } else {
            // Default
            radius = g_colDefaultRadius;
            yMinOffset = 0;
            ckHeight = 768;
        }

        // PSX: GetTicketIssuer check
        Thing* ticketIssuer = thing->GetTicketIssuer();

        if (doWall) {
            HandleThingWall(thing, radius, yMinOffset, ckHeight);
        }

        if (ticketIssuer) {
            // Standing on an obstacle - just land, skip floor check
            thing->Land();
        } else {
            HandleThingFloor(thing, radius, yMinOffset, ckHeight);
        }

        // PSX: if climbing state and not on ground and no ticket, let go of ledge
        if (!doWall && !thing->GetTicketIssuer() && !(thing->flags & TF_ON_GROUND)) {
            // LetGoOfLedge not yet reversed
        }
    }

    // Post-loop: re-process combat targets with their collision bbox
    if (g_combatTarget1 != nullptr) {
        Humanoid* h = (Humanoid*)g_combatTarget1;
        HandleThingWall(g_combatTarget1, g_colDefaultRadius, h->collBboxMin.y, h->collBboxMax.z);
    }
    if (g_combatTarget2 != nullptr) {
        Humanoid* h = (Humanoid*)g_combatTarget2;
        HandleThingWall(g_combatTarget2, g_colDefaultRadius, h->collBboxMin.y, h->collBboxMax.z);
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
