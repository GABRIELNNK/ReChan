#pragma once
#include "core.h"
#include "gen/savegame.h"
#include "extra/customtext.h"
#include "xclib/xccolour.h"
#include <initializer_list>
#include <vector>

class xcFont;
class tTexture;
struct xcSectionMan;

enum MenuPage : s32 {
    MenuPage_None,
    MenuPage_Frontend, 
    MenuPage_Pause,
    MenuPage_Title,
    MenuPage_StartGame,
    MenuPage_Options,
    MenuPage_Controls,
    MenuPage_Controller,
    MenuPage_KeyBindings,
    MenuPage_Display,
    MenuPage_Sound,
    MenuPage_LoadSlots,
    MenuPage_SaveSlots,
    MenuPage_DeleteSlots,
    MenuPage_LoadConfirm,
    MenuPage_SaveConfirm,
    MenuPage_DeleteConfirm,
    MenuPage_NewGameConfirm,
    MenuPage_ExitLevelConfirm,
    MenuPage_QuitConfirm,
    MenuPage_Quitting,
    MenuPage_Location,
    MenuPage_Count,
};

enum EntryType : u8 {
    EntryType_Button,
    EntryType_Slider,
    EntryType_List,
    EntryType_Toggle,
    EntryType_Info,
}; 

enum EntryBinding : u8 {
    EntryBinding_None,
    EntryBinding_MusicVol,
    EntryBinding_EffectsVol,
    EntryBinding_DialogVol,
    EntryBinding_Stereo,
    EntryBinding_Shock,
    EntryBinding_PlayerConfig,
    EntryBinding_DisplayResolution,
    EntryBinding_DisplayScreenMode,
    EntryBinding_DisplayVsync,
    EntryBinding_DisplayFrameRate,
    EntryBinding_DisplayMsaa,
};

enum EntryEvent : u8 {
    EntryEvent_None,
    EntryEvent_GoPage,
    EntryEvent_Resume,
    EntryEvent_Back,
    EntryEvent_NewGame,
    EntryEvent_ExitToHub,
    EntryEvent_QuitGame,
    EntryEvent_Credits, 
    EntryEvent_Load, 
    EntryEvent_Save,
    EntryEvent_Delete,
    EntryEvent_LoadConfirmYes,
    EntryEvent_SaveConfirmYes,
    EntryEvent_DeleteConfirmYes,
    EntryEvent_LocationSelect,
};

#define MAX_ENTRIES_PER_MENU (12)
#define MAX_CUSTOM_MENU_TOKEN (8)

#define DEF_WINDOW_W 392
#define DEF_WINDOW_H 108

// Layout tuning for auto-sized custom FE windows.
#define DEF_WINDOW_CENTER_X 256
#define DEF_WINDOW_CENTER_Y 114
#define DEF_ROW_STEP 16
#define DEF_TITLE_BAR_H 30
#define DEF_BOTTOM_BAR_H 16
#define DEF_BORDER_W 1
#define DEF_CONTENT_PAD 4
#define DEF_CONTENT_TOP_PAD 4
#define DEF_CONTENT_BOTTOM_PAD 4
#define DEF_ROW_TEXT_H 12
#define DEF_INFO_ROW_EXTRA (0)

#define DEF_LABEL_X_PAD 18
#define DEF_VALUE_X_PAD 18
#define DEF_METER_W 74
#define DEF_METER_H 8

#define DEF_TITLE_INSET_X 48
#define DEF_TITLE_INSET_Y 6
#define DEF_TITLE_INSET_H 20

#define DEF_HELP_Y_PAD 3
#define DEF_HELP_LEFT_X_OFF (-28)
#define DEF_HELP_RIGHT_X_OFF 42

// Prompt/help spacing tuning
#define DEF_HELP_GROUP_GAP_PX 14.0f

// Key Bindings page tuning
#define DEF_KEYBIND_WINDOW_W 420
#define DEF_KEYBIND_WINDOW_H 200
#define DEF_KEYBIND_SLOT_COUNT 2
#define DEF_KEYBIND_VISIBLE_ROWS 8
#define DEF_KEYBIND_ROW_STEP 14
#define DEF_KEYBIND_SLOT_W 76
#define DEF_KEYBIND_SLOT_GAP 8
#define DEF_KEYBIND_TABLE_SIDE_PAD 8
#define DEF_KEYBIND_ACTION_COL_GAP 8
#define DEF_KEYBIND_HIT_PAD 2
#define DEF_KEYBIND_ROW_TOP_PAD 2
#define DEF_KEYBIND_CELL_PAD 2
#define DEF_KEYBIND_HELP_SCALE 0.62f
#define DEF_KEYBIND_HELP_GAP_PX 8.0f

