#include "gen/common.h"
#include "fecustommenumgr.h"
#include "gen/display.h"
#include "pc/inputaction.h"
#include "pc/tim.h"
#include "gen/game.h"
#include "ai/fevolume.h"
#include "fe/femenumgr.h"
#include "fe/gamemenu.h"
#include "snd/fesnd.h"
#include "snd/rsevent.h"
#include "snd/sound.h"
#include "pc/settings.h"
#include "pc/textmgr.h"
#include "gen/time.h"
#include "xclib/xccolour.h"
#include "xclib/xclib.h"
#include "gen/scoremgr.h"
#include "gen/world.h"
#include "p3d/texture.h"
#include "pddi/pdditex.h"
#include "p3d/context.h"
#include "p3d/input.h"

feCustomMenuMgr* g_feCustomMenuMgr = nullptr;

static constexpr s32 kFrameRateOptionValues[] = { 30, 60, 120, 0 };
static constexpr s32 kMsaaOptionValues[] = { 0, 2, 4, 8, 16 };
static constexpr s32 kKeyBindingActionCount = ACTION_OPEN_CLOSE_MENU;
static constexpr u32 HASH_LEVEL_SCREEN = 0x0598ABF8;
static constexpr u32 HASH_LEVEL_EXECUTE = 0x893DA6B2;
static constexpr u32 HASH_LOCATION_OVERLAY = 42519405;
static constexpr u32 HASH_TEXT_LEVELNAME = 0x39AA5899;

static constexpr f32 DEF_MENU_TITLE_SCALE = 0.34f;
static constexpr f32 DEF_MENU_TEXT_SCALE = 0.4f;
static constexpr f32 DEF_REDEFINE_KEY_TEXT_SCALE = 0.3f;
static constexpr f32 DEF_MENU_PROMPT_SCALE = 0.3f;
static constexpr f32 DEF_MENU_DRAGON_COUNT_SCALE = 0.5f;
static constexpr f32 DEF_CONTROLLER_ACTION_SCALE = 0.2f;

static f32 ScrollArrowPulseScale(u32 frameCounter, s32 phaseOffset) {
    const u32 phase = (frameCounter + (u32)phaseOffset) % 24u;
    const f32 ramp = (phase < 12u)
        ? ((f32)phase / 12.0f)
        : ((f32)(24u - phase) / 12.0f);
    return 0.92f + ramp * 0.16f;
}

struct LocationRuntimeInfo {
    s32 levelID = 0;
    s32 levelIndex = 0;
    s32 subLevel = 0;
    s32 collectCount = 0;
    bool hasGoldDragon = false;
    bool showDragons = true;
    u8 grade = 1;
    bool hasGrade = false;
    const char* levelName = nullptr;
};

static const char* GetSpecialLocationToken(s32 levelID) {
    switch (levelID) {
        case 6: return "FE_LOC_TMP";
        case 7: return "FE_LOC_DST";
        case 8: return "FE_LOC_FAC";
        case 11: return "FE_LOC_CHN";
        case 12: return "FE_LOC_WTR";
        case 13: return "FE_LOC_SWR";
        case 14: return "FE_LOC_ROF";
        default: return nullptr;
    }
}

static const char* ResolveLocationSpecialTitle(s32 levelIndex) {
    if (!g_feMenuMgr) {
        return nullptr;
    }

    xcSection* section = g_feMenuMgr->GetSection();
    if (!section) {
        return nullptr;
    }

    xcOverlayData* overlay = g_feMenuMgr->FindOverlay(HASH_LOCATION_OVERLAY);
    if (!overlay) {
        return nullptr;
    }

    xcTextPrim* levelNameText = (xcTextPrim*)overlay->GetTextObj(HASH_TEXT_LEVELNAME, section->rawData);
    if (!levelNameText) {
        return nullptr;
    }

    xcTextPrim copy = *levelNameText;
    copy.paletteIdx = (u8)levelIndex;
    const u32 strHash = copy.GetStringHash();
    return section->FindString(strHash);
}

static const char* GradeToLetter(u8 grade) {
    switch (grade) {
        case 1:
            return "D";
        case 2: 
            return "C";
        case 3:
            return "B";
        case 4: 
            return "A";
        default: 
            if (grade >= 5) {
                return "A+";
            }      
            break;
    }

    return " ";
}

static bool ResolveLocationRuntimeInfo(LocationRuntimeInfo* outInfo) {
    if (!outInfo || !g_game) {
        return false;
    }

    World* world = g_game->GetWorld();
    if (!world) {
        return false;
    }

    s32 levelIndex = 0;
    s32 subLevel = 0;
    bool resolvedFromMenu = false;

    if (g_feMenuMgr) {
        hdMenu* levelMenu = g_feMenuMgr->FindMenu(g_feMenuMgr->startScreenHashes[1]);
        if (levelMenu) {
            hdMenuItem* execute = levelMenu->FindItem(HASH_LEVEL_EXECUTE);
            if (execute) {
                u32 packedLevel = 0;
                u32 packedPetal = 0;
                World::UnpackLevelName(execute->itemFlags, packedLevel, packedPetal);
                levelIndex = (s32)packedLevel;
                subLevel = (s32)packedPetal;
                resolvedFromMenu = true;
            }
        }

        if (!resolvedFromMenu && g_feMenuMgr->frontEndVolume) {
            const s32 levelCode = g_feMenuMgr->frontEndVolume->levelCode;
            const s32 levelID = levelCode / 10;
            levelIndex = world->LevelIDToIndex(levelID);
            subLevel = levelCode % 10;
            resolvedFromMenu = true;
        }
    }

    if (!resolvedFromMenu) {
        levelIndex = (s32)world->GetCurrentLevelIndex();
        subLevel = (s32)world->GetCurrentPetalIndex();
    }

    if (levelIndex < 0) {
        levelIndex = 0;
    }
    if (subLevel < 0) {
        subLevel = 0;
    }

    const s32 petalCount = world->GetLevelPetalCountFromIndex((u32)levelIndex);
    if (petalCount > 0 && subLevel >= petalCount) {
        subLevel = petalCount - 1;
    }

    outInfo->levelIndex = levelIndex;
    outInfo->subLevel = subLevel;
    outInfo->levelID = world->GetLevelIDFromIndex((u32)levelIndex);
    outInfo->showDragons = (outInfo->levelID >= 1 && outInfo->levelID <= 5);
    outInfo->levelName = world->GetLevelNameFromIndex((u32)levelIndex);

    if (g_scoreManager) {
        const s32 statIndex = levelIndex * 3 + subLevel;
        if (statIndex >= 0 && statIndex < 21) {
            const PetalStats& ps = g_scoreManager->petalStats[statIndex];
            outInfo->collectCount = ps.collectCount;
            outInfo->hasGoldDragon = g_scoreManager->CalcGDrags(ps.collectCount);
            outInfo->hasGrade = (ps.fightScore >= 0);
            outInfo->grade = outInfo->hasGrade ? ps.grade : 1;
        }
    }

    return true;
}

static void BeginNewGameReset() {
    if (g_scoreManager) {
        g_scoreManager->HandleGameBegin();
    }

    if (g_game) {
        World* world = g_game->GetWorld();
        if (world) {
            world->Unload();
            world->UnloadLevelPart2();
            world->UnloadPermanent();
        }
    }

    if (g_scoreManager) {
        g_scoreManager->drunkenMasterUnlocked = 0;
    }
}

static bool IsKeyBindingAction(Action action) {
    return action >= ACTION_JUMP && action < ACTION_OPEN_CLOSE_MENU;
}

static void SetDesktopBindingCodeUnique(Action action, s32 slot, s32 code) {
    if (!g_actionInput) {
        return;
    }

    if (!IsKeyBindingAction(action) || slot < 0 || slot >= DEF_KEYBIND_SLOT_COUNT) {
        return;
    }

    if (code != 0) {
        for (s32 actionIndex = 0; actionIndex < kKeyBindingActionCount; actionIndex++) {
            const Action currentAction = (Action)actionIndex;
            for (s32 currentSlot = 0; currentSlot < DEF_KEYBIND_SLOT_COUNT; currentSlot++) {
                if (currentAction == action && currentSlot == slot) {
                    continue;
                }

                if (g_actionInput->GetDesktopBindingCode(currentAction, currentSlot) == code) {
                    g_actionInput->SetDesktopBindingCode(currentAction, currentSlot, 0);
                }
            }
        }
    }

    g_actionInput->SetDesktopBindingCode(action, slot, code);
}

static void BuildActionTokenFallbackLabel(const char* token, char* outText, s32 outTextLen) {
    if (!outText || outTextLen <= 0) {
        return;
    }

    outText[0] = '\0';

    if (!token) {
        return;
    }

    s32 write = 0;
    for (s32 i = 0; token[i] != '\0' && write + 1 < outTextLen; i++) {
        char ch = token[i];
        if (ch == '_') {
            ch = ' ';
        }
        outText[write++] = ch;
    }
    outText[write] = '\0';
}

static const char* GetControllerLogicalLabelToken(u8 logicalIndex) {
    switch (logicalIndex) {
        case 0: return "FE_CSD";
        case 1: return "FE_CST";
        case 2: return "FE_CCT";
        case 3: return "FE_CDR";
        case 4: return "FE_CKK";
        case 5: return "FE_CGR";
        case 6: return "FE_CJP";
        case 7: return "FE_CPN";
        default: return nullptr;
    }
}

static void BuildDesktopBindingPromptText(Action action, s32 slot, char* outText, s32 outTextLen) {
    if (!outText || outTextLen <= 0) {
        return;
    }

    outText[0] = '\0';

    if (!g_actionInput || slot < 0 || slot >= DEF_KEYBIND_SLOT_COUNT) {
        snprintf(outText, outTextLen, "-");
        return;
    }

    const s32 code = g_actionInput->GetDesktopBindingCode(action, slot);
    if (code == 0) {
        snprintf(outText, outTextLen, "-");
        return;
    }

    snprintf(outText, outTextLen, "<BND:%d>", code);
}

static void SetPromptText(char* dst, s32 dstLen, const char* fmt, const char* a = nullptr, const char* b = nullptr, const char* c = nullptr) {
    if (!dst || dstLen <= 0) {
        return;
    }

    if (!fmt) {
        dst[0] = '\0';
        return;
    }

    if (a && b && c) {
        snprintf(dst, dstLen, fmt, a, b, c);
    }
    else if (a && b) {
        snprintf(dst, dstLen, fmt, a, b);
    }
    else if (a) {
        snprintf(dst, dstLen, fmt, a);
    }
    else {
        snprintf(dst, dstLen, "%s", fmt);
    }
}

static s32 ClampFrameRateOptionIndex(s32 index) {
    if (index < 0) {
        return 0;
    }

    const s32 maxIndex = (s32)(sizeof(kFrameRateOptionValues) / sizeof(kFrameRateOptionValues[0])) - 1;
    if (index > maxIndex) {
        return maxIndex;
    }
    return index;
}

static s32 FrameRateOptionIndexToValue(s32 index) {
    return kFrameRateOptionValues[ClampFrameRateOptionIndex(index)];
}

static s32 FrameRateValueToOptionIndex(s32 fps) {
    if (fps <= 0) {
        return (s32)(sizeof(kFrameRateOptionValues) / sizeof(kFrameRateOptionValues[0])) - 1;
    }

    s32 bestIndex = 0;
    s32 bestDist = 0x7fffffff;

    for (u32 i = 0; i < (u32)(sizeof(kFrameRateOptionValues) / sizeof(kFrameRateOptionValues[0])); i++) {
        const s32 candidate = kFrameRateOptionValues[i];
        if (candidate <= 0) {
            continue;
        }

        const s32 dist = (fps > candidate) ? (fps - candidate) : (candidate - fps);
        if (dist < bestDist) {
            bestDist = dist;
            bestIndex = (s32)i;
        }
    }

    return bestIndex;
}

static const char* GetFrameRateDisplayToken(s32 index) {
    switch (FrameRateOptionIndexToValue(index)) {
        case 30: return "FE_FR30";
        case 60: return "FE_FR60";
        case 120: return "FE_FR120";
        case 0: return "FE_MAX";
        default: return nullptr;
    }
}

static s32 ClampMsaaOptionIndex(s32 index) {
    if (index < 0) {
        return 0;
    }

    const s32 maxIndex = (s32)(sizeof(kMsaaOptionValues) / sizeof(kMsaaOptionValues[0])) - 1;
    if (index > maxIndex) {
        return maxIndex;
    }
    return index;
}

static s32 MsaaOptionIndexToSamples(s32 index) {
    return kMsaaOptionValues[ClampMsaaOptionIndex(index)];
}

static s32 MsaaSamplesToOptionIndex(s32 samples) {
    s32 bestIndex = 0;
    s32 bestDist = 0x7fffffff;

    for (u32 i = 0; i < (u32)(sizeof(kMsaaOptionValues) / sizeof(kMsaaOptionValues[0])); i++) {
        const s32 candidate = kMsaaOptionValues[i];
        const s32 dist = (samples > candidate) ? (samples - candidate) : (candidate - samples);
        if (dist < bestDist) {
            bestDist = dist;
            bestIndex = (s32)i;
        }
    }

    return bestIndex;
}

static s32 WrapStepValue(s32 current, s32 step, s32 lo, s32 hi, s32 dir) {
    if (lo > hi) {
        const s32 tmp = lo;
        lo = hi;
        hi = tmp;
    }

    if (step <= 0) {
        step = 1;
    }

    s32 v = current + ((dir > 0) ? step : -step);
    if (v < lo) {
        return hi;
    }
    if (v > hi) {
        return lo;
    }
    return v;
}

static s32 WrapStepValueSlider(s32 current, s32 step, s32 lo, s32 hi, s32 dir) {
    if (lo > hi) {
        const s32 tmp = lo;
        lo = hi;
        hi = tmp;
    }

    if (step <= 0) {
        step = 1;
    }

    s32 v = current + ((dir > 0) ? step : -step);
    return v;
}

static const char* GetMsaaDisplayToken(s32 index) {
    switch (MsaaOptionIndexToSamples(index)) {
        case 2:  return "FE_A2X";
        case 4:  return "FE_A4X";
        case 8:  return "FE_A8X";
        case 16: return "FE_A16X";
        default: return nullptr;
    }
}

static const char* GetLanguageDisplayToken(s32 index) {
    switch ((GameLanguage)index) {
        case LangEnglish: return "FE_LGEN";
        case LangGerman: return "FE_LGER";
        case LangFrench: return "FE_LFRE";
        case LangItalian: return "FE_LITA";
        case LangSpanish: return "FE_LSPA";
        default: return nullptr;
    }
}

static bool IsVolumeSliderBinding(EntryBinding binding) {
    return binding == EntryBinding_MusicVol
        || binding == EntryBinding_EffectsVol
        || binding == EntryBinding_DialogVol;
}

static s32 GetValueChangeSoundId(const Entry& entry) {
    if (entry.type == EntryType_Slider && IsVolumeSliderBinding(entry.binding)) {
        return FE_SND_MENU_8;
    }

    // Non-volume value adjustments should use the same sound as Enter/confirm.
    return FE_SND_MENU_5;
}

static bool IsSaveSlotPage(MenuPage page) {
    return page == MenuPage_LoadSlots
        || page == MenuPage_SaveSlots
        || page == MenuPage_DeleteSlots;
}

static bool IsAutoEntryPosition(const Entry& entry) {
    return entry.posX == 0 && entry.posY == 0;
}

