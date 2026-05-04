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
#include "p3d/texture.h"
#include <cmath>
#include <vector>

feCustomMenuMgr* g_feCustomMenuMgr = nullptr;

void feCustomMenuMgr::BuildPages() {
    auto& feTitle = AddPage(MenuPage_Title, "FE_TTL", "Menu_Title", MenuPage_None, 0, false, -1, -1);
    SetEntries(feTitle, {
        Button("FE_NWG", EntryEvent_NewGame),
        Button("FE_LDG", EntryEvent_Load),
        Button("FE_OPT", EntryEvent_GoPage, MenuPage_Options),
        Button("FE_XTG", EntryEvent_GoPage, MenuPage_QuitConfirm),
               });

    auto& feMain = AddPage(MenuPage_Frontend, "FE_MNM", "Menu_Title", MenuPage_None, 0, false, -1, -1);
    SetEntries(feMain, {
        Button("FE_RSM", EntryEvent_Resume),
        Button("FE_QTG", EntryEvent_GoPage, MenuPage_QuitConfirm),
        Button("FE_LDG", EntryEvent_Load),
        Button("FE_SVG", EntryEvent_Save),
        Button("FE_OPT", EntryEvent_GoPage, MenuPage_Options),
               });

    auto& feOpts = AddPage(MenuPage_Options, "FE_OPT", "Menu_GameOption", MenuPage_Frontend, 4, false, -1, -1);
    SetEntries(feOpts, {
        Button("FE_CTL", EntryEvent_GoPage, MenuPage_Controller),
        Button("FE_DIS", EntryEvent_GoPage, MenuPage_Display),
        Button("FE_SND", EntryEvent_GoPage, MenuPage_Sound),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feCtrl = AddPage(MenuPage_Controller, "FE_CTL", "Menu_Controller", MenuPage_Options, 0, false, -1, -1);
    SetEntries(feCtrl, {
        Toggle("FE_CSH", EntryBinding_Shock),
        Button("FE_BCK", EntryEvent_Back),
               });

    auto& feDisplay = AddPage(MenuPage_Display, "FE_DIS", "Menu_GameOption", MenuPage_Options, 1, false, -1, -1);
    SetEntries(feDisplay, {
        Slider("FE_RES", EntryBinding_DisplayResolution, 1, 0, 64),
        Slider("FE_FSC", EntryBinding_DisplayScreenMode, 1, 0, 2),
        Toggle("FE_VYS", EntryBinding_DisplayVsync),
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

    auto& feNewGame = AddPage(MenuPage_NewGameConfirm, "FE_NGQ", "Menu_Confirmation", MenuPage_Frontend, 0, false, -1, -1);
    SetEntries(feNewGame, {
        Button("FE_YS", EntryEvent_NewGame),
        Button("FE_NO", EntryEvent_Back),
               });

    auto& feExitLevel = AddPage(MenuPage_ExitLevelConfirm, "FE_EXL", "Menu_Confirmation", MenuPage_Pause, 2, true, -1, -1);
    SetEntries(feExitLevel, {
        Info("FE_EXLR"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_ExitToHub),
               });

    auto& feQuit = AddPage(MenuPage_QuitConfirm, "FE_XGQ", "Menu_Confirmation", MenuPage_None, 0, false, -1, -1);
    SetEntries(feQuit, {
        Info("FE_XGM"),
        Button("FE_NO", EntryEvent_Back),
        Button("FE_YS", EntryEvent_QuitGame),
               });

    auto& feQuitting = AddPage(MenuPage_Quitting, "FE_XGQ", "Menu_Confirmation", MenuPage_None, 0, false, -1, -1);
    SetEntries(feQuitting, {
        Info("FE_QUI"),
               });

    auto& pauseMain = AddPage(MenuPage_Pause, "FE_PSD", "Menu_GameOption", MenuPage_None, 0, true, -1, 120);
    SetEntries(pauseMain, {
        Button("FE_RSG", EntryEvent_Resume),
        Button("FE_OPT", EntryEvent_GoPage, MenuPage_Options),
        Button("FE_EXL", EntryEvent_GoPage, MenuPage_ExitLevelConfirm),
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
                        // Discard staged resolution when leaving that row via mouse.
                        const Entry& prev = page->entries[m_cursor];
                        if (prev.binding == EntryBinding_DisplayResolution) {
                            m_pendingResolutionActive = false;
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
        Adjust(-1);  // PSX: no sound for LEFT
    }
    if (g_actionInput->JustPressed(ACTION_MENU_RIGHT)) {
        Adjust(1);   // PSX: no sound for RIGHT
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

    // Leaving the resolution row without confirming discards staged value.
    if (prevCursor != m_cursor) {
        const Entry& prev = m_pages[m_currPage].entries[prevCursor];
        if (prev.binding == EntryBinding_DisplayResolution) {
            m_pendingResolutionActive = false;
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

    if (e->type == EntryType_Slider) {
        if (e->binding == EntryBinding_DisplayResolution && m_pendingResolutionActive) {
            ApplyValue(*e, m_pendingResolutionIndex);
            m_pendingResolutionActive = false;
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
            // Title-page New Game should use title loop's fade->OpenFE flow.
            if (m_currPage != MenuPage_Title && g_game)
                g_game->SetState(GameState::Init);
            m_result = 4;
            break;
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
            break;
        case EntryEvent_Save:
            break;
    }
}

void feCustomMenuMgr::GoBack() {
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
    if (e->binding == EntryBinding_None || dir == 0) return;

    if (e->type == EntryType_Toggle) {
        const s32 v = GetBoundValue(*e) ? 0 : 1;
        ApplyValue(*e, v);
    }
    else if (e->type == EntryType_Slider) {
        if (e->binding == EntryBinding_DisplayResolution) {
            const s32 current = m_pendingResolutionActive ? m_pendingResolutionIndex : GetBoundValue(*e);
            s32 v = current + dir * e->step;
            s32 maxIndex = 0;
            if (g_display) {
                const s32 count = g_display->GetResolutionCount();
                maxIndex = (count > 0) ? (count - 1) : 0;
            }
            if (v < 0) v = 0;
            if (v > maxIndex) v = maxIndex;
            m_pendingResolutionIndex = v;
            m_pendingResolutionActive = true;
            return;
        }

        if (e->binding == EntryBinding_MusicVol ||
            e->binding == EntryBinding_EffectsVol ||
            e->binding == EntryBinding_DialogVol) {
            static constexpr s32 kSegments = DEF_SLIDER_CIRCLE_SEGMENTS;
            s32 current = GetBoundValue(*e);
            if (current < 0) current = 0;
            if (current > 100) current = 100;

            s32 seg = (current * kSegments) / 100;
            seg += (dir > 0) ? 1 : -1;
            if (seg < 0) seg = 0;
            if (seg > kSegments) seg = kSegments;

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

        s32 v = GetBoundValue(*e) + dir * e->step;
        if (v < e->lo) v = e->lo;
        if (v > e->hi) v = e->hi;
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
    else if (e.binding == EntryBinding_DisplayResolution) {
        if (g_display) g_display->SetResolutionIndex(v);
    }

    g_settings.Save(SETTINGS_PATH);
}

void feCustomMenuMgr::PlaySound(s32 id) const {
    if (g_frontEndSound)
        g_frontEndSound->ProcessSoundEvent(id);
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

static void DrawCircleDotPSX(s32 cx, s32 cy, s32 radius,
                             u8 outlineR, u8 outlineG, u8 outlineB, u8 outlineA,
                             u8 fillR, u8 fillG, u8 fillB, u8 fillA) {
    if (radius <= 0)
        return;

    static tTexture* s_dotFilled = nullptr;
    static tTexture* s_dotEmpty = nullptr;
    static s32 s_cachedRadiusPx = -1;
    static s32 s_cachedOuterPx = -1;
    static s32 s_cachedRingPx = -1;
    static s32 s_cachedInnerPx = -1;
    static s32 s_cachedCrossPx = -1;
    static u32 s_cachedColorKey = 0;

    const auto packRgba = [](u8 r, u8 g, u8 b, u8 a) -> u32 {
        return ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
    };
    const auto psxToTexChan = [](u8 v) -> u8 {
        u32 scaled = ((u32)v * 255u + 64u) / 128u;
        return (scaled > 255u) ? 255u : (u8)scaled;
    };

    const u8 ringTexR = psxToTexChan(DEF_SLIDER_CIRCLE_RING_R);
    const u8 ringTexG = psxToTexChan(DEF_SLIDER_CIRCLE_RING_G);
    const u8 ringTexB = psxToTexChan(DEF_SLIDER_CIRCLE_RING_B);
    const u8 fillTexR = psxToTexChan(fillR);
    const u8 fillTexG = psxToTexChan(fillG);
    const u8 fillTexB = psxToTexChan(fillB);

    const f32 scaleY = SCREEN_SCALE_Y(1.0f);
    s32 rasterDiv = DEF_SLIDER_CIRCLE_RASTER_DIV;
    if (rasterDiv < 1) rasterDiv = 1;
    if (rasterDiv > 4) rasterDiv = 4;
    const f32 rasterScaleY = scaleY / (f32)rasterDiv;

    s32 radiusPx = (s32)std::round((f32)radius * rasterScaleY);
    if (radiusPx < 1)
        radiusPx = 1;

    s32 outerPx = (s32)std::round((f32)DEF_SLIDER_CIRCLE_OUTER_OUTLINE_THICKNESS * rasterScaleY);
    s32 ringPx = (s32)std::round((f32)DEF_SLIDER_CIRCLE_RING_THICKNESS * rasterScaleY);
    s32 innerPx = (s32)std::round((f32)DEF_SLIDER_CIRCLE_INNER_OUTLINE_THICKNESS * rasterScaleY);
    s32 crossPx = (s32)std::round((f32)DEF_SLIDER_CIRCLE_CROSS_HALF * rasterScaleY);

    if (outerPx < 0) outerPx = 0;
    if (ringPx < 1) ringPx = 1;
    if (innerPx < 0) innerPx = 0;
    if (crossPx < 0) crossPx = 0;

    if (outerPx > radiusPx) outerPx = radiusPx;
    if (outerPx + ringPx > radiusPx) {
        ringPx = radiusPx - outerPx;
        if (ringPx < 1) {
            ringPx = 1;
            if (outerPx > radiusPx - 1) outerPx = radiusPx - 1;
        }
    }
    if (outerPx + ringPx + innerPx > radiusPx - 1) {
        innerPx = (radiusPx - 1) - (outerPx + ringPx);
        if (innerPx < 0) innerPx = 0;
    }

    const u32 colorKey =
        packRgba(outlineR, outlineG, outlineB, outlineA) ^
        (packRgba(ringTexR, ringTexG, ringTexB, DEF_SLIDER_CIRCLE_RING_A) * 33u) ^
        (packRgba(DEF_SLIDER_CIRCLE_INNER_OUTLINE_R, DEF_SLIDER_CIRCLE_INNER_OUTLINE_G, DEF_SLIDER_CIRCLE_INNER_OUTLINE_B, DEF_SLIDER_CIRCLE_INNER_OUTLINE_A) * 97u) ^
        (packRgba(fillTexR, fillTexG, fillTexB, fillA) * 131u) ^
        (packRgba(DEF_SLIDER_CIRCLE_CROSS_R, DEF_SLIDER_CIRCLE_CROSS_G, DEF_SLIDER_CIRCLE_CROSS_B, DEF_SLIDER_CIRCLE_CROSS_A) * 193u);

    const bool needRebuild =
        !s_dotFilled || !s_dotEmpty ||
        s_cachedRadiusPx != radiusPx ||
        s_cachedOuterPx != outerPx ||
        s_cachedRingPx != ringPx ||
        s_cachedInnerPx != innerPx ||
        s_cachedCrossPx != crossPx ||
        s_cachedColorKey != colorKey;

    if (needRebuild) {
        if (s_dotFilled) { s_dotFilled->Release(); s_dotFilled = nullptr; }
        if (s_dotEmpty) { s_dotEmpty->Release(); s_dotEmpty = nullptr; }

        s32 pad = DEF_SLIDER_CIRCLE_TEXTURE_PAD;
        if (pad < 0) pad = 0;
        const s32 size = radiusPx * 2 + pad * 2 + 1;
        const f32 cxp = (f32)(size - 1) * 0.5f;
        const f32 cyp = (f32)(size - 1) * 0.5f;

        const f32 outerR = (f32)radiusPx;
        const f32 ringOuterR = outerR - (f32)outerPx;
        const f32 innerOutlineOuterR = ringOuterR - (f32)ringPx;
        const f32 holeR = innerOutlineOuterR - (f32)innerPx;

        const f32 outerR2 = outerR * outerR;
        const f32 ringOuterR2 = ringOuterR * ringOuterR;
        const f32 innerOutlineOuterR2 = innerOutlineOuterR * innerOutlineOuterR;
        const f32 holeR2 = holeR * holeR;

        auto buildDotPixels = [&](bool filledCenter) {
            s32 ss = DEF_SLIDER_CIRCLE_SUPERSAMPLE;
            if (ss < 1) ss = 1;
            if (ss > 2) ss = 2;

            std::vector<u32> pixels((size_t)size * (size_t)size, 0u);
            for (s32 y = 0; y < size; y++) {
                for (s32 x = 0; x < size; x++) {
                    u32 accR = 0, accG = 0, accB = 0, accA = 0;
                    for (s32 sy = 0; sy < ss; sy++) {
                        for (s32 sx = 0; sx < ss; sx++) {
                            const f32 invSs = 1.0f / (f32)ss;
                            const f32 sampleOffX = ((f32)sx + 0.5f) * invSs;
                            const f32 sampleOffY = ((f32)sy + 0.5f) * invSs;
                            const f32 px = (f32)x + sampleOffX;
                            const f32 py = (f32)y + sampleOffY;
                            const f32 dx = px - cxp;
                            const f32 dy = py - cyp;
                            const f32 d2 = dx * dx + dy * dy;

                            u8 sr = 0, sg = 0, sb = 0, sa = 0;
                            if (d2 <= outerR2) {
                                if (d2 > ringOuterR2) {
                                    sr = outlineR; sg = outlineG; sb = outlineB; sa = outlineA;
                                }
                                else if (d2 > innerOutlineOuterR2) {
                                    sr = ringTexR; sg = ringTexG; sb = ringTexB; sa = DEF_SLIDER_CIRCLE_RING_A;
                                }
                                else if (d2 > holeR2) {
                                    sr = DEF_SLIDER_CIRCLE_INNER_OUTLINE_R; sg = DEF_SLIDER_CIRCLE_INNER_OUTLINE_G; sb = DEF_SLIDER_CIRCLE_INNER_OUTLINE_B; sa = DEF_SLIDER_CIRCLE_INNER_OUTLINE_A;
                                }
                                else if (filledCenter) {
                                    sr = fillTexR; sg = fillTexG; sb = fillTexB; sa = fillA;
                                }
                            }

                            // Optional center cross over hole/fill.
                            if (crossPx > 0) {
                                const s32 ix = (s32)std::floor(px + 0.5f);
                                const s32 iy = (s32)std::floor(py + 0.5f);
                                const s32 cxi = (s32)std::floor(cxp + 0.5f);
                                const s32 cyi = (s32)std::floor(cyp + 0.5f);
                                if ((iy == cyi && std::abs(ix - cxi) <= crossPx) ||
                                    (ix == cxi && std::abs(iy - cyi) <= crossPx)) {
                                    sr = DEF_SLIDER_CIRCLE_CROSS_R;
                                    sg = DEF_SLIDER_CIRCLE_CROSS_G;
                                    sb = DEF_SLIDER_CIRCLE_CROSS_B;
                                    sa = DEF_SLIDER_CIRCLE_CROSS_A;
                                }
                            }

                            accR += sr;
                            accG += sg;
                            accB += sb;
                            accA += sa;
                        }
                    }

                    const u32 sampleCount = (u32)(ss * ss);
                    const u8 r = (u8)(accR / sampleCount);
                    const u8 g = (u8)(accG / sampleCount);
                    const u8 b = (u8)(accB / sampleCount);
                    const u8 a = (u8)(accA / sampleCount);
                    pixels[(size_t)y * (size_t)size + (size_t)x] = packRgba(r, g, b, a);
                }
            }
            return pixels;
        };

        std::vector<u32> filledPixels = buildDotPixels(true);
        std::vector<u32> emptyPixels = buildDotPixels(false);

        s_dotFilled = new tTexture();
        s_dotFilled->Create(size, size, 32, 8, filledPixels.data());
        s_dotEmpty = new tTexture();
        s_dotEmpty->Create(size, size, 32, 8, emptyPixels.data());

        s_cachedRadiusPx = radiusPx;
        s_cachedOuterPx = outerPx;
        s_cachedRingPx = ringPx;
        s_cachedInnerPx = innerPx;
        s_cachedCrossPx = crossPx;
        s_cachedColorKey = colorKey;
    }

    tTexture* dotTex = (fillA > 0) ? s_dotFilled : s_dotEmpty;
    if (!dotTex)
        return;

    const f32 cxScreen = SCALE_AND_CENTER_X((f32)cx);
    const f32 cyScreen = SCREEN_SCALE_Y((f32)cy);
    const s32 texW = dotTex->GetWidth();
    const s32 texH = dotTex->GetHeight();
    const f32 drawW = (f32)texW * (f32)rasterDiv;
    const f32 drawH = (f32)texH * (f32)rasterDiv;
    const f32 drawX = std::floor(cxScreen - drawW * 0.5f + 0.5f);
    const f32 drawY = std::floor(cyScreen - drawH * 0.5f + 0.5f);
    ScreenDraw::DrawQuad(dotTex, drawX, drawY, drawW, drawH,
                         0.0f, 0.0f, 1.0f, 1.0f,
                         128, 128, 128, 255);
}

static void DrawSliderCircleMeterPSX(s32 rightX, s32 centerY, s32 value,
                                     u8 fillR, u8 fillG, u8 fillB, u8 fillA) {
    static constexpr s32 kSegments = DEF_SLIDER_CIRCLE_SEGMENTS;
    static constexpr s32 kRadius = DEF_SLIDER_CIRCLE_RADIUS;
    static constexpr s32 kStep = DEF_SLIDER_CIRCLE_STEP;
    static constexpr u8 kOutlineR = DEF_SLIDER_CIRCLE_OUTER_OUTLINE_R;
    static constexpr u8 kOutlineG = DEF_SLIDER_CIRCLE_OUTER_OUTLINE_G;
    static constexpr u8 kOutlineB = DEF_SLIDER_CIRCLE_OUTER_OUTLINE_B;
    static constexpr u8 kOutlineA = DEF_SLIDER_CIRCLE_OUTER_OUTLINE_A;
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    // Strict 0..100 to 0..6 mapping: each step spans 100/6.
    s32 filled = (value * kSegments) / 100;
    if (filled < 0) filled = 0;
    if (filled > kSegments) filled = kSegments;

    const s32 diam = kRadius * 2 + 1;
    const s32 totalW = (kSegments - 1) * kStep + diam;
    const s32 startX = rightX - totalW + 1;

    for (s32 i = 0; i < kSegments; i++) {
        const s32 cx = startX + i * kStep + kRadius;
        const bool isFilled = (i < filled);
        // Empty segments keep a hollow center; filled segments fill the center.
        const u8 dotA = isFilled ? fillA : 0;
        DrawCircleDotPSX(cx, centerY, kRadius,
                         kOutlineR, kOutlineG, kOutlineB, kOutlineA,
                         fillR, fillG, fillB, dotA);
    }
}

static xcFont* ResolvePsxSliderMeterFontFromSectionMan(xcSectionMan* sectionMan) {
    if (!sectionMan || !sectionMan->section)
        return nullptr;

    xcSection* sec = sectionMan->section;
    if (!sec || !sec->overlays || !sec->rawData)
        return nullptr;

    // FE/GAME sound menu hash and hdItemSelection value text hash from PSX path.
    static constexpr u32 HASH_SOUND_MENU = 0x061CD029u;
    // PSX hdItemSelection value text object hash used by hdSndItemSelection.
    static constexpr u32 HASH_VALUE_TEXT = (u32)(-922957088);

    // Prefer the exact sound menu overlay first.
    if (xcOverlayData* soundOverlay = sec->FindOverlay(HASH_SOUND_MENU)) {
        u8* valueObj = soundOverlay->GetTextObj(HASH_VALUE_TEXT, sec->rawData);
        if (valueObj) {
            xcTextPrim* valueText = reinterpret_cast<xcTextPrim*>(valueObj);
            xcFont* font = sectionMan->FindFont(valueText->fontHash);
            if (font)
                return font;
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
        xcFont* font = sectionMan->FindFont(valueText->fontHash);
        if (font)
            return font;
    }

    return nullptr;
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
        const s32 leftX = x + DEF_BOTTOM_ORN_LEFT_X + i * DEF_BOTTOM_ORN_STEP;
        DrawMenuOrnament(ornamentTex, leftX, bottomY0 + DEF_BOTTOM_ORN_Y_OFF);
    }
    for (s32 i = 0; i < DEF_BOTTOM_ORN_COUNT; i++) {
        const s32 rightX = x + w - DEF_BOTTOM_ORN_RIGHT_X + i * DEF_BOTTOM_ORN_STEP;
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
        else if (item.type == EntryType_Slider && item.binding != EntryBinding_None) {
            bodyFont->DrawText(label, labelScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), 0);
            bodyFont->DrawText(label, labelScreenX, rowScreenY, color, 0);

            if (item.binding == EntryBinding_DisplayResolution) {
                s32 idx = GetBoundValue(item);
                if (selected && m_pendingResolutionActive) {
                    idx = m_pendingResolutionIndex;
                }

                char resText[32];
                strcpy_s(resText, "AUTO");
                if (g_display) {
                    pddiVideoMode mode;
                    if (g_display->GetResolutionMode(idx, mode)) {
                        sprintf_s(resText, "%dx%d", mode.width, mode.height);
                    }
                }

                bodyFont->DrawText(resText, valueScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
                bodyFont->DrawText(resText, valueScreenX, rowScreenY, color, XC_JUST_RIGHT);
            }
            else if (item.binding == EntryBinding_DisplayScreenMode) {
                const s32 mode = GetBoundValue(item);
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
            else {
                DrawSliderCircleMeterPSX(
                    valueX + DEF_SLIDER_CIRCLE_X_OFF,
                    rowY + DEF_SLIDER_CIRCLE_Y_OFF,
                    GetBoundValue(item),
                    DEF_SLIDER_CIRCLE_FILL_R,
                    DEF_SLIDER_CIRCLE_FILL_G,
                    DEF_SLIDER_CIRCLE_FILL_B,
                    DEF_SLIDER_CIRCLE_FILL_A);
            }
        }
        else if (item.type == EntryType_Toggle && item.binding != EntryBinding_None) {
            const s32 toggle = GetBoundValue(item);
            bodyFont->DrawText(label, labelScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), 0);
            bodyFont->DrawText(label, labelScreenX, rowScreenY, color, 0);
            bodyFont->DrawText(toggle ? "ON" : "OFF", valueScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_RIGHT);
            bodyFont->DrawText(toggle ? "ON" : "OFF", valueScreenX, rowScreenY, color, XC_JUST_RIGHT);
        }
        else {
            bodyFont->DrawText(label, centerScreenX + 1.0f, rowScreenY + 1.0f, (u32)(DEF_TEXT_SHADOW_A << 24), XC_JUST_CENTER);
            bodyFont->DrawText(label, centerScreenX, rowScreenY, color, XC_JUST_CENTER);
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

    // Help text in the bottom bar
    xcFont* helpFont = FindFont("Beats_lo", "Beats_mid");
    if (helpFont) {
        helpFont->SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));
        const s32 bottomBarY = panelY + panelH - DEF_BOTTOM_BAR_H;
        const s32 helpY = bottomBarY + DEF_HELP_Y_PAD;
        helpFont->DrawText("X Select", SCALE_AND_CENTER_X((f32)(DEF_WINDOW_CENTER_X + DEF_HELP_LEFT_X_OFF)), SCREEN_SCALE_Y((f32)helpY), (u32)((DEF_HELP_TEXT_A << 24) | (DEF_HELP_TEXT_R << 16) | (DEF_HELP_TEXT_G << 8) | DEF_HELP_TEXT_B), XC_JUST_CENTER);
        helpFont->DrawText("T Back", SCALE_AND_CENTER_X((f32)(DEF_WINDOW_CENTER_X + DEF_HELP_RIGHT_X_OFF)), SCREEN_SCALE_Y((f32)helpY), (u32)((DEF_HELP_TEXT_A << 24) | (DEF_HELP_TEXT_R << 16) | (DEF_HELP_TEXT_G << 8) | DEF_HELP_TEXT_B), XC_JUST_CENTER);
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
