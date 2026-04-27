#include "fe/hud.h"
#include "gen/common.h"
#include "gen/game.h"
#include "gen/world.h"
#include "gen/scoremgr.h"
#include "xclib/xclib.h"
#include "gen/handler.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "gen/ai.h"
#include "pc/inputaction.h"
#include "p3d/context.h"
#include "pddi/pddidev.h"

HUD* g_hud = nullptr;
char HUD::szBossStatic[32] = {};

// PSX: gp+1084 - screen names table
static const char* s_hudScreenNames[] = { "HUD", nullptr };
static constexpr u32 kHubPromptOverlayHash = 0x050794E7u;

static s32 RoundToNearestS32(f32 v) {
    return (v >= 0.0f) ? static_cast<s32>(v + 0.5f) : static_cast<s32>(v - 0.5f);
}

static s32 GetHudWidescreenShiftX() {
#if FIX_ASPECT_RATIO
    // The 4:3 overlay content is centered inside the screen by SCALE_AND_CENTER_X.
    // We move edge-anchored HUD toward a 16:9-safe frame and clamp there on ultrawide.
    const f32 scaledWidth = SCREEN_SCALE_X(static_cast<f32>(DEFAULT_SCREEN_WIDTH));
    const f32 targetWidth = SCREEN_EFFECTIVE_WIDTH;
    const f32 targetOffsetPx = (targetWidth - scaledWidth) * 0.5f;
    if (targetOffsetPx <= 0.0f) {
        return 0;
    }

    const f32 screenUnitsPerHudPixel = SCREEN_SCALE_X(1.0f);
    if (screenUnitsPerHudPixel <= 0.0f) {
        return 0;
    }

    return RoundToNearestS32(targetOffsetPx / screenUnitsPerHudPixel);
#else
    return 0;
#endif
}

static s32 ClassifyHudOverlayAnchorX(xcOverlayData* overlay, u8* rawData) {
    if (!overlay || !rawData) {
        return 0;
    }

    const xcOverlayItem* items = overlay->GetItems();
    bool found = false;
    s32 minX = 0;
    s32 maxX = 0;

    auto trackX = [&](s32 x) {
        if (!found) { minX = maxX = x; found = true; }
        else { if (x < minX) minX = x; if (x > maxX) maxX = x; }
    };

    for (u32 i = 0; i < overlay->primCount; i++) {
        u8* prim = rawData + items[i].dataOffset;
        xcPrimHeader* hdr = reinterpret_cast<xcPrimHeader*>(prim);
        if (hdr->subtype == 5) {
            continue;
        }

        if (hdr->type == XC_PRIM_POLYF4) {
            const xcPolyF4Prim* poly = reinterpret_cast<const xcPolyF4Prim*>(prim);
            s16 bx0, by0, bx1, by1;
            poly->GetBounds(bx0, by0, bx1, by1);
            trackX(bx0);
            trackX(bx1);
            continue;
        }
        if (hdr->type == XC_PRIM_POLYG4) {
            const xcPolyG4Prim* poly = reinterpret_cast<const xcPolyG4Prim*>(prim);
            s16 bx0, by0, bx1, by1;
            poly->GetBounds(bx0, by0, bx1, by1);
            trackX(bx0);
            trackX(bx1);
            continue;
        }

        oxOvl helper;
        helper.overlay = overlay;
        s16 x = 0, y = 0;
        if (helper.GetPrimPos(prim, x, y)) {
            trackX(x);
        }
    }

    if (!found) {
        return 0;
    }

    const s32 centerX = (minX + maxX) / 2;
    const s32 leftThreshold = static_cast<s32>(DEFAULT_SCREEN_WIDTH * 3.0f / 8.0f);
    const s32 rightThreshold = static_cast<s32>(DEFAULT_SCREEN_WIDTH * 5.0f / 8.0f);
    if (centerX <= leftThreshold) {
        return -1;
    }
    if (centerX >= rightThreshold) {
        return 1;
    }

    return 0;
}