void feCustomMenuMgr::BuildPages() {
    auto& feTitle = AddPage(MenuPage_Title, "FE_TTL", "Menu_Title", MenuPage_None, 0, false, -1, -1);
    SetEntries(feTitle, {
        Button("FE_STG", EntryEvent_GoPage, MenuPage_StartGame),
        Button("FE_OPT", EntryEvent_GoPage, MenuPage_Options),
        Button("FE_QTG", EntryEvent_GoPage, MenuPage_QuitConfirm),
               });

    auto& feMain = AddPage(MenuPage_Frontend, "FE_MNM", "Menu_Title", MenuPage_None, 0, false, -1, -1);
    SetEntries(feMain, {
        Button("FE_RSM", EntryEvent_Resume),
        Button("FE_STG", EntryEvent_GoPage, MenuPage_StartGame),
        Button("FE_OPT", EntryEvent_GoPage, MenuPage_Options),
        Button("FE_QTG", EntryEvent_GoPage, MenuPage_QuitConfirm),
               });

    auto& feStart = AddPage(MenuPage_StartGame, "FE_STG", "Menu_GameOption", MenuPage_Frontend, 1, false, -1, -1);
    SetEntries(feStart, {
        Button("FE_NWG", EntryEvent_NewGame),
        Button("FE_LDG", EntryEvent_Load),
        Button("FE_SVG", EntryEvent_Save),
        Button("FE_DLG", EntryEvent_Delete),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feOpts = AddPage(MenuPage_Options, "FE_OPT", "Menu_GameOption", MenuPage_Frontend, 2, false, -1, -1);
    SetEntries(feOpts, {
        Button("FE_CTL", EntryEvent_GoPage, MenuPage_Controller),
        Button("FE_KBD", EntryEvent_GoPage, MenuPage_KeyBindings),
        Button("FE_DIS", EntryEvent_GoPage, MenuPage_Display),
        Button("FE_SND", EntryEvent_GoPage, MenuPage_Sound),
        Button("FE_CRE", EntryEvent_Credits),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feCtrl = AddPage(MenuPage_Controller, "FE_CTL", "Menu_Controller", MenuPage_Options, 0, false, DEF_CONTROLLER_WINDOW_W, DEF_CONTROLLER_WINDOW_H);
    SetEntries(feCtrl, {
        Toggle("FE_CSH", EntryBinding_Shock),
        List("FE_CCF", EntryBinding_PlayerConfig, 1, 0, 2),
        Button("FE_BCK", EntryEvent_Back),
               }, 0, 42);

    auto& feKeyBindings = AddPage(MenuPage_KeyBindings, "FE_KBD", "Menu_Controller", MenuPage_Controller, 1, false,
            DEF_KEYBIND_WINDOW_W, DEF_KEYBIND_WINDOW_H);
    SetEntries(feKeyBindings, {
                Button("FE_BCK", EntryEvent_Back),
               }, 0, 64);

    auto& feDisplay = AddPage(MenuPage_Display, "FE_DIS", "Menu_GameOption", MenuPage_Options, 2, false, -1, -1);
    SetEntries(feDisplay, {
        List("FE_RES", EntryBinding_DisplayResolution, 1, 0, 64),
        List("FE_FSC", EntryBinding_DisplayScreenMode, 1, 0, 2),
        Toggle("FE_VYS", EntryBinding_DisplayVsync),
        List("FE_FPS", EntryBinding_DisplayFrameRate, 1, 0, 3),
        List("FE_MSA", EntryBinding_DisplayMsaa, 1, 0, 4),
        List("FE_LNG", EntryBinding_Language, 1, 0, (s32)NumLanguages - 1),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feSnd = AddPage(MenuPage_Sound, "FE_SND", "Menu_Sound", MenuPage_Options, 3, false, -1, -1);
    SetEntries(feSnd, {
        Slider("FE_EFV", EntryBinding_EffectsVol, 10, 0, 100),
        Slider("FE_MSV", EntryBinding_MusicVol,   10, 0, 100),
        Slider("FE_VCV", EntryBinding_DialogVol,  10, 0, 100),
        Toggle("FE_STR", EntryBinding_Stereo),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feLoadSlots = AddPage(MenuPage_LoadSlots, "FE_LDG", "Menu_GameOption", MenuPage_StartGame, 1, false, -1, -1);
    SetEntries(feLoadSlots, {
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_NTD", EntryEvent_Load),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feSaveSlots = AddPage(MenuPage_SaveSlots, "FE_SVG", "Menu_GameOption", MenuPage_StartGame, 2, false, -1, -1);
    SetEntries(feSaveSlots, {
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_NTD", EntryEvent_Save),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feDeleteSlots = AddPage(MenuPage_DeleteSlots, "FE_DLG", "Menu_GameOption", MenuPage_StartGame, 3, false, -1, -1);
    SetEntries(feDeleteSlots, {
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_NTD", EntryEvent_Delete),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feLoadConfirm = AddPage(MenuPage_LoadConfirm, "FE_LDG", "Menu_Confirmation", MenuPage_LoadSlots, 0, false, -1, -1);
    SetEntries(feLoadConfirm, {
        Info("FE_LDQ"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_LoadConfirmYes),
               });

    auto& feSaveConfirm = AddPage(MenuPage_SaveConfirm, "FE_SVG", "Menu_Confirmation", MenuPage_SaveSlots, 0, false, -1, -1);
    SetEntries(feSaveConfirm, {
        Info("FE_SVQ"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_SaveConfirmYes),
               });

    auto& feDeleteConfirm = AddPage(MenuPage_DeleteConfirm, "FE_DLG", "Menu_Confirmation", MenuPage_DeleteSlots, 0, false, -1, -1);
    SetEntries(feDeleteConfirm, {
        Info("FE_DLQ"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_DeleteConfirmYes),
               });

    auto& feSaveDone = AddPage(MenuPage_SaveDone, "FE_SVG", "Menu_Confirmation", MenuPage_SaveSlots, 0, false, -1, -1);
    SetEntries(feSaveDone, {
        Info("FE_SVD"),
        Button("FE_OK", EntryEvent_Back),
               });

    auto& feDeleteDone = AddPage(MenuPage_DeleteDone, "FE_DLG", "Menu_Confirmation", MenuPage_DeleteSlots, 0, false, -1, -1);
    SetEntries(feDeleteDone, {
        Info("FE_DLD"),
        Button("FE_OK", EntryEvent_Back),
               });

    auto& feNewGame = AddPage(MenuPage_NewGameConfirm, "FE_NWG", "Menu_Confirmation", MenuPage_Frontend, 0, false, -1, -1);
    SetEntries(feNewGame, {
        Info("FE_NGQ"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_NewGame),
               });

    auto& feExitLevel = AddPage(MenuPage_ExitLevelConfirm, "FE_EXL", "Menu_Confirmation", MenuPage_Pause, 2, true, -1, -1);
    SetEntries(feExitLevel, {
        Info("FE_EXLR"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_ExitToHub),
               });

    auto& feQuit = AddPage(MenuPage_QuitConfirm, "FE_QTG", "Menu_Confirmation", MenuPage_None, 0, false, -1, -1);
    SetEntries(feQuit, {
        Info("FE_XGM"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_QuitGame),
               });

    auto& feQuitting = AddPage(MenuPage_Quitting, "FE_QTG", "Menu_Confirmation", MenuPage_None, 0, false, -1, -1);
    SetEntries(feQuitting, {
        Info("FE_QUI"),
               });

    auto& pauseMain = AddPage(MenuPage_Pause, "FE_PSD", "Menu_GameOption", MenuPage_None, 0, true, -1, 120);
    SetEntries(pauseMain, {
        Button("FE_RSG", EntryEvent_Resume),
        Button("FE_OPT", EntryEvent_GoPage, MenuPage_Options),
        Button("FE_EXL", EntryEvent_GoPage, MenuPage_ExitLevelConfirm),
        Button("FE_QTG", EntryEvent_GoPage, MenuPage_QuitConfirm),
               });

    auto& feLocation = AddPage(MenuPage_Location, "", "Menu_Location", MenuPage_None, 0, false, DEF_WINDOW_W, 130);
    SetEntries(feLocation, {
        Button("", EntryEvent_LocationSelect),
               });
}

void feCustomMenuMgr::Init(CustomText* textSystem) {
    m_text = textSystem;

    BuildPages();
    LoadControllerOverlayTexture();
    LoadMenuOrnamentTexture();
    LoadSplashTextures();
    LoadSliderTextures();
    LoadScrollArrowTexture();

    m_pulse.Start();
    SetPage(MenuPage_None);
}

void feCustomMenuMgr::Shutdown() {
    if (m_titleScreenTexture) {
        m_titleScreenTexture->Release();
        m_titleScreenTexture = nullptr;
    }
    if (m_loadingScreenTexture) {
        m_loadingScreenTexture->Release();
        m_loadingScreenTexture = nullptr;
    }
    if (m_controllerTexture) {
        m_controllerTexture->Release();
        m_controllerTexture = nullptr;
    }
    if (m_menuOrnamentTexture) {
        m_menuOrnamentTexture->Release();
        m_menuOrnamentTexture = nullptr;
    }
    if (m_scrollArrowTexture) {
        m_scrollArrowTexture->Release();
        m_scrollArrowTexture = nullptr;
    }
    if (m_redDragonTex) {
        m_redDragonTex->Release();
        m_redDragonTex = nullptr;
    }
    if (m_goldDragonTex) {
        m_goldDragonTex->Release();
        m_goldDragonTex = nullptr;
    }
    if (m_greyDragonTex) {
        m_greyDragonTex->Release();
        m_greyDragonTex = nullptr;
    }
    if (m_sliderOTex) {
        m_sliderOTex->Release();
        m_sliderOTex = nullptr;
    }
    if (m_sliderFTex) {
        m_sliderFTex->Release();
        m_sliderFTex = nullptr;
    }

    m_titleScreenTextureTried = false;
    m_loadingScreenTextureTried = false;
    m_text = nullptr;
}

s32 feCustomMenuMgr::Invoke() {
    if (!m_active)
        return (s32)GameResult::ResumePlay;

    m_result = 1;

    // Quitting countdown: no input processed, just tick down then close.
    if (m_currPage == MenuPage_Quitting) {
        if (m_quitTimerSec > 0.0f) {
            const f32 dt = g_time ? g_time->GetDeltaTime() : (1.0f / 30.0f);
            m_quitTimerSec -= dt;
        }
        if (m_quitTimerSec <= 0.0f) {
            m_quitTimerSec = 0.0f;
            if (g_game)
                g_game->SetState(GameState::End);
        }
        return m_result;
    }

    if (!g_actionInput)
        return m_result;

    if (m_currPage == MenuPage_KeyBindings) {
        if (m_keyBindActionCursor < 0 || m_keyBindActionCursor >= kKeyBindingActionCount) {
            m_keyBindActionCursor = 0;
        }

        if (m_keyBindSlotCursor < 0 || m_keyBindSlotCursor >= DEF_KEYBIND_SLOT_COUNT) {
            m_keyBindSlotCursor = 0;
        }

        auto isKeyBindBackSelected = [this]() {
            return m_cursor == 0;
        };

        auto setKeyBindBackSelected = [this](bool selected) {
            m_cursor = selected ? 0 : -1;
        };

        auto clampKeyBindScroll = [this]() {
            if (m_keyBindActionCursor < m_keyBindScrollTop) {
                m_keyBindScrollTop = m_keyBindActionCursor;
            }
            if (m_keyBindActionCursor >= m_keyBindScrollTop + DEF_KEYBIND_VISIBLE_ROWS) {
                m_keyBindScrollTop = m_keyBindActionCursor - (DEF_KEYBIND_VISIBLE_ROWS - 1);
            }

            const s32 maxScrollTop = (kKeyBindingActionCount > DEF_KEYBIND_VISIBLE_ROWS)
                ? (kKeyBindingActionCount - DEF_KEYBIND_VISIBLE_ROWS)
                : 0;
            if (m_keyBindScrollTop < 0) {
                m_keyBindScrollTop = 0;
            }
            if (m_keyBindScrollTop > maxScrollTop) {
                m_keyBindScrollTop = maxScrollTop;
            }
        };

        double sx = 0.0;
        double sy = 0.0;
        g_actionInput->GetMousePosition(sx, sy);

        bool mouseMoved = false;
        if (!m_mousePosInitialized) {
            m_lastMouseX = sx;
            m_lastMouseY = sy;
            m_mousePosInitialized = true;
        }
        else if (m_mouseInputActive) {
            mouseMoved = (std::fabs(sx - m_lastMouseX) > 0.5) || (std::fabs(sy - m_lastMouseY) > 0.5);
            m_lastMouseX = sx;
            m_lastMouseY = sy;
        }
        else {
            mouseMoved = (std::fabs(sx - m_lastMouseX) > 4.0) || (std::fabs(sy - m_lastMouseY) > 4.0);
            if (mouseMoved) {
                m_lastMouseX = sx;
                m_lastMouseY = sy;
            }
        }

        const bool leftClick = g_actionInput->IsMouseButtonTriggered(MouseBtn::Left);
        const bool rightClick = g_actionInput->IsMouseButtonTriggered(MouseBtn::Right);
        const s32 scroll = g_actionInput->ConsumeScrollDelta();
        // Key bindings page should respond only to keyboard/gamepad for non-mouse gating.
        const bool nonMouseInput = g_actionInput->HadKeyboardInputThisFrame() || g_actionInput->HadGamepadInputThisFrame();

        if ((mouseMoved || leftClick || rightClick || scroll != 0) && !nonMouseInput) {
            if (!m_mouseInputActive && g_display) {
                g_display->SetCursorVisible(true);
            }
            m_mouseInputActive = true;
        }

        if (m_keyBindCaptureActive) {
            if (m_keyBindCaptureBlockFrames > 0) {
                m_keyBindCaptureBlockFrames--;
                return m_result;
            }

            const s32 capturedKey = g_actionInput->GetTriggeredKeyThisFrame();
            const s32 capturedMouse = g_actionInput->GetTriggeredMouseButtonThisFrame();
            const Action action = (Action)m_keyBindActionCursor;

            if (capturedKey == KEY_ESCAPE ||
                (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_BACK))) {
                m_keyBindCaptureActive = false;
                PlaySound(FE_SND_MENU_5);
                return m_result;
            }

            if ((nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_CLEAR)) ||
                capturedKey == KEY_DELETE || capturedKey == KEY_BACKSPACE) {
                SetDesktopBindingCodeUnique(action, m_keyBindSlotCursor, 0);
                g_settings.Save(SETTINGS_PATH);
                m_keyBindCaptureActive = false;
                PlaySound(FE_SND_MENU_5);
                return m_result;
            }

            if (capturedKey != 0) {
                SetDesktopBindingCodeUnique(action, m_keyBindSlotCursor, ActionInput::EncodeKeyboardBindingCode(capturedKey));
                g_settings.Save(SETTINGS_PATH);
                m_keyBindCaptureActive = false;
                PlaySound(FE_SND_MENU_5);
                return m_result;
            }

            if (capturedMouse != MouseBtn::NONE) {
                SetDesktopBindingCodeUnique(action, m_keyBindSlotCursor, ActionInput::EncodeMouseBindingCode(capturedMouse));
                g_settings.Save(SETTINGS_PATH);
                m_keyBindCaptureActive = false;
                PlaySound(FE_SND_MENU_5);
                return m_result;
            }

            if (nonMouseInput && m_mouseInputActive) {
                m_mouseInputActive = false;
                if (g_display) {
                    g_display->SetCursorVisible(false);
                }
            }

            return m_result;
        }

        s32 hoverActionIndex = -1;
        s32 hoverMenuEntry = -1;
        s32 hoverRowIndex = -1;
        s32 hoverSlotIndex = m_keyBindSlotCursor;

        if (m_mouseInputActive) {
            const f32 screenW = g_display ? (f32)g_display->GetScreenWidth() : DEFAULT_SCREEN_WIDTH;
            const f32 screenH = g_display ? (f32)g_display->GetScreenHeight() : DEFAULT_SCREEN_HEIGHT;
            const f32 aspect = g_display ? g_display->GetAspectRatio() : (4.0f / 3.0f);
#if FIX_ASPECT_RATIO
            const f32 effectiveW = screenW * DEFAULT_ASPECT_RATIO / aspect;
            const f32 offsetX = (screenW - effectiveW) * 0.5f;
            const f32 psxX = ((f32)sx - offsetX) * DEFAULT_SCREEN_WIDTH / effectiveW;
#else
            const f32 psxX = (f32)sx * DEFAULT_SCREEN_WIDTH / screenW;
#endif
            const f32 psxY = (f32)sy * DEFAULT_SCREEN_HEIGHT / screenH;

            const PageDef* page = &m_pages[m_currPage];
            const s32 panelX = DEF_WINDOW_CENTER_X - page->frameW / 2;
            const s32 panelY = DEF_WINDOW_CENTER_Y - page->frameH / 2;
            const s32 contentTop = panelY + DEF_TITLE_BAR_H + DEF_CONTENT_TOP_PAD;
            const s32 labelX = panelX + DEF_LABEL_X_PAD + DEF_KEYBIND_X_PAD;
            const s32 headerY = contentTop + DEF_CONTENT_PAD + DEF_TEXT_Y_OFF;
            const s32 firstRowY = headerY + DEF_KEYBIND_ROW_STEP;
            const s32 slotW = DEF_KEYBIND_SLOT_W;
            const s32 slotGap = DEF_KEYBIND_SLOT_GAP;
            const s32 slot2Right = panelX + page->frameW - DEF_VALUE_X_PAD - DEF_KEYBIND_X_PAD;
            const s32 slot2Left = slot2Right - slotW;
            const s32 slot1Right = slot2Left - slotGap;
            const s32 slot1Left = slot1Right - slotW;
            const s32 actionLabelRight = slot1Left - DEF_KEYBIND_ACTION_COL_GAP;
            const s32 visibleRows = (kKeyBindingActionCount - m_keyBindScrollTop < DEF_KEYBIND_VISIBLE_ROWS)
                ? (kKeyBindingActionCount - m_keyBindScrollTop)
                : DEF_KEYBIND_VISIBLE_ROWS;

            if (psxX >= (f32)panelX && psxX < (f32)(panelX + page->frameW)) {
                for (s32 row = 0; row < visibleRows; row++) {
                    const s32 rowTop = firstRowY + row * DEF_KEYBIND_ROW_STEP - DEF_KEYBIND_ROW_TOP_PAD;
                    const s32 rowBottom = rowTop + DEF_KEYBIND_ROW_STEP;
                    if (psxY < (f32)rowTop || psxY >= (f32)rowBottom) {
                        continue;
                    }

                    const s32 actionIndex = m_keyBindScrollTop + row;
                    if (actionIndex < 0 || actionIndex >= kKeyBindingActionCount) {
                        continue;
                    }

                    if (psxX >= (f32)(slot2Left - DEF_KEYBIND_HIT_PAD) && psxX < (f32)(slot2Left + slotW + DEF_KEYBIND_HIT_PAD)) {
                        hoverSlotIndex = 1;
                    }
                    else if (psxX >= (f32)(slot1Left - DEF_KEYBIND_HIT_PAD) && psxX < (f32)(slot1Left + slotW + DEF_KEYBIND_HIT_PAD)) {
                        hoverSlotIndex = 0;
                    }
                    else if (psxX >= (f32)(labelX - DEF_KEYBIND_CELL_PAD - DEF_KEYBIND_HIT_PAD) && psxX < (f32)actionLabelRight) {
                        hoverSlotIndex = m_keyBindSlotCursor;
                    }
                    else {
                        continue;
                    }

                    hoverActionIndex = actionIndex;
                    hoverRowIndex = row;
                    break;
                }

                if (page->numEntries > 0) {
                    const s32 rowSpan = (page->numEntries > 0) ? ((page->numEntries - 1) * DEF_ROW_STEP) : 0;
                    const s32 extraH = CalcPageExtraHeight(*page);
                    const s32 entryBlockH = DEF_CONTENT_PAD + rowSpan + DEF_ROW_TEXT_H + extraH;
                    const s32 bodyAvailH = page->frameH - DEF_TITLE_BAR_H - DEF_BOTTOM_BAR_H - DEF_CONTENT_TOP_PAD - DEF_CONTENT_BOTTOM_PAD;
                    const s32 bodyCenterPad = (bodyAvailH > entryBlockH) ? ((bodyAvailH - entryBlockH) / 2) : 0;
                    const s32 menuFirstY = panelY + DEF_TITLE_BAR_H + DEF_CONTENT_TOP_PAD + bodyCenterPad + DEF_CONTENT_PAD;
                    const s32 menuLabelX = panelX + DEF_LABEL_X_PAD;
                    const s32 menuValueX = panelX + page->frameW - DEF_VALUE_X_PAD;
                    const s32 menuCenterX = DEF_WINDOW_CENTER_X;

                    s32 backRowTop = 0;
                    ResolveEntryLayout(*page, 0,
                                       menuFirstY, menuLabelX, menuValueX, menuCenterX,
                                       &backRowTop, nullptr, nullptr, nullptr, nullptr);

                    const s32 backRowH = DEF_ROW_STEP + GetEntryExtraHeight(*page, page->entries[0]);
                    if (psxY >= (f32)backRowTop && psxY < (f32)(backRowTop + backRowH)) {
                        hoverMenuEntry = 0;
                    }
                }
            }

            if (hoverActionIndex >= 0 && isKeyBindBackSelected()) {
                setKeyBindBackSelected(false);
                PlaySound(FE_SND_MENU_7);
            }

            if (hoverActionIndex >= 0 && hoverActionIndex != m_keyBindActionCursor) {
                m_keyBindActionCursor = hoverActionIndex;
                clampKeyBindScroll();
                PlaySound(FE_SND_MENU_7);
            }

            if (hoverActionIndex >= 0 && hoverSlotIndex != m_keyBindSlotCursor) {
                m_keyBindSlotCursor = hoverSlotIndex;
                PlaySound(FE_SND_MENU_7);
            }

            if (hoverActionIndex < 0 && hoverMenuEntry == 0 && !isKeyBindBackSelected()) {
                setKeyBindBackSelected(true);
                PlaySound(FE_SND_MENU_7);
            }

            if (scroll != 0) {
                if (isKeyBindBackSelected()) {
                    setKeyBindBackSelected(false);
                }

                const s32 oldScrollTop = m_keyBindScrollTop;
                const s32 maxScrollTop = (kKeyBindingActionCount > DEF_KEYBIND_VISIBLE_ROWS)
                    ? (kKeyBindingActionCount - DEF_KEYBIND_VISIBLE_ROWS)
                    : 0;
                const s32 dir = (scroll > 0) ? -1 : 1;
                const s32 scrollSteps = (scroll > 0) ? scroll : -scroll;

                for (s32 step = 0; step < scrollSteps; step++) {
                    m_keyBindScrollTop += dir;
                    if (m_keyBindScrollTop < 0) {
                        m_keyBindScrollTop = 0;
                        break;
                    }
                    if (m_keyBindScrollTop > maxScrollTop) {
                        m_keyBindScrollTop = maxScrollTop;
                        break;
                    }
                }

                if (m_keyBindScrollTop != oldScrollTop) {
                    if (hoverRowIndex >= 0) {
                        m_keyBindActionCursor = m_keyBindScrollTop + hoverRowIndex;
                    }

                    if (m_keyBindActionCursor < m_keyBindScrollTop) {
                        m_keyBindActionCursor = m_keyBindScrollTop;
                    }
                    if (m_keyBindActionCursor >= m_keyBindScrollTop + DEF_KEYBIND_VISIBLE_ROWS) {
                        m_keyBindActionCursor = m_keyBindScrollTop + (DEF_KEYBIND_VISIBLE_ROWS - 1);
                    }
                    if (m_keyBindActionCursor >= kKeyBindingActionCount) {
                        m_keyBindActionCursor = kKeyBindingActionCount - 1;
                    }

                    PlaySound(FE_SND_MENU_7);
                }
            }

            if (leftClick && hoverActionIndex >= 0) {
                setKeyBindBackSelected(false);
                m_keyBindCaptureActive = true;
                m_keyBindCaptureBlockFrames = 1;
                PlaySound(FE_SND_MENU_5);
                return m_result;
            }

            if (leftClick && hoverMenuEntry == 0) {
                setKeyBindBackSelected(true);
                PlaySound(FE_SND_MENU_5);
                Confirm();
                return m_result;
            }

            if (rightClick) {
                PlaySound(FE_SND_MENU_5);
                GoBack();
                return m_result;
            }
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_UP)) {
            if (isKeyBindBackSelected()) {
                setKeyBindBackSelected(false);
                m_keyBindActionCursor = kKeyBindingActionCount - 1;
                clampKeyBindScroll();
            }
            else if (m_keyBindActionCursor > 0) {
                m_keyBindActionCursor--;
                clampKeyBindScroll();
            }
            else {
                setKeyBindBackSelected(true);
            }
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_DOWN)) {
            if (isKeyBindBackSelected()) {
                setKeyBindBackSelected(false);
                m_keyBindActionCursor = 0;
                clampKeyBindScroll();
            }
            else if (m_keyBindActionCursor < (kKeyBindingActionCount - 1)) {
                m_keyBindActionCursor++;
                clampKeyBindScroll();
            }
            else {
                setKeyBindBackSelected(true);
            }
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && !isKeyBindBackSelected() && g_actionInput->JustPressed(ACTION_MENU_LEFT)) {
            m_keyBindSlotCursor--;
            if (m_keyBindSlotCursor < 0) {
                m_keyBindSlotCursor = DEF_KEYBIND_SLOT_COUNT - 1;
            }
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && !isKeyBindBackSelected() && g_actionInput->JustPressed(ACTION_MENU_RIGHT)) {
            m_keyBindSlotCursor++;
            if (m_keyBindSlotCursor >= DEF_KEYBIND_SLOT_COUNT) {
                m_keyBindSlotCursor = 0;
            }
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_CONFIRM)) {
            PlaySound(FE_SND_MENU_5);
            if (isKeyBindBackSelected()) {
                Confirm();
            }
            else {
                m_keyBindCaptureActive = true;
                m_keyBindCaptureBlockFrames = 1;
            }
        }

        const s32 clearKey = g_actionInput->GetTriggeredKeyThisFrame();
        if (!isKeyBindBackSelected() && ((nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_CLEAR)) ||
            clearKey == KEY_DELETE || clearKey == KEY_BACKSPACE)) {
            const Action action = (Action)m_keyBindActionCursor;
            SetDesktopBindingCodeUnique(action, m_keyBindSlotCursor, 0);
            g_settings.Save(SETTINGS_PATH);
            PlaySound(FE_SND_MENU_5);
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_BACK)) {
            PlaySound(FE_SND_MENU_5);
            GoBack();
        }

        if (nonMouseInput && m_mouseInputActive) {
            m_mouseInputActive = false;
            if (g_display) {
                g_display->SetCursorVisible(false);
            }
        }

        return m_result;
    }

    double sx = 0.0;
    double sy = 0.0;
    g_actionInput->GetMousePosition(sx, sy);

    bool mouseMoved = false;
    if (!m_mousePosInitialized) {
        m_lastMouseX = sx;
        m_lastMouseY = sy;
        m_mousePosInitialized = true;
    }
    else if (m_mouseInputActive) {
        // Only track delta while mouse is active; when inactive we keep the
        // saved position so we measure distance from where it was deactivated.
        mouseMoved = (std::fabs(sx - m_lastMouseX) > 0.5) || (std::fabs(sy - m_lastMouseY) > 0.5);
        m_lastMouseX = sx;
        m_lastMouseY = sy;
    }
    else {
        // Require a larger intentional movement to re-activate from keyboard/gamepad mode.
        mouseMoved = (std::fabs(sx - m_lastMouseX) > 4.0) || (std::fabs(sy - m_lastMouseY) > 4.0);
        if (mouseMoved) {
            m_lastMouseX = sx;
            m_lastMouseY = sy;
        }
    }

    const bool leftClick = g_actionInput->IsMouseButtonTriggered(MouseBtn::Left);
    const bool rightClick = g_actionInput->IsMouseButtonTriggered(MouseBtn::Right);
    const s32 scroll = g_actionInput->ConsumeScrollDelta();
    const Entry* e = &m_pages[m_currPage].entries[m_cursor];
    auto prevVal = GetBoundValue(*e);

    // Determine non-mouse input first so it can suppress mouse re-activation below.
    const bool nonMouseInput = g_actionInput->HadKeyboardInputThisFrame() || g_actionInput->HadGamepadInputThisFrame();

    // Mouse activity takes menu control back and shows cursor,
    // but only when no keyboard/gamepad input is happening this frame.
    if ((mouseMoved || leftClick || rightClick || scroll != 0) && !nonMouseInput) {
        if (!m_mouseInputActive && g_display) {
            g_display->SetCursorVisible(true);
        }
        m_mouseInputActive = true;
    }

    // Mouse hover and click
    if (m_mouseInputActive) {
        // Convert screen to PSX coordinate space
        const f32 screenW = g_display ? (f32)g_display->GetScreenWidth() : DEFAULT_SCREEN_WIDTH;
        const f32 screenH = g_display ? (f32)g_display->GetScreenHeight() : DEFAULT_SCREEN_HEIGHT;
        const f32 aspect = g_display ? g_display->GetAspectRatio() : (4.0f / 3.0f);
#if FIX_ASPECT_RATIO
        const f32 effectiveW = screenW * DEFAULT_ASPECT_RATIO / aspect;
        const f32 offsetX = (screenW - effectiveW) * 0.5f;
        const f32 psxX = ((f32)sx - offsetX) * DEFAULT_SCREEN_WIDTH / effectiveW;
#else
        const f32 psxX = (f32)sx * DEFAULT_SCREEN_WIDTH / screenW;
#endif
        const f32 psxY = (f32)sy * DEFAULT_SCREEN_HEIGHT / screenH;

        // Row hit test
        const PageDef* page = &m_pages[m_currPage];
        const s32 panelX = DEF_WINDOW_CENTER_X - page->frameW / 2;
        const s32 panelY = DEF_WINDOW_CENTER_Y - page->frameH / 2;
        const s32 rowSpan = (page->numEntries > 0) ? ((page->numEntries - 1) * DEF_ROW_STEP) : 0;
        const s32 extraH = CalcPageExtraHeight(*page);
        const s32 entryBlockH = DEF_CONTENT_PAD + rowSpan + DEF_ROW_TEXT_H + extraH;
        const s32 bodyAvailH = page->frameH - DEF_TITLE_BAR_H - DEF_BOTTOM_BAR_H - DEF_CONTENT_TOP_PAD - DEF_CONTENT_BOTTOM_PAD;
        const s32 bodyCenterPad = (bodyAvailH > entryBlockH) ? ((bodyAvailH - entryBlockH) / 2) : 0;
        const s32 firstY = panelY + DEF_TITLE_BAR_H + DEF_CONTENT_TOP_PAD + bodyCenterPad + DEF_CONTENT_PAD;
        const s32 baseLabelX = panelX + DEF_LABEL_X_PAD;
        const s32 baseValueX = panelX + page->frameW - DEF_VALUE_X_PAD;
        const s32 baseCenterX = DEF_WINDOW_CENTER_X;

        if (psxX >= (f32)panelX && psxX < (f32)(panelX + page->frameW)) {
            for (s32 i = 0; i < page->numEntries; i++) {
                s32 rowTop = 0;
                ResolveEntryLayout(*page, i,
                                   firstY, baseLabelX, baseValueX, baseCenterX,
                                   &rowTop, nullptr, nullptr, nullptr, nullptr);
                const s32 rowH = DEF_ROW_STEP + GetEntryExtraHeight(*page, page->entries[i]);
                if (psxY >= (f32)rowTop && psxY < (f32)(rowTop + rowH)) {
                    if (i != m_cursor && page->entries[i].type != EntryType_Info) {
                        // Discard staged display values when leaving that row via mouse.
                        const Entry& prev = page->entries[m_cursor];
                        if (prev.binding == EntryBinding_DisplayResolution) {
                            m_pendingResolutionActive = false;
                        }
                        if (prev.binding == EntryBinding_DisplayScreenMode) {
                            m_pendingScreenModeActive = false;
                        }
                        if (prev.binding == EntryBinding_DisplayMsaa) {
                            m_pendingMsaaActive = false;
                        }
                        m_cursor = i;
                        PlaySound(FE_SND_MENU_7);
                    }
                    break;
                }
            }
        }

        if (leftClick) {
            PlaySound(FE_SND_MENU_5);
            Confirm();
        }
        if (rightClick) {
            if (m_currPage != MenuPage_Frontend && m_currPage != MenuPage_Pause) {
                PlaySound(FE_SND_MENU_5);
            }
            GoBack();
        }
        if (scroll != 0) {
            Adjust(scroll > 0 ? 1 : -1);

            if (prevVal != GetBoundValue(*e)) {
                PlaySound(GetValueChangeSoundId(*e));
            }
        }
    }

    if (g_actionInput->JustPressed(ACTION_MENU_UP)) {
        MoveCursor(-1);
        PlaySound(FE_SND_MENU_7);
    }

    if (g_actionInput->JustPressed(ACTION_MENU_DOWN)) {
        MoveCursor(1);
        PlaySound(FE_SND_MENU_7);
    }

    if (g_actionInput->JustPressed(ACTION_MENU_LEFT)) {
        Adjust(-1);

        if (prevVal != GetBoundValue(*e)) {
            PlaySound(GetValueChangeSoundId(*e));
        }
    }
    if (g_actionInput->JustPressed(ACTION_MENU_RIGHT)) {
        Adjust(1);

        if (prevVal != GetBoundValue(*e)) {
            PlaySound(GetValueChangeSoundId(*e));
        }
    }

    if (g_actionInput->JustPressed(ACTION_MENU_CONFIRM)) {
        PlaySound(FE_SND_MENU_5); Confirm();
    }

    if (g_actionInput->JustPressed(ACTION_MENU_BACK)) {
        // FE_SND_MENU_SPECIAL_4 triggers HandleCursorEvent(5) -> jcsFadeOutEngine(2),
        // which can leave audio faded out in this custom flow.
        // Use a non-fade back sound for submenu navigation.
        if (m_currPage != MenuPage::MenuPage_Frontend && m_currPage != MenuPage::MenuPage_Pause) {
            PlaySound(FE_SND_MENU_5);
        }
        GoBack();
    }
    else if (g_actionInput->IsGamepadActive() && g_actionInput->JustPressed(ACTION_OPEN_CLOSE_MENU)) {
        Deactivate();
    }

    // Any keyboard/gamepad input disables mouse hover until mouse moves again.
    if (nonMouseInput && m_mouseInputActive) {
        m_mouseInputActive = false;
        if (g_display) {
            g_display->SetCursorVisible(false);
        }
    }

    return m_result;
}

void feCustomMenuMgr::SetPage(MenuPage page) {
    m_prevPage = m_currPage;
    m_currPage = page;
    m_cursor = 0;
    m_result = 1;
    m_pendingResolutionActive = false;
    m_pendingScreenModeActive = false;
    m_pendingMsaaActive = false;
    m_keyBindCaptureActive = false;
    m_keyBindCaptureBlockFrames = 0;

    if (m_currPage == MenuPage_None) {
        m_pendingLoadSlot = -1;
        m_pendingSaveSlot = -1;
        m_pendingDeleteSlot = -1;
    }

    if (m_currPage == MenuPage_KeyBindings) {
        m_keyBindActionCursor = 0;
        m_keyBindSlotCursor = 0;
        m_keyBindScrollTop = 0;
    }

    if (m_currPage == MenuPage_StartGame) {
        const bool showSave = (m_pages[MenuPage_StartGame].parentPage == MenuPage_Frontend);
        if (showSave) {
            SetEntries(m_pages[MenuPage_StartGame], {
                Button("FE_NWG", EntryEvent_NewGame),
                Button("FE_LDG", EntryEvent_Load),
                Button("FE_SVG", EntryEvent_Save),
                Button("FE_DLG", EntryEvent_Delete),
                Button("FE_BCK", EntryEvent_Back),
                       });
        }
        else {
            SetEntries(m_pages[MenuPage_StartGame], {
                Button("FE_NWG", EntryEvent_NewGame),
                Button("FE_LDG", EntryEvent_Load),
                Button("FE_DLG", EntryEvent_Delete),
                Button("FE_BCK", EntryEvent_Back),
                       });
        }
    }

    if (IsSaveSlotPage(m_currPage)) {
        RefreshSaveSlots();
    }

    if (m_currPage != MenuPage_None) {
        PageDef& mutablePage = m_pages[m_currPage];
        if (mutablePage.autoFrameH) {
            const s32 extraH = CalcPageExtraHeight(mutablePage);
            mutablePage.frameH = CalcAutoFrameHeight(mutablePage.numEntries, extraH);
        }
    }

    // Advance cursor past any leading Info entries.
    if (m_currPage != MenuPage_None) {
        const PageDef& pg = m_pages[m_currPage];
        while (m_cursor >= 0 && m_cursor < pg.numEntries && pg.entries[m_cursor].type == EntryType_Info) {
            m_cursor++;
        }
    }

    if (m_currPage == MenuPage_KeyBindings) {
        // Key bindings uses its own row/slot selection; keep Back unselected until navigated.
        m_cursor = -1;
    }
}

void feCustomMenuMgr::Activate(MenuPage startPage) {
    m_active = true;
    m_cursor = 0;
    LoadControllerOverlayTexture();

    if (g_display) {
        g_display->SetCursorCaptured(false);
        g_display->SetCursorVisible(false);
    }

    m_mouseInputActive = false;
    m_mousePosInitialized = false;
    if (startPage != MenuPage_Title) {
        rsEvent(RS_MUTE, 0, 0, 0);
    }

    if (startPage == MenuPage_Pause && g_game && g_game->GetState() == GameState::Play) {
        s32 track = 23;
        rsEvent(RS_LOAD_AND_PLAY_DIALOG, 0, track, 0x1C);
    }

    SetPage(startPage);

    PlaySound(FE_SND_MENU_MOVE);
}

void feCustomMenuMgr::Deactivate() {
    PlaySound(FE_SND_MENU_ACCEPT);
    if (m_currPage != MenuPage_Title) {
        rsEvent(RS_UNMUTE, 0, 0, 0);
    }
    if (g_display) g_display->SetCursorCaptured(true);

    m_active = false;
    m_cursor = 0;
    SetPage(MenuPage_None);
    m_result = (s32)GameResult::ResumePlay;
}

void feCustomMenuMgr::MoveCursor(s32 dir) {
    const s32 prevCursor = m_cursor;
    const PageDef& pg = m_pages[m_currPage];
    s32 next = m_cursor + dir;
    // Wrap around.
    if (next < 0) next = pg.numEntries - 1;
    if (next >= pg.numEntries) next = 0;
    // Skip over Info entries.
    const s32 limit = pg.numEntries;
    for (s32 tries = 0; tries < limit; tries++) {
        if (pg.entries[next].type != EntryType_Info) break;
        next += dir;
        if (next < 0) next = pg.numEntries - 1;
        if (next >= pg.numEntries) next = 0;
    }
    m_cursor = next;

    // Leaving a staged display row without confirming discards staged value.
    if (prevCursor != m_cursor) {
        const Entry& prev = m_pages[m_currPage].entries[prevCursor];
        if (prev.binding == EntryBinding_DisplayResolution) {
            m_pendingResolutionActive = false;
        }
        if (prev.binding == EntryBinding_DisplayScreenMode) {
            m_pendingScreenModeActive = false;
        }
        if (prev.binding == EntryBinding_DisplayMsaa) {
            m_pendingMsaaActive = false;
        }
    }
}

bool feCustomMenuMgr::InvokeLocationSelection() {
    if (!g_feMenuMgr) {
        return false;
    }

    hdMenu* levelMenu = g_feMenuMgr->FindMenu(HASH_LEVEL_SCREEN);
    if (!levelMenu) {
        LOG("[CustomMenu] Location select failed: level menu not found");
        return false;
    }

    hdMenuItem* executeItem = levelMenu->FindItem(HASH_LEVEL_EXECUTE);
    if (!executeItem || !executeItem->callback) {
        LOG("[CustomMenu] Location select failed: execute callback missing");
        return false;
    }

    const s32 callbackResult = executeItem->callback(executeItem);
    if (callbackResult == 4 || callbackResult == 8) {
        m_result = callbackResult;
        return true;
    }

    LOG("[CustomMenu] Location select callback returned unexpected result=%d", callbackResult);
    return false;
}

void feCustomMenuMgr::Confirm() {
    const Entry* e = &m_pages[m_currPage].entries[m_cursor];

    if (e->type == EntryType_Toggle) {
        const s32 v = GetBoundValue(*e) ? 0 : 1;
        ApplyValue(*e, v);
        return;
    }

    if (e->type == EntryType_List) {
        if (e->binding == EntryBinding_DisplayResolution && m_pendingResolutionActive) {
            ApplyValue(*e, m_pendingResolutionIndex);
            m_pendingResolutionActive = false;
        }
        else if (e->binding == EntryBinding_DisplayScreenMode && m_pendingScreenModeActive) {
            ApplyValue(*e, m_pendingScreenMode);
            m_pendingScreenModeActive = false;
        }
        else if (e->binding == EntryBinding_DisplayMsaa && m_pendingMsaaActive) {
            ApplyValue(*e, m_pendingMsaaIndex);
            m_pendingMsaaActive = false;
        }
        return;
    }

    // Button
    switch (e->event) {
        case EntryEvent_GoPage:
            m_pages[e->goPage].parentPage = m_currPage;
            m_pages[e->goPage].parentEntry = m_cursor;
            SetPage(e->goPage);
            break;
        case EntryEvent_Resume:
            m_result = 8;
            break;
        case EntryEvent_Back:
            GoBack();
            break;
        case EntryEvent_NewGame:
        {
            const bool fromTitle = g_game && g_game->GetState() == GameState::TitleLoop;
            if (!fromTitle && m_currPage != MenuPage_NewGameConfirm) {
                m_pages[MenuPage_NewGameConfirm].parentPage = m_currPage;
                m_pages[MenuPage_NewGameConfirm].parentEntry = m_cursor;
                SetPage(MenuPage_NewGameConfirm);
                break;
            }

            BeginNewGameReset();

            if (!fromTitle && g_game) {
                g_game->QueueTitleNewGameStart();
                g_game->SetState(GameState::Init);
            }
            m_result = 4;
            break;
        }
        case EntryEvent_ExitToHub:
            if (g_game)
                g_game->SetState(GameState::OpenLocationMenu);
            m_result = 4;
            break;
        case EntryEvent_QuitGame:
            m_quitTimerSec = DEF_QUIT_TIMER_SEC;
            SetPage(MenuPage_Quitting);
            break;
        case EntryEvent_Credits:
            rsEvent(RS_STOP_MUSIC, 0, 0, 0);

            if (g_game)
                g_game->PlayMovie("credits.str", 1, 1);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            rsEvent(RS_SET_LOCATION, 22, 0, 0);
            rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);
            break;
        case EntryEvent_Load:
            if (m_currPage != MenuPage_LoadSlots) {
                m_pages[MenuPage_LoadSlots].parentPage = m_currPage;
                m_pages[MenuPage_LoadSlots].parentEntry = m_cursor;
                SetPage(MenuPage_LoadSlots);
                break;
            }

            if (m_cursor >= 0 && m_cursor < SAVEGAME_SLOT_COUNT) {
                const bool fromTitle = g_game && g_game->GetState() == GameState::TitleLoop;
                if (fromTitle) {
                    if (!SaveGameLoadSlot(m_cursor)) {
                        PlaySound(16);
                        break;
                    }
                    m_result = 4;
                    break;
                }

                if (!m_saveSlots[m_cursor].occupied) {
                    PlaySound(FE_SND_MENU_7);
                    break;
                }

                m_pendingLoadSlot = m_cursor;
                m_pages[MenuPage_LoadConfirm].parentPage = MenuPage_LoadSlots;
                m_pages[MenuPage_LoadConfirm].parentEntry = m_cursor;
                SetPage(MenuPage_LoadConfirm);
            }
            break;
        case EntryEvent_Save:
            if (m_currPage != MenuPage_SaveSlots) {
                m_pages[MenuPage_SaveSlots].parentPage = m_currPage;
                m_pages[MenuPage_SaveSlots].parentEntry = m_cursor;
                SetPage(MenuPage_SaveSlots);
                break;
            }

            if (m_cursor >= 0 && m_cursor < SAVEGAME_SLOT_COUNT) {
                m_pendingSaveSlot = m_cursor;
                m_pages[MenuPage_SaveConfirm].parentPage = MenuPage_SaveSlots;
                m_pages[MenuPage_SaveConfirm].parentEntry = m_cursor;
                SetPage(MenuPage_SaveConfirm);
            }
            break;
        case EntryEvent_Delete:
            if (m_currPage != MenuPage_DeleteSlots) {
                m_pages[MenuPage_DeleteSlots].parentPage = m_currPage;
                m_pages[MenuPage_DeleteSlots].parentEntry = m_cursor;
                SetPage(MenuPage_DeleteSlots);
                break;
            }

            if (m_cursor >= 0 && m_cursor < SAVEGAME_SLOT_COUNT) {
                if (!m_saveSlots[m_cursor].occupied) {
                    PlaySound(16);
                    break;
                }

                m_pendingDeleteSlot = m_cursor;
                m_pages[MenuPage_DeleteConfirm].parentPage = MenuPage_DeleteSlots;
                m_pages[MenuPage_DeleteConfirm].parentEntry = m_cursor;
                SetPage(MenuPage_DeleteConfirm);
            }
            break;
        case EntryEvent_LoadConfirmYes:
            if (m_pendingLoadSlot >= 0 && m_pendingLoadSlot < SAVEGAME_SLOT_COUNT) {
                if (SaveGameLoadSlot(m_pendingLoadSlot) && SaveGameApplyPendingLoad(g_game) && g_game) {
                    m_pendingLoadSlot = -1;
                    g_game->SetState(GameState::QueueLevelLoad);
                    m_result = 4;
                }
                else {
                    PlaySound(FE_SND_MENU_7);
                }
            }
            else {
                PlaySound(FE_SND_MENU_7);
            }
            break;
        case EntryEvent_SaveConfirmYes:
            if (m_pendingSaveSlot >= 0 && m_pendingSaveSlot < SAVEGAME_SLOT_COUNT) {
                if (SaveGameWriteSlot(m_pendingSaveSlot)) {
                    RefreshSaveSlots();
                    const s32 savedSlot = m_pendingSaveSlot;
                    m_pendingSaveSlot = -1;
                    m_pages[MenuPage_SaveDone].parentPage = MenuPage_SaveSlots;
                    m_pages[MenuPage_SaveDone].parentEntry = savedSlot;
                    SetPage(MenuPage_SaveDone);
                }
                else {
                    PlaySound(FE_SND_MENU_7);
                }
            }
            else {
                PlaySound(FE_SND_MENU_7);
            }
            break;
        case EntryEvent_DeleteConfirmYes:
            if (m_pendingDeleteSlot >= 0 && m_pendingDeleteSlot < SAVEGAME_SLOT_COUNT) {
                if (SaveGameDeleteSlot(m_pendingDeleteSlot)) {
                    RefreshSaveSlots();
                    const s32 deletedSlot = m_pendingDeleteSlot;
                    m_pendingDeleteSlot = -1;
                    m_pages[MenuPage_DeleteDone].parentPage = MenuPage_DeleteSlots;
                    m_pages[MenuPage_DeleteDone].parentEntry = deletedSlot;
                    SetPage(MenuPage_DeleteDone);
                }
                else {
                    PlaySound(FE_SND_MENU_7);
                }
            }
            else {
                PlaySound(FE_SND_MENU_7);
            }
            break;
        case EntryEvent_LocationSelect:
            if (!InvokeLocationSelection()) {
                PlaySound(FE_SND_MENU_7);
            }
            break;
    }
}

void feCustomMenuMgr::GoBack() {
    if (m_currPage == MenuPage_LoadConfirm) {
        m_pendingLoadSlot = -1;
    }
    if (m_currPage == MenuPage_SaveConfirm) {
        m_pendingSaveSlot = -1;
    }
    if (m_currPage == MenuPage_DeleteConfirm) {
        m_pendingDeleteSlot = -1;
    }

    if (m_currPage == MenuPage_Location) {
        m_result = 8;
        return;
    }

    if (m_currPage == MenuPage::MenuPage_Frontend || m_currPage == MenuPage::MenuPage_Pause || m_currPage == MenuPage::MenuPage_Title) {
        Deactivate();
        return;
    }

    auto targetPage = m_pages[m_currPage].parentPage;
    auto parentEntry = m_pages[m_currPage].parentEntry;
    if (targetPage != MenuPage_None)
        SetPage(targetPage);
    else
        SetPage(m_prevPage);

    m_cursor = parentEntry;
}

void feCustomMenuMgr::Adjust(s32 dir) {
    const Entry* e = &m_pages[m_currPage].entries[m_cursor];
    if (e->binding == EntryBinding_None || dir == 0)
        return;

    if (e->type == EntryType_Toggle) {
        const s32 v = GetBoundValue(*e) ? 0 : 1;
        ApplyValue(*e, v);
    }
    else if (e->type == EntryType_List) {
        if (e->binding == EntryBinding_PlayerConfig) {
            const s32 v = WrapStepValue(GetBoundValue(*e), e->step, e->lo, e->hi, dir);
            ApplyValue(*e, v);
            return;
        }

        if (e->binding == EntryBinding_Language) {
            const s32 current = GetBoundValue(*e);
            const s32 v = WrapStepValue(current, e->step, e->lo, e->hi, dir);
            ApplyValue(*e, v);
            return;
        }

        if (e->binding == EntryBinding_DisplayResolution) {
            const s32 current = m_pendingResolutionActive ? m_pendingResolutionIndex : GetBoundValue(*e);
            s32 maxIndex = 0;
            if (g_display) {
                const s32 count = g_display->GetResolutionCount();
                maxIndex = (count > 0) ? (count - 1) : 0;
            }
            const s32 v = WrapStepValue(current, e->step, 0, maxIndex, dir);
            m_pendingResolutionIndex = v;
            m_pendingResolutionActive = true;
            return;
        }

        if (e->binding == EntryBinding_DisplayScreenMode) {
            const s32 current = m_pendingScreenModeActive ? m_pendingScreenMode : GetBoundValue(*e);
            const s32 v = WrapStepValue(current, e->step, e->lo, e->hi, dir);
            m_pendingScreenMode = v;
            m_pendingScreenModeActive = true;
            return;
        }

        if (e->binding == EntryBinding_DisplayMsaa) {
            const s32 current = m_pendingMsaaActive ? m_pendingMsaaIndex : GetBoundValue(*e);
            const s32 v = WrapStepValue(current, e->step, e->lo, e->hi, dir);
            m_pendingMsaaIndex = v;
            m_pendingMsaaActive = true;
            return;
        }

        if (e->binding == EntryBinding_DisplayFrameRate) {
            const s32 current = GetBoundValue(*e);
            const s32 v = WrapStepValue(current, e->step, e->lo, e->hi, dir);
            ApplyValue(*e, v);
            return;
        }
    }
    else if (e->type == EntryType_Slider) {
        if (e->binding == EntryBinding_MusicVol ||
            e->binding == EntryBinding_EffectsVol ||
            e->binding == EntryBinding_DialogVol) {
            static constexpr s32 kSegments = DEF_SLIDER_CIRCLE_SEGMENTS;
            s32 current = GetBoundValue(*e);
            if (current < 0) current = 0;
            if (current > 100) current = 100;

            s32 seg = (current * kSegments) / 100;
            seg = WrapStepValueSlider(seg, 1, 0, kSegments, dir);

            s32 v = 0;
            if (seg <= 0) {
                v = 0;
            }
            else if (seg >= kSegments) {
                v = 100;
            }
            else {
                // Pick the midpoint of this segment's value bucket so render quantization is stable.
                const s32 lo = ((seg * 100) + (kSegments - 1)) / kSegments;
                const s32 hi = ((((seg + 1) * 100) + (kSegments - 1)) / kSegments) - 1;
                v = (lo + hi) / 2;
            }
            ApplyValue(*e, v);
            return;
        }

        const s32 v = WrapStepValue(GetBoundValue(*e), e->step, e->lo, e->hi, dir);
        ApplyValue(*e, v);
    }
}

s32 feCustomMenuMgr::GetBoundValue(const Entry& e) const {
    switch (e.binding) {
        case EntryBinding_MusicVol: return g_sound ? (s32)g_sound->flag0 : 100;
        case EntryBinding_EffectsVol: return g_sound ? (s32)g_sound->flag2 : 100;
        case EntryBinding_DialogVol: return g_sound ? (s32)g_sound->flag1 : 100;
        case EntryBinding_Stereo: return (g_sound && g_sound->activeFlag) ? 1 : 0;
        case EntryBinding_Shock: return GetShock() ? 1 : 0;
        case EntryBinding_PlayerConfig: return g_inputManager ? (s32)g_inputManager->GetPlayerConfig() : 0;
        case EntryBinding_Language: return (s32)g_customText.GetLanguage();
        case EntryBinding_DisplayResolution: return g_display ? g_display->GetResolutionIndex() : 0;
        case EntryBinding_DisplayScreenMode: return g_display ? g_display->GetScreenMode() : Display::GetDefaultScreenMode();
        case EntryBinding_DisplayVsync: return g_display ? g_display->GetVsync() : Display::GetDefaultVsync();
        case EntryBinding_DisplayFrameRate:
        {
            const s32 fps = g_time ? g_time->targetFPS : 30;
            return FrameRateValueToOptionIndex(fps);
        }
        case EntryBinding_DisplayMsaa:
        {
            const s32 samples = g_display ? g_display->GetMSAA() : Display::GetDefaultMSAA();
            return MsaaSamplesToOptionIndex(samples);
        }
        default: return 0;
    }
}

void feCustomMenuMgr::ApplyValue(const Entry& e, s32 v) {
    if (v < e.lo) v = e.lo;
    if (v > e.hi) v = e.hi;

    if (e.binding == EntryBinding_MusicVol) {
        rsEvent(RS_SET_MUSIC_VOL, v, 0, 0);
        if (g_sound) g_sound->flag0 = (s16)v;
    }
    else if (e.binding == EntryBinding_EffectsVol) {
        rsEvent(RS_SET_EFFECTS_VOL, v, 0, 0);
        rsEvent(RS_SET_EFFECTS_VOL_AUX, v, 0, 0);
        if (g_sound) g_sound->flag2 = (s16)v;
    }
    else if (e.binding == EntryBinding_DialogVol) {
        rsEvent(RS_SET_DIALOG_VOL, v, 0, 0);
        if (g_sound) g_sound->flag1 = (s16)v;
    }
    else if (e.binding == EntryBinding_Stereo) {
        rsEvent(v ? RS_SET_STEREO : RS_SET_MONO, 0, 0, 0);
        if (g_sound) g_sound->activeFlag = v;
    }
    else if (e.binding == EntryBinding_Shock) {
        SetShock(v);
        if (v) {
            // Menu feedback: full-strength pulse that survives high-FPS frame pacing.
            SetActuator(0, 255, 60);
            UpdateActuator(0);
        }
        else {
            Shock(SHOCK_CLEAR);
        }
    }
    else if (e.binding == EntryBinding_PlayerConfig) {
        if (g_inputManager) {
            const s16* currentMode[2] = {
                g_inputManager->controls[0].modeMap,
                g_inputManager->controls[1].modeMap,
            };

            g_inputManager->SetPlayerConfig((u8)v);
            const u8* playerMap = g_inputManager->PlayerMapArray();
            for (s16 padIndex = 0; padIndex < 2; ++padIndex) {
                g_inputManager->SetControlMapArray(padIndex, playerMap);
                if (currentMode[padIndex]) {
                    g_inputManager->SetControlModeArray(padIndex, currentMode[padIndex]);
                }
            }
        }
    }
    else if (e.binding == EntryBinding_Language) {
        if (v < 0 || v >= (s32)NumLanguages) {
            v = (s32)LangEnglish;
        }
        g_customText.SetLanguage((GameLanguage)v);
    }
    else if (e.binding == EntryBinding_DisplayScreenMode) {
        if (g_display) g_display->SetScreenMode(v);
        Display::SetDefaultScreenMode(v);
    }
    else if (e.binding == EntryBinding_DisplayVsync) {
        if (g_display) g_display->SetVsync(v);
        Display::SetDefaultVsync(v);
    }
    else if (e.binding == EntryBinding_DisplayFrameRate) {
        const s32 fps = FrameRateOptionIndexToValue(v);
        if (g_time) {
            g_time->targetFPS = fps;
        }
    }
    else if (e.binding == EntryBinding_DisplayMsaa) {
        const s32 samples = MsaaOptionIndexToSamples(v);
        if (g_display) {
            g_display->SetMSAA(samples);
        }
        else {
            Display::SetDefaultMSAA(samples);
        }
    }
    else if (e.binding == EntryBinding_DisplayResolution) {
        if (g_display) g_display->SetResolutionIndex(v);
    }

    g_settings.Save(SETTINGS_PATH);
}

void feCustomMenuMgr::PlaySound(s32 id) const {
    if (g_frontEndSound)
        g_frontEndSound->ProcessSoundEvent(id);
}

void feCustomMenuMgr::RefreshSaveSlots() {
    for (s32 i = 0; i < SAVEGAME_SLOT_COUNT; i++) {
        SaveGameSlotInfo info = {};
        SaveGameQuerySlotInfo(i, &info);
        m_saveSlots[i] = info;
    }
}

void feCustomMenuMgr::BuildSaveSlotLabel(s32 slotIndex, char* outText, s32 outTextLen) const {
    if (!outText || outTextLen <= 0) {
        return;
    }

    outText[0] = '\0';

    if (slotIndex < 0 || slotIndex >= SAVEGAME_SLOT_COUNT) {
        return;
    }

    const SaveGameSlotInfo& slot = m_saveSlots[slotIndex];
    if (slot.occupied) {
        const char* fmt = Localize("FE_SLD");
        if (!fmt) {
            fmt = "Save Slot %d - %s";
        }

        const char* dateText = slot.dateText[0] ? slot.dateText : "Unknown";
        snprintf(outText, outTextLen, fmt, slotIndex + 1, dateText);
    }
    else {
        const char* fmt = Localize("FE_SLE");
        if (!fmt) {
            fmt = "Save Slot %d Empty";
        }
        snprintf(outText, outTextLen, fmt, slotIndex + 1);
    }
}

PageDef& feCustomMenuMgr::AddPage(
    MenuPage id, const char* title,
    const char* overlay, MenuPage parent, s32 parentEntry, bool pause,
    s32 frameW, s32 frameH) {
    PageDef def;
    strcpy_s(def.titleToken, title);
    strcpy_s(def.overlayName, overlay);
    def.parentPage = parent;
    def.parentEntry = parentEntry;
    def.isPause = pause;
    def.autoFrameH = (frameH == -1);
    def.frameW = (frameW == -1) ? DEF_WINDOW_W : frameW;
    def.frameH = def.autoFrameH ? DEF_WINDOW_H : frameH;
    def.entriesOffsetX = 0;
    def.entriesOffsetY = 0;
    def.numEntries = 0;

    m_pages[id] = def;
    return m_pages[id];
}

void feCustomMenuMgr::SetEntries(PageDef& page, std::initializer_list<Entry> list,
                                 s32 entriesOffsetX, s32 entriesOffsetY) {
    s32 n = 0;
    for (const Entry& e : list) {
        if (n >= MAX_ENTRIES_PER_MENU)
            break;
        page.entries[n++] = e;
    }
    page.numEntries = n;
    page.entriesOffsetX = entriesOffsetX;
    page.entriesOffsetY = entriesOffsetY;
    if (page.autoFrameH) {
        const s32 extraH = CalcPageExtraHeight(page);
        page.frameH = CalcAutoFrameHeight(page.numEntries, extraH);
    }
}

Entry feCustomMenuMgr::Button(const char* tok, EntryEvent ev, MenuPage go) {
    Entry e;
    memset(e.token, 0, sizeof(e.token));
    strcpy_s(e.token, tok);
    e.type = EntryType_Button;
    e.event = ev;
    e.goPage = go;
    e.binding = EntryBinding_None;
    e.step = 0; e.lo = 0; e.hi = 0;
    e.posX = 0; e.posY = 0;
    return e;
}

Entry feCustomMenuMgr::Slider(const char* tok, EntryBinding binding, s32 step, s32 lo, s32 hi) {
    Entry e;
    memset(e.token, 0, sizeof(e.token));
    strcpy_s(e.token, tok);
    e.type = EntryType_Slider;
    e.event = EntryEvent_None;
    e.goPage = MenuPage_None;
    e.binding = binding;
    e.step = step; e.lo = lo; e.hi = hi;
    e.posX = 0; e.posY = 0;
    return e;
}

Entry feCustomMenuMgr::List(const char* tok, EntryBinding binding, s32 step, s32 lo, s32 hi) {
    Entry e;
    memset(e.token, 0, sizeof(e.token));
    strcpy_s(e.token, tok);
    e.type = EntryType_List;
    e.event = EntryEvent_None;
    e.goPage = MenuPage_None;
    e.binding = binding;
    e.step = step; e.lo = lo; e.hi = hi;
    e.posX = 0; e.posY = 0;
    return e;
}

Entry feCustomMenuMgr::Toggle(const char* tok, EntryBinding binding) {
    Entry e;
    memset(e.token, 0, sizeof(e.token));
    strcpy_s(e.token, tok);
    e.type = EntryType_Toggle;
    e.event = EntryEvent_None;
    e.goPage = MenuPage_None;
    e.binding = binding;
    e.step = 1; e.lo = 0; e.hi = 1;
    e.posX = 0; e.posY = 0;
    return e;
}

Entry feCustomMenuMgr::Info(const char* tok) {
    Entry e;
    memset(e.token, 0, sizeof(e.token));
    strcpy_s(e.token, tok);
    e.type = EntryType_Info;
    e.event = EntryEvent_None;
    e.goPage = MenuPage_None;
    e.binding = EntryBinding_None;
    e.step = 0; e.lo = 0; e.hi = 0;
    e.posX = 0; e.posY = 0;
    return e;
}

s32 feCustomMenuMgr::CalcAutoFrameHeight(s32 numEntries, s32 extraH) {
    if (numEntries <= 0) {
        return DEF_WINDOW_H;
    }

    const s32 rowSpan = (numEntries - 1) * DEF_ROW_STEP;
    const s32 bodyHeight = DEF_CONTENT_TOP_PAD + DEF_CONTENT_PAD + rowSpan + DEF_ROW_TEXT_H + DEF_CONTENT_BOTTOM_PAD + extraH;
    return DEF_TITLE_BAR_H + DEF_BOTTOM_BAR_H + bodyHeight;
}

s32 feCustomMenuMgr::GetEntryExtraHeight(const PageDef& page, const Entry& entry) const {
    if (entry.type != EntryType_Info)
        return 0;

    const char* label = Localize(entry.token);
    if (!label)
        label = entry.token;

    const f32 wrapWidth = SCREEN_SCALE_X((f32)(page.frameW - DEF_LABEL_X_PAD * 2));
    s32 lines = 1;
    if (g_textManager && g_textManager->SetFontByName(DEF_MENU_FONT_NAME)) {
        g_textManager->SetScale(SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE), SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE));
        g_textManager->SetWrapWidth(wrapWidth);
        g_textManager->SetLineSpacing(0);
        g_textManager->SetPromptsEnabled(true);
        lines = g_textManager->CountWrappedLines(label);
        g_textManager->SetWrapWidth(0.0f);
    }
    if (lines < 1)
        lines = 1;

    return DEF_INFO_ROW_EXTRA + (lines - 1) * DEF_ROW_STEP;
}

s32 feCustomMenuMgr::CalcEntryYExtra(const PageDef& page, s32 upToIndex) const {
    s32 extra = 0;
    for (s32 i = 0; i < upToIndex; i++) {
        extra += GetEntryExtraHeight(page, page.entries[i]);
    }
    return extra;
}

s32 feCustomMenuMgr::CalcPageExtraHeight(const PageDef& page) const {
    s32 extra = 0;
    for (s32 i = 0; i < page.numEntries; i++) {
        extra += GetEntryExtraHeight(page, page.entries[i]);
    }
    return extra;
}

void feCustomMenuMgr::ResolveEntryLayout(const PageDef& page, s32 entryIndex,
                                         s32 firstY, s32 baseLabelX, s32 baseValueX, s32 baseCenterX,
                                         s32* outRowTop, s32* outRowTextY,
                                         s32* outLabelX, s32* outValueX, s32* outCenterX) const {
    const Entry& entry = page.entries[entryIndex];
    const s32 autoRowTop = firstY + entryIndex * DEF_ROW_STEP + CalcEntryYExtra(page, entryIndex);
    const s32 autoRowTextY = autoRowTop + DEF_TEXT_Y_OFF;

    s32 rowTop = autoRowTop;
    s32 rowTextY = autoRowTextY;
    s32 labelX = baseLabelX;
    s32 valueX = baseValueX;
    s32 centerX = baseCenterX;

    if (IsAutoEntryPosition(entry)) {
        rowTop += page.entriesOffsetY;
        rowTextY += page.entriesOffsetY;
        labelX += page.entriesOffsetX;
        valueX += page.entriesOffsetX;
        centerX += page.entriesOffsetX;
    }
    else {
        const s32 rowShiftX = entry.posX - baseCenterX;
        centerX = entry.posX;
        labelX = baseLabelX + rowShiftX;
        valueX = baseValueX + rowShiftX;
        rowTextY = entry.posY;
        rowTop = rowTextY - DEF_TEXT_Y_OFF;
    }

    if (outRowTop) {
        *outRowTop = rowTop;
    }
    if (outRowTextY) {
        *outRowTextY = rowTextY;
    }
    if (outLabelX) {
        *outLabelX = labelX;
    }
    if (outValueX) {
        *outValueX = valueX;
    }
    if (outCenterX) {
        *outCenterX = centerX;
    }
}

void feCustomMenuMgr::LoadControllerOverlayTexture() {
    if (m_controllerTexture) {
        return;
    }

    m_controllerTexture = tTexture::LoadFromImagePath(kControllerOverlayTexturePath);
    if (!m_controllerTexture) {
        LOG("[CustomMenu] Failed to load %s", kControllerOverlayTexturePath);
    }
}

void feCustomMenuMgr::LoadMenuOrnamentTexture() {
    m_menuOrnamentTexture = tTexture::LoadFromImagePath(kMenuOrnamentTexturePath);
    if (!m_menuOrnamentTexture) {
        LOG("[CustomMenu] Failed to load %s", kMenuOrnamentTexturePath);
    }

    m_redDragonTex = tTexture::LoadFromImagePath(kRedDragonTexturePath);
    if (!m_redDragonTex) {
        LOG("[CustomMenu] Failed to load %s", kRedDragonTexturePath);
    }
    m_goldDragonTex = tTexture::LoadFromImagePath(kGoldDragonTexturePath);
    if (!m_goldDragonTex) {
        LOG("[CustomMenu] Failed to load %s", kGoldDragonTexturePath);
    }
    m_greyDragonTex = tTexture::LoadFromImagePath(kGreyDragonTexturePath);
    if (!m_greyDragonTex) {
        LOG("[CustomMenu] Failed to load %s", kGreyDragonTexturePath);
    }
}

void feCustomMenuMgr::LoadSplashTextures() {
    if (!m_titleScreenTexture && !m_titleScreenTextureTried) {
        m_titleScreenTextureTried = true;
        m_titleScreenTexture = tTexture::LoadFromImagePath(kTitleScreenTexturePath);
        if (m_titleScreenTexture && m_titleScreenTexture->GetTexture()) {
            m_titleScreenTexture->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
        }
        if (!m_titleScreenTexture) {
            LOG("[CustomMenu] Failed to load title splash texture (%s)", kTitleScreenTexturePath);
        }
    }

    if (!m_loadingScreenTexture && !m_loadingScreenTextureTried) {
        m_loadingScreenTextureTried = true;
        m_loadingScreenTexture = tTexture::LoadFromImagePath(kLoadingScreenTexturePath);
        if (m_loadingScreenTexture && m_loadingScreenTexture->GetTexture()) {
            m_loadingScreenTexture->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
        }
        if (!m_loadingScreenTexture) {
            LOG("[CustomMenu] Failed to load loading splash texture (%s)", kLoadingScreenTexturePath);
        }
    }
}

void feCustomMenuMgr::LoadSliderTextures() {
    if (!m_sliderOTex) {
        m_sliderOTex = tTexture::LoadFromImagePath(kSliderOTexturePath);
        m_sliderOTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);

        if (!m_sliderOTex) {
            LOG("[CustomMenu] Failed to load slider empty texture (%s)", kSliderOTexturePath);
        }
    }

    if (!m_sliderFTex) {
        m_sliderFTex = tTexture::LoadFromImagePath(kSliderFTexturePath);
        m_sliderFTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);

        if (!m_sliderFTex) {
            LOG("[CustomMenu] Failed to load slider filled texture (%s)", kSliderFTexturePath);
        }
    }
}

void feCustomMenuMgr::LoadScrollArrowTexture() {
    if (m_scrollArrowTexture) {
        return;
    }

    m_scrollArrowTexture = tTexture::LoadFromImagePath(kScrollArrowTexturePath);
    if (m_scrollArrowTexture && m_scrollArrowTexture->GetTexture()) {
        m_scrollArrowTexture->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (!m_scrollArrowTexture) {
        LOG("[CustomMenu] Failed to load scroll arrow texture (%s)", kScrollArrowTexturePath);
    }
}

bool feCustomMenuMgr::GetSplashScreenRect(f32* outX, f32* outY, f32* outW, f32* outH) const {
    const f32 screenW = SCREEN_WIDTH;
    const f32 screenH = SCREEN_HEIGHT;
    if (screenW <= 0.0f || screenH <= 0.0f) {
        return false;
    }

    f32 drawW = screenW;
    f32 drawH = drawW / kSplashScreenAspect;
    if (drawH > screenH) {
        drawH = screenH;
        drawW = drawH * kSplashScreenAspect;
    }

    const f32 drawX = (screenW - drawW) * 0.5f;
    const f32 drawY = (screenH - drawH) * 0.5f;

    if (outX) {
        *outX = drawX;
    }
    if (outY) {
        *outY = drawY;
    }
    if (outW) {
        *outW = drawW;
    }
    if (outH) {
        *outH = drawH;
    }

    return true;
}

void feCustomMenuMgr::DrawSplashTexture16x9(tTexture* texture) {
    if (!texture) {
        return;
    }

    f32 drawX = 0.0f;
    f32 drawY = 0.0f;
    f32 drawW = 0.0f;
    f32 drawH = 0.0f;
    if (!GetSplashScreenRect(&drawX, &drawY, &drawW, &drawH)) {
        return;
    }

    // Clear to black first so narrower/taller targets produce clean letterbox bars.
    ScreenDraw::DrawColoredRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 255);
    ScreenDraw::DrawQuad(texture, drawX, drawY, drawW, drawH);
}

bool feCustomMenuMgr::DrawTitleScreen() {
    LoadSplashTextures();
    if (!m_titleScreenTexture) {
        return false;
    }

    DrawSplashTexture16x9(m_titleScreenTexture);
    return true;
}

void feCustomMenuMgr::DrawTitleStartPrompt(s32 baseX, s32 baseY) {
    f32 bgX = 0.0f;
    f32 bgY = 0.0f;
    f32 bgW = 0.0f;
    f32 bgH = 0.0f;
    if (!GetSplashScreenRect(&bgX, &bgY, &bgW, &bgH)) {
        return;
    }

    const f32 refW = SCREEN_SCALE_X(DEFAULT_SCREEN_WIDTH);
    const f32 refH = SCREEN_SCALE_Y(DEFAULT_SCREEN_HEIGHT);
    if (refW <= 0.0f || refH <= 0.0f) {
        return;
    }

    const f32 splashScaleX = bgW / refW;
    const f32 splashScaleY = bgH / refH;

    if (!g_textManager || !g_textManager->SetFontByName(DEF_MENU_FONT_NAME)) {
        return;
    }

    m_pulse.Update();
    const xcColour1555 pulseColor = m_pulse.GetColor();

    const char* promptText = Localize("FE_PST");
    const f32 promptX = bgX + SCREEN_SCALE_X((f32)baseX) * splashScaleX;
    const f32 promptY = bgY + SCREEN_SCALE_Y((f32)baseY) * splashScaleY;

    g_textManager->SetScale(SCREEN_SCALE_Y(DEF_MENU_TITLE_SCALE), SCREEN_SCALE_Y(DEF_MENU_TITLE_SCALE));
    g_textManager->SetAlignment(TextAlign_Center);
    g_textManager->SetWrapWidth(0.0f);
    g_textManager->SetLineSpacing(0);
    g_textManager->SetPromptsEnabled(true);
    g_textManager->SetShadow(false);
    g_textManager->SetOutline(true);
    g_textManager->SetColor(pulseColor.GetRed8(),
                            pulseColor.GetGreen8(),
                            pulseColor.GetBlue8(),
                            255);
    g_textManager->PrintString(promptText, promptX, promptY);
}

bool feCustomMenuMgr::DrawLoadingScreen() {
    LoadSplashTextures();
    if (!m_loadingScreenTexture) {
        return false;
    }

    DrawSplashTexture16x9(m_loadingScreenTexture);
    return true;
}

static void DrawGouraudRectPSX(f32 x, f32 y, f32 w, f32 h,
                               u8 topR, u8 topG, u8 topB,
                               u8 bottomR, u8 bottomG, u8 bottomB,
                               u8 alpha) {
    const f32 x0 = SCALE_AND_CENTER_X(x);
    const f32 y0 = SCREEN_SCALE_Y(y);
    const f32 x1 = SCALE_AND_CENTER_X(x + w);
    const f32 y1 = SCREEN_SCALE_Y(y + h);

    ScreenDraw::DrawGouraudQuad(
        x0, y0, topR, topG, topB, alpha,
        x1, y0, topR, topG, topB, alpha,
        x0, y1, bottomR, bottomG, bottomB, alpha,
        x1, y1, bottomR, bottomG, bottomB, alpha);

}

static void DrawGouraudRectPSX(s32 x, s32 y, s32 w, s32 h,
                               u8 topR, u8 topG, u8 topB,
                               u8 bottomR, u8 bottomG, u8 bottomB,
                               u8 alpha) {
    DrawGouraudRectPSX((f32)x, (f32)y, (f32)w, (f32)h,
                       topR, topG, topB,
                       bottomR, bottomG, bottomB,
                       alpha);
}

static f32 GetMenuBorderPx() {
    return SCREEN_SCALE_Y((f32)DEF_BORDER_W);
}

static void DrawRectPSX(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) {
    if (w <= 0 || h <= 0)
        return;

    ScreenDraw::DrawColoredRect(
        SCALE_AND_CENTER_X(x),
        SCREEN_SCALE_Y(y),
        SCREEN_SCALE_X(w),
        SCREEN_SCALE_Y(h),
        r, g, b, a);
}

static bool IsNeutralMenuColor(const xcColour1555& color) {
    return color.GetRed8() == 128 && color.GetGreen8() == 128 && color.GetBlue8() == 128;
}

static void DrawSliderCircleMeterPSX(f32 rightX, f32 textY, f32 value, tTexture* sliderOTex, tTexture* sliderFTex) {
    static constexpr s32 kSegments = DEF_SLIDER_CIRCLE_SEGMENTS;
    static constexpr f32 kSliderIconSize = 12.0f;

    if (value < 0) 
        value = 0;
    if (value > 100) 
        value = 100;

    s32 filled = (s32)((value * (f32)kSegments) / 100.0f);
    if (filled < 0) filled = 0;
    if (filled > kSegments) filled = kSegments;

    if (!sliderOTex || !sliderFTex) {
        return;
    }

    const f32 step = DEF_SLIDER_CIRCLE_STEP;
    const f32 baseX = rightX - kSegments * step;
    const f32 baseY = textY;

    for (s32 i = 0; i < kSegments; i++) {
        const bool isFilled = (i < filled);
        tTexture* tex = isFilled ? sliderFTex : sliderOTex;
        const f32 drawX = baseX + i * step + (step - kSliderIconSize) / 2;
        ScreenDraw::DrawQuad(tex,
                             SCALE_AND_CENTER_X(drawX), SCREEN_SCALE_Y(baseY),
                             SCREEN_SCALE_Y(kSliderIconSize), SCREEN_SCALE_Y(kSliderIconSize),
                             0.0f, 0.0f, 1.0f, 1.0f,
                             255, 255, 255, 255);
    }
}

static void DrawSliderCircleMeterPSX(s32 rightX, s32 textY, s32 value, tTexture* sliderOTex, tTexture* sliderFTex) {
    DrawSliderCircleMeterPSX((f32)rightX, (f32)textY, (f32)value, sliderOTex, sliderFTex);
}

static void DrawUniformBorderRectPSX(f32 x, f32 y, f32 w, f32 h, f32 borderPx,
                                     u8 r, u8 g, u8 b, u8 a) {
    if (w <= 0 || h <= 0 || borderPx <= 0.0f)
        return;

    const f32 x0 = SCALE_AND_CENTER_X(x);
    const f32 y0 = SCREEN_SCALE_Y(y);
    const f32 x1 = SCALE_AND_CENTER_X(x + w);
    const f32 y1 = SCREEN_SCALE_Y(y + h);
    const f32 rectW = x1 - x0;
    const f32 rectH = y1 - y0;
    if (rectW <= 0.0f || rectH <= 0.0f)
        return;

    const f32 t = borderPx;
    ScreenDraw::DrawColoredRect(x0, y0, rectW, t, r, g, b, a);
    ScreenDraw::DrawColoredRect(x0, y1 - t, rectW, t, r, g, b, a);
    ScreenDraw::DrawColoredRect(x0, y0, t, rectH, r, g, b, a);
    ScreenDraw::DrawColoredRect(x1 - t, y0, t, rectH, r, g, b, a);
}

static void DrawUniformBorderRectPSX(s32 x, s32 y, s32 w, s32 h, f32 borderPx,
                                     u8 r, u8 g, u8 b, u8 a) {
    DrawUniformBorderRectPSX((f32)x, (f32)y, (f32)w, (f32)h, borderPx, r, g, b, a);
}

static void DrawUniformHLinePSX(f32 x, f32 y, f32 w, f32 linePx,
                                u8 r, u8 g, u8 b, u8 a) {
    if (w <= 0 || linePx <= 0.0f)
        return;

    const f32 x0 = SCALE_AND_CENTER_X(x);
    const f32 x1 = SCALE_AND_CENTER_X(x + w);
    const f32 y0 = SCREEN_SCALE_Y(y);
    const f32 drawW = x1 - x0;
    if (drawW <= 0.0f)
        return;

    ScreenDraw::DrawColoredRect(x0, y0, drawW, linePx, r, g, b, a);
}

static void DrawUniformHLinePSX(s32 x, s32 y, s32 w, f32 linePx,
                                u8 r, u8 g, u8 b, u8 a) {
    DrawUniformHLinePSX((f32)x, (f32)y, (f32)w, linePx, r, g, b, a);
}

static void DrawUniformVLinePSX(f32 x, f32 y, f32 h, f32 linePx,
                                u8 r, u8 g, u8 b, u8 a) {
    if (h <= 0 || linePx <= 0.0f)
        return;

    const f32 x0 = SCALE_AND_CENTER_X(x);
    const f32 y0 = SCREEN_SCALE_Y(y);
    const f32 y1 = SCREEN_SCALE_Y(y + h);
    const f32 drawH = y1 - y0;
    if (drawH <= 0.0f)
        return;

    ScreenDraw::DrawColoredRect(x0, y0, linePx, drawH, r, g, b, a);
}

static void DrawUniformVLinePSX(s32 x, s32 y, s32 h, f32 linePx,
                                u8 r, u8 g, u8 b, u8 a) {
    DrawUniformVLinePSX((f32)x, (f32)y, (f32)h, linePx, r, g, b, a);
}

static void DrawUniformBorderFillRectPSX(f32 x, f32 y, f32 w, f32 h, f32 borderPx,
                                         u8 borderR, u8 borderG, u8 borderB, u8 borderA,
                                         u8 fillR, u8 fillG, u8 fillB, u8 fillA) {
    if (w <= 0 || h <= 0)
        return;

    const f32 x0 = SCALE_AND_CENTER_X(x);
    const f32 y0 = SCREEN_SCALE_Y(y);
    const f32 x1 = SCALE_AND_CENTER_X(x + w);
    const f32 y1 = SCREEN_SCALE_Y(y + h);
    const f32 rectW = x1 - x0;
    const f32 rectH = y1 - y0;
    if (rectW <= 0.0f || rectH <= 0.0f)
        return;

    ScreenDraw::DrawColoredRect(x0, y0, rectW, rectH, borderR, borderG, borderB, borderA);

    if (borderPx <= 0.0f)
        return;

    const f32 innerX = x0 + borderPx;
    const f32 innerY = y0 + borderPx;
    const f32 innerW = rectW - borderPx * 2.0f;
    const f32 innerH = rectH - borderPx * 2.0f;
    if (innerW > 0.0f && innerH > 0.0f) {
        ScreenDraw::DrawColoredRect(innerX, innerY, innerW, innerH, fillR, fillG, fillB, fillA);
    }
}

static void DrawUniformBorderFillRectPSX(s32 x, s32 y, s32 w, s32 h, f32 borderPx,
                                         u8 borderR, u8 borderG, u8 borderB, u8 borderA,
                                         u8 fillR, u8 fillG, u8 fillB, u8 fillA) {
    DrawUniformBorderFillRectPSX((f32)x, (f32)y, (f32)w, (f32)h, borderPx,
                                 borderR, borderG, borderB, borderA,
                                 fillR, fillG, fillB, fillA);
}

static void DrawGouraudRectPSXVertical(f32 x, f32 y, f32 w, f32 h,
                                       u8 leftR, u8 leftG, u8 leftB,
                                       u8 rightR, u8 rightG, u8 rightB,
                                       u8 alpha) {
    const f32 x0 = SCALE_AND_CENTER_X(x);
    const f32 y0 = SCREEN_SCALE_Y(y);
    const f32 x1 = SCALE_AND_CENTER_X(x + w);
    const f32 y1 = SCREEN_SCALE_Y(y + h);

    ScreenDraw::DrawGouraudQuad(
        x0, y0, leftR, leftG, leftB, alpha,
        x1, y0, rightR, rightG, rightB, alpha,
        x0, y1, leftR, leftG, leftB, alpha,
        x1, y1, rightR, rightG, rightB, alpha);
}

static void DrawMenuOrnament(tTexture* symbolTex, f32 x, f32 y) {
    if (!symbolTex) {
        return;
    }

    ScreenDraw::DrawQuad(
        symbolTex,
        SCALE_AND_CENTER_X(x),
        SCREEN_SCALE_Y(y),
        SCREEN_SCALE_X((f32)DEF_ORN_W),
        SCREEN_SCALE_Y((f32)DEF_ORN_H),
        0.0f, 0.0f, 1.0f, 1.0f,
        DEF_ORN_R, DEF_ORN_G, DEF_ORN_B, DEF_ORN_A);
}

static void DrawMenuOrnament(tTexture* symbolTex, s32 x, s32 y) {
    DrawMenuOrnament(symbolTex, (f32)x, (f32)y);
}

void feCustomMenuMgr::DrawMenuWindow(s32 x, s32 y, s32 w, s32 h, const char* title) const {
    const s32 titleY0 = y;
    const s32 titleY1 = y + DEF_TITLE_BAR_H;
    const s32 bodyY0 = titleY1;
    const s32 bodyY1 = y + h - DEF_BOTTOM_BAR_H;
    const s32 bottomY0 = bodyY1;
    const s32 titleInsetX = DEF_TITLE_INSET_X;
    const s32 titleInsetY = DEF_TITLE_INSET_Y;
    const s32 titleInsetH = DEF_TITLE_INSET_H;
    const s32 titleInsetW = w - titleInsetX * 2;
    const f32 framePx = GetMenuBorderPx();

    // Outer red frame
    DrawUniformBorderRectPSX(x, y, w, h, framePx, DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);

    // Gold bars: dark edge -> bright center -> dark edge (two quads per bar)
    {
        const s32 bx = x + DEF_BORDER_W;
        const s32 bw = w - DEF_BORDER_W * 2;
        const s32 titleInnerH = DEF_TITLE_BAR_H - DEF_BORDER_W;
        const s32 titleHalf = titleInnerH / 2;
        DrawGouraudRectPSX(bx, titleY0 + DEF_BORDER_W, bw, titleHalf,
                           DEF_BAR_EDGE_R, DEF_BAR_EDGE_G, DEF_BAR_EDGE_B,
                           DEF_BAR_MID_R, DEF_BAR_MID_G, DEF_BAR_MID_B,
                           DEF_BAR_ALPHA);
        DrawGouraudRectPSX(bx, titleY0 + DEF_BORDER_W + titleHalf, bw, titleInnerH - titleHalf,
                           DEF_BAR_MID_R, DEF_BAR_MID_G, DEF_BAR_MID_B,
                           DEF_BAR_EDGE_R, DEF_BAR_EDGE_G, DEF_BAR_EDGE_B,
                           DEF_BAR_ALPHA);

        const s32 botInnerH = DEF_BOTTOM_BAR_H - DEF_BORDER_W;
        const s32 botHalf = botInnerH / 2;
        DrawGouraudRectPSX(bx, bottomY0, bw, botHalf,
                           DEF_BAR_EDGE_R, DEF_BAR_EDGE_G, DEF_BAR_EDGE_B,
                           DEF_BAR_MID_R, DEF_BAR_MID_G, DEF_BAR_MID_B,
                           DEF_BAR_ALPHA);
        DrawGouraudRectPSX(bx, bottomY0 + botHalf, bw, botInnerH - botHalf,
                           DEF_BAR_MID_R, DEF_BAR_MID_G, DEF_BAR_MID_B,
                           DEF_BAR_EDGE_R, DEF_BAR_EDGE_G, DEF_BAR_EDGE_B,
                           DEF_BAR_ALPHA);
    }

    // Body fill
    DrawRect((f32)(x + DEF_BORDER_W), (f32)bodyY0, (f32)(w - DEF_BORDER_W * 2), (f32)(bodyY1 - bodyY0),
             DEF_BODY_R, DEF_BODY_G, DEF_BODY_B, DEF_BODY_A);

    // Frame
    DrawUniformHLinePSX(x + DEF_BORDER_W, bodyY0, w - DEF_BORDER_W * 2, framePx, DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);
    DrawUniformHLinePSX(x + DEF_BORDER_W, bodyY1 - DEF_BORDER_W, w - DEF_BORDER_W * 2, framePx, DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);

    // Black inset title box
    DrawUniformBorderFillRectPSX(x + titleInsetX, y + titleInsetY, titleInsetW, titleInsetH, framePx,
                                 DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A,
                                 DEF_TITLE_INSET_FILL_R, DEF_TITLE_INSET_FILL_G, DEF_TITLE_INSET_FILL_B, DEF_TITLE_INSET_FILL_A);

    // Decorative bar marks
    tTexture* ornamentTex = m_menuOrnamentTexture;
    DrawMenuOrnament(ornamentTex, x + 18, y + 10);
    DrawMenuOrnament(ornamentTex, x + w - 32, y + 10);
    for (s32 i = 0; i < DEF_BOTTOM_ORN_COUNT; i++) {
        const s32 leftX = x + 18 + i * DEF_BOTTOM_ORN_STEP;
        DrawMenuOrnament(ornamentTex, leftX, bottomY0 + DEF_BOTTOM_ORN_Y_OFF);
    }
    for (s32 i = 0; i < DEF_BOTTOM_ORN_COUNT; i++) {
        const s32 rightX = x + w - (i * DEF_BOTTOM_ORN_STEP) - 32;
        DrawMenuOrnament(ornamentTex, rightX, bottomY0 + DEF_BOTTOM_ORN_Y_OFF);
    }

    // Title text
    if (title && g_textManager && g_textManager->SetFontByName(DEF_MENU_FONT_NAME)) {
        g_textManager->SetScale(SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE), SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE));
        g_textManager->SetAlignment(TextAlign_Center);
        g_textManager->SetWrapWidth(0.0f);
        g_textManager->SetLineSpacing(0);
        g_textManager->SetPromptsEnabled(true);
        g_textManager->SetShadow(false);
        g_textManager->SetOutline(false);
        const s32 titleTextY = y + titleInsetH / 2;
        const f32 titleX = SCALE_AND_CENTER_X((f32)DEF_WINDOW_CENTER_X);
        const f32 titleY = SCREEN_SCALE_Y((f32)titleTextY);
        g_textManager->SetColor(DEF_TITLE_TEXT_R, DEF_TITLE_TEXT_G, DEF_TITLE_TEXT_B);
        g_textManager->PrintString(title, titleX, titleY);
    }
}

