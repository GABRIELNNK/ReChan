#include "gen/common.h"
#include "fecustommenumgr.h"
#include "gen/display.h"
#include "fe/xcfont.h"
#include "xclib/xclib.h"
#include "pc/inputaction.h"
#include "pc/tim.h"
#include "gen/game.h"
#include "fe/femenumgr.h"
#include "fe/gamemenu.h"
#include "snd/fesnd.h"
#include "snd/rsevent.h"
#include "snd/sound.h"
#include "pc/settings.h"
#include "gen/time.h"
#include "xclib/xccolour.h"
#include "gen/scoremgr.h"
#include "gen/world.h"
#include "p3d/texture.h"
#include <cmath>
#include <cstdio>

feCustomMenuMgr* g_feCustomMenuMgr = nullptr;

static constexpr s32 kFrameRateOptionValues[] = { 30, 60, 120, 0 };
static constexpr s32 kMsaaOptionValues[] = { 0, 2, 4, 8, 16 };
static constexpr s32 kKeyBindingActionCount = ACTION_OPEN_CLOSE_MENU;

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

static void BuildActionDisplayName(Action action, char* outText, s32 outTextLen) {
    if (!outText || outTextLen <= 0) {
        return;
    }

    outText[0] = '\0';

    const char* token = ActionToToken(action);
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
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feCtrl = AddPage(MenuPage_Controller, "FE_CTL", "Menu_Controller", MenuPage_Options, 0, false, -1, -1);
    SetEntries(feCtrl, {
        Toggle("FE_CSH", EntryBinding_Shock),
        Button("FE_BCK", EntryEvent_Back),
               });

    AddPage(MenuPage_KeyBindings, "FE_KBD", "Menu_Controller", MenuPage_Controller, 1, false,
            DEF_KEYBIND_WINDOW_W, DEF_KEYBIND_WINDOW_H);

    auto& feDisplay = AddPage(MenuPage_Display, "FE_DIS", "Menu_GameOption", MenuPage_Options, 1, false, -1, -1);
    SetEntries(feDisplay, {
        List("FE_RES", EntryBinding_DisplayResolution, 1, 0, 64),
        List("FE_FSC", EntryBinding_DisplayScreenMode, 1, 0, 2),
        Toggle("FE_VYS", EntryBinding_DisplayVsync),
        List("FE_FPS", EntryBinding_DisplayFrameRate, 1, 0, 3),
        List("FE_MSA", EntryBinding_DisplayMsaa, 1, 0, 4),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feSnd = AddPage(MenuPage_Sound, "FE_SND", "Menu_Sound", MenuPage_Options, 2, false, -1, -1);
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
}

void feCustomMenuMgr::Init(CustomText* textSystem) {
    m_text = textSystem;

    if (!m_menuArt) {
        m_menuArt = new xcSectionMan();
        if (!m_menuArt->LoadSection("XC/FE.1")) {
            LOG("[CustomMenu] Failed to load XC/FE.1 for menu art");
            delete m_menuArt;
            m_menuArt = nullptr;
        }
    }

    BuildPages();

    MenuColorStart(m_pulse);
    SetPage(MenuPage_None);
}

void feCustomMenuMgr::Shutdown() {
    m_cellTextures.clear();
    m_texReady = false;
    delete m_menuArt;
    m_menuArt = nullptr;
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
        // Key bindings page should respond only to keyboard + mouse.
        const bool nonMouseInput = g_actionInput->HadKeyboardInputThisFrame();

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
                (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_BACK)) ||
                (nonMouseInput && g_actionInput->JustPressed(ACTION_OPEN_CLOSE_MENU))) {
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
            const s32 labelX = panelX + DEF_LABEL_X_PAD;
            const s32 headerY = contentTop + DEF_CONTENT_PAD + DEF_TEXT_Y_OFF;
            const s32 firstRowY = headerY + DEF_KEYBIND_ROW_STEP;
            const s32 slotW = DEF_KEYBIND_SLOT_W;
            const s32 slotGap = DEF_KEYBIND_SLOT_GAP;
            const s32 slot2Right = panelX + page->frameW - DEF_VALUE_X_PAD;
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

            if (scroll != 0) {
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
                m_keyBindCaptureActive = true;
                m_keyBindCaptureBlockFrames = 1;
                PlaySound(FE_SND_MENU_5);
                return m_result;
            }

            if (rightClick) {
                PlaySound(FE_SND_MENU_5);
                GoBack();
                return m_result;
            }
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_UP)) {
            m_keyBindActionCursor--;
            if (m_keyBindActionCursor < 0) {
                m_keyBindActionCursor = kKeyBindingActionCount - 1;
            }
            clampKeyBindScroll();
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_DOWN)) {
            m_keyBindActionCursor++;
            if (m_keyBindActionCursor >= kKeyBindingActionCount) {
                m_keyBindActionCursor = 0;
            }
            clampKeyBindScroll();
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_LEFT)) {
            m_keyBindSlotCursor--;
            if (m_keyBindSlotCursor < 0) {
                m_keyBindSlotCursor = DEF_KEYBIND_SLOT_COUNT - 1;
            }
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_RIGHT)) {
            m_keyBindSlotCursor++;
            if (m_keyBindSlotCursor >= DEF_KEYBIND_SLOT_COUNT) {
                m_keyBindSlotCursor = 0;
            }
            PlaySound(FE_SND_MENU_7);
        }

        if (nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_CONFIRM)) {
            m_keyBindCaptureActive = true;
            m_keyBindCaptureBlockFrames = 1;
            PlaySound(FE_SND_MENU_5);
        }

        const s32 clearKey = g_actionInput->GetTriggeredKeyThisFrame();
        if ((nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_CLEAR)) ||
            clearKey == KEY_DELETE || clearKey == KEY_BACKSPACE) {
            const Action action = (Action)m_keyBindActionCursor;
            SetDesktopBindingCodeUnique(action, m_keyBindSlotCursor, 0);
            g_settings.Save(SETTINGS_PATH);
            PlaySound(FE_SND_MENU_5);
        }

        if ((nonMouseInput && g_actionInput->JustPressed(ACTION_MENU_BACK)) ||
            (nonMouseInput && g_actionInput->JustPressed(ACTION_OPEN_CLOSE_MENU))) {
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
        xcFont* hoverFont = FindFont("Beats_lo", "Beats_mid");
        if (hoverFont)
            hoverFont->SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));

        const s32 rowSpan = (page->numEntries > 0) ? ((page->numEntries - 1) * DEF_ROW_STEP) : 0;
        const s32 extraH = CalcPageExtraHeight(*page, hoverFont);
        const s32 entryBlockH = DEF_CONTENT_PAD + rowSpan + DEF_ROW_TEXT_H + extraH;
        const s32 bodyAvailH = page->frameH - DEF_TITLE_BAR_H - DEF_BOTTOM_BAR_H - DEF_CONTENT_TOP_PAD - DEF_CONTENT_BOTTOM_PAD;
        const s32 bodyCenterPad = (bodyAvailH > entryBlockH) ? ((bodyAvailH - entryBlockH) / 2) : 0;
        const s32 firstY = panelY + DEF_TITLE_BAR_H + DEF_CONTENT_TOP_PAD + bodyCenterPad + DEF_CONTENT_PAD;

        if (psxX >= (f32)panelX && psxX < (f32)(panelX + page->frameW)) {
            for (s32 i = 0; i < page->numEntries; i++) {
                const s32 rowTop = firstY + i * DEF_ROW_STEP + CalcEntryYExtra(*page, i, hoverFont);
                const s32 rowH = DEF_ROW_STEP + GetEntryExtraHeight(*page, page->entries[i], hoverFont);
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

    if (g_actionInput->JustPressed(ACTION_MENU_BACK) ||
        g_actionInput->JustPressed(ACTION_OPEN_CLOSE_MENU)) {
        // FE_SND_MENU_SPECIAL_4 triggers HandleCursorEvent(5) -> jcsFadeOutEngine(2),
        // which can leave audio faded out in this custom flow.
        // Use a non-fade back sound for submenu navigation.
        if (m_currPage != MenuPage::MenuPage_Frontend && m_currPage != MenuPage::MenuPage_Pause) {
            PlaySound(FE_SND_MENU_5);
        }
        GoBack();
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

    // Advance cursor past any leading Info entries.
    if (m_currPage != MenuPage_None) {
        const PageDef& pg = m_pages[m_currPage];
        while (m_cursor < pg.numEntries && pg.entries[m_cursor].type == EntryType_Info) {
            m_cursor++;
        }
    }
}

void feCustomMenuMgr::Activate(MenuPage startPage) {
    m_active = true;
    m_cursor = 0;

    if (g_display) {
        g_display->SetCursorCaptured(false);
        g_display->SetCursorVisible(false);
    }

    m_mouseInputActive = false;
    m_mousePosInitialized = false;
    if (startPage != MenuPage_Title) {
        rsEvent(RS_MUTE, 0, 0, 0);
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
            if (g_game)
                g_game->SetState(GameState::PlayMovieCredits);
            m_result = 4;
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
                    SetPage(MenuPage_SaveSlots);
                    m_cursor = savedSlot;
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
                    SetPage(MenuPage_DeleteSlots);
                    m_cursor = deletedSlot;
                }
                else {
                    PlaySound(FE_SND_MENU_7);
                }
            }
            else {
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
        Shock(v ? SHOCK_0 : SHOCK_CLEAR);
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
    def.frameW = (frameW == -1) ? DEF_WINDOW_W : frameW;
    def.frameH = frameH;
    def.numEntries = 0;

    m_pages[id] = def;
    return m_pages[id];
}

void feCustomMenuMgr::SetEntries(PageDef& page, std::initializer_list<Entry> list) {
    s32 n = 0;
    for (const Entry& e : list) {
        if (n >= MAX_ENTRIES_PER_MENU)
            break;
        page.entries[n++] = e;
    }
    page.numEntries = n;
    if (page.frameH == -1) {
        xcFont* bodyFont = FindFont("Beats_lo", "Beats_mid");
        if (bodyFont)
            bodyFont->SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));
        const s32 extraH = CalcPageExtraHeight(page, bodyFont);
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

s32 feCustomMenuMgr::GetEntryExtraHeight(const PageDef& page, const Entry& entry, xcFont* font) const {
    if (entry.type != EntryType_Info)
        return 0;

    if (!font)
        return DEF_INFO_ROW_EXTRA;

    const char* label = Localize(entry.token);
    if (!label)
        label = entry.token;

    const f32 wrapWidth = SCREEN_SCALE_X((f32)(page.frameW - DEF_LABEL_X_PAD * 2));
    s32 lines = font->CountWrappedLines(label, wrapWidth);
    if (lines < 1)
        lines = 1;

    return DEF_INFO_ROW_EXTRA + (lines - 1) * DEF_ROW_STEP;
}

s32 feCustomMenuMgr::CalcEntryYExtra(const PageDef& page, s32 upToIndex, xcFont* font) const {
    s32 extra = 0;
    for (s32 i = 0; i < upToIndex; i++) {
        extra += GetEntryExtraHeight(page, page.entries[i], font);
    }
    return extra;
}

s32 feCustomMenuMgr::CalcPageExtraHeight(const PageDef& page, xcFont* font) const {
    s32 extra = 0;
    for (s32 i = 0; i < page.numEntries; i++) {
        extra += GetEntryExtraHeight(page, page.entries[i], font);
    }
    return extra;
}

void feCustomMenuMgr::EnsureTextures() {
    if (m_texReady)
        return;

    m_cellTextures.clear();

    xcSection* sec = nullptr;
    //if (IsPausePage()) {
    //    sec = g_gameMenu ? g_gameMenu->GetSection() : nullptr;
    //    if (!sec || sec->numCells == 0) {
    //        sec = g_feMenuMgr ? g_feMenuMgr->GetSection() : nullptr;
    //    }
    //}
    //else {
    sec = g_feMenuMgr ? g_feMenuMgr->GetSection() : nullptr;
    if (!sec || sec->numCells == 0) {
        sec = g_gameMenu ? g_gameMenu->GetSection() : nullptr;
    }
    //}

    // Title screen: g_feMenuMgr not yet created, fall back to our own loaded copy
    if ((!sec || sec->numCells == 0) && m_menuArt) {
        sec = m_menuArt->section;
    }

    if (!sec || sec->numCells == 0 || !sec->images) {
        return; // Not ready yet — don't set m_texReady; retry next frame
    }

    m_texReady = true;

    const xcInventoryItem* imgItems = sec->images->GetItems();
    for (s32 i = 0; i < sec->numCells; i++) {
        xcCellImage* cell = sec->cells[i];
        if (!cell)
            continue;
        tTexture* tex = cell->GetTexture();
        if (!tex)
            continue;
        const u32 nameHash = imgItems ? imgItems[i].hash : 0u;
        m_cellTextures.push_back({ tex, (s16)cell->width, (s16)cell->height, nameHash });
    }
}

static void DrawGouraudRectPSX(s32 x, s32 y, s32 w, s32 h,
                               u8 topR, u8 topG, u8 topB,
                               u8 bottomR, u8 bottomG, u8 bottomB,
                               u8 alpha) {
    const f32 x0 = SCALE_AND_CENTER_X((f32)x);
    const f32 y0 = SCREEN_SCALE_Y((f32)y);
    const f32 x1 = SCALE_AND_CENTER_X((f32)(x + w));
    const f32 y1 = SCREEN_SCALE_Y((f32)(y + h));

    ScreenDraw::DrawGouraudQuad(
        x0, y0, topR, topG, topB, alpha,
        x1, y0, topR, topG, topB, alpha,
        x0, y1, bottomR, bottomG, bottomB, alpha,
        x1, y1, bottomR, bottomG, bottomB, alpha);

}

static f32 GetMenuBorderPx() {
    return SCREEN_SCALE_Y((f32)DEF_BORDER_W);
}

static void DrawRectPSX(s32 x, s32 y, s32 w, s32 h, u8 r, u8 g, u8 b, u8 a) {
    if (w <= 0 || h <= 0)
        return;

    ScreenDraw::DrawColoredRect(
        SCALE_AND_CENTER_X((f32)x),
        SCREEN_SCALE_Y((f32)y),
        SCREEN_SCALE_X((f32)w),
        SCREEN_SCALE_Y((f32)h),
        r, g, b, a);
}

static void BuildPsxSliderMeterString(s32 value, char* buf, s32 bufLen) {
    static constexpr s32 kSegments = DEF_SLIDER_CIRCLE_SEGMENTS;

    if (!buf || bufLen <= 0)
        return;

    if (value < 0) value = 0;
    if (value > 100) value = 100;

    s32 filled = (value * kSegments) / 100;
    if (filled < 0) filled = 0;
    if (filled > kSegments) filled = kSegments;

    s32 i = 0;
    for (; i < kSegments && i < (bufLen - 1); i++) {
        buf[i] = (i < filled) ? 'o' : 'f';
    }
    buf[i] = '\0';
}

struct PsxSliderMeterStyle {
    xcFont* font = nullptr;
    u32 color = 0xFF808080u;
};

static void DrawSliderCircleMeterPSX(const PsxSliderMeterStyle& style, s32 rightX, s32 textY, s32 value) {
    xcFont* meterFont = style.font;
    if (!meterFont)
        return;

    char meterText[DEF_SLIDER_CIRCLE_SEGMENTS + 1];
    BuildPsxSliderMeterString(value, meterText, (s32)sizeof(meterText));

    meterFont->SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));
    const f32 screenX = SCALE_AND_CENTER_X((f32)rightX);
    const f32 screenY = SCREEN_SCALE_Y((f32)textY);
    meterFont->DrawText(meterText, screenX + 1.0f, screenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
    meterFont->DrawText(meterText, screenX, screenY, style.color, XC_JUST_RIGHT);
}

static xcFont* ResolvePsxSliderMeterFontByHash(xcSectionMan* sectionMan, u32 fontHash) {
    if (sectionMan) {
        if (xcFont* font = sectionMan->FindFont(fontHash)) {
            return font;
        }
    }

    if (g_oxFontFile && g_oxFontFile->sectionMan && g_oxFontFile->sectionMan != sectionMan) {
        if (xcFont* font = g_oxFontFile->sectionMan->FindFont(fontHash)) {
            return font;
        }
    }

    return nullptr;
}

static PsxSliderMeterStyle ResolvePsxSliderMeterStyleFromSectionMan(xcSectionMan* sectionMan) {
    PsxSliderMeterStyle style;

    if (!sectionMan || !sectionMan->section)
        return style;

    xcSection* sec = sectionMan->section;
    if (!sec || !sec->overlays || !sec->rawData)
        return style;

    // PSX SNDSELECT item overlays used by FE/GAME sound menus.
    static constexpr u32 HASH_SOUND_ITEM_OVERLAYS[] = {
        0x1B5DD3F5u, // Sound_Effect
        0xB3DA1CE9u, // Sound_Music
        0xB47983DEu, // Sound_Voice
    };
    // PSX hdItemSelection value text object hash used by selection items.
    static constexpr u32 HASH_VALUE_TEXT = 0xC8FCCAE0u;

    for (u32 overlayHash : HASH_SOUND_ITEM_OVERLAYS) {
        xcOverlayData* soundOverlay = sec->FindOverlay(overlayHash);
        if (!soundOverlay)
            continue;

        u8* valueObj = soundOverlay->GetTextObj(HASH_VALUE_TEXT, sec->rawData);
        if (!valueObj)
            continue;

        xcTextPrim* valueText = reinterpret_cast<xcTextPrim*>(valueObj);
        xcFont* font = ResolvePsxSliderMeterFontByHash(sectionMan, valueText->fontHash);
        if (font) {
            style.font = font;
            style.color = valueText->GetColor();
            return style;
        }
    }

    const xcInventoryItem* items = sec->overlays->GetItems();
    for (u32 i = 0; i < sec->overlays->itemCount; i++) {
        xcOverlayData* overlay = reinterpret_cast<xcOverlayData*>(sec->rawData + items[i].dataOffset);
        if (!overlay)
            continue;

        u8* valueObj = overlay->GetTextObj(HASH_VALUE_TEXT, sec->rawData);
        if (!valueObj)
            continue;

        xcTextPrim* valueText = reinterpret_cast<xcTextPrim*>(valueObj);
        xcFont* font = ResolvePsxSliderMeterFontByHash(sectionMan, valueText->fontHash);
        if (font) {
            style.font = font;
            style.color = valueText->GetColor();
            return style;
        }
    }

    return style;
}

static PsxSliderMeterStyle ResolvePsxSliderMeterStyle(xcSectionMan* sectionMan) {
    return ResolvePsxSliderMeterStyleFromSectionMan(sectionMan);
}

static void DrawUniformBorderRectPSX(s32 x, s32 y, s32 w, s32 h, f32 borderPx,
                                     u8 r, u8 g, u8 b, u8 a) {
    if (w <= 0 || h <= 0 || borderPx <= 0.0f)
        return;

    const f32 x0 = SCALE_AND_CENTER_X((f32)x);
    const f32 y0 = SCREEN_SCALE_Y((f32)y);
    const f32 x1 = SCALE_AND_CENTER_X((f32)(x + w));
    const f32 y1 = SCREEN_SCALE_Y((f32)(y + h));
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

static void DrawUniformHLinePSX(s32 x, s32 y, s32 w, f32 linePx,
                                u8 r, u8 g, u8 b, u8 a) {
    if (w <= 0 || linePx <= 0.0f)
        return;

    const f32 x0 = SCALE_AND_CENTER_X((f32)x);
    const f32 x1 = SCALE_AND_CENTER_X((f32)(x + w));
    const f32 y0 = SCREEN_SCALE_Y((f32)y);
    const f32 drawW = x1 - x0;
    if (drawW <= 0.0f)
        return;

    ScreenDraw::DrawColoredRect(x0, y0, drawW, linePx, r, g, b, a);
}

static void DrawUniformVLinePSX(s32 x, s32 y, s32 h, f32 linePx,
                                u8 r, u8 g, u8 b, u8 a) {
    if (h <= 0 || linePx <= 0.0f)
        return;

    const f32 x0 = SCALE_AND_CENTER_X((f32)x);
    const f32 y0 = SCREEN_SCALE_Y((f32)y);
    const f32 y1 = SCREEN_SCALE_Y((f32)(y + h));
    const f32 drawH = y1 - y0;
    if (drawH <= 0.0f)
        return;

    ScreenDraw::DrawColoredRect(x0, y0, linePx, drawH, r, g, b, a);
}

static void DrawUniformBorderFillRectPSX(s32 x, s32 y, s32 w, s32 h, f32 borderPx,
                                         u8 borderR, u8 borderG, u8 borderB, u8 borderA,
                                         u8 fillR, u8 fillG, u8 fillB, u8 fillA) {
    if (w <= 0 || h <= 0)
        return;

    const f32 x0 = SCALE_AND_CENTER_X((f32)x);
    const f32 y0 = SCREEN_SCALE_Y((f32)y);
    const f32 x1 = SCALE_AND_CENTER_X((f32)(x + w));
    const f32 y1 = SCREEN_SCALE_Y((f32)(y + h));
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

static void DrawGouraudRectPSXVertical(s32 x, s32 y, s32 w, s32 h,
                                       u8 leftR, u8 leftG, u8 leftB,
                                       u8 rightR, u8 rightG, u8 rightB,
                                       u8 alpha) {
    const f32 x0 = SCALE_AND_CENTER_X((f32)x);
    const f32 y0 = SCREEN_SCALE_Y((f32)y);
    const f32 x1 = SCALE_AND_CENTER_X((f32)(x + w));
    const f32 y1 = SCREEN_SCALE_Y((f32)(y + h));

    ScreenDraw::DrawGouraudQuad(
        x0, y0, leftR, leftG, leftB, alpha,
        x1, y0, rightR, rightG, rightB, alpha,
        x0, y1, leftR, leftG, leftB, alpha,
        x1, y1, rightR, rightG, rightB, alpha);
}

static void DrawMenuOrnament(tTexture* symbolTex, s32 x, s32 y) {
    ScreenDraw::DrawQuad(
        symbolTex,
        SCALE_AND_CENTER_X((f32)x),
        SCREEN_SCALE_Y((f32)y),
        SCREEN_SCALE_X((f32)DEF_ORN_W),
        SCREEN_SCALE_Y((f32)DEF_ORN_H),
        0.0f, 0.0f, 1.0f, 1.0f,
        DEF_ORN_R, DEF_ORN_G, DEF_ORN_B, DEF_ORN_A);
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
    DrawRect(x + DEF_BORDER_W, bodyY0, w - DEF_BORDER_W * 2, bodyY1 - bodyY0, DEF_BODY_R, DEF_BODY_G, DEF_BODY_B, DEF_BODY_A);

    // Frame
    DrawUniformHLinePSX(x + DEF_BORDER_W, bodyY0, w - DEF_BORDER_W * 2, framePx, DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);
    DrawUniformHLinePSX(x + DEF_BORDER_W, bodyY1 - DEF_BORDER_W, w - DEF_BORDER_W * 2, framePx, DEF_FRAME_R, DEF_FRAME_G, DEF_FRAME_B, DEF_FRAME_A);

    // Black inset title box
    DrawUniformBorderFillRectPSX(x + titleInsetX, y + titleInsetY, titleInsetW, titleInsetH, framePx,
                                 DEF_TITLE_INSET_BORDER_R, DEF_TITLE_INSET_BORDER_G, DEF_TITLE_INSET_BORDER_B, DEF_TITLE_INSET_BORDER_A,
                                 DEF_TITLE_INSET_FILL_R, DEF_TITLE_INSET_FILL_G, DEF_TITLE_INSET_FILL_B, DEF_TITLE_INSET_FILL_A);

    // Decorative bar marks
    tTexture* ornamentTex = m_cellTextures.empty() ? nullptr : m_cellTextures[0].tex;
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
    xcFont* titleFont = FindFont("Beats_mid", "Beats_xl");
    if (title && titleFont) {
        titleFont->SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));
        const s32 titleTextY = y + titleInsetH / 2;
        const f32 titleX = SCALE_AND_CENTER_X((f32)DEF_WINDOW_CENTER_X);
        const f32 titleY = SCREEN_SCALE_Y((f32)titleTextY);
        const u32 titleColor = ((u32)DEF_TITLE_TEXT_A << 24) | ((u32)DEF_TITLE_TEXT_R << 16) | ((u32)DEF_TITLE_TEXT_G << 8) | (u32)DEF_TITLE_TEXT_B;
        titleFont->DrawText(title, titleX + 1.0f, titleY + 1.0f, (u32)(DEF_TITLE_SHADOW_A << 24), XC_JUST_CENTER);
        titleFont->DrawText(title, titleX, titleY, titleColor, XC_JUST_CENTER);
    }
}

void feCustomMenuMgr::Render() {
    if (!m_active)
        return;

    const PageDef* page = &m_pages[m_currPage];
    if (!page)
        return;

    MenuColorNext(m_pulse);

    const s32 panelX = DEF_WINDOW_CENTER_X - page->frameW / 2;
    const s32 panelY = DEF_WINDOW_CENTER_Y - page->frameH / 2;
    const s32 panelW = page->frameW;
    const s32 panelH = page->frameH;

    EnsureTextures();
    const char* title = Localize(page->titleToken);
    if (!title)
        title = page->titleToken;

    DrawMenuWindow(panelX, panelY, panelW, panelH, title);

    // Build normalColor directly (PSX scale: 128 = neutral/1.0 for the tint shader)
    const u32 normalColor = (0xFFu << 24) | ((u32)DEF_TEXT_NORM_B << 16) | ((u32)DEF_TEXT_NORM_G << 8) | (u32)DEF_TEXT_NORM_R;
    const u32 selectedColor = m_pulse.Get8();
    xcFont* bodyFont = FindFont("Beats_lo", "Beats_mid");
    if (!bodyFont) return;
    bodyFont->SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));

    PsxSliderMeterStyle sliderMeterStyle;
    if (g_feMenuMgr && g_feMenuMgr->sectionMan) {
        sliderMeterStyle = ResolvePsxSliderMeterStyle(g_feMenuMgr->sectionMan);
    }
    if (!sliderMeterStyle.font && g_gameMenu && g_gameMenu->sectionMan) {
        sliderMeterStyle = ResolvePsxSliderMeterStyle(g_gameMenu->sectionMan);
    }
    if (!sliderMeterStyle.font && m_menuArt) {
        sliderMeterStyle = ResolvePsxSliderMeterStyle(m_menuArt);
    }

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
    const s32 extraH = CalcPageExtraHeight(*page, bodyFont);
    const s32 entryBlockH = DEF_CONTENT_PAD + rowSpan + DEF_ROW_TEXT_H + extraH;
    const s32 bodyAvailH = panelH - DEF_TITLE_BAR_H - DEF_BOTTOM_BAR_H - DEF_CONTENT_TOP_PAD - DEF_CONTENT_BOTTOM_PAD;
    const s32 bodyCenterPad = (bodyAvailH > entryBlockH) ? ((bodyAvailH - entryBlockH) / 2) : 0;
    const s32 firstY = contentTop + bodyCenterPad + DEF_CONTENT_PAD;
    // Shift entry center left to the midpoint of the usable content area.
    const s32 contentCenterX = hasDragonPanel
        ? (panelX + DEF_BORDER_W + (dragonBoxX - DEF_BORDER_W - panelX) / 2)
        : DEF_WINDOW_CENTER_X;

    if (m_currPage == MenuPage_KeyBindings) {
        const s32 headerY = contentTop + DEF_CONTENT_PAD + DEF_TEXT_Y_OFF;
        const s32 firstRowY = headerY + DEF_KEYBIND_ROW_STEP;
        const s32 slotW = DEF_KEYBIND_SLOT_W;
        const s32 slotGap = DEF_KEYBIND_SLOT_GAP;
        const s32 slot2Right = panelX + panelW - DEF_VALUE_X_PAD;
        const s32 slot2Left = slot2Right - slotW;
        const s32 slot1Right = slot2Left - slotGap;
        const s32 slot1Left = slot1Right - slotW;
        const s32 visibleRows = (kKeyBindingActionCount - m_keyBindScrollTop < DEF_KEYBIND_VISIBLE_ROWS)
            ? (kKeyBindingActionCount - m_keyBindScrollTop)
            : DEF_KEYBIND_VISIBLE_ROWS;

        const s32 tableLeft = labelX - DEF_KEYBIND_TABLE_SIDE_PAD;
        const s32 tableRight = slot2Left + slotW + DEF_KEYBIND_TABLE_SIDE_PAD;
        const s32 tableW = tableRight - tableLeft;

        const f32 headerYScreen = SCREEN_SCALE_Y((f32)headerY);
        bodyFont->DrawText("Action", SCALE_AND_CENTER_X((f32)labelX), headerYScreen, normalColor, 0);
        bodyFont->DrawText("Bind 1", SCALE_AND_CENTER_X((f32)(slot1Left + slotW / 2)), headerYScreen, normalColor, XC_JUST_CENTER);
        bodyFont->DrawText("Bind 2", SCALE_AND_CENTER_X((f32)(slot2Left + slotW / 2)), headerYScreen, normalColor, XC_JUST_CENTER);

        for (s32 row = 0; row < visibleRows; row++) {
            const s32 actionIndex = m_keyBindScrollTop + row;
            const Action action = (Action)actionIndex;
            const bool selectedRow = (actionIndex == m_keyBindActionCursor);
            const s32 rowY = firstRowY + row * DEF_KEYBIND_ROW_STEP;
            const s32 rowTextY = rowY + DEF_TEXT_Y_OFF;

            if ((row & 1) == 0) {
                DrawRect(tableLeft, rowY - DEF_KEYBIND_ROW_TOP_PAD, tableW, DEF_KEYBIND_ROW_STEP,
                         DEF_KEYBIND_STRIPE_DARK_R, DEF_KEYBIND_STRIPE_DARK_G, DEF_KEYBIND_STRIPE_DARK_B, DEF_KEYBIND_STRIPE_DARK_A);
            }
            else {
                DrawRect(tableLeft, rowY - DEF_KEYBIND_ROW_TOP_PAD, tableW, DEF_KEYBIND_ROW_STEP,
                         DEF_KEYBIND_STRIPE_WARM_R, DEF_KEYBIND_STRIPE_WARM_G, DEF_KEYBIND_STRIPE_WARM_B, DEF_KEYBIND_STRIPE_WARM_A);
            }

            if (selectedRow) {
                const s32 cellLeft = (m_keyBindSlotCursor == 0) ? slot1Left : slot2Left;
                DrawRect(cellLeft - DEF_KEYBIND_CELL_PAD, rowY - DEF_KEYBIND_ROW_TOP_PAD,
                         slotW + DEF_KEYBIND_CELL_PAD * 2, DEF_KEYBIND_ROW_STEP,
                         DEF_KEYBIND_ACTIVE_FILL_R, DEF_KEYBIND_ACTIVE_FILL_G, DEF_KEYBIND_ACTIVE_FILL_B, DEF_KEYBIND_ACTIVE_FILL_A);
                DrawUniformBorderRectPSX(cellLeft - DEF_KEYBIND_CELL_PAD, rowY - DEF_KEYBIND_ROW_TOP_PAD,
                                         slotW + DEF_KEYBIND_CELL_PAD * 2, DEF_KEYBIND_ROW_STEP,
                                         GetMenuBorderPx(), m_pulse.GetRed8(), m_pulse.GetGreen8(), m_pulse.GetBlue8(), 255);
            }

            char actionName[64] = {};
            char slot0Label[32] = {};
            char slot1Label[32] = {};
            BuildActionDisplayName(action, actionName, (s32)sizeof(actionName));
            if (g_actionInput) {
                g_actionInput->GetDesktopBindingLabel(action, 0, slot0Label, (s32)sizeof(slot0Label));
                g_actionInput->GetDesktopBindingLabel(action, 1, slot1Label, (s32)sizeof(slot1Label));
            }

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
            const u32 actionColor = normalColor;
            const u32 slot0Color = selectedSlot0 ? selectedColor : normalColor;
            const u32 slot1Color = selectedSlot1 ? selectedColor : normalColor;
            const f32 rowScreenY = SCREEN_SCALE_Y((f32)rowTextY);
            bodyFont->DrawText(actionName, SCALE_AND_CENTER_X((f32)labelX), rowScreenY, actionColor, 0);
            bodyFont->DrawText(slot0Label, SCALE_AND_CENTER_X((f32)(slot1Left + slotW / 2)), rowScreenY, slot0Color, XC_JUST_CENTER);
            bodyFont->DrawText(slot1Label, SCALE_AND_CENTER_X((f32)(slot2Left + slotW / 2)), rowScreenY, slot1Color, XC_JUST_CENTER);
        }

        char scrollText[32] = {};
        snprintf(scrollText, (s32)sizeof(scrollText), "%d-%d/%d",
                 m_keyBindScrollTop + 1,
                 m_keyBindScrollTop + visibleRows,
                 kKeyBindingActionCount);
        bodyFont->DrawText(scrollText,
                           SCALE_AND_CENTER_X((f32)(panelX + panelW - DEF_VALUE_X_PAD)),
                           SCREEN_SCALE_Y((f32)(panelY + panelH - DEF_BOTTOM_BAR_H - DEF_CONTENT_BOTTOM_PAD - DEF_ROW_TEXT_H + DEF_TEXT_Y_OFF)),
                           normalColor,
                           XC_JUST_RIGHT);
    }
    else {
        for (s32 i = 0; i < page->numEntries; i++) {
            const Entry& item = page->entries[i];
            const bool selected = (i == m_cursor);
            const u32 color = selected ? selectedColor : normalColor;
            const s32 rowY = firstY + i * DEF_ROW_STEP + CalcEntryYExtra(*page, i, bodyFont) + DEF_TEXT_Y_OFF;

            const char* label = Localize(item.token);
            if (!label) label = item.token;

            const f32 rowScreenY = SCREEN_SCALE_Y((f32)rowY);
            const f32 labelScreenX = SCALE_AND_CENTER_X((f32)labelX);
            const f32 valueScreenX = SCALE_AND_CENTER_X((f32)valueX);
            const f32 centerScreenX = SCALE_AND_CENTER_X((f32)contentCenterX);

            if (item.type == EntryType_Info) {
                const u32 infoColor = (u32)((DEF_INFO_TEXT_A << 24) | (DEF_INFO_TEXT_B << 16) | (DEF_INFO_TEXT_G << 8) | DEF_INFO_TEXT_R);
                const f32 wrapWidth = SCREEN_SCALE_X((f32)(page->frameW - DEF_LABEL_X_PAD * 2));
                bodyFont->SetWrapX(wrapWidth);
                bodyFont->DrawText(label, centerScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_INFO_SHADOW_A << 24), XC_JUST_CENTER);
                bodyFont->DrawText(label, centerScreenX, rowScreenY, infoColor, XC_JUST_CENTER);
                bodyFont->SetWrapX(0.0f);
            }
            else if (item.type == EntryType_List && item.binding != EntryBinding_None) {
                const u32 sliderColor = color;

                bodyFont->DrawText(label, labelScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), 0);
                bodyFont->DrawText(label, labelScreenX, rowScreenY, sliderColor, 0);

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

                    bodyFont->DrawText(resText, valueScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
                    bodyFont->DrawText(resText, valueScreenX, rowScreenY, sliderColor, XC_JUST_RIGHT);
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

                    bodyFont->DrawText(modeText, valueScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
                    bodyFont->DrawText(modeText, valueScreenX, rowScreenY, color, XC_JUST_RIGHT);
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

                    bodyFont->DrawText(msaaText, valueScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
                    bodyFont->DrawText(msaaText, valueScreenX, rowScreenY, color, XC_JUST_RIGHT);
                }
                else if (item.binding == EntryBinding_DisplayFrameRate) {
                    const char* frameRateToken = GetFrameRateDisplayToken(GetBoundValue(item));
                    const char* frameRateText = frameRateToken ? Localize(frameRateToken) : nullptr;

                    if (!frameRateText) {
                        continue;
                    }

                    bodyFont->DrawText(frameRateText, valueScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
                    bodyFont->DrawText(frameRateText, valueScreenX, rowScreenY, color, XC_JUST_RIGHT);
                }
            }
            else if (item.type == EntryType_Slider && item.binding != EntryBinding_None) {
                bodyFont->DrawText(label, labelScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), 0);
                bodyFont->DrawText(label, labelScreenX, rowScreenY, color, 0);

                DrawSliderCircleMeterPSX(
                    sliderMeterStyle,
                    valueX + DEF_SLIDER_CIRCLE_X_OFF,
                    rowY,
                    GetBoundValue(item));
            }
            else if (item.type == EntryType_Toggle && item.binding != EntryBinding_None) {
                const s32 toggle = GetBoundValue(item);
                const char* toggleToken = toggle ? "FE_ON" : "FE_OFF";
                const char* toggleText = Localize(toggleToken);

                if (!toggleText) {
                    continue;
                }

                bodyFont->DrawText(label, labelScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), 0);
                bodyFont->DrawText(label, labelScreenX, rowScreenY, color, 0);
                bodyFont->DrawText(toggleText, valueScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
                bodyFont->DrawText(toggleText, valueScreenX, rowScreenY, color, XC_JUST_RIGHT);
            }
            else {
                if (IsSaveSlotPage(m_currPage) && i < SAVEGAME_SLOT_COUNT) {
                    char slotLabel[96] = {};
                    BuildSaveSlotLabel(i, slotLabel, (s32)sizeof(slotLabel));
                    bodyFont->DrawText(slotLabel, labelScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), 0);
                    bodyFont->DrawText(slotLabel, labelScreenX, rowScreenY, color, 0);
                }
                else {
                    bodyFont->DrawText(label, centerScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_CENTER);
                    bodyFont->DrawText(label, centerScreenX, rowScreenY, color, XC_JUST_CENTER);
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
        const u32 dragonColor = 0xFF808080u;

        // Gold_dr font: character '1' renders the gold dragon glyph
        xcFont* dragonFont = FindFont("Gold_dr", nullptr);
        if (dragonFont) {
            dragonFont->SetScale(SCREEN_SCALE_X(1.25f), SCREEN_SCALE_Y(1.25f));
            dragonFont->DrawText("1", SCALE_AND_CENTER_X((f32)dragonCenterX) + 1.0f, SCREEN_SCALE_Y((f32)(dragonIconY + DEF_TEXT_Y_OFF)) + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_CENTER);
            dragonFont->DrawText("1", SCALE_AND_CENTER_X((f32)dragonCenterX), SCREEN_SCALE_Y((f32)(dragonIconY + DEF_TEXT_Y_OFF)), dragonColor, XC_JUST_CENTER);
        }

        s32 totalGold = g_scoreManager ? g_scoreManager->GetTotalGoldDragon() : 0;
        if (totalGold > 99) totalGold = 99;
        char dragonCountStr[8];
        sprintf_s(dragonCountStr, "%d", totalGold);

        bodyFont->SetScale(SCREEN_SCALE_X(1.2f), SCREEN_SCALE_Y(1.2f));
        bodyFont->DrawText(dragonCountStr, SCALE_AND_CENTER_X((f32)dragonCenterX) + 1.0f, SCREEN_SCALE_Y((f32)(dragonCountY + DEF_TEXT_Y_OFF)) + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_CENTER);
        bodyFont->DrawText(dragonCountStr, SCALE_AND_CENTER_X((f32)dragonCenterX), SCREEN_SCALE_Y((f32)(dragonCountY + DEF_TEXT_Y_OFF)), dragonColor, XC_JUST_CENTER);
    }

    // Help prompts in the bottom bar
    xcFont* helpFont = FindFont("Beats_lo", "Beats_mid");
    if (helpFont && m_currPage != MenuPage_Quitting) {
        f32 helpScale = 0.8f;
        f32 promptGap = DEF_HELP_GROUP_GAP_PX;
        if (m_currPage == MenuPage_KeyBindings) {
            helpScale = DEF_KEYBIND_HELP_SCALE;
            promptGap = DEF_KEYBIND_HELP_GAP_PX;
        }
        helpFont->SetScale(SCREEN_SCALE_X(helpScale), SCREEN_SCALE_Y(helpScale));
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

        const u32 helpColor = (u32)((DEF_HELP_TEXT_A << 24) | (DEF_HELP_TEXT_R << 16) | (DEF_HELP_TEXT_G << 8) | DEF_HELP_TEXT_B);
        const s32 bottomBarY = panelY + panelH - DEF_BOTTOM_BAR_H;
        const f32 helpY = SCREEN_SCALE_Y((f32)(bottomBarY + DEF_HELP_Y_PAD));
        const f32 centerScreenX = SCALE_AND_CENTER_X((f32)DEF_WINDOW_CENTER_X);

        f32 totalWidth = 0.0f;
        for (s32 i = 0; i < promptCount; i++) {
            totalWidth += helpFont->MeasureText(prompts[i]);
            if (i + 1 < promptCount) {
                totalWidth += promptGap;
            }
        }

        f32 cursorX = centerScreenX - totalWidth * 0.5f;
        for (s32 i = 0; i < promptCount; i++) {
            helpFont->DrawText(prompts[i], cursorX, helpY, helpColor, 0);
            cursorX += helpFont->MeasureText(prompts[i]);

            if (i + 1 < promptCount) {
                cursorX += promptGap;
            }
        }
    }
}

void feCustomMenuMgr::DrawRect(s32 x, s32 y, s32 w, s32 h, u8 r, u8 g, u8 b, u8 a) const {
    const f32 nx = SCALE_AND_CENTER_X((f32)x);
    const f32 ny = SCREEN_SCALE_Y((f32)y);
    const f32 nw = SCREEN_SCALE_X((f32)w);
    const f32 nh = SCREEN_SCALE_Y((f32)h);
    ScreenDraw::DrawColoredRect(nx, ny, nw, nh, r, g, b, a);
}

void feCustomMenuMgr::DrawHighlight(s32 x, s32 y, s32 w, s32 h) const {
    DrawRect(x, y, w, h,
             m_pulse.GetRed8(), m_pulse.GetGreen8(), m_pulse.GetBlue8(), 55);
}

xcFont* feCustomMenuMgr::FindFont(const char* first, const char* second) const {
    if (!g_oxFontFile)
        return nullptr;

    xcFont* f = g_oxFontFile->FindFont(first);

    if (!f && second)
        f = g_oxFontFile->FindFont(second);

    if (!f)
        f = g_oxFontFile->FindFont("Red_dr");

    if (!f)
        f = g_oxFontFile->FindFont("Gold_dr");
    return f;
}

void feCustomMenuMgr::BuildMeter(s32 value, char* buf) {
    if (value < 0) value = 0;
    if (value > 100) value = 100;

    const s32 filled = (value + 9) / 10;
    for (s32 i = 0; i < 10; i++) buf[i] = (i < filled) ? 'o' : 'f';

    buf[10] = '\0';
}

const char* feCustomMenuMgr::Localize(const char* token) const {
    if (!m_text || !token)
        return nullptr;

    return m_text->GetString(token);
}