// Key Bindings row stripe colors (transparent black / warm yellow)
#define DEF_KEYBIND_STRIPE_DARK_R 0
#define DEF_KEYBIND_STRIPE_DARK_G 0
#define DEF_KEYBIND_STRIPE_DARK_B 0
#define DEF_KEYBIND_STRIPE_DARK_A 100
#define DEF_KEYBIND_STRIPE_WARM_R 50
#define DEF_KEYBIND_STRIPE_WARM_G 30
#define DEF_KEYBIND_STRIPE_WARM_B 0
#define DEF_KEYBIND_STRIPE_WARM_A 100

// Selected binding cell background (pure black, opaque)
#define DEF_KEYBIND_ACTIVE_FILL_R 0
#define DEF_KEYBIND_ACTIVE_FILL_G 0
#define DEF_KEYBIND_ACTIVE_FILL_B 0
#define DEF_KEYBIND_ACTIVE_FILL_A 255

#define DEF_BOTTOM_ORN_COUNT 1
#define DEF_BOTTOM_ORN_STEP 24
#define DEF_BOTTOM_ORN_LEFT_X 16
#define DEF_BOTTOM_ORN_RIGHT_X 104
#define DEF_BOTTOM_ORN_Y_OFF 2

#define DEF_ORN_W 16
#define DEF_ORN_H 10
#define DEF_ORN_R 205
#define DEF_ORN_G 205
#define DEF_ORN_B 205
#define DEF_ORN_A 255

// Gouraud bar gradient colors: bright center highlight, dark at top/bottom edges
#define DEF_BAR_MID_R 255
#define DEF_BAR_MID_G 220
#define DEF_BAR_MID_B 12
#define DEF_BAR_EDGE_R 100
#define DEF_BAR_EDGE_G 40
#define DEF_BAR_EDGE_B 0
#define DEF_BAR_ALPHA 255

// Outer border / separator lines
#define DEF_FRAME_R 130
#define DEF_FRAME_G 0
#define DEF_FRAME_B 0
#define DEF_FRAME_A 255

// Body fill
#define DEF_BODY_R 0
#define DEF_BODY_G 0
#define DEF_BODY_B 0
#define DEF_BODY_A 140

// Title inset box: outer border then inner fill
#define DEF_TITLE_INSET_BORDER_R 130
#define DEF_TITLE_INSET_BORDER_G 0
#define DEF_TITLE_INSET_BORDER_B 0
#define DEF_TITLE_INSET_BORDER_A 255
#define DEF_TITLE_INSET_FILL_R 0
#define DEF_TITLE_INSET_FILL_G 0
#define DEF_TITLE_INSET_FILL_B 0
#define DEF_TITLE_INSET_FILL_A 255
#define DEF_TITLE_TEXT_R 200
#define DEF_TITLE_TEXT_G 200
#define DEF_TITLE_TEXT_B 200
#define DEF_TITLE_TEXT_A 255

// Text colors — values are PSX colour scale where 128 = 1.0 (neutral brightness).
// Values above 128 amplify past white (the shader divides by 128, not 255).
// The menu pulse uses (66..112, 36..73, 0) — stay in the same range for static text.
#define DEF_TEXT_NORM_R 108
#define DEF_TEXT_NORM_G 68
#define DEF_TEXT_NORM_B 0
#define DEF_TEXT_LOCK_R 82
#define DEF_TEXT_LOCK_G 58
#define DEF_TEXT_LOCK_B 20
#define DEF_TEXT_Y_OFF (-2)
#define DEF_TEXT_SHADOW_A 0x90
#define DEF_TITLE_SHADOW_A 0xC0

// Info entry text (confirmation/detail lines)
#define DEF_INFO_TEXT_R 108
#define DEF_INFO_TEXT_G 68
#define DEF_INFO_TEXT_B 0
#define DEF_INFO_TEXT_A 255
#define DEF_INFO_SHADOW_A 0x90