void feCustomMenuMgr::RenderKeyBindingsPage(s32 panelX, s32 panelY, s32 panelW, s32 panelH,
                                            const xcColour1555& normalColor,
                                            const xcColour1555& selectedColor) const {
    if (!g_textManager || !g_textManager->SetFontByName(DEF_MENU_FONT_NAME)) {
        return;
    }
    g_textManager->SetScale(SCREEN_SCALE_Y(DEF_REDEFINE_KEY_TEXT_SCALE), SCREEN_SCALE_Y(DEF_REDEFINE_KEY_TEXT_SCALE));
    g_textManager->SetWrapWidth(0.0f);
    g_textManager->SetLineSpacing(0);
    g_textManager->SetPromptsEnabled(true);
    g_textManager->SetShadow(false);
    g_textManager->SetOutline(true);

    const f32 contentTop = (f32)(panelY + DEF_TITLE_BAR_H + DEF_CONTENT_TOP_PAD);
    const f32 labelX = (f32)(panelX + DEF_LABEL_X_PAD + DEF_KEYBIND_X_PAD);
    const f32 headerY = contentTop + DEF_CONTENT_PAD + DEF_TEXT_Y_OFF;
    const f32 firstRowY = headerY + DEF_KEYBIND_ROW_STEP;
    const f32 slotW = (f32)DEF_KEYBIND_SLOT_W;
    const f32 slotGap = (f32)DEF_KEYBIND_SLOT_GAP;
    const f32 slot2Right = (f32)(panelX + panelW - DEF_VALUE_X_PAD - DEF_KEYBIND_X_PAD);
    const f32 slot2Left = slot2Right - slotW;
    const f32 slot1Right = slot2Left - slotGap;
    const f32 slot1Left = slot1Right - slotW;
    const s32 visibleRows = (kKeyBindingActionCount - m_keyBindScrollTop < DEF_KEYBIND_VISIBLE_ROWS)
        ? (kKeyBindingActionCount - m_keyBindScrollTop)
        : DEF_KEYBIND_VISIBLE_ROWS;

    const f32 tableLeft = labelX - DEF_KEYBIND_TABLE_SIDE_PAD;
    const f32 tableRight = slot2Left + slotW + DEF_KEYBIND_TABLE_SIDE_PAD;
    const f32 tableW = tableRight - tableLeft;
    const bool canScrollUp = (m_keyBindScrollTop > 0);
    const bool canScrollDown = (m_keyBindScrollTop + visibleRows < kKeyBindingActionCount);

    const f32 headerYScreen = SCREEN_SCALE_Y(headerY);
    const char* actionHeader = Localize("FE_KBACT");
    const char* bind1Header = Localize("FE_KBBN1");
    const char* bind2Header = Localize("FE_KBBN2");

    g_textManager->SetAlignment(TextAlign_Left);
    g_textManager->SetColor(normalColor.GetRed8(), normalColor.GetGreen8(), normalColor.GetBlue8());
    g_textManager->PrintString(actionHeader ? actionHeader : "Action", SCALE_AND_CENTER_X(labelX), headerYScreen);
    g_textManager->SetAlignment(TextAlign_Center);
    g_textManager->PrintString(bind1Header ? bind1Header : "Bind 1", SCALE_AND_CENTER_X(slot1Left + slotW / 2), headerYScreen);
    g_textManager->PrintString(bind2Header ? bind2Header : "Bind 2", SCALE_AND_CENTER_X(slot2Left + slotW / 2), headerYScreen);

    const bool backSelected = (m_cursor == 0);

    for (s32 row = 0; row < visibleRows; row++) {
        const s32 actionIndex = m_keyBindScrollTop + row;
        const Action action = (Action)actionIndex;
        const bool selectedRow = !backSelected && (actionIndex == m_keyBindActionCursor);
        const f32 rowY = firstRowY + row * DEF_KEYBIND_ROW_STEP;
        const f32 rowTextY = rowY + 0.8f;

        if ((row & 1) == 0) {
            DrawRect(tableLeft, (f32)(rowY - DEF_KEYBIND_ROW_TOP_PAD), tableW, (f32)DEF_KEYBIND_ROW_STEP,
                     DEF_KEYBIND_STRIPE_DARK_R, DEF_KEYBIND_STRIPE_DARK_G, DEF_KEYBIND_STRIPE_DARK_B, DEF_KEYBIND_STRIPE_DARK_A);
        }
        else {
            DrawRect(tableLeft, (f32)(rowY - DEF_KEYBIND_ROW_TOP_PAD), tableW, (f32)DEF_KEYBIND_ROW_STEP,
                     DEF_KEYBIND_STRIPE_WARM_R, DEF_KEYBIND_STRIPE_WARM_G, DEF_KEYBIND_STRIPE_WARM_B, DEF_KEYBIND_STRIPE_WARM_A);
        }

        if (selectedRow) {
            const f32 cellLeft = (m_keyBindSlotCursor == 0) ? slot1Left : slot2Left;
            DrawRect((f32)(cellLeft - DEF_KEYBIND_CELL_PAD), (f32)(rowY - DEF_KEYBIND_ROW_TOP_PAD),
                     (f32)(slotW + DEF_KEYBIND_CELL_PAD * 2), (f32)DEF_KEYBIND_ROW_STEP,
                     DEF_KEYBIND_ACTIVE_FILL_R, DEF_KEYBIND_ACTIVE_FILL_G, DEF_KEYBIND_ACTIVE_FILL_B, DEF_KEYBIND_ACTIVE_FILL_A);
            DrawUniformBorderRectPSX((f32)(cellLeft - DEF_KEYBIND_CELL_PAD), (f32)(rowY - DEF_KEYBIND_ROW_TOP_PAD),
                                     (f32)(slotW + DEF_KEYBIND_CELL_PAD * 2), (f32)DEF_KEYBIND_ROW_STEP,
                                     GetMenuBorderPx(), m_pulse.GetRed8(), m_pulse.GetGreen8(), m_pulse.GetBlue8(), 255);
        }

        char actionName[64] = {};
        char slot0Label[32] = {};
        char slot1Label[32] = {};
        const char* actionToken = ActionToToken(action);
        const char* localizedAction = actionToken ? Localize(actionToken) : nullptr;
        if (localizedAction && localizedAction[0] != '\0') {
            snprintf(actionName, (s32)sizeof(actionName), "%s", localizedAction);
        }
        else {
            BuildActionTokenFallbackLabel(actionToken, actionName, (s32)sizeof(actionName));
        }
        BuildDesktopBindingPromptText(action, 0, slot0Label, (s32)sizeof(slot0Label));
        BuildDesktopBindingPromptText(action, 1, slot1Label, (s32)sizeof(slot1Label));

        if (selectedRow && m_keyBindCaptureActive) {
            if (m_keyBindSlotCursor == 0) {
                snprintf(slot0Label, (s32)sizeof(slot0Label), "?");
            }
            else {
                snprintf(slot1Label, (s32)sizeof(slot1Label), "?");
            }
        }

        const bool selectedSlot0 = selectedRow && m_keyBindSlotCursor == 0;
        const bool selectedSlot1 = selectedRow && m_keyBindSlotCursor == 1;
        const f32 rowScreenY = SCREEN_SCALE_Y(rowTextY);
        g_textManager->SetAlignment(TextAlign_Left);
        g_textManager->SetColor(normalColor.GetRed8(), normalColor.GetGreen8(), normalColor.GetBlue8());
        g_textManager->PrintString(actionName, SCALE_AND_CENTER_X(labelX), rowScreenY);
        g_textManager->SetAlignment(TextAlign_Center);
        g_textManager->SetColor(selectedSlot0 ? selectedColor.GetRed8() : normalColor.GetRed8(),
                         selectedSlot0 ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                         selectedSlot0 ? selectedColor.GetBlue8() : normalColor.GetBlue8());
        g_textManager->PrintString(slot0Label, SCALE_AND_CENTER_X((slot1Left + slotW / 2)), rowScreenY);
        g_textManager->SetColor(selectedSlot1 ? selectedColor.GetRed8() : normalColor.GetRed8(),
                         selectedSlot1 ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                         selectedSlot1 ? selectedColor.GetBlue8() : normalColor.GetBlue8());
        g_textManager->PrintString(slot1Label, SCALE_AND_CENTER_X((slot2Left + slotW / 2)), rowScreenY);
    }

    char scrollText[32] = {};
    snprintf(scrollText, (s32)sizeof(scrollText), "%d-%d/%d",
             m_keyBindScrollTop + 1,
             m_keyBindScrollTop + visibleRows,
             kKeyBindingActionCount);
    g_textManager->SetAlignment(TextAlign_Right);
    g_textManager->SetColor(normalColor.GetRed8(), normalColor.GetGreen8(), normalColor.GetBlue8());
    g_textManager->PrintString(scrollText,
                               SCALE_AND_CENTER_X((panelX + panelW - DEF_VALUE_X_PAD)),
                               SCREEN_SCALE_Y((panelY + panelH - DEF_BOTTOM_BAR_H - DEF_CONTENT_BOTTOM_PAD - DEF_ROW_TEXT_H + DEF_TEXT_Y_OFF)));

    if (m_scrollArrowTexture && (canScrollUp || canScrollDown)) {
        const u32 frameCounter = g_time ? g_time->GetFrameCounter() : 0u;
        const f32 slotW = 20.0f;
        const f32 slotH = 20.0f;
        const f32 slotGap = 2.0f;
        const f32 leftX = (f32)(panelX + DEF_BORDER_W + 4);
        const f32 bottomY = (f32)(panelY + panelH - DEF_BOTTOM_BAR_H - DEF_CONTENT_BOTTOM_PAD - 2);
        const f32 topY = (f32)(panelY + DEF_TITLE_BAR_H + DEF_CONTENT_BOTTOM_PAD);

        auto drawArrow = [&](f32 slotX, f32 slotY, bool up, s32 phaseOffset) {
            const f32 pulse = ScrollArrowPulseScale(frameCounter, phaseOffset);
            const f32 drawW = SCREEN_SCALE_Y(slotW * pulse);
            const f32 drawH = SCREEN_SCALE_Y(slotH * pulse);
            const f32 baseX = SCALE_AND_CENTER_X(slotX);
            const f32 baseY = SCREEN_SCALE_Y(slotY);
            const f32 drawX = baseX + (SCREEN_SCALE_X(slotW) - drawW) * 0.5f;
            const f32 drawY = baseY + (SCREEN_SCALE_Y(slotH) - drawH) * 0.5f;
            const f32 v0 = up ? 1.0f : 0.0f;
            const f32 v1 = up ? 0.0f : 1.0f;
            ScreenDraw::DrawQuad(m_scrollArrowTexture,
                                 drawX, drawY, drawW, drawH,
                                 0.0f, v0, 1.0f, v1,
                                 m_pulse.GetRed8(), m_pulse.GetGreen8(), m_pulse.GetBlue8(), 255);
        };

        if (canScrollDown) {
            drawArrow(leftX, bottomY - slotH, false, 0);
        }
        if (canScrollUp) {
            drawArrow(leftX, topY, true, 12);
        }
    }
}