static void ShiftHudOverlayX(xcOverlayData* overlay, u8* rawData, s32 deltaX) {
    if (!overlay || !rawData || deltaX == 0) {
        return;
    }

    const xcOverlayItem* items = overlay->GetItems();
    for (u32 i = 0; i < overlay->primCount; i++) {
        u8* prim = rawData + items[i].dataOffset;
        xcPrimHeader* hdr = reinterpret_cast<xcPrimHeader*>(prim);
        if (hdr->subtype == 5) {
            continue;
        }

        // For poly prims, directly shift all 4 vertices.
        // SetPrimPos for POLYF4/G4 does not perform a pure translation
        // (it moves only the TR/BR vertices, not TL/BL), so we manipulate
        // the vertex data directly as a PC-only widescreen fixup.
        if (hdr->type == XC_PRIM_POLYF4) {
            xcPolyF4Prim* poly = reinterpret_cast<xcPolyF4Prim*>(prim);
            poly->x0 = static_cast<s16>(poly->x0 + deltaX);
            poly->x1 = static_cast<s16>(poly->x1 + deltaX);
            poly->x2 = static_cast<s16>(poly->x2 + deltaX);
            poly->x3 = static_cast<s16>(poly->x3 + deltaX);
            continue;
        }
        if (hdr->type == XC_PRIM_POLYG4) {
            xcPolyG4Prim* poly = reinterpret_cast<xcPolyG4Prim*>(prim);
            poly->x0 = static_cast<s16>(poly->x0 + deltaX);
            poly->x1 = static_cast<s16>(poly->x1 + deltaX);
            poly->x2 = static_cast<s16>(poly->x2 + deltaX);
            poly->x3 = static_cast<s16>(poly->x3 + deltaX);
            continue;
        }

        // Sprite and text prims: SetPrimPos correctly updates the matrix position.
        oxOvl helper;
        helper.overlay = overlay;
        s16 x = 0, y = 0;
        if (helper.GetPrimPos(prim, x, y)) {
            helper.SetPrimPos(prim, static_cast<s16>(x + deltaX), y);
        }
    }
}

// Per-overlay widescreen anchor: tracks direction and currently applied X offset
// so per-frame updates can apply only the delta (works at any live aspect ratio).
struct HudOverlayAnchor {
    xcOverlayData* overlay;
    s8 direction;   // -1=left-anchored, 0=center, +1=right-anchored
    s32 appliedX;   // PSX units currently added to this overlay's prims
};

static constexpr s32 kMaxHudAnchors = 128;
static HudOverlayAnchor s_hudAnchors[kMaxHudAnchors];
static s32 s_hudAnchorCount = 0;

// PC: called from SelfInit - discovers every overlay in the section and classifies it.
static void BuildHudWidescreenAnchors(xcSection* sectionPtr, u8* rawData) {
    s_hudAnchorCount = 0;
    if (!sectionPtr || !rawData || !sectionPtr->overlays) {
        return;
    }

    const xcInventory* inv = sectionPtr->overlays;
    const xcInventoryItem* items = inv->GetItems();
    for (u32 i = 0; i < inv->itemCount && s_hudAnchorCount < kMaxHudAnchors; i++) {
        xcOverlayData* ovl = reinterpret_cast<xcOverlayData*>(rawData + items[i].dataOffset);
        HudOverlayAnchor& a = s_hudAnchors[s_hudAnchorCount++];
        a.overlay = ovl;
        a.direction = static_cast<s8>(ClassifyHudOverlayAnchorX(ovl, rawData));
        a.appliedX = 0;
    }

    if (inv->itemCount > static_cast<u32>(kMaxHudAnchors)) {
        LOG("[HUD] widescreen anchors truncated: overlays=%u max=%d", inv->itemCount, kMaxHudAnchors);
    }
}

// PC: called every frame from SelfUpdate - recomputes shiftX and shifts only the delta.
static void UpdateHudWidescreenAnchors(u8* rawData) {
    if (!rawData || s_hudAnchorCount == 0) {
        return;
    }

    const s32 shiftX = GetHudWidescreenShiftX();
    for (s32 i = 0; i < s_hudAnchorCount; i++) {
        HudOverlayAnchor& a = s_hudAnchors[i];
        if (a.direction == 0 || !a.overlay) {
            continue;
        }
        const s32 targetX = a.direction * shiftX;
        const s32 deltaX = targetX - a.appliedX;
        if (deltaX != 0) {
            ShiftHudOverlayX(a.overlay, rawData, deltaX);
            a.appliedX = targetX;
        }
    }
}

// PC: remove currently applied X offsets so HUD sub-object updates can run in
// canonical PSX overlay space, then reapply target offsets after updates.
static void ResetHudWidescreenAnchors(u8* rawData) {
    if (!rawData || s_hudAnchorCount == 0) {
        return;
    }

    for (s32 i = 0; i < s_hudAnchorCount; i++) {
        HudOverlayAnchor& a = s_hudAnchors[i];
        if (!a.overlay || a.appliedX == 0) {
            continue;
        }

        ShiftHudOverlayX(a.overlay, rawData, -a.appliedX);
        a.appliedX = 0;
    }
}

