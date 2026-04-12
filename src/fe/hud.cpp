#include "fe/hud.h"
#include "gen/game.h"
#include "gen/world.h"
#include "gen/scoremgr.h"
#include "xclib/xclib.h"
#include "gen/handler.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/ai.h"

HUD* g_hud = nullptr;
char HUD::szBossStatic[32] = {};

// PSX: gp+1084 - screen names table
static const char* s_hudScreenNames[] = { "HUD", nullptr };

// PSX: DisplayXHUD__3HUDP7Handler (HUD.CPP:352, 0x8003F658)
static void DisplayXHUD(Handler*) {
    if (g_hud) {
        g_hud->Display();
    }
}

// PSX: _3HUD (HUD.CPP:318, 0x8003F44C)
HUD::HUD() {
    MARKFUNCTION(0x8003F44C);
    currentFoe = nullptr;
    bossHandle = nullptr;
    visible = 1;
    dragonShowState = 0;
    foeTracking = 0;
}

// PSX: __3HUD (HUD.CPP:331, 0x8003F53C)
HUD::~HUD() {
    MARKFUNCTION(0x8003F53C);
}

// PSX: Display__3HUD (HUD.CPP:357, 0x8003F67C)
void HUD::Display() {
    MARKFUNCTION(0x8003F67C);
    // PSX: EnterLayer(view0, 4)
    if (tally.state == 9) {
        GetGameData();
    }
    Update();
    if (section) {
        section->Draw();
    }
    // PSX: ExitLayer(view0, 4)
}

// PSX: SetHUDVisible__3HUDii (HUD.CPP:367, 0x8003F6F8)
void HUD::SetHUDVisible(s32 vis, s32 arg3) {
    MARKFUNCTION(0x8003F6F8);
    visible = vis;
    EnableInput(vis);
    if (visible) {
        World* world = g_game ? g_game->GetWorld() : nullptr;
        if (world && world->GetCurLevelID() == 7) {
            return;
        }
        playerHealth.SetVisible(1);
        hits.SetVisible(1);
        tally.Show(0);
        destSelect.Hide();
        takes.SetPauseState(dragonShowState, 1);
        dragon.SetPauseState(dragonShowState, 1);
        if (dragonShowState) {
            takes.Play();
            dragon.Play();
        }
    } else {
        playerHealth.SetVisible(0);
        bossHealth.SetVisible(0);
        foeHealth.SetVisible(0);
        hits.SetVisible(0);
        tally.Show(0);
        hits.hitCount = 0;
        destSelect.Hide();
        takes.SetPauseState(0, 1);
        dragon.SetPauseState(0, arg3);
    }
    if (g_scoreManager) {
        dragon.SetNum(g_scoreManager->currentCollectCount);
        dragon.SetGoldDragons(g_scoreManager->currentGoldDragons);
    }
    if (Player::s_player) {
        takes.SetNumber(Player::s_player->livesLeft - 1);
    }
}

// PSX: DisplayTally__3HUDi (HUD.CPP:413, 0x8003F8A4)
void HUD::DisplayTally(s32 tallyType) {
    MARKFUNCTION(0x8003F8A4);
    SetHUDVisible(0, 1);
    tally.Start(tallyType);
}

// PSX: ShowDestLevel__3HUD (HUD.CPP:418, 0x8003F8D0)
void HUD::ShowDestLevel() {
    MARKFUNCTION(0x8003F8D0);
    SetHUDVisible(0, 1);
    s32 totalGold = 0;
    if (g_scoreManager) {
        totalGold = g_scoreManager->GetTotalGoldDragon();
    }
    destSelect.Start(totalGold);
}

// PSX: SelfInit__3HUD (HUD.CPP:431, 0x8003F918)
void HUD::SelfInit() {
    MARKFUNCTION(0x8003F918);
    currentFoe = nullptr;
    bossHandle = nullptr;
    foeTracking = 0;
    EnableInput(1);
    playerHealth.SetMax(205);
    DebugDisplay(0);

    u8* raw = section ? section->rawData : nullptr;

    // playerHealth (+120)
    playerHealth.rawData = raw;
    playerHealth.Init(FindOverlay((u32)0xD6549407));

    // bossHealth (+156)
    bossHealth.rawData = raw;
    bossHealth.Init(FindOverlay((u32)0xA7160677));
    bossHealth.SetVisible(0);

    // foeHealth (+192)
    foeHealth.rawData = raw;
    foeHealth.Init(FindOverlay((u32)0x8A0C569E));
    foeHealth.SetVisible(0);

    // takes (+292)
    takes.rawData = raw;
    takes.Init(FindOverlay((u32)0x0A88DB30));
    takes.SetAnimInfo(-150, 0, 15, 15);
    takes.SetVisible(1);

    // dragon (+420)
    dragon.rawData = raw;
    dragon.Init(FindOverlay((u32)0xC98817DB));
    dragon.SetAnimInfo(0, 60, 15, 15);
    dragon.SetVisible(1);

    // hdHits (+228)
    hits.Init(this);
    hits.hitCount = 0;

    // destSelect (+560)
    destSelect.Init(this);

    // tally (+584)
    tally.Init(this);
    tally.takesOvlPtr = &takes;
}