void feCustomMenuMgr::Render() {
    if (!m_active)
        return;

    const PageDef* page = &m_pages[m_currPage];
    if (!page)
        return;

    m_pulse.Update();

    const s32 panelX = DEF_WINDOW_CENTER_X - page->frameW / 2;
    const s32 panelY = DEF_WINDOW_CENTER_Y - page->frameH / 2;
    const s32 panelW = page->frameW;
    const s32 panelH = page->frameH;

    const char* title = Localize(page->titleToken);
    if (!title)
        title = page->titleToken;
    char locationTitle[64] = {};

    // For the location page the title bar must show the selected destination name.
    if (m_currPage == MenuPage_Location) {
        LocationRuntimeInfo info = {};
        if (ResolveLocationRuntimeInfo(&info)) {
            if (info.levelID >= 1 && info.levelID <= 5) {
                const char* levelFmt = Localize("FE_LVL");
                if (!levelFmt || levelFmt[0] == '\0') {
                    levelFmt = "Level %d";
                }

                snprintf(locationTitle, sizeof(locationTitle), levelFmt, info.subLevel + 1);
                title = locationTitle;
            }
            else {
                bool hasSpecialTitle = false;
                const char* specialToken = GetSpecialLocationToken(info.levelID);
                if (specialToken) {
                    const char* localizedSpecial = Localize(specialToken);
                    if (localizedSpecial && localizedSpecial[0] != '\0') {
                        title = localizedSpecial;
                        hasSpecialTitle = true;
                    }
                }

                if (!hasSpecialTitle) {
                    const char* specialTitle = ResolveLocationSpecialTitle(info.levelIndex);
                    if (specialTitle && specialTitle[0] != '\0') {
                        title = specialTitle;
                        hasSpecialTitle = true;
                    }
                }

                if (!hasSpecialTitle && info.levelName && info.levelName[0] != '\0') {
                    title = info.levelName;
                    hasSpecialTitle = true;
                }

                if (!hasSpecialTitle) {
                    const char* levelFmt = Localize("FE_LVL");
                    if (!levelFmt || levelFmt[0] == '\0') {
                        levelFmt = "Level %d";
                    }

                    snprintf(locationTitle, sizeof(locationTitle), levelFmt, info.subLevel + 1);
                    title = locationTitle;
                }
            }
        }
    }

    DrawMenuWindow(panelX, panelY, panelW, panelH, title);

    // Build normalColor directly (PSX scale: 128 = neutral/1.0 for the tint shader)
    const xcColour1555 normalColor{ DEF_TEXT_NORM_R, DEF_TEXT_NORM_G, DEF_TEXT_NORM_B };
    const xcColour1555 selectedColor = m_pulse.GetColor();
    if (!g_textManager || !g_textManager->SetFontByName(DEF_MENU_FONT_NAME)) {
        return;
    }
    g_textManager->SetScale(SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE), SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE));
    g_textManager->SetWrapWidth(0.0f);
    g_textManager->SetLineSpacing(0);
    g_textManager->SetPromptsEnabled(true);
    g_textManager->SetShadow(false);
    g_textManager->SetOutline(true);

    // Dragon panel: present on pause page only
    static constexpr s32 DRAGON_PANEL_W = 88;
    const bool hasDragonPanel = (m_currPage == MenuPage_Pause);
    const s32 dragonBoxW = hasDragonPanel ? DRAGON_PANEL_W : 0;
    const s32 dragonBoxX = panelX + panelW - dragonBoxW;

    const s32 contentTop = panelY + DEF_TITLE_BAR_H + DEF_CONTENT_TOP_PAD;
    const s32 labelX = panelX + DEF_LABEL_X_PAD;
    // When the dragon panel is present, clamp valueX so it doesn't overlap.
    const s32 valueX = hasDragonPanel ? (dragonBoxX - DEF_VALUE_X_PAD) : (panelX + panelW - DEF_VALUE_X_PAD);
    const s32 rowSpan = (page->numEntries > 0) ? ((page->numEntries - 1) * DEF_ROW_STEP) : 0;
    const s32 extraH = CalcPageExtraHeight(*page);
    const s32 entryBlockH = DEF_CONTENT_PAD + rowSpan + DEF_ROW_TEXT_H + extraH;
    const s32 bodyAvailH = panelH - DEF_TITLE_BAR_H - DEF_BOTTOM_BAR_H - DEF_CONTENT_TOP_PAD - DEF_CONTENT_BOTTOM_PAD;
    const s32 bodyCenterPad = (bodyAvailH > entryBlockH) ? ((bodyAvailH - entryBlockH) / 2) : 0;
    const s32 firstY = contentTop + bodyCenterPad + DEF_CONTENT_PAD;
    // Shift entry center left to the midpoint of the usable content area.
    const s32 contentCenterX = hasDragonPanel
        ? (panelX + DEF_BORDER_W + (dragonBoxX - DEF_BORDER_W - panelX) / 2)
        : DEF_WINDOW_CENTER_X;

    switch (m_currPage) {
        case MenuPage_KeyBindings:
            RenderKeyBindingsPage(panelX, panelY, panelW, panelH, normalColor, selectedColor);
            break;
        case MenuPage_Controller:
            RenderControllerOverlay(panelX, panelY);
            break;
        case MenuPage_Location:
            RenderLocationPage();
            break;
        default:
            break;
    }

    g_textManager->SetScale(SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE), SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE));

    if (m_currPage != MenuPage_Location) {
        for (s32 i = 0; i < page->numEntries; i++) {
            const Entry& item = page->entries[i];
            const bool selected = (i == m_cursor);

            s32 rowY = 0;
            s32 rowLabelX = labelX;
            s32 rowValueX = valueX;
            s32 rowCenterX = contentCenterX;
            ResolveEntryLayout(*page, i,
                               firstY, labelX, valueX, contentCenterX,
                               nullptr, &rowY, &rowLabelX, &rowValueX, &rowCenterX);

            const char* label = Localize(item.token);
            if (!label) label = item.token;

            const f32 rowScreenY = SCREEN_SCALE_Y((f32)rowY);
            const f32 labelScreenX = SCALE_AND_CENTER_X((f32)rowLabelX);
            const f32 valueScreenX = SCALE_AND_CENTER_X((f32)rowValueX);
            const f32 centerScreenX = SCALE_AND_CENTER_X((f32)rowCenterX);

            if (item.type == EntryType_Info) {
                const f32 wrapWidth = SCREEN_SCALE_X((f32)(page->frameW - DEF_LABEL_X_PAD * 2));
                g_textManager->SetAlignment(TextAlign_Center);
                g_textManager->SetWrapWidth(wrapWidth);
                g_textManager->SetColor(DEF_INFO_TEXT_R, DEF_INFO_TEXT_G, DEF_INFO_TEXT_B);
                g_textManager->PrintString(label, centerScreenX, rowScreenY);
                g_textManager->SetWrapWidth(0.0f);
            }
            else if (item.type == EntryType_List && item.binding != EntryBinding_None) {
                g_textManager->SetAlignment(TextAlign_Left);
                g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                 selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                 selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                g_textManager->PrintString(label, labelScreenX, rowScreenY);

                if (item.binding == EntryBinding_DisplayResolution) {
                    s32 idx = GetBoundValue(item);
                    if (selected && m_pendingResolutionActive) {
                        idx = m_pendingResolutionIndex;
                    }

                    const char* resText = nullptr;

                    char resTextBuf[32];
                    if (g_display) {
                        pddiVideoMode mode;
                        if (g_display->GetResolutionMode(idx, mode)) {
                            sprintf_s(resTextBuf, "%dx%d", mode.width, mode.height);
                            resText = resTextBuf;
                        }
                    }

                    if (!resText) {
                        resText = Localize("FE_AUT");
                    }

                    if (!resText) {
                        continue;
                    }

                    g_textManager->SetAlignment(TextAlign_Right);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(resText, valueScreenX, rowScreenY);
                }
                else if (item.binding == EntryBinding_DisplayScreenMode) {
                    s32 mode = GetBoundValue(item);
                    if (selected && m_pendingScreenModeActive) {
                        mode = m_pendingScreenMode;
                    }
                    const char* modeText = Localize("FE_SCF");
                    if (mode == ScreenMode_Borderless) {
                        modeText = Localize("FE_SCB");
                    }
                    else if (mode == ScreenMode_Windowed) {
                        modeText = Localize("FE_SCW");
                    }

                    g_textManager->SetAlignment(TextAlign_Right);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(modeText, valueScreenX, rowScreenY);
                }
                else if (item.binding == EntryBinding_DisplayMsaa) {
                    s32 msaaIndex = GetBoundValue(item);
                    if (selected && m_pendingMsaaActive) {
                        msaaIndex = m_pendingMsaaIndex;
                    }
                    const s32 msaaSamples = MsaaOptionIndexToSamples(msaaIndex);
                    const char* msaaToken = (msaaSamples == 0)
                        ? "FE_OFF"
                        : GetMsaaDisplayToken(msaaIndex);
                    const char* msaaText = msaaToken ? Localize(msaaToken) : nullptr;

                    if (!msaaText) {
                        continue;
                    }

                    g_textManager->SetAlignment(TextAlign_Right);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(msaaText, valueScreenX, rowScreenY);
                }
                else if (item.binding == EntryBinding_DisplayFrameRate) {
                    const char* frameRateToken = GetFrameRateDisplayToken(GetBoundValue(item));
                    const char* frameRateText = frameRateToken ? Localize(frameRateToken) : nullptr;

                    if (!frameRateText) {
                        continue;
                    }

                    g_textManager->SetAlignment(TextAlign_Right);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(frameRateText, valueScreenX, rowScreenY);
                }
                else if (item.binding == EntryBinding_PlayerConfig) {
                    const s32 cfg = GetBoundValue(item);
                    const char* cfgToken = (cfg == 0) ? "FE_CF1"
                        : (cfg == 1) ? "FE_CF2"
                        : "FE_CF3";
                    const char* cfgText = Localize(cfgToken);

                    if (!cfgText) {
                        continue;
                    }

                    g_textManager->SetAlignment(TextAlign_Right);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(cfgText, valueScreenX, rowScreenY);
                }
                else if (item.binding == EntryBinding_Language) {
                    const char* langToken = GetLanguageDisplayToken(GetBoundValue(item));
                    const char* langText = langToken ? Localize(langToken) : nullptr;

                    if (!langText) {
                        continue;
                    }

                    g_textManager->SetAlignment(TextAlign_Right);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(langText, valueScreenX, rowScreenY);
                }
            }
            else if (item.type == EntryType_Slider && item.binding != EntryBinding_None) {
                g_textManager->SetAlignment(TextAlign_Left);
                g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                 selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                 selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                g_textManager->PrintString(label, labelScreenX, rowScreenY);

                DrawSliderCircleMeterPSX(
                    rowValueX,
                    rowY,
                    GetBoundValue(item),
                    m_sliderOTex,
                    m_sliderFTex);
            }
            else if (item.type == EntryType_Toggle && item.binding != EntryBinding_None) {
                const s32 toggle = GetBoundValue(item);
                const char* toggleToken = toggle ? "FE_ON" : "FE_OFF";
                const char* toggleText = Localize(toggleToken);

                if (!toggleText) {
                    continue;
                }

                g_textManager->SetAlignment(TextAlign_Left);
                g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                 selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                 selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                g_textManager->PrintString(label, labelScreenX, rowScreenY);

                g_textManager->SetAlignment(TextAlign_Right);
                g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                 selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                 selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                g_textManager->PrintString(toggleText, valueScreenX, rowScreenY);
            }
            else {
                if (IsSaveSlotPage(m_currPage) && i < SAVEGAME_SLOT_COUNT) {
                    char slotLabel[96] = {};
                    BuildSaveSlotLabel(i, slotLabel, (s32)sizeof(slotLabel));
                    g_textManager->SetAlignment(TextAlign_Left);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(slotLabel, labelScreenX, rowScreenY);
                }
                else {
                    g_textManager->SetAlignment(TextAlign_Center);
                    g_textManager->SetColor(selected ? selectedColor.GetRed8() : normalColor.GetRed8(),
                                     selected ? selectedColor.GetGreen8() : normalColor.GetGreen8(),
                                     selected ? selectedColor.GetBlue8() : normalColor.GetBlue8());
                    g_textManager->PrintString(label, centerScreenX, rowScreenY);
                }
            }
        }
    }

    // Gold dragon count panel (pause menu only)
    if (hasDragonPanel) {
        const s32 dragonBodyY = panelY + DEF_TITLE_BAR_H;
        const s32 dragonBodyH = panelH - DEF_TITLE_BAR_H - DEF_BOTTOM_BAR_H;
        const s32 dragonBackHalf = dragonBodyH / 2;
        const s32 dragonInset = 6;
        const s32 dragonInnerX = dragonBoxX + dragonInset;
        const s32 dragonInnerY = dragonBodyY + dragonInset;
        const s32 dragonInnerW = dragonBoxW - dragonInset * 2;
        const s32 dragonInnerH = dragonBodyH - dragonInset * 2;
        const f32 dragonFramePx = GetMenuBorderPx();

        // Back layer: full gold box.
        DrawGouraudRectPSX(dragonBoxX, dragonBodyY, dragonBoxW, dragonBackHalf,
                           DEF_BAR_EDGE_R, DEF_BAR_EDGE_G, DEF_BAR_EDGE_B,
                           DEF_BAR_MID_R, DEF_BAR_MID_G, DEF_BAR_MID_B,
                           DEF_BAR_ALPHA);
        DrawGouraudRectPSX(dragonBoxX, dragonBodyY + dragonBackHalf, dragonBoxW, dragonBodyH - dragonBackHalf,
                           DEF_BAR_MID_R, DEF_BAR_MID_G, DEF_BAR_MID_B,
                           DEF_BAR_EDGE_R, DEF_BAR_EDGE_G, DEF_BAR_EDGE_B,
                           DEF_BAR_ALPHA);
        DrawUniformHLinePSX(dragonBoxX, dragonBodyY + dragonBodyH - DEF_BORDER_W, dragonBoxW, dragonFramePx,
                            DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);
        DrawUniformBorderRectPSX(dragonBoxX, dragonBodyY, dragonBoxW, dragonBodyH, dragonFramePx,
                                 DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);

        // Front layer: smaller pure-black panel with red outline.
        DrawUniformBorderFillRectPSX(dragonInnerX, dragonInnerY, dragonInnerW, dragonInnerH, dragonFramePx,
                                     DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A,
                                     0, 0, 0, 255);

        const s32 dragonCenterX = dragonInnerX + dragonInnerW / 2;
        const s32 dragonMidY = dragonInnerY + dragonInnerH / 2;
        const s32 dragonIconY = dragonMidY - 22;
        const s32 dragonCountY = dragonIconY + 32;
        ScreenDraw::DrawQuad(m_goldDragonTex,
                             SCALE_AND_CENTER_X((f32)dragonCenterX - 24.0f), SCREEN_SCALE_Y(dragonIconY + DEF_TEXT_Y_OFF),
                             SCREEN_SCALE_Y(32), SCREEN_SCALE_Y(32),
                             0.0f, 0.0f, 1.0f, 1.0f, 255, 255, 255, 255);

        s32 totalGold = g_scoreManager ? g_scoreManager->GetTotalGoldDragon() : 0;
        if (totalGold > 99) totalGold = 99;
        char dragonCountStr[8];
        sprintf_s(dragonCountStr, "%d", totalGold);

        g_textManager->SetScale(SCREEN_SCALE_Y(DEF_MENU_DRAGON_COUNT_SCALE), SCREEN_SCALE_Y(DEF_MENU_DRAGON_COUNT_SCALE));
        g_textManager->SetAlignment(TextAlign_Center);
        g_textManager->SetWrapWidth(0.0f);
        g_textManager->SetLineSpacing(0);
        g_textManager->SetPromptsEnabled(true);
        g_textManager->SetShadow(false);
        g_textManager->SetOutline(true);
        g_textManager->SetColor(200, 200, 200);
        g_textManager->PrintString(dragonCountStr, SCALE_AND_CENTER_X((f32)dragonCenterX), SCREEN_SCALE_Y((f32)(dragonCountY + DEF_TEXT_Y_OFF)));
    }

    // Help prompts in the bottom bar
    if (g_textManager && g_textManager->SetFontByName(DEF_MENU_FONT_NAME) && m_currPage != MenuPage_Quitting) {
        f32 helpScale = DEF_MENU_PROMPT_SCALE;
        f32 promptGap = DEF_HELP_GROUP_GAP_PX;

        g_textManager->SetScale(SCREEN_SCALE_Y(helpScale), SCREEN_SCALE_Y(helpScale));
        g_textManager->SetAlignment(TextAlign_Left);
        g_textManager->SetWrapWidth(0.0f);
        g_textManager->SetLineSpacing(0);
        g_textManager->SetPromptsEnabled(true);
        g_textManager->SetShadow(false);
        g_textManager->SetOutline(false);
        const Entry* selectedEntry = nullptr;
        if (m_currPage >= 0 && m_currPage < MenuPage_Count) {
            const PageDef& currentPage = m_pages[m_currPage];
            if (m_cursor >= 0 && m_cursor < currentPage.numEntries) {
                selectedEntry = &currentPage.entries[m_cursor];
            }
        }

        char prompts[6][96] = {};
        s32 promptCount = 0;
        auto pushPrompt = [&](const char* token, const char* fallback) {
            if (promptCount >= (s32)(sizeof(prompts) / sizeof(prompts[0]))) {
                return;
            }

            const char* localized = Localize(token);
            SetPromptText(prompts[promptCount], (s32)sizeof(prompts[promptCount]), "%s", localized ? localized : fallback);
            promptCount++;
        };

        if (m_currPage == MenuPage_KeyBindings) {
            if (m_keyBindCaptureActive) {
                pushPrompt("FE_KBPR", "Press key or mouse");
                pushPrompt("FE_KBCLR", "<ACT:MENU_CLEAR> Unbind");
                pushPrompt("FE_KBCAN", "<ACT:MENU_BACK> Cancel");
            }
            else if (m_cursor == 0) {
                pushPrompt("FE_HPSEL", "<ACT:MENU_CONFIRM> Select");
                pushPrompt("FE_KBBCK", "<ACT:MENU_BACK> Back");
            }
            else {
                pushPrompt("FE_KBSLT", "<ACT:MENU_LEFT>/<ACT:MENU_RIGHT> Slot");
                pushPrompt("FE_KBBND", "<ACT:MENU_CONFIRM> Bind");
                pushPrompt("FE_KBCLR", "<ACT:MENU_CLEAR> Unbind");
                pushPrompt("FE_KBBCK", "<ACT:MENU_BACK> Back");
            }
        }
        else {
            if (selectedEntry) {
                if (selectedEntry->type == EntryType_Slider) {
                    pushPrompt("FE_HPADJ", "<ACT:MENU_LEFT> / <ACT:MENU_RIGHT> Adjust");
                }
                else if (selectedEntry->type == EntryType_List) {
                    pushPrompt("FE_HPADJ", "<ACT:MENU_LEFT> / <ACT:MENU_RIGHT> Adjust");
                    pushPrompt("FE_HPSET", "<ACT:MENU_CONFIRM> Set");
                }
                else if (selectedEntry->type == EntryType_Toggle) {
                    pushPrompt("FE_HPADJ", "<ACT:MENU_LEFT> / <ACT:MENU_RIGHT> Adjust");
                    pushPrompt("FE_HPTGL", "<ACT:MENU_CONFIRM> Toggle");
                }
                else {
                    pushPrompt("FE_HPSEL", "<ACT:MENU_CONFIRM> Select");
                }
            }

            pushPrompt("FE_HPBCK", "<ACT:MENU_BACK> Back");
        }

        const s32 bottomBarY = panelY + panelH - DEF_BOTTOM_BAR_H;
        const f32 helpY = SCREEN_SCALE_Y((f32)(bottomBarY + DEF_HELP_Y_PAD));
        const f32 centerScreenX = SCALE_AND_CENTER_X((f32)DEF_WINDOW_CENTER_X);

        f32 totalWidth = 0.0f;
        for (s32 i = 0; i < promptCount; i++) {
            totalWidth += g_textManager->MeasureString(prompts[i]).width;
            if (i + 1 < promptCount) {
                totalWidth += promptGap;
            }
        }

        f32 cursorX = centerScreenX - totalWidth * 0.5f;
        g_textManager->SetColor(DEF_HELP_TEXT_R, DEF_HELP_TEXT_G, DEF_HELP_TEXT_B);
        for (s32 i = 0; i < promptCount; i++) {
            g_textManager->PrintString(prompts[i], cursorX, helpY);
            cursorX += g_textManager->MeasureString(prompts[i]).width;

            if (i + 1 < promptCount) {
                cursorX += promptGap;
            }
        }
    }
}

