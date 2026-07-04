#include "extra/cheats.h"

#if NEW_CHEATS

#include "ai/player.h"
#include "fe/femenumgr.h"
#include "gen/game.h"
#include "gen/scoremgr.h"
#include "gen/world.h"
#include "p3d/context.h"
#include "pddi/pddidev.h"
#include <cstring>

static bool s_allDragons = false;
static bool s_allLevels = false;
static bool s_godMode = false;
static bool s_onePunchMan = false;
static bool s_heavenBound = false;
static bool s_bobbleHead = false;
static bool s_stuntquake = false;
static bool s_mirrorWorld = false;
static PetalStats s_dragonSnapshot[21] = {};
static PetalStats s_levelSnapshot[21] = {};
static u8 s_currentGoldSnapshot = 0;
static bool s_dragonSnapshotValid = false;
static bool s_levelSnapshotValid = false;

static bool* GetCheatFlag(CheatOption option) {
    switch (option) {
        case CheatOption::AllDragons: return &s_allDragons;
        case CheatOption::AllLevels: return &s_allLevels;
        case CheatOption::GodMode: return &s_godMode;
        case CheatOption::OnePunchMan: return &s_onePunchMan;
        case CheatOption::HeavenBound: return &s_heavenBound;
        case CheatOption::BobbleHead: return &s_bobbleHead;
        case CheatOption::Stuntquake: return &s_stuntquake;
        case CheatOption::MirrorWorld: return &s_mirrorWorld;
    }
    return nullptr;
}

bool IsCheatEnabled(CheatOption option) {
    const bool* flag = GetCheatFlag(option);
    return flag && *flag;
}

void ApplyProgressCheats() {
    if (g_scoreManager) {
        if (s_allDragons) {
            std::memcpy(s_dragonSnapshot, g_scoreManager->petalStats, sizeof(s_dragonSnapshot));
            s_currentGoldSnapshot = g_scoreManager->currentGoldDragons;
            s_dragonSnapshotValid = true;
            g_scoreManager->GiveAllDragons();
        }
        if (s_allLevels) {
            std::memcpy(s_levelSnapshot, g_scoreManager->petalStats, sizeof(s_levelSnapshot));
            s_levelSnapshotValid = true;
            g_scoreManager->OpenAllLevels();
        }
    }
    World* world = g_game ? g_game->GetWorld() : nullptr;
    if (s_allLevels && g_feMenuMgr && world && world->GetCurLevelID() == 7) {
        g_feMenuMgr->OpenDoors();
    }
    if (s_godMode && Player::s_player) {
        Player::s_player->health = Player::s_player->maxHealth;
    }
}

void SetCheatEnabled(CheatOption option, bool enabled) {
    bool* flag = GetCheatFlag(option);
    if (!flag) return;
    if (*flag == enabled) return;

    if (!enabled && g_scoreManager) {
        if (option == CheatOption::AllDragons && s_dragonSnapshotValid) {
            for (s32 i = 0; i < 21; ++i) {
                g_scoreManager->petalStats[i].collectCount = s_dragonSnapshot[i].collectCount;
                g_scoreManager->petalStats[i].goldDragons = s_dragonSnapshot[i].goldDragons;
            }
            g_scoreManager->currentGoldDragons = s_currentGoldSnapshot;
            s_dragonSnapshotValid = false;
        }
        else if (option == CheatOption::AllLevels && s_levelSnapshotValid) {
            for (s32 i = 0; i < 21; ++i) {
                g_scoreManager->petalStats[i].fightScore = s_levelSnapshot[i].fightScore;
            }
            s_levelSnapshotValid = false;
        }
    }

    *flag = enabled;

    if (option == CheatOption::MirrorWorld && p3d::context) {
        p3d::context->SetWorldMirror(enabled);
    }

    if (!enabled) return;

    if (g_scoreManager && option == CheatOption::AllDragons) {
        std::memcpy(s_dragonSnapshot, g_scoreManager->petalStats, sizeof(s_dragonSnapshot));
        s_currentGoldSnapshot = g_scoreManager->currentGoldDragons;
        s_dragonSnapshotValid = true;
        g_scoreManager->GiveAllDragons();
    }
    else if (g_scoreManager && option == CheatOption::AllLevels) {
        std::memcpy(s_levelSnapshot, g_scoreManager->petalStats, sizeof(s_levelSnapshot));
        s_levelSnapshotValid = true;
        g_scoreManager->OpenAllLevels();
        World* world = g_game ? g_game->GetWorld() : nullptr;
        if (g_feMenuMgr && world && world->GetCurLevelID() == 7) g_feMenuMgr->OpenDoors();
    }
    else if (option == CheatOption::GodMode && Player::s_player) {
        Player::s_player->health = Player::s_player->maxHealth;
    }
}

void ResetCheats() {
    SetCheatEnabled(CheatOption::AllDragons, false);
    SetCheatEnabled(CheatOption::AllLevels, false);
    SetCheatEnabled(CheatOption::GodMode, false);
    SetCheatEnabled(CheatOption::OnePunchMan, false);
    SetCheatEnabled(CheatOption::HeavenBound, false);
    SetCheatEnabled(CheatOption::BobbleHead, false);
    SetCheatEnabled(CheatOption::Stuntquake, false);
    SetCheatEnabled(CheatOption::MirrorWorld, false);
}

#endif