// Help bar text (dark, near-black)
#define DEF_HELP_TEXT_R 0
#define DEF_HELP_TEXT_G 0
#define DEF_HELP_TEXT_B 0
#define DEF_HELP_TEXT_A 255

// Slider colors
#define DEF_SLIDER_FRAME_R 130
#define DEF_SLIDER_FRAME_G 0
#define DEF_SLIDER_FRAME_B 0
#define DEF_SLIDER_FRAME_A 255

#define DEF_SLIDER_TRACK_R 10
#define DEF_SLIDER_TRACK_G 10
#define DEF_SLIDER_TRACK_B 10
#define DEF_SLIDER_TRACK_A 220

#define DEF_SLIDER_FILL_R 108
#define DEF_SLIDER_FILL_G 68
#define DEF_SLIDER_FILL_B 0
#define DEF_SLIDER_FILL_A 255

// Custom slider circles (custom FE menu path)
#define DEF_SLIDER_CIRCLE_SEGMENTS 6
#define DEF_SLIDER_CIRCLE_RADIUS 6
#define DEF_SLIDER_CIRCLE_STEP 24
#define DEF_SLIDER_CIRCLE_X_OFF (0)
#define DEF_SLIDER_CIRCLE_Y_OFF 4
#define DEF_SLIDER_CIRCLE_SUPERSAMPLE 1
#define DEF_SLIDER_CIRCLE_TEXTURE_PAD 0
#define DEF_SLIDER_CIRCLE_RASTER_DIV 2

#define DEF_SLIDER_CIRCLE_OUTER_THICKNESS 1
#define DEF_SLIDER_CIRCLE_RING_THICKNESS 1

#define DEF_SLIDER_CIRCLE_OUTER_OUTLINE_THICKNESS 1
#define DEF_SLIDER_CIRCLE_INNER_OUTLINE_THICKNESS 1

#define DEF_SLIDER_CIRCLE_OUTLINE_R DEF_FRAME_R
#define DEF_SLIDER_CIRCLE_OUTLINE_G DEF_FRAME_G
#define DEF_SLIDER_CIRCLE_OUTLINE_B DEF_FRAME_B
#define DEF_SLIDER_CIRCLE_OUTLINE_A DEF_FRAME_A

#define DEF_SLIDER_CIRCLE_OUTER_OUTLINE_R DEF_SLIDER_CIRCLE_OUTLINE_R
#define DEF_SLIDER_CIRCLE_OUTER_OUTLINE_G DEF_SLIDER_CIRCLE_OUTLINE_G
#define DEF_SLIDER_CIRCLE_OUTER_OUTLINE_B DEF_SLIDER_CIRCLE_OUTLINE_B
#define DEF_SLIDER_CIRCLE_OUTER_OUTLINE_A DEF_SLIDER_CIRCLE_OUTLINE_A

#define DEF_SLIDER_CIRCLE_INNER_OUTLINE_R DEF_SLIDER_CIRCLE_OUTLINE_R
#define DEF_SLIDER_CIRCLE_INNER_OUTLINE_G DEF_SLIDER_CIRCLE_OUTLINE_G
#define DEF_SLIDER_CIRCLE_INNER_OUTLINE_B DEF_SLIDER_CIRCLE_OUTLINE_B
#define DEF_SLIDER_CIRCLE_INNER_OUTLINE_A DEF_SLIDER_CIRCLE_OUTLINE_A

#define DEF_SLIDER_CIRCLE_RING_R DEF_TEXT_NORM_R
#define DEF_SLIDER_CIRCLE_RING_G DEF_TEXT_NORM_G
#define DEF_SLIDER_CIRCLE_RING_B DEF_TEXT_NORM_B
#define DEF_SLIDER_CIRCLE_RING_A 255

#define DEF_SLIDER_CIRCLE_CROSS_HALF 0
#define DEF_SLIDER_CIRCLE_CROSS_R DEF_SLIDER_CIRCLE_OUTLINE_R
#define DEF_SLIDER_CIRCLE_CROSS_G DEF_SLIDER_CIRCLE_OUTLINE_G
#define DEF_SLIDER_CIRCLE_CROSS_B DEF_SLIDER_CIRCLE_OUTLINE_B
#define DEF_SLIDER_CIRCLE_CROSS_A DEF_SLIDER_CIRCLE_OUTLINE_A