void feCustomMenuMgr::RenderLocationPage() const {
    LocationRuntimeInfo info = {};
    if (!ResolveLocationRuntimeInfo(&info)) {
        return;
    }

    if (!g_textManager || !g_textManager->SetFontByName(DEF_MENU_FONT_NAME)) {
        return;
    }
    g_textManager->SetScale(SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE), SCREEN_SCALE_Y(DEF_MENU_TEXT_SCALE));
    g_textManager->SetWrapWidth(0.0f);
    g_textManager->SetLineSpacing(0);
    g_textManager->SetPromptsEnabled(true);
    g_textManager->SetShadow(false);
    g_textManager->SetOutline(true);

    static constexpr s32 LOC_ICON_SIZE = 28;
    static constexpr s32 LOC_ICON_GAP = 14;

    const PageDef& locationPage = m_pages[MenuPage_Location];
    const s32 panelW = locationPage.frameW;
    const s32 panelH = locationPage.frameH;
    const s32 panelX = DEF_WINDOW_CENTER_X - panelW / 2;
    const s32 panelY = DEF_WINDOW_CENTER_Y - panelH / 2;
    const s32 bodyY0 = panelY + DEF_TITLE_BAR_H;
    const s32 bodyY1 = panelY + panelH - DEF_BOTTOM_BAR_H;
    const f32 framePx = GetMenuBorderPx();

    const s32 lineX = panelX + DEF_BORDER_W;
    const s32 lineW = panelW - DEF_BORDER_W * 2;

    const s32 topLineY = bodyY0;
    const s32 gradeTopY = bodyY0;
    const s32 midLineY = gradeTopY + LOC_ICON_SIZE;
    const s32 row1TopY = midLineY;
    const s32 row2TopY = row1TopY + LOC_ICON_SIZE;
    const s32 botLineY = row2TopY + LOC_ICON_SIZE + 2;

    const s32 gridW = 5 * LOC_ICON_SIZE + 4 * LOC_ICON_GAP;
    const s32 gridLeftX = DEF_WINDOW_CENTER_X - gridW / 2;

    const s32 labelX = panelX - DEF_LABEL_X_PAD + DEF_WINDOW_W / 2;
    const s32 gradeBoxX = labelX + 12;
    const s32 gradeBoxW = 64;
    const s32 gradeBoxH = 18;
    const s32 goldIconX = gradeBoxX + gradeBoxW + 12;
    const s32 goldIconY = gradeTopY;

    // Red separator lines
    DrawUniformHLinePSX(lineX, midLineY, lineW, framePx, DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);

    // Grade black box
    DrawUniformBorderFillRectPSX(gradeBoxX, gradeTopY + 5, gradeBoxW, gradeBoxH, framePx,
                                 DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A,
                                 0, 0, 0, 255);

    // Grade label
    {
        const char* lbl = Localize("FE_GRD");
        const f32 lx = SCALE_AND_CENTER_X((f32)labelX);
        const f32 ly = SCREEN_SCALE_Y((f32)(gradeTopY + 8));
        g_textManager->SetAlignment(TextAlign_Right);
        g_textManager->SetColor(DEF_TEXT_NORM_R, DEF_TEXT_NORM_G, DEF_TEXT_NORM_B);
        g_textManager->PrintString(lbl, lx, ly);
    }

    // Grade letter from stored per-petal score stats.
    {
        const char* text = GradeToLetter(info.hasGrade ? info.grade : 0);
        const f32 cx = SCALE_AND_CENTER_X((f32)(gradeBoxX + gradeBoxW / 2));
        const f32 cy = SCREEN_SCALE_Y((f32)(gradeTopY + 8));
        g_textManager->SetAlignment(TextAlign_Center);
        g_textManager->SetColor(DEF_TITLE_TEXT_R, DEF_TITLE_TEXT_G, DEF_TITLE_TEXT_B);
        g_textManager->PrintString(text, cx, cy);
    }

    if (info.showDragons) {
        // Gold dragon icon at right of grade row, derived from collect count.
        {
            tTexture* iconTex = info.hasGoldDragon ? m_goldDragonTex : m_greyDragonTex;
            if (iconTex) {
                ScreenDraw::DrawQuad(iconTex,
                                     SCALE_AND_CENTER_X((f32)goldIconX), SCREEN_SCALE_Y((f32)goldIconY),
                                     SCREEN_SCALE_Y((f32)LOC_ICON_SIZE), SCREEN_SCALE_Y((f32)LOC_ICON_SIZE),
                                     0.0f, 0.0f, 1.0f, 1.0f, 255, 255, 255, 255);
            }
        }

        // Dragon bar icons from stored collect count.
        {
            const s32 rowTopY[2] = { row1TopY, row2TopY };
            const s32 unlocked = (info.collectCount < 0) ? 0 : ((info.collectCount > 10) ? 10 : info.collectCount);
            for (s32 row = 0; row < 2; row++) {
                for (s32 col = 0; col < 5; col++) {
                    const s32 idx = row * 5 + col;
                    tTexture* tex = (idx < unlocked) ? m_redDragonTex : m_greyDragonTex;
                    if (!tex)
                        continue;
                    const s32 ix = gridLeftX + col * (LOC_ICON_SIZE + LOC_ICON_GAP);
                    ScreenDraw::DrawQuad(tex,
                                         SCALE_AND_CENTER_X((f32)ix), SCREEN_SCALE_Y((f32)rowTopY[row]),
                                         SCREEN_SCALE_Y((f32)LOC_ICON_SIZE), SCREEN_SCALE_Y((f32)LOC_ICON_SIZE),
                                         0.0f, 0.0f, 1.0f, 1.0f, 255, 255, 255, 255);
                }
            }
        }
    }
}

