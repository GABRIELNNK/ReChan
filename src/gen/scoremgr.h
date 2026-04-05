// scoremgr.h - ScoreManager class reversed from PSX SCOREMGR.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\SCOREMGR.CPP
// ScoreManager tracks fight/combo/style scores, collectibles, grades,
// per-level stats, and the fighting chain bonus system.
#pragma once

#include "gen/manager.h"

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

    // PSX: IsValid__14CheckpointInfo (SCOREMGR.CPP:910, 0x8004D72C)
    bool IsValid() const;

    // PSX: SetValidState__14CheckpointInfoi (SCOREMGR.CPP:935, 0x8004D798)
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

    // PSX: __12ScoreManager (SCOREMGR.CPP:241, 0x8004CC5C)
    ScoreManager();

    // PSX: _._12ScoreManager (SCOREMGR.CPP:254, 0x8004CCA8)
    ~ScoreManager() override;

    // PSX: InternalOpen__12ScoreManager (SCOREMGR.CPP:268, 0x8004CCD8)
    void InternalOpen() override;

    // PSX: InternalClose__12ScoreManager (SCOREMGR.CPP:284, 0x8004CD64)
    void InternalClose() override;

    // PSX: InternalReset__12ScoreManager (SCOREMGR.CPP:294, 0x8004CD84)
    void InternalReset() override;

    // PSX: InitGameStats__12ScoreManager (SCOREMGR.CPP:303, 0x8004CD8C)
    void InitGameStats();

    // PSX: InitLevelStats__12ScoreManager (SCOREMGR.CPP:326, 0x8004CDEC)
    void InitLevelStats();

    // PSX: SetPar__12ScoreManager (SCOREMGR.CPP:364, 0x8004CEA0)
    void SetPar();

    // PSX: OpenAllLevels__12ScoreManager (SCOREMGR.CPP:376, 0x8004CEE0)
    void OpenAllLevels();

    // PSX: GiveAllDragons__12ScoreManager (SCOREMGR.CPP:392, 0x8004CF24)
    void GiveAllDragons();

    // PSX: Step__12ScoreManager (SCOREMGR.CPP:418, 0x8004CF84)
    void Step();

    // PSX: HandleLevelBegin__12ScoreManager (SCOREMGR.CPP:429, 0x8004CFA4)
    void HandleLevelBegin();

    // PSX: HandleLevelEnd__12ScoreManager (SCOREMGR.CPP:442, 0x8004CFC4)
    void HandleLevelEnd();

    // PSX: HandleLevelAbort__12ScoreManager (SCOREMGR.CPP:478, 0x8004D0D4)
    void HandleLevelAbort();

    // PSX: GetLevelEndRating__12ScoreManager (SCOREMGR.CPP:491, 0x8004D0DC)
    s32 GetLevelEndRating();

    // PSX: OpenPetal__12ScoreManagerUlUl (SCOREMGR.CPP:527, 0x8004D144)
    void OpenPetal(u32 level, u32 petal);

    // PSX: HandleCheckpoint__12ScoreManager (SCOREMGR.CPP:542, 0x8004D184)
    void HandleCheckpoint();

    // PSX: HandleCheckpointBegin__12ScoreManager (SCOREMGR.CPP:562, 0x8004D1DC)
    void HandleCheckpointBegin();

    // PSX: Print__C12ScoreManager (SCOREMGR.CPP:584, 0x8004D234)
    void Print() const;

    // PSX: RegisterCollectible__12ScoreManagerPC11Collectiblei (SCOREMGR.CPP:611, 0x8004D260)
    void RegisterCollectible(const Collectible* collectible, s32 type);

    // PSX: RegisterGotCollectible__12ScoreManagerPC11Collectiblei (SCOREMGR.CPP:649, 0x8004D2E0)
    void RegisterGotCollectible(const Collectible* collectible, s32 type);

    // PSX: AddFightPoints__12ScoreManagerl (SCOREMGR.CPP:693, 0x8004D388)
    void AddFightPoints(s32 points);

    // PSX: AddComboPoints__12ScoreManagerl (SCOREMGR.CPP:698, 0x8004D39C)
    void AddComboPoints(s32 points);

    // PSX: AddStylePoints__12ScoreManagerl (SCOREMGR.CPP:703, 0x8004D3B0)
    void AddStylePoints(s32 points);

    // PSX: StepFighting__12ScoreManager (SCOREMGR.CPP:735, 0x8004D3C4)
    void StepFighting();

    // PSX: BreakFightingChain__12ScoreManager (SCOREMGR.CPP:762, 0x8004D408)
    void BreakFightingChain();

    // PSX: AddFightingPoints__12ScoreManagerl (SCOREMGR.CPP:780, 0x8004D45C)
    void AddFightingPoints(s32 points);

    // PSX: HandleGameBegin__12ScoreManager (SCOREMGR.CPP:798, 0x8004D4C8)
    void HandleGameBegin();

    // PSX: CalcGrade__12ScoreManager (SCOREMGR.CPP:809, 0x8004D518)
    u8 CalcGrade();

    // PSX: CalcGradeXTakes__12ScoreManagerUc (SCOREMGR.CPP:839, 0x8004D5AC)
    s32 CalcGradeXTakes(u8 grade);

    // PSX: CalcGDrags__12ScoreManageri (SCOREMGR.CPP:845, 0x8004D5C0)
    bool CalcGDrags(s32 collectCount);

    // PSX: GetTotalGoldDragon__12ScoreManager (SCOREMGR.CPP:851, 0x8004D5CC)
    s32 GetTotalGoldDragon();

    // PSX: IsDrunkenMasterSuitEnabled__12ScoreManager (SCOREMGR.CPP:883, 0x8004D69C)
    bool IsDrunkenMasterSuitEnabled();
};

extern ScoreManager* g_scoreManager;
