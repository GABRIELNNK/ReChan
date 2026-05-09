#pragma once
#include "gen/config.h"
#include "fe/oxscrmgr.h"
#include "fe/hditem.h"

#if FIX_ASPECT_RATIO
#include "extra/hudwsfix.h"
#endif

class Humanoid;
struct ThingHandle;

// HUD (712 bytes on PSX: HUD.CPP:318)
// Inherits oxScreenManager (48 bytes on PSX, larger on PC).
// Contains embedded sub-objects for health bars, dragon count,
// destination select, tally screen, etc.
//
// PSX sub-object layout (byte offsets from HUD base):
//   +0:   oxScreenManager (48)
//   +48:  oxScreen embedded (16)
//   +64:  ccNode/Handler for display callback (24)
//   +88:  callback ptr
//   +92:  callback data
//   +96:  padding
//   +100: currentFoe ptr
//   +104: bossHandle ptr
//   +108: visible (s32)
//   +112: dragonShowState (s32)
//   +116: foeTracking (s32)
//   +120: hdHealth playerHealth (36)
//   +156: hdHealth bossHealth (36)
//   +192: hdHealth foeHealth (36)
//   +228: hdHits (64)
//   +292: hdAnimTextOvl takes (128)
//   +420: hdDragon (140)
//   +560: hdDestSelect (24)
//   +584: hdTally (128)
class HUD : public oxScreenManager {
public:
    Humanoid* currentFoe = nullptr;     // +100
    ThingHandle* bossHandle = nullptr;  // +104
    s32 visible = 1;                    // +108
    s32 dragonShowState = 0;            // +112
    s32 foeTracking = 0;               // +116
    s32 inputEnabled = 0;

    hdHealth playerHealth;          // +120
    hdHealth bossHealth;            // +156
    hdHealth foeHealth;             // +192
    hdHits hits;                    // +228
    hdAnimTextOvl takes;            // +292
    hdDragon dragon;                // +420
    hdDestSelect destSelect;        // +560
    hdTally tally;                  // +584

    struct Handler* displayHandler = nullptr;

#if FIX_ASPECT_RATIO
private:
    HUDWidescreenPatches widescreenPatches;
public:
#endif

    // Static boss name buffer (PSX: szBossStatic)
    static char szBossStatic[32];

    HUD();
    ~HUD() override;

    void Display();
    void SetHUDVisible(s32 vis, s32 arg3);
    void ShowDestLevel();
    void OnLoadLevel();
    void OnUnloadLevel();
    void EnableInput(s32 enable);
    void DebugDisplay(s32 flag);
    void UpdateHealth(s32 playerHP, s32 foeHP);
    void SetFoe(Humanoid* foe);
    void UpdateFoe(Humanoid* foe);
    void ShowBossHealth(const char* name);
    void GetGameData();
    void UpdateBonusScore();
    void TriggerBonusUpdate();
    void DisplayTake(s32 takeCount, s32 showAnim);
    void DisplayExtraTake();
    void DisplayTally(s32 tallyType);
    void ToggleShowAll();
    void InternalReset();

    uintptr_t FindScreen(u32 id) override;
    const char** GetScreenNames() override;

    void SelfInit() override;
    void SelfUpdate() override;
};

extern HUD* g_hud;