void feCustomMenuMgr::DrawRect(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) const {
    const f32 nx = SCALE_AND_CENTER_X(x);
    const f32 ny = SCREEN_SCALE_Y(y);
    const f32 nw = SCREEN_SCALE_X(w);
    const f32 nh = SCREEN_SCALE_Y(h);
    ScreenDraw::DrawColoredRect(nx, ny, nw, nh, r, g, b, a);
}

void feCustomMenuMgr::DrawHighlight(f32 x, f32 y, f32 w, f32 h) const {
    DrawRect(x, y, w, h,
             m_pulse.GetRed8(), m_pulse.GetGreen8(), m_pulse.GetBlue8(), 55);
}

const char* feCustomMenuMgr::Localize(const char* token) const {
    if (!m_text || !token)
        return nullptr;

    return m_text->GetString(token);
}

void feCustomMenuMgr::RenderControllerOverlay(s32 panelX, s32 panelY) const {
    if (!m_controllerTexture) {
        return;
    }

    const f32 screenTexW = SCREEN_SCALE_Y(128);
    const f32 screenTexH = SCREEN_SCALE_Y(128);
    const f32 screenTexX = SCALE_AND_CENTER_X(DEFAULT_SCREEN_WIDTH / 2);
    const f32 screenTexY = SCREEN_SCALE_Y(panelY + 32.0f);

    ScreenDraw::DrawQuad(m_controllerTexture, screenTexX - screenTexW / 2, screenTexY, screenTexW, screenTexH,
                         0.0f, 0.0f, 1.0f, 1.0f, 255, 255, 255, 255);

    struct ButtonLabel {
        s32 physicalIndex;
        const char* token;
        f32 relX;
        f32 relY;
        TextAlign alignment;
    };

    const ButtonLabel buttons[] = {
        { 0,  nullptr,  -104.0f,  7.0f,  TextAlign_Right },
        { 2,  nullptr,  -104.0f,  16.0f, TextAlign_Right },
        { -1, "FE_CMV", -104.0f, 37.5f, TextAlign_Right },
        { -1, "FE_CMV", -104.0f, 55.5f, TextAlign_Right },
        { -1, "FE_CNU", -104.0f, 71.5f, TextAlign_Right },

        { 1,  nullptr,  104.0f,   7.0f,  TextAlign_Left },
        { 3,  nullptr,  104.0f,   16.0f, TextAlign_Left },
        { 4,  nullptr,  104.0f,   23.0f, TextAlign_Left },
        { 7,  nullptr,  104.0f,   29.5f, TextAlign_Left },
        { 5,  nullptr,  104.0f,   36.5f, TextAlign_Left },
        { 6,  nullptr,  104.0f,   44.0f, TextAlign_Left },
        { -1, "FE_CNU", 104.0f,  54.0f, TextAlign_Left },
        { -1, "FE_CMO", 104.0f,  71.5f, TextAlign_Left },
    };

    if (!g_textManager || !g_textManager->SetFontByName(DEF_MENU_FONT_NAME)) {
        return;
    }
    g_textManager->SetScale(SCREEN_SCALE_Y(DEF_CONTROLLER_ACTION_SCALE), SCREEN_SCALE_Y(DEF_CONTROLLER_ACTION_SCALE));
    g_textManager->SetWrapWidth(0.0f);
    g_textManager->SetLineSpacing(0);
    g_textManager->SetPromptsEnabled(true);
    g_textManager->SetShadow(false);
    g_textManager->SetOutline(true);
    const u8* playerMap = g_inputManager ? g_inputManager->PlayerMapArray() : nullptr;

    for (const auto& btn : buttons) {
        char displayName[32] = {};
        const char* text = nullptr;

        if (btn.token) {
            text = Localize(btn.token);
        }
        else if (btn.physicalIndex >= 0 && playerMap && btn.physicalIndex < 16) {
            text = Localize(GetControllerLogicalLabelToken(playerMap[btn.physicalIndex]));
        }

        if (!text) {
            displayName[0] = '\0';
        }
        else {
            snprintf(displayName, sizeof(displayName), "%s", text);
        }

        if (displayName[0] != '\0') {
            const f32 labelScreenX = screenTexX + SCREEN_SCALE_X(btn.relX);
            const f32 labelScreenY = screenTexY + SCREEN_SCALE_Y(btn.relY);

            g_textManager->SetAlignment(btn.alignment);
            g_textManager->SetColor(DEF_TEXT_NORM_R, DEF_TEXT_NORM_G, DEF_TEXT_NORM_B);
            g_textManager->PrintString(displayName, labelScreenX, labelScreenY);
        }
    }

    g_textManager->SetScale(1.0f, 1.0f);
}