static bool StretchHubBottomBarsInOverlay(xcOverlayData* ovl, u8* rawData, s16 leftX, s16 rightX) {
    if (!ovl || !rawData) {
        return false;
    }

    bool changed = false;
    const xcOverlayItem* items = ovl->GetItems();

    for (u32 i = 0; i < ovl->primCount; i++) {
        u8* primData = rawData + items[i].dataOffset;
        xcPrimHeader* hdr = reinterpret_cast<xcPrimHeader*>(primData);
        if (hdr->subtype == 5) {
            continue;
        }

        if (hdr->type == XC_PRIM_POLYF4) {
            xcPolyF4Prim* poly = reinterpret_cast<xcPolyF4Prim*>(primData);

            s16 minX = poly->x0;
            s16 maxX = poly->x0;
            s16 minY = poly->y0;
            s16 maxY = poly->y0;
            if (poly->x1 < minX) minX = poly->x1; if (poly->x1 > maxX) maxX = poly->x1;
            if (poly->x2 < minX) minX = poly->x2; if (poly->x2 > maxX) maxX = poly->x2;
            if (poly->x3 < minX) minX = poly->x3; if (poly->x3 > maxX) maxX = poly->x3;
            if (poly->y1 < minY) minY = poly->y1; if (poly->y1 > maxY) maxY = poly->y1;
            if (poly->y2 < minY) minY = poly->y2; if (poly->y2 > maxY) maxY = poly->y2;
            if (poly->y3 < minY) minY = poly->y3; if (poly->y3 > maxY) maxY = poly->y3;

            if (minY < static_cast<s16>(DEFAULT_SCREEN_HEIGHT - 96.0f) ||
                (maxX - minX) < static_cast<s16>(DEFAULT_SCREEN_WIDTH * 0.35f) ||
                (maxY - minY) < 8) {
                continue;
            }

            const s16 mid = static_cast<s16>((minX + maxX) / 2);
            poly->x0 = (poly->x0 <= mid) ? leftX : rightX;
            poly->x1 = (poly->x1 <= mid) ? leftX : rightX;
            poly->x2 = (poly->x2 <= mid) ? leftX : rightX;
            poly->x3 = (poly->x3 <= mid) ? leftX : rightX;
            changed = true;
            continue;
        }

        if (hdr->type == XC_PRIM_POLYG4) {
            xcPolyG4Prim* poly = reinterpret_cast<xcPolyG4Prim*>(primData);

            s16 minX = poly->x0;
            s16 maxX = poly->x0;
            s16 minY = poly->y0;
            s16 maxY = poly->y0;
            if (poly->x1 < minX) minX = poly->x1; if (poly->x1 > maxX) maxX = poly->x1;
            if (poly->x2 < minX) minX = poly->x2; if (poly->x2 > maxX) maxX = poly->x2;
            if (poly->x3 < minX) minX = poly->x3; if (poly->x3 > maxX) maxX = poly->x3;
            if (poly->y1 < minY) minY = poly->y1; if (poly->y1 > maxY) maxY = poly->y1;
            if (poly->y2 < minY) minY = poly->y2; if (poly->y2 > maxY) maxY = poly->y2;
            if (poly->y3 < minY) minY = poly->y3; if (poly->y3 > maxY) maxY = poly->y3;

            if (minY < static_cast<s16>(DEFAULT_SCREEN_HEIGHT - 96.0f) ||
                (maxX - minX) < static_cast<s16>(DEFAULT_SCREEN_WIDTH * 0.35f) ||
                (maxY - minY) < 8) {
                continue;
            }

            const s16 mid = static_cast<s16>((minX + maxX) / 2);
            poly->x0 = (poly->x0 <= mid) ? leftX : rightX;
            poly->x1 = (poly->x1 <= mid) ? leftX : rightX;
            poly->x2 = (poly->x2 <= mid) ? leftX : rightX;
            poly->x3 = (poly->x3 <= mid) ? leftX : rightX;
            changed = true;
        }
    }

    return changed;
}