// PSX: EnableInput__3HUDi (HUD.CPP:542, 0x8003FB84)
void HUD::EnableInput(s32 enable) {
    MARKFUNCTION(0x8003FB84);
    // PSX: FindButtonMapping(0,0) -> SetButtonCallback/ClearButtonCallback
    // Requires InputManager button callback system (not yet reversed)
}

// PSX: DebugDisplay__3HUDi (HUD.CPP:553, 0x8003FBD0)
void HUD::DebugDisplay(s32 flag) {
    MARKFUNCTION(0x8003FBD0);
    if (flag) {
        return;
    }
    u8* raw = section ? section->rawData : nullptr;
    if (!raw) {
        return;
    }
    xcOverlayData* ovl = FindOverlay((u32)0x050794E7);
    if (!ovl) {
        return;
    }
    static u32 emptyToken = xcRegisterRuntimeString("");
    u32 textHashes[4] = {
        (u32)(-1157722119),
        (u32)1069371927,
        (u32)337565664,
        (u32)1823646459
    };
    for (s32 i = 0; i < 4; i++) {
        xcTextPrim* text = (xcTextPrim*)ovl->GetTextObj(textHashes[i], raw);
        if (text) {
            text->StringHashes()[text->paletteIdx] = emptyToken;
        }
    }
}

// PSX: SelfUpdate__3HUD (HUD.CPP:611, 0x8003FC10)
void HUD::SelfUpdate() {
    MARKFUNCTION(0x8003FC10);
    hits.Update();
    foeHealth.Update();
    playerHealth.Update();
    bossHealth.Update();
    takes.Update();
    dragon.Update();
    tally.Update();
    destSelect.Update();

    if (!foeHealth.IsVisible()) {
        hits.TriggerUpdate();
    }
}

// PSX: FindScreen__3HUDUl (HUD.CPP:633, 0x8003FCA0)
uintptr_t HUD::FindScreen(u32 id) {
    MARKFUNCTION(0x8003FCA0);
    if (id) {
        // PSX: returns this+48 (embedded oxScreen)
        return id;
    }
    return 0;
}

// PSX: GetScreenNames__3HUD (HUD.CPP:640, 0x8003FCB4)
const char** HUD::GetScreenNames() {
    MARKFUNCTION(0x8003FCB4);
    return s_hudScreenNames;
}

// PSX: GetGameData__3HUD (HUD.CPP:645, 0x8003FCC0)
void HUD::GetGameData() {
    MARKFUNCTION(0x8003FCC0);
    s32 foeHP = -1;
    if (foeTracking && currentFoe) {
        foeHP = currentFoe->health;
    }
    s32 playerHP = 0;
    if (Player::s_player) {
        playerHP = Player::s_player->health;
    }
    UpdateHealth(playerHP, foeHP);
    foeTracking = 0;

    if (!g_scoreManager) {
        return;
    }

    s32 changed = 0;
    if (dragon.dragonCount != g_scoreManager->currentCollectCount ||
        dragon.goldDragonFlag != g_scoreManager->currentGoldDragons) {
        changed = 1;
    }
    if (changed) {
        dragon.SetNum(g_scoreManager->currentCollectCount);
        dragon.SetGoldDragons(g_scoreManager->currentGoldDragons);
        dragon.Play();
    }
}

// PSX: UpdateBonusScore__3HUDllRC10tagLVector (HUD.CPP:690, 0x8003FDF8)
void HUD::UpdateBonusScore() {
    MARKFUNCTION(0x8003FDF8);
    hits.IncrementHits();
}

// PSX: TriggerBonusUpdate__3HUD (HUD.CPP:695, 0x8003FE18)
void HUD::TriggerBonusUpdate() {
    MARKFUNCTION(0x8003FE18);
    hits.TriggerUpdate();
}

// PSX: DisplayTake__3HUDib (HUD.CPP:700, 0x8003FE38)
void HUD::DisplayTake(s32 takeCount, s32 showAnim) {
    MARKFUNCTION(0x8003FE38);
    takes.SetNumber(takeCount - 1);
    takes.SetPauseState(showAnim, 1);
    takes.Play();
}

// PSX: DisplayExtraTake__3HUDRC10tagLVector (HUD.CPP:708, 0x8003FE7C)
void HUD::DisplayExtraTake() {
    MARKFUNCTION(0x8003FE7C);
    if (Player::s_player) {
        takes.SetNumber(Player::s_player->livesLeft - 1);
    }
    takes.Play();
}

