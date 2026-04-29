#pragma once
#include "core.h"

struct Wall;
struct ccList;
class DynamicThing;

void ExtendRange(s32& rangeMin, s32 value, s32& rangeMax);
void HTW_FillWallArray(s32 radius, s32 height, s32 checkHeight);
s32 HTW_HandleWallCollisions(DynamicThing* thing, s32 radius, s32 height, s32 checkHeight);
void HTW_HandleHandFootCollisions(DynamicThing* thing);
void HandleThingWall(DynamicThing* thing, s32 radius, s32 height, s32 checkHeight);
void HandleThingFloor(DynamicThing* thing, s32 radius, s32 yMinOffset, s32 checkHeight);
void ClearThingFloorHeights(ccList& list);
void HandleThingEnvironmentCollisions(ccList& thingList);
void HandleHumanoidObstacleCollisions(ccList& obstacleList);
void HandlePickupObstacleCollisions(ccList& pickupList);
void HandleHumanoidPickupCollisions(ccList& humanoidList, ccList& pickupList);

// COLMGR globals (PSX: gp-relative)
extern Wall* g_wallPtrArray[];        // gp+2828: scratch buffer for wall pointers (64 entries)
extern s32 g_filledWallCount;         // gp+4172: number of walls in g_wallPtrArray
extern s32 g_maxWallIterations;       // gp+2836: max correction iterations per frame
extern s32 g_wallHitCounter;          // gp+2840: incremented on wall hit in combat
extern s32 g_floorSearchMargin;       // gp+2844: floor AABB search expansion
extern s32 g_climbSearchRadius;       // gp+2848: climb state search radius
extern s32 g_floorYSearchOffset;      // gp+2852: vertical offset for floor search
extern s32 g_floorStandingTol;        // gp+2856: landing detection tolerance
extern s32 g_colDefaultHeight;        // gp+2904: default humanoid collision height
extern s32 g_colMaxRadius;            // gp+2908: max collision radius (close encounters)
extern s32 g_colDefaultRadius;        // gp+2912: default collision radius
extern s32 g_colSlopeTol;             // gp+2884: slope tolerance for sliding
extern s32 g_colSlopeForce;           // gp+2888: slope sliding force multiplier
extern DynamicThing* g_combatTarget1; // gp+4176: combat target #1
extern DynamicThing* g_combatTarget2; // gp+4180: combat target #2