// PC hub-only fix: expand the bottom prompt black bar to the full 16:9-safe width.
static void FixHubPromptBottomBar(xcSection* sectionPtr, u8* rawData) {
#if FIX_ASPECT_RATIO
    if (!sectionPtr || !rawData) {
        return;
    }

    if (!g_game || !g_game->GetWorld() || g_game->GetWorld()->GetCurLevelID() != 7) {
        return;
    }

    if (!sectionPtr->overlays) {
        return;
    }

    const f32 screenUnitsPerHudPixel = SCREEN_SCALE_X(1.0f);
    if (screenUnitsPerHudPixel <= 0.0f) {
        return;
    }

    const f32 scaledWidth = SCREEN_SCALE_X(DEFAULT_SCREEN_WIDTH);
    const f32 centerOffsetPx = (SCREEN_WIDTH - scaledWidth) * 0.5f;
    const f32 leftPx = SCREEN_EFFECTIVE_OFFSET_X;
    const f32 rightPx = leftPx + SCREEN_EFFECTIVE_WIDTH;

    const s16 leftX = static_cast<s16>(RoundToNearestS32((leftPx - centerOffsetPx) / screenUnitsPerHudPixel));
    const s16 rightX = static_cast<s16>(RoundToNearestS32((rightPx - centerOffsetPx) / screenUnitsPerHudPixel));

    bool changed = false;

    const xcInventoryItem* ovlItem = sectionPtr->overlays->FindItem(kHubPromptOverlayHash);
    if (ovlItem) {
        xcOverlayData* ovl = reinterpret_cast<xcOverlayData*>(rawData + ovlItem->dataOffset);
        changed = StretchHubBottomBarsInOverlay(ovl, rawData, leftX, rightX);
    }

    // Fallback: if hash changes across builds/assets, scan all HUD overlays once.
    if (!changed) {
        const xcInventory* inv = sectionPtr->overlays;
        const xcInventoryItem* items = inv->GetItems();
        for (u32 i = 0; i < inv->itemCount; i++) {
            xcOverlayData* ovl = reinterpret_cast<xcOverlayData*>(rawData + items[i].dataOffset);
            if (StretchHubBottomBarsInOverlay(ovl, rawData, leftX, rightX)) {
                changed = true;
            }
        }
    }
#endif
}

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

// PSX: InternalReset__3HUD (HUD.CPP:350, 0x8003F650)
void HUD::InternalReset() {
    MARKFUNCTION(0x8003F650);
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
#if FIX_ASPECT_RATIO
        if (p3d::context) {
            const s32 scissorX = RoundToNearestS32(SCREEN_EFFECTIVE_OFFSET_X);
            const s32 scissorW = RoundToNearestS32(SCREEN_EFFECTIVE_WIDTH);
            const s32 scissorH = RoundToNearestS32(SCREEN_HEIGHT);

            // Clamp HUD rendering to the 16:9-safe region (ultrawide side gutters discarded).
            p3d::context->SetScissor(scissorX, 0, scissorW, scissorH);
            section->Draw();

            // Restore frame scissor for subsequent rendering passes.
            p3d::context->SetScissor(0, 0, RoundToNearestS32(SCREEN_WIDTH), scissorH);
        }
        else {
            section->Draw();
        }
#else
        section->Draw();
#endif
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
    xcOverlayData* playerHealthOvl = FindOverlay((u32)0xD6549407);
    xcOverlayData* bossHealthOvl = FindOverlay((u32)0xA7160677);
    xcOverlayData* foeHealthOvl = FindOverlay((u32)0x8A0C569E);
    xcOverlayData* takesOvl = FindOverlay((u32)0x0A88DB30);
    xcOverlayData* dragonOvl = FindOverlay((u32)0xC98817DB);
    xcOverlayData* hitsOvl = FindOverlay((u32)0x71AB6E45);

    // PC: classify all HUD overlays for per-frame widescreen anchoring.
    BuildHudWidescreenAnchors(section, raw);

    // playerHealth (+120)
    playerHealth.rawData = raw;
    playerHealth.Init(playerHealthOvl);

    // bossHealth (+156)
    bossHealth.rawData = raw;
    bossHealth.Init(bossHealthOvl);
    bossHealth.SetVisible(0);

    // foeHealth (+192)
    foeHealth.rawData = raw;
    foeHealth.Init(foeHealthOvl);
    foeHealth.SetVisible(0);

    // takes (+292)
    takes.rawData = raw;
    takes.Init(takesOvl);
    takes.SetAnimInfo(-150, 0, 15, 15);
    takes.SetVisible(1);

    // dragon (+420)
    dragon.rawData = raw;
    dragon.Init(dragonOvl);
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
    inputEnabled = enable;
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
    // PC: run HUD updates in canonical PSX space, then apply widescreen anchor.
    u8* raw = section ? section->rawData : nullptr;
    if (raw) {
        ResetHudWidescreenAnchors(raw);
    }

    // PSX uses InputManager button callbacks for logical button 0 (L2 / Status Display).
    // Until that callback layer is reversed on PC, mirror the same trigger here.
    if (inputEnabled && g_inputManager) {
        World* world = g_game ? g_game->GetWorld() : nullptr;
        if (world && world->GetCurLevelID() != 7) {
            Button* statusButton = g_inputManager->GetButtonForBit(0, 0);
            if (statusButton && statusButton->rawInput && !statusButton->prevInput) {
                ToggleShowAll();
            }
        }
    }

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

    if (raw) {
        UpdateHudWidescreenAnchors(raw);
        FixHubPromptBottomBar(section, raw);
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
    s_hudAnchorCount = 0;
    if (bossHandle) {
        bossHandle->refCount--;
        if (bossHandle->refCount == 0) {
            delete bossHandle;
        }
        bossHandle = nullptr;
    }
}
