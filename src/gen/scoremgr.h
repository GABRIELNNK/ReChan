#pragma once
#include "gen/manager.h"

// ScoreManager tracks fight/combo/style scores, collectibles, grades,
// per-level stats, and the fighting chain bonus system.

class Collectible;

// Per-petal stats stored in the level history array.
// 7 levels x 3 petals = 21 entries, 16 bytes each = 336 bytes at +28.
struct PetalStats {
    s32 fightScore;     // +0: best fight score for this petal
    s32 comboScore;     // +4: best combo score
    s32 styleScore;     // +8: best style score
    u8  grade;          // +12: best grade (0-5)
    u8  collectCount;   // +13: collectibles found
    u8  goldDragons;    // +14: gold dragons earned
    u8  pad;            // +15
};

// CheckpointInfo (56 bytes on PSX)
// Lives at Player+636 on PSX.
// Stores checkpoint state for retry-from-checkpoint.
struct CheckpointInfo {
    s32 field0;         // +0
    s32 field4;         // +4
    s32 field8;         // +8
    s32 field12;        // +12
    s32 field16;        // +16
    s32 field20;        // +20
    s32 field24;        // +24
    s32 field28;        // +28: kill threshold
    s32 isValid;        // +32: 1 = valid checkpoint
    s32 levelIndex;     // +36: world level index when set
    s32 petalIndex;     // +40: world petal index when set
    s32 field44;        // +44
    s32 field48;        // +48
    s32 field52;        // +52

    bool IsValid() const;
    void SetValidState(s32 state);
};

// ScoreManager (504 bytes on PSX) - inherits Manager
// PSX layout:
//   +0:    Manager base (28 bytes)
//   +28:   petalStats[21] (336 bytes) - 7 levels x 3 petals
//   +364:  currentFightScore (s32)
//   +368:  currentComboScore (s32)
//   +372:  currentStyleScore (s32)
//   +376:  currentGrade (u8)
//   +377:  currentCollectCount (u8)
//   +378:  currentGoldDragons (u8)
//   +379:  pad
//   +380:  checkpointFightScore (s32)
//   +384:  checkpointComboScore (s32)
//   +388:  checkpointStyleScore (s32)
//   +392:  checkpointGrade+collect (s32)
//   +396:  parValue (s32)
//   +400:  collectibleRegistry[10] (40 bytes)
//   +440:  collectibleGot[10] (40 bytes)
//   +480:  collectibleCount (s32)
//   +484:  field484 (s32)
//   +488:  fightingChainTotal (s32)
//   +492:  fightingChainLast (s32)
//   +496:  fightingChainTimer (s32)
//   +500:  drunkenMasterUnlocked (s32)
class ScoreManager : public Manager {
public:
    PetalStats petalStats[21];      // +28: 7 levels x 3 petals (336 bytes)
    s32 currentFightScore;          // +364
    s32 currentComboScore;          // +368
    s32 currentStyleScore;          // +372
    u8  currentGrade;               // +376
    u8  currentCollectCount;        // +377
    u8  currentGoldDragons;         // +378
    u8  pad379;                     // +379
    s32 checkpointFightScore;       // +380
    s32 checkpointComboScore;       // +384
    s32 checkpointStyleScore;       // +388
    s32 checkpointGradeCollect;     // +392
    s32 parValue;                   // +396
    s32 collectibleRegistry[10];    // +400
    s32 collectibleGot[10];         // +440
    s32 collectibleCount;           // +480
    s32 field484;                   // +484
    s32 fightingChainTotal;         // +488
    s32 fightingChainLast;          // +492
    s32 fightingChainTimer;         // +496
    s32 drunkenMasterUnlocked;      // +500

    // PC
    float secondsPassed;

    ScoreManager();
    ~ScoreManager() override;
    void InternalOpen() override;
    void InternalClose() override;
    void InternalReset() override;
    void InitGameStats();
    void InitLevelStats();
    void SetPar();
    void OpenAllLevels();
    void GiveAllDragons();
    void Step();
    void HandleLevelBegin();
    void HandleLevelEnd();
    void HandleLevelAbort();
    s32 GetLevelEndRating();
    void OpenPetal(u32 level, u32 petal);
    void HandleCheckpoint();
    void HandleCheckpointBegin();
    void Print() const;
    s32 RegisterCollectible(const Collectible* collectible, s32 type);
    s32 RegisterGotCollectible(const Collectible* collectible, s32 type);
    void AddFightPoints(s32 points);
    void AddComboPoints(s32 points);
    void AddStylePoints(s32 points);
    void StepFighting();
    void BreakFightingChain();
    void AddFightingPoints(s32 points);
    void HandleGameBegin();
    u8 CalcGrade();
    s32 CalcGradeXTakes(u8 grade);
    bool CalcGDrags(s32 collectCount);
    s32 GetTotalGoldDragon();
    bool IsDrunkenMasterSuitEnabled();
};

extern ScoreManager* g_scoreManager;
