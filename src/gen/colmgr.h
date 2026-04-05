// colmgr.h - Collision Manager free functions
// Original: C:\CHAN\GAME\SRC\GEN\COLMGR.CPP
#pragma once

#include "common.h"

struct Wall;
struct ccList;
class DynamicThing;

// PSX: ExtendRange__FRllT0 (COLMGR.CPP:195) 0x800A7B80
void ExtendRange(s32& rangeMin, s32 value, s32& rangeMax);

// PSX: HTW_FillWallArray__Flll (COLMGR.CPP:240) 0x800A7BBC
void HTW_FillWallArray(s32 radius, s32 velX, s32 velZ);

// PSX: HTW_HandleWallCollisions__FP12DynamicThinglll (COLMGR.CPP:319) 0x800A7E38
s32 HTW_HandleWallCollisions(DynamicThing* thing, s32 radius, s32 height, s32 checkHeight);

// PSX: HTW_HandleHandFootCollisions__FP12DynamicThing (COLMGR.CPP:409) 0x800A7FAC
void HTW_HandleHandFootCollisions(DynamicThing* thing);

// PSX: HandleThingWall__FP12DynamicThinglll (COLMGR.CPP:510) 0x800A8290
void HandleThingWall(DynamicThing* thing, s32 radius, s32 height, s32 checkHeight);

// PSX: HandleThingFloor__FP12DynamicThinglll (COLMGR.CPP:795) 0x800A8614
void HandleThingFloor(DynamicThing* thing, s32 height, s32 radius, s32 checkHeight);

// PSX: ClearThingFloorHeights__FR6ccList (COLMGR.CPP:1352) 0x800A9284
void ClearThingFloorHeights(ccList& list);

// PSX: HandleThingEnvironmentCollisions__FR6ccList (COLMGR.CPP:1395) 0x800A92C4
void HandleThingEnvironmentCollisions(ccList& thingList);

// PSX: HandleHumanoidObstacleCollisions__FR6ccList (COLMGR.CPP:1667) 0x800A96EC
void HandleHumanoidObstacleCollisions(ccList& obstacleList);

// PSX: HandlePickupObstacleCollisions__FR6ccList (COLMGR.CPP:1699) 0x800A9740
void HandlePickupObstacleCollisions(ccList& obstacleList);

// PSX: HandleHumanoidPickupCollisions__FR6ccListT0 (COLMGR.CPP:1732) 0x800A9794
void HandleHumanoidPickupCollisions(ccList& humanoidList, ccList& pickupList);

// COLMGR globals (PSX: gp-relative)
extern Wall* g_wallPtrArray[];        // gp+2828: scratch buffer for wall pointers (64 entries)
extern s32 g_filledWallCount;         // gp+4172: number of walls in g_wallPtrArray
extern s32 g_maxWallIterations;       // gp+2836: max correction iterations per frame
extern s32 g_wallHitCounter;          // gp+2840: incremented on wall hit in combat
extern s32 g_floorSearchMargin;       // gp+2844: floor AABB search expansion
extern s32 g_floorYSearchOffset;      // gp+2852: vertical offset for floor search
extern s32 g_floorStandingTol;        // gp+2856: landing detection tolerance
extern DynamicThing* g_combatTarget1; // gp+4176: combat target #1
extern DynamicThing* g_combatTarget2; // gp+4180: combat target #2