// PSX: UpdateHealth__3HUDll (HUD.CPP:719, 0x8003FEB8)
void HUD::UpdateHealth(s32 playerHP, s32 foeHP) {
    MARKFUNCTION(0x8003FEB8);
    // Player health bar
    s32 pval = 0;
    if (playerHP) {
        pval = playerHP + 5;
    }
    playerHealth.SetValue(pval);

    // Foe health bar (bonus from max health scaling)
    u32 foeBonus = 0;
    if (foeTracking && currentFoe) {
        foeBonus = (u16)currentFoe->maxHealth / 40;
    }
    if (foeHP >= 0) {
        s32 fval;
        if (foeHP) {
            fval = foeHP + foeBonus;
            if (fval < 6) {
                fval = 5;
            }
        } else {
            fval = 0;
        }
        foeHealth.SetValue(fval);
        foeHealth.SetTtlive(60);
    }

    // Boss health bar
    if (bossHandle) {
        Thing* boss = bossHandle->owner;
        if (boss) {
            bossHealth.SetValue(boss->health);
        } else {
            bossHealth.SetVisible(0);
            bossHandle->refCount--;
            if (bossHandle->refCount == 0) {
                delete bossHandle;
            }
            bossHandle = nullptr;
        }
    }
}

// PSX: SetFoe__3HUDP8Humanoid (HUD.CPP:767, 0x8003FFF4)
void HUD::SetFoe(Humanoid* foe) {
    MARKFUNCTION(0x8003FFF4);
    if (!currentFoe) {
        return;
    }
    // Check if foe is a boss type - bosses don't use the foe health bar
    s32 isBoss = 0;
    switch (foe->thingType) {
        case AITypes::TT_GRONTAR:
        case AITypes::TT_PAUL:
        case AITypes::TT_OSCAR:
        case AITypes::TT_DANTE:
        case AITypes::TT_BUTCH:
            isBoss = 1;
            break;
        default:
            isBoss = 0;
            break;
    }
    if (!isBoss) {
        foeHealth.SetText(nullptr);
        foeHealth.SetMax(foe->maxHealth);
        currentFoe = foe;
        foeTracking = 1;
    }
}

// PSX: UpdateFoe__3HUDP8Humanoid (HUD.CPP:793)
void HUD::UpdateFoe(Humanoid* foe) {
    MARKFUNCTION(0x80040064);
    if (foeHealth.IsVisible()) {
        if (foe == currentFoe) {
            foeTracking = 1;
        }
    }
}

// PSX: ShowBossHealth__3HUDPCc (HUD.CPP:820, 0x80040094)
void HUD::ShowBossHealth(const char* name) {
    MARKFUNCTION(0x80040094);
    if (bossHealth.IsVisible()) {
        return;
    }
    if (!g_ai) {
        return;
    }
    u32 hash = p3dHash(name);
    Thing* boss = (Thing*)g_ai->humanoidList.FindNodeCRC(hash);
    if (!boss) {
        return;
    }
    bossHealth.SetText(name);
    bossHealth.SetVisible(1);
    bossHealth.SetMax(boss->maxHealth);
    bossHealth.SetValue(boss->health);
    const char* bossName = boss->GetName();
    if (bossName) {
        strncpy(szBossStatic, bossName, sizeof(szBossStatic) - 1);
        szBossStatic[sizeof(szBossStatic) - 1] = 0;
    } else {
        szBossStatic[0] = 0;
    }
    bossHealth.SetText(szBossStatic);
    bossHandle = boss->GetThingHandle();
    bossHandle->refCount++;
}

// PSX: ToggleShowAll__3HUD (HUD.CPP:853, 0x80040150)
void HUD::ToggleShowAll() {
    MARKFUNCTION(0x80040150);
    if (!visible) {
        return;
    }
    dragonShowState = (dragonShowState == 0) ? 1 : 0;
    dragon.SetPauseState(dragonShowState, 1);
    if (g_scoreManager) {
        dragon.SetNum(g_scoreManager->currentCollectCount);
    }
    dragon.Play();
    takes.SetPauseState(dragonShowState, 1);
    if (Player::s_player) {
        takes.SetNumber(Player::s_player->livesLeft - 1);
    }
    takes.Play();
}

// PSX: OnLoadLevel__3HUD (HUD.CPP:887, 0x8004028C)
void HUD::OnLoadLevel() {
    MARKFUNCTION(0x8004028C);
}

// PSX: OnUnloadLevel__3HUD (HUD.CPP:891, 0x80040294)
void HUD::OnUnloadLevel() {
    MARKFUNCTION(0x80040294);
    if (bossHandle) {
        bossHandle->refCount--;
        if (bossHandle->refCount == 0) {
            delete bossHandle;
        }
        bossHandle = nullptr;
    }
}