#define DEF_SLIDER_CIRCLE_FILL_R DEF_TEXT_NORM_R
#define DEF_SLIDER_CIRCLE_FILL_G DEF_TEXT_NORM_G
#define DEF_SLIDER_CIRCLE_FILL_B DEF_TEXT_NORM_B
#define DEF_SLIDER_CIRCLE_FILL_A 255

#define DEF_SLIDER_CIRCLE_EMPTY_R DEF_BODY_R
#define DEF_SLIDER_CIRCLE_EMPTY_G DEF_BODY_G
#define DEF_SLIDER_CIRCLE_EMPTY_B DEF_BODY_B
#define DEF_SLIDER_CIRCLE_EMPTY_A 255

#define DEF_QUIT_TIMER_SEC 0.2f

struct Entry {
    char token[MAX_CUSTOM_MENU_TOKEN + 1];
    EntryType type;
    EntryEvent event;
    MenuPage goPage;
    EntryBinding binding;
    s32 step;
    s32 lo;
    s32 hi;
    s32 posX;
    s32 posY;
};

struct PageDef {
    char titleToken[MAX_CUSTOM_MENU_TOKEN + 1];
    char overlayName[24];
    MenuPage parentPage;
    s32 parentEntry;
    bool isPause;
    s32 frameW;
    s32 frameH;
    s32 entriesOffsetX;
    s32 entriesOffsetY;
    s32 numEntries;
    Entry entries[MAX_ENTRIES_PER_MENU];

    PageDef() {
        memset(titleToken, 0, sizeof(titleToken));
        memset(overlayName, 0, sizeof(overlayName));
        parentPage = MenuPage_None;
        parentEntry = 0;
        isPause = false;
        frameW = 260;
        frameH = 150;
        entriesOffsetX = 0;
        entriesOffsetY = 0;
        numEntries = 0;
        memset(entries, 0, sizeof(entries));
    }
};

class feCustomMenuMgr {
public:
    void Init(CustomText* textSystem);
    void Shutdown();

    s32 Invoke(); // 1 = stay in menu, 4 = state change, 8 = resume play
    void Render();
    bool DrawTitleScreen();
    bool DrawLoadingScreen();
    void DrawTitleStartPrompt(s32 baseX, s32 baseY, u32 pulseColorABGR);
    bool GetSplashScreenRect(f32* outX, f32* outY, f32* outW, f32* outH) const;

    void Activate(MenuPage startPage = MenuPage_Frontend);
    void Deactivate();
    bool IsActive() const { return m_active; }
    MenuPage GetCurrentPage() const { return m_currPage; }

private:
    void BuildPages();
    PageDef& AddPage(MenuPage id, const char* title,
                     const char* overlay, MenuPage parent, s32 parentEntry, bool pause,
                     s32 frameW = 260, s32 frameH = 150);
    static s32 CalcAutoFrameHeight(s32 numEntries, s32 extraH = 0);
    s32 GetEntryExtraHeight(const PageDef& page, const Entry& entry, xcFont* font) const;
    s32 CalcEntryYExtra(const PageDef& page, s32 upToIndex, xcFont* font) const;
    s32 CalcPageExtraHeight(const PageDef& page, xcFont* font) const;
    void ResolveEntryLayout(const PageDef& page, s32 entryIndex, xcFont* font,
                            s32 firstY, s32 baseLabelX, s32 baseValueX, s32 baseCenterX,
                            s32* outRowTop, s32* outRowTextY,
                            s32* outLabelX, s32* outValueX, s32* outCenterX) const;
    void SetEntries(PageDef& page, std::initializer_list<Entry> list,
                    s32 entriesOffsetX = 0, s32 entriesOffsetY = 0);
    static Entry Button(const char* tok, EntryEvent ev, MenuPage go = MenuPage::MenuPage_None);
    static Entry Slider(const char* tok, EntryBinding binding, s32 step, s32 lo, s32 hi);
    Entry List(const char* tok, EntryBinding binding, s32 step, s32 lo, s32 hi);
    static Entry Toggle(const char* tok, EntryBinding binding);
    static Entry Info(const char* tok);

    void SetPage(MenuPage page);
    void MoveCursor(s32 dir);
    void Confirm();
    void GoBack();
    void Adjust(s32 dir);
    s32 GetBoundValue(const Entry& e) const;
    void ApplyValue(const Entry& e, s32 v);
    void PlaySound(s32 id) const;
    void RefreshSaveSlots();
    void BuildSaveSlotLabel(s32 slotIndex, char* outText, s32 outTextLen) const;

    struct CellTex { tTexture* tex; s16 w, h; u32 nameHash; };

    void LoadControllerOverlayTexture();
    void LoadSplashTextures();
    void EnsureTextures();

    static constexpr const char* kControllerOverlayTexturePath = "pc/textures/frontend/controller_overlay_default.png";
    static constexpr const char* kTitleScreenTexturePath        = "pc/textures/frontend/title_screen.png";
    static constexpr const char* kLoadingScreenTexturePath      = "pc/textures/frontend/loading_screen.png";
    static constexpr const char* kRedDragonTexturePath          = "pc/textures/frontend/red_dragon.png";
    static constexpr const char* kGoldDragonTexturePath         = "pc/textures/frontend/gold_dragon.png";
    static constexpr const char* kGreyDragonTexturePath         = "pc/textures/frontend/grey_dragon.png";
    static constexpr f32 kSplashScreenAspect = 16.0f / 9.0f;

    void DrawSplashTexture16x9(tTexture* texture);

    void DrawMenuWindow(s32 x, s32 y, s32 w, s32 h, const char* title) const;

    void DrawRect(s32 x, s32 y, s32 w, s32 h, u8 r, u8 g, u8 b, u8 a) const;
    void DrawHighlight(s32 x, s32 y, s32 w, s32 h) const;
    xcFont* FindFont(const char* first, const char* second = nullptr) const;

    static void BuildMeter(s32 value, char* buf);

    const char* Localize(const char* token) const;

    void RenderControllerOverlay(s32 panelX, s32 panelY) const;
    void RenderKeyBindingsPage(s32 panelX, s32 panelY, s32 panelW, s32 panelH,
                               xcFont* bodyFont, u32 normalColor, u32 selectedColor) const;
    void RenderLocationPage() const;
    bool InvokeLocationSelection();

    CustomText* m_text = nullptr;
    PageDef m_pages[MenuPage_Count];
    std::vector<CellTex> m_cellTextures;
    xcSectionMan* m_menuArt = nullptr;
    tTexture* m_titleScreenTexture = nullptr;
    tTexture* m_loadingScreenTexture = nullptr;
    mutable tTexture* m_controllerTexture = nullptr;
    mutable tTexture* m_redDragonTex      = nullptr;
    mutable tTexture* m_goldDragonTex     = nullptr;
    mutable tTexture* m_greyDragonTex     = nullptr;
    bool m_titleScreenTextureTried = false;
    bool m_loadingScreenTextureTried = false;
    mutable bool m_dragonTexTried  = false;
    bool m_texReady = false;
    MenuPage m_currPage = MenuPage_None;
    MenuPage m_prevPage = MenuPage_None;
    s32 m_cursor = 0;
    s32 m_result = 1;
    xcColour1555 m_pulse{ 0x8000 };
    // Resolution selection is staged while focused and committed on confirm.
    bool m_pendingResolutionActive = false;
    s32 m_pendingResolutionIndex = 0;

    // Screen mode selection is staged while focused and committed on confirm.
    bool m_pendingScreenModeActive = false;
    s32 m_pendingScreenMode = 0;

    // MSAA selection is staged while focused and committed on confirm.
    bool m_pendingMsaaActive = false;
    s32 m_pendingMsaaIndex = 0;

    // Input mode tracking: when false, mouse hover is ignored until mouse moves/clicks.
    bool m_mouseInputActive = true;
    bool m_mousePosInitialized = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    // Key bindings page selection/capture state.
    s32 m_keyBindActionCursor = 0;
    s32 m_keyBindSlotCursor = 0;
    s32 m_keyBindScrollTop = 0;
    bool m_keyBindCaptureActive = false;
    s32 m_keyBindCaptureBlockFrames = 0;

    SaveGameSlotInfo m_saveSlots[SAVEGAME_SLOT_COUNT] = {};
    s32 m_pendingLoadSlot = -1;
    s32 m_pendingSaveSlot = -1;
    s32 m_pendingDeleteSlot = -1;

    bool m_active = 0;
    f32 m_quitTimerSec = 0.0f; // seconds remaining before game actually closes
};

extern feCustomMenuMgr* g_feCustomMenuMgr;
