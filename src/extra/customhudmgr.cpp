#include "extra/customhudmgr.h"

#include "fe/hud.h"
#include "ai/humanoid.h"
#include "ai/player.h"
#include "ai/thing.h"
#include "gen/common.h"
#include "gen/camera.h"
#include "gen/display.h"
#include "gen/game.h"
#include "gen/scoremgr.h"
#include "gen/time.h"
#include "gen/world.h"
#include "extra/customtext.h"
#include "pc/textmgr.h"
#include "pc/tim.h"
#include "snd/fesnd.h"
#include "p3d/texture.h"
#include "pddi/pdditex.h"
#include "xclib/xclib.h"

static const char* kHudBodyFontName = "Menu";
static const char* kHudTitleFontName = "HUDTitle";
static const char* kHudTitleFontPath = "pc/fonts/Europa Grotesk SH DemBol.ttf";

static const char* kTakeTexturePath = "pc/textures/collectibles/take.png";
static const char* kRedDragonTexturePath = "pc/textures/collectibles/red_dragon.png";
static const char* kGoldDragonTexturePath = "pc/textures/collectibles/gold_dragon.png";
static const char* kGreyDragonTexturePath = "pc/textures/collectibles/grey_dragon.png";
static const char* kBowlTexturePath = "pc/textures/collectibles/bowl.png";
static const char* kMilkTexturePath = "pc/textures/collectibles/milk.png";
static const char* kNoodleTexturePath = "pc/textures/collectibles/noodlebox.png";
static const char* kOrnamentTexturePath = "pc/textures/frontend/menu_ornament.png";

// Menu-matched palette for destination banner styling.
static const u8 kMenuFrameR = 130;
static const u8 kMenuFrameG = 0;
static const u8 kMenuFrameB = 0;
static const u8 kMenuFrameA = 255;
static const u8 kMenuTitleTextR = 255;
static const u8 kMenuTitleTextG = 255;
static const u8 kMenuTitleTextB = 255;
static const u8 kMenuTextNormR = 222;
static const u8 kMenuTextNormG = 222;
static const u8 kMenuTextNormB = 222;
static const u8 kDestBannerFillA = 160;

// Health timing tuning.
static const f32 kHealthDamageHoldSeconds = 0.55f;
static const f32 kHealthDamageCatchupRate = 0.75f;
static const f32 kHealthHealLerpRate = 2.8f;
static const f32 kHealthDamageLerpRate = 6.5f;
static const f32 kHealthDamageTriggerEpsilon = 0.001f;
static const f32 kHealthDeltaSeamOverlapPx = 0.65f;

// Health shake tuning (screen-space pixels).
static const f32 kHealthShakeDurationSeconds = 0.22f;
static const f32 kHealthShakeFrequency = 56.0f;
static const f32 kPlayerHealthShakeAmpX = 2.3f;
static const f32 kPlayerHealthShakeAmpY = 1.5f;
static const f32 kEnemyHealthShakeAmpX = 1.6f;
static const f32 kEnemyHealthShakeAmpY = 1.0f;
static const f32 kPlayerHealthShakePhaseY = 1.5707963f;
static const f32 kEnemyHealthShakePhaseY = 1.0471976f;

// Player health card layout.
static const f32 kPlayerHealthNameX = 14.0f;
static const f32 kPlayerHealthNameY = 8.0f;
static const f32 kPlayerHealthBarX = 14.0f;
static const f32 kPlayerHealthBarY = 16.0f;
static const f32 kPlayerHealthBarW = 80.0f;
static const f32 kPlayerHealthBarH = 8.0f;
static const f32 kPlayerHealthNameScale = 0.24f;

// Enemy/boss health card layout.
static const f32 kEnemyHealthBarWFromScreenH = 0.076f;
static const f32 kEnemyHealthBarHFromScreenH = 0.016f;
static const f32 kEnemyHealthBarYOffsetFromScreenH = 0.072f;
static const f32 kEnemyHealthLabelYOffsetFromScreenH = 0.024f;
static const f32 kEnemyHealthLabelScale = 0.20f;

// Shared health bar colors.
static const u8 kHealthBarBgR = 58;
static const u8 kHealthBarBgG = 18;
static const u8 kHealthBarBgB = 0;
static const u8 kHealthBarBgA = 196;
// Bottom color of the gold gouraud fill.
static const u8 kHealthBarFillR = 146;
static const u8 kHealthBarFillG = 90;
static const u8 kHealthBarFillB = 0;
static const u8 kHealthBarFillA = 255;
// Top color of the gold gouraud fill.
static const u8 kHealthBarFillTopR = 252;
static const u8 kHealthBarFillTopG = 232;
static const u8 kHealthBarFillTopB = 0;
static const u8 kHealthBarFillTopA = 255;
static const u8 kHealthBarOutlineR = 246;
static const u8 kHealthBarOutlineG = 246;
static const u8 kHealthBarOutlineB = 246;
static const u8 kHealthBarOutlineA = 208;
static const f32 kHealthBarOutlineThicknessPx = 1.0f;

// Delayed-damage colors (used by both player and enemy bars).
static const u8 kHealthDamageDeltaR = 228;
static const u8 kHealthDamageDeltaG = 96;
static const u8 kHealthDamageDeltaB = 72;
static const u8 kHealthDamageDeltaA = 168;
static const u8 kHealthDamageDeltaTopR = 255;
static const u8 kHealthDamageDeltaTopG = 216;
static const u8 kHealthDamageDeltaTopB = 216;
static const u8 kHealthDamageDeltaTopA = 136;

// Health text colors.
static const u8 kPlayerHealthNameR = 252;
static const u8 kPlayerHealthNameG = 239;
static const u8 kPlayerHealthNameB = 220;
static const u8 kPlayerHealthNameA = 255;
static const u8 kBossHealthLabelR = 246;
static const u8 kBossHealthLabelG = 224;
static const u8 kBossHealthLabelB = 188;
static const u8 kBossHealthLabelA = 255;

// Inventory pulse animation tuning.
static const f32 kInventoryPulseSeconds = 0.80f;
static const f32 kInventoryPulseIconScale = 0.30f;
static const f32 kInventoryPulseIconLift = 2.4f;
static const f32 kInventoryPulseTextScale = 0.24f;
static const f32 kInventoryPulseTextLift = 1.7f;
static const f32 kInventoryPulseTextColorBoost = 34.0f;
static const f32 kInventoryPulseShakeFrequency = 52.0f;
static const f32 kInventoryIconShakeAmpX = 1.05f;
static const f32 kInventoryIconShakeAmpY = 0.70f;
static const f32 kInventoryTextShakeAmpX = 0.82f;
static const f32 kInventoryRedShakePhaseX = 0.0f;
static const f32 kInventoryRedShakePhaseY = 1.0471976f;
static const f32 kInventoryGoldShakePhaseX = 1.5707963f;
static const f32 kInventoryGoldShakePhaseY = 2.3561945f;
static const f32 kInventoryLivesShakePhaseX = 3.1415926f;
static const f32 kInventoryLivesShakePhaseY = 0.7853982f;
static const f32 kInventoryTextShakePhaseOffsetX = 0.35f;
static const f32 kInventoryTextGapFromIcon = 2.0f;
static const f32 kInventoryTextCenterToPrintYOffset = 5.0f;
static const f32 kHudGlobalFadeSpeed = 6.0f;

// Hit combo text shake on increase.
static const f32 kHitTextShakeDurationSeconds = 0.36f;
static const f32 kHitTextShakeFrequency = 72.0f;
static const f32 kHitTextShakeAmpX = 5.2f;
static const f32 kHitTextShakeAmpY = 3.4f;
static const f32 kHitTextShakePhaseY = 1.5707963f;

// Tally layout and style tuning.
static const f32 kTallyPanelX = 20.0f;
static const f32 kTallyPanelY = 20.0f;
static const f32 kTallyPanelW = 198.0f;
static const f32 kTallyPanelH = 148.0f;
static const f32 kTallyRowH = 16.0f;
static const f32 kTallyRowSpacing = 18.0f;
static const f32 kTallyHeaderTextScale = 0.36f;
static const f32 kTallyLabelScale = 0.34f;
static const f32 kTallyValueScale = 0.34f;
static const f32 kTallyValueBoxW = 60.0f;
static const f32 kTallyDragonRowsYOffset = 8.0f;
static const f32 kTallyGoldBonusTextScale = 0.30f;
static const f32 kTallyGoldBonusScreenX = DEFAULT_SCREEN_WIDTH * 0.5f;
static const f32 kTallyGoldBonusScreenY = DEFAULT_SCREEN_HEIGHT * 0.5f;
static const f32 kTallyGoldBonusStartScaleMul = 8.0f;
static const f32 kTallyGoldBonusFadeInPortion = 0.24f;
static const f32 kTallyGoldBonusFadeOutPortion = 0.30f;
static const f32 kTallyGoldBonusSettleLiftPx = 14.0f;
static const f32 kTallyGoldBonusGlowScaleMul = 1.28f;
static const f32 kTallyGoldBonusGlowAlphaMul = 0.46f;
static const f32 kTallyGoldBonusWidthExpandMul = 1.30f;
static const f32 kTallyMovieBonusAnimSeconds = 1.35f;
static const f32 kTallyMovieBonusStartScaleMul = 6.0f;
static const f32 kTallyMovieBonusFadeInPortion = 0.14f;
static const f32 kTallyMovieBonusSettleLiftPx = 12.0f;
static const f32 kTallyMovieBonusScreenX = DEFAULT_SCREEN_WIDTH * 0.5f;
static const f32 kTallyMovieBonusScreenY = 178.0f;
static const f32 kTallyMovieBonusShakeAmpX = 2.2f;
static const f32 kTallyMovieBonusShakeAmpY = 1.6f;
static const f32 kTallyMovieBonusShakeFreqX = 5.3f;
static const f32 kTallyMovieBonusShakeFreqY = 3.7f;

static const f32 kTallyPanelTopBarH = 30.0f;
static const u8 kTallyPanelBarMidR = 255;
static const u8 kTallyPanelBarMidG = 255;
static const u8 kTallyPanelBarMidB = 24;
static const u8 kTallyPanelBarEdgeR = 199;
static const u8 kTallyPanelBarEdgeG = 80;
static const u8 kTallyPanelBarEdgeB = 0;
static const u8 kTallyPanelBarA = 255;
static const u8 kTallyPanelOutlineR = 130;
static const u8 kTallyPanelOutlineG = 0;
static const u8 kTallyPanelOutlineB = 0;
static const u8 kTallyPanelOutlineA = 255;
static const u8 kTallyPanelBodyR = 0;
static const u8 kTallyPanelBodyG = 0;
static const u8 kTallyPanelBodyB = 0;
static const u8 kTallyPanelBodyA = 140;

static const u8 kTallyRowFillR = 0;
static const u8 kTallyRowFillG = 0;
static const u8 kTallyRowFillB = 0;
static const u8 kTallyRowFillA = 255;
static const u8 kTallyRowOutlineR = 120;
static const u8 kTallyRowOutlineG = 0;
static const u8 kTallyRowOutlineB = 0;
static const u8 kTallyRowOutlineA = 208;
static const u8 kTallyLabelR = 215;
static const u8 kTallyLabelG = 135;
static const u8 kTallyLabelB = 0;
static const u8 kTallyTitleR = 255;
static const u8 kTallyTitleG = 255;
static const u8 kTallyTitleB = 255;
static const u8 kTallyValueGoldR = 250;
static const u8 kTallyValueGoldG = 214;
static const u8 kTallyValueGoldB = 148;

// Tally SFX events from hdTally state progression.
static const s32 kTallySoundStart = 21;
static const s32 kTallySoundFightDone = 29;
static const s32 kTallySoundComboDone = 27;
static const s32 kTallySoundStyleDone = 31;
static const s32 kTallySoundGradeDone = 22;
static const s32 kTallySoundRDragonDone = 23;
static const s32 kTallySoundGoldBonus = 25;
static const s32 kTallySoundGDragonDone = 24;
static const f32 kTallyGoldLeadInSeconds = 1.0f;
static const f32 kTallyGoldBonusAnimSeconds = 1.20f;
static const f32 kTallyGoldBonusAnimFrequency = 20.0f;
static const f32 kTallyContinuePromptScale = 0.34f;
static const f32 kTallyContinuePromptX = DEFAULT_SCREEN_WIDTH * 0.5f;
static const f32 kTallyContinuePromptY = 206.0f;
static const char* kTallyContinuePromptToken = "FE_TLC";
static const char* kTallyHeaderToken = "FE_TLHD";
static const char* kTallyFightLabelToken = "FE_TLFG";
static const char* kTallyComboLabelToken = "FE_TLCB";
static const char* kTallyStyleLabelToken = "FE_TLSY";
static const char* kTallyGradeLabelToken = "FE_TLGD";
static const char* kTallyRedDragonsLabelToken = "FE_TLRD";
static const char* kTallyGoldDragonsLabelToken = "FE_TLGO";
static const char* kTallyBonusGoldTextToken = "FE_TLBG";
static const char* kTallyDragonBonusTextToken = "FE_TLDB";
static const char* kTallyMovieBonusTextToken = "FE_TLMB";
static const char* kHudPlayerNameToken = "FE_HPNM";
static const char* kHudBossLabelToken = "FE_HBOS";
static const char* kDestSelectTitleToken = "FE_LOC_DST";
static const char* kDestLevelFormatToken = "FE_LVL";
static const char* kDestTotalGoldFormatToken = "FE_DTG";
static const char* kDestTravelToToken = "FE_DTRV";
static const char* kDestCompletedToken = "FE_DCMP";
static const char* kDestNotCompletedToken = "FE_DNCM";

// Tally number interpolation from 0 -> target.
static const f32 kTallyScoreCountRate = 8.0f;
static const f32 kTallyDragonCountRate = 6.0f;
static const s32 kTallyScoreMinStep = 1;
static const s32 kTallyDragonMinStep = 1;

static f32 HudSafeWidth() {
    const f32 screenW = SCREEN_WIDTH;
    const f32 screenH = SCREEN_HEIGHT;
    const f32 maxW16x9 = screenH * TARGET_ASPECT_RATIO;
    return (screenW > maxW16x9) ? maxW16x9 : screenW;
}

static f32 HudSafeOffsetX() {
    const f32 safeW = HudSafeWidth();
    const f32 screenW = SCREEN_WIDTH;
    if (screenW > safeW) {
        return (screenW - safeW) * 0.5f;
    }
    return 0.0f;
}

static f32 HudX(f32 x) {
    return HudSafeOffsetX() + (x / DEFAULT_SCREEN_WIDTH) * HudSafeWidth();
}

static f32 HudY(f32 y) {
    return SCREEN_SCALE_Y(y);
}

static f32 HudW(f32 w) {
    return (w / DEFAULT_SCREEN_WIDTH) * HudSafeWidth();
}

static f32 HudH(f32 h) {
    return SCREEN_SCALE_Y(h);
}

static f32 s_hudDrawAlpha = 1.0f;

static f32 ClampHudAlpha(f32 alpha) {
    if (alpha < 0.0f) {
        return 0.0f;
    }
    if (alpha > 1.0f) {
        return 1.0f;
    }
    return alpha;
}

static void SetHudDrawAlpha(f32 alpha) {
    s_hudDrawAlpha = ClampHudAlpha(alpha);
}

static u8 ApplyHudAlphaU8(u8 a) {
    if (a == 0) {
        return 0;
    }

    const s32 scaled = (s32)((f32)a * s_hudDrawAlpha + 0.5f);
    if (scaled < 0) {
        return 0;
    }
    if (scaled > 255) {
        return 255;
    }
    return (u8)scaled;
}

static f32 Clamp01(f32 value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static f32 GetHudDeltaSeconds() {
    f32 dt = g_time ? g_time->GetDeltaTime() : (1.0f / 30.0f);
    if (dt <= 0.0f || dt > 0.20f) {
        dt = 1.0f / 30.0f;
    }
    return dt;
}

static void UpdateDelayedHealthRatio(f32 targetRatio,
                                     f32 dt,
                                     f32* primaryRatio,
                                     f32* delayedRatio,
                                     f32* holdTimer,
                                     bool* outDamageTriggered = nullptr) {
    if (!primaryRatio || !delayedRatio || !holdTimer) {
        return;
    }

    if (outDamageTriggered) {
        *outDamageTriggered = false;
    }

    const f32 target = Clamp01(targetRatio);

    if (*primaryRatio < 0.0f || *delayedRatio < 0.0f) {
        *primaryRatio = target;
        *delayedRatio = target;
        *holdTimer = 0.0f;
        return;
    }

    const f32 prevPrimary = *primaryRatio;
    const bool healing = (target > *primaryRatio + kHealthDamageTriggerEpsilon);
    const f32 moveRate = healing ? kHealthHealLerpRate : kHealthDamageLerpRate;
    const f32 maxStep = moveRate * dt;

    if (*primaryRatio < target) {
        *primaryRatio += maxStep;
        if (*primaryRatio > target) {
            *primaryRatio = target;
        }
    }
    else if (*primaryRatio > target) {
        *primaryRatio -= maxStep;
        if (*primaryRatio < target) {
            *primaryRatio = target;
        }
    }

    if (healing) {
        *delayedRatio = *primaryRatio;
        *holdTimer = 0.0f;
        return;
    }

    if (target < prevPrimary - kHealthDamageTriggerEpsilon) {
        *holdTimer = kHealthDamageHoldSeconds;
        if (outDamageTriggered) {
            *outDamageTriggered = true;
        }
    }

    if (*holdTimer > 0.0f) {
        *holdTimer -= dt;
        if (*holdTimer < 0.0f) {
            *holdTimer = 0.0f;
        }
    }
    else if (*delayedRatio > *primaryRatio) {
        *delayedRatio -= kHealthDamageCatchupRate * dt;
        if (*delayedRatio < *primaryRatio) {
            *delayedRatio = *primaryRatio;
        }
    }

    if (*delayedRatio < *primaryRatio) {
        *delayedRatio = *primaryRatio;
    }
}

static void DrawHealthDamageDelta(f32 x,
                                  f32 y,
                                  f32 w,
                                  f32 h,
                                  f32 primaryRatio,
                                  f32 delayedRatio,
                                  u8 r,
                                  u8 g,
                                  u8 b,
                                  u8 a,
                                  u8 topR,
                                  u8 topG,
                                  u8 topB,
                                  u8 topA) {
    const f32 innerX = x + kHealthBarOutlineThicknessPx;
    const f32 innerY = y + kHealthBarOutlineThicknessPx;
    const f32 innerW = w - kHealthBarOutlineThicknessPx * 2.0f;
    const f32 innerH = h - kHealthBarOutlineThicknessPx * 2.0f;
    if (innerW <= 0.0f || innerH <= 0.0f) {
        return;
    }

    const f32 live = Clamp01(primaryRatio);
    const f32 delayed = Clamp01(delayedRatio);
    if (delayed <= live) {
        return;
    }

    f32 deltaX = innerX + innerW * live;
    f32 deltaW = innerW * (delayed - live);

    // Slight overlap avoids a visible seam between live fill and delayed segment.
    deltaX -= kHealthDeltaSeamOverlapPx;
    deltaW += kHealthDeltaSeamOverlapPx;

    if (deltaX < innerX) {
        deltaW -= (innerX - deltaX);
        deltaX = innerX;
    }

    const f32 maxRight = innerX + innerW * delayed;
    if (deltaX + deltaW > maxRight) {
        deltaW = maxRight - deltaX;
    }

    if (deltaW <= 0.0f) {
        return;
    }

    ScreenDraw::DrawColoredRect(deltaX, innerY, deltaW, innerH, r, g, b, ApplyHudAlphaU8(a));
    ScreenDraw::DrawColoredRect(deltaX, innerY, deltaW, 1.0f, topR, topG, topB, ApplyHudAlphaU8(topA));
}

static void UpdateDamageShakeTimer(f32 dt, bool damageTriggered, f32* timer) {
    if (!timer) {
        return;
    }

    if (damageTriggered) {
        *timer = kHealthShakeDurationSeconds;
        return;
    }

    if (*timer > 0.0f) {
        *timer -= dt;
        if (*timer < 0.0f) {
            *timer = 0.0f;
        }
    }
}

static f32 GetDamageShakeOffset(f32 timer, f32 amplitude, f32 phase) {
    if (timer <= 0.0f || amplitude <= 0.0f) {
        return 0.0f;
    }

    f32 t = timer / kHealthShakeDurationSeconds;
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    const f32 progress = 1.0f - t;
    return (f32)std::sin(progress * kHealthShakeFrequency + phase) * amplitude * t;
}

static void UpdateIncreasePulse(s32 value,
                                s32* prevValue,
                                f32* pulseTimer,
                                f32 dt) {
    if (!prevValue || !pulseTimer) {
        return;
    }

    if (*pulseTimer > 0.0f) {
        *pulseTimer -= dt;
        if (*pulseTimer < 0.0f) {
            *pulseTimer = 0.0f;
        }
    }

    if (*prevValue >= 0 && value > *prevValue) {
        *pulseTimer = kInventoryPulseSeconds;
    }

    *prevValue = value;
}

static f32 PulseAmountFromTimer(f32 timer) {
    if (timer <= 0.0f) {
        return 0.0f;
    }

    f32 t = timer / kInventoryPulseSeconds;
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    const f32 progress = 1.0f - t;
    const f32 triangle = (progress < 0.5f) ? (progress * 2.0f) : ((1.0f - progress) * 2.0f);
    return triangle * t;
}

static f32 GetInventoryPulseShakeOffset(f32 timer, f32 amplitude, f32 phase) {
    if (timer <= 0.0f || amplitude <= 0.0f) {
        return 0.0f;
    }

    f32 t = timer / kInventoryPulseSeconds;
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    const f32 progress = 1.0f - t;
    return (f32)std::sin(progress * kInventoryPulseShakeFrequency + phase) * amplitude * t;
}

static f32 GetHitTextShakeOffset(f32 timer, f32 amplitude, f32 phase) {
    if (timer <= 0.0f || amplitude <= 0.0f) {
        return 0.0f;
    }

    f32 t = timer / kHitTextShakeDurationSeconds;
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }

    const f32 progress = 1.0f - t;
    return (f32)std::sin(progress * kHitTextShakeFrequency + phase) * amplitude * t;
}

static const char* TrimLeftOrZero(const char* text) {
    if (!text) {
        return "0";
    }

    while (*text == ' ') {
        text++;
    }

    if (*text == 0) {
        return "0";
    }

    return text;
}

static s32 ParseHudCounterValue(const char* text) {
    const char* trimmed = TrimLeftOrZero(text);
    char* end = nullptr;
    long value = std::strtol(trimmed, &end, 10);
    if (end == trimmed || value <= 0) {
        return 0;
    }
    if (value > 99999999L) {
        value = 99999999L;
    }
    return (s32)value;
}

static s32 StepDisplayedCounter(s32 current, s32 target, f32 dt, f32 rate, s32 minStep) {
    if (target <= 0) {
        return 0;
    }
    if (current >= target) {
        return target;
    }

    const s32 delta = target - current;
    s32 step = minStep + (s32)((f32)delta * rate * dt);
    if (step < minStep) {
        step = minStep;
    }
    if (step > delta) {
        step = delta;
    }

    return current + step;
}

static u8 SaturatingAddU8(u8 base, s32 add) {
    s32 value = (s32)base + add;
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    return (u8)value;
}

static bool IsHubLevelActive() {
    World* world = g_game ? g_game->GetWorld() : nullptr;
    return world && world->IsCurrentLevelHub();
}

static const char* GetHudTitleFontName() {
    if (g_textManager && g_textManager->FindFont(kHudTitleFontName)) {
        return kHudTitleFontName;
    }
    return kHudBodyFontName;
}

static bool BeginHudText(const char* fontName,
                         f32 scale,
                         TextAlign align,
                         u8 r,
                         u8 g,
                         u8 b,
                         u8 a,
                         bool outline,
                         bool shadow) {
    if (!g_textManager || !g_textManager->SetFontByName(fontName)) {
        return false;
    }

    const f32 scaled = SCREEN_SCALE_Y(scale);
    g_textManager->SetScale(scaled, scaled);
    g_textManager->SetAlignment(align);
    g_textManager->SetWrapWidth(0.0f);
    g_textManager->SetLineSpacing(0);
    g_textManager->SetPromptsEnabled(true);

    // Keep text, shadow, and outline alpha in sync so fade animations don't leave hard edges.
    const u8 shadowAlpha = (a < 220) ? a : 220;
    const u8 outlineAlpha = a;
    g_textManager->SetShadow(shadow, 1.0f, 1.0f, 0, 0, 0, ApplyHudAlphaU8(shadowAlpha));
    g_textManager->SetOutline(outline, 1.0f, 0, 0, 0, ApplyHudAlphaU8(outlineAlpha));
    g_textManager->SetColor(r, g, b, ApplyHudAlphaU8(a));
    return true;
}

static void DrawFilledRect(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) {
    ScreenDraw::DrawColoredRect(HudX(x), HudY(y), HudW(w), HudH(h), r, g, b, ApplyHudAlphaU8(a));
}

static void DrawScreenFilledRect(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) {
    ScreenDraw::DrawColoredRect(x, y, w, h, r, g, b, ApplyHudAlphaU8(a));
}

static void DrawScreenOutlineRect(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a, f32 thickPx) {
    const f32 t = (thickPx < 1.0f) ? 1.0f : thickPx;
    const u8 drawA = ApplyHudAlphaU8(a);
    ScreenDraw::DrawColoredRect(x, y, w, t, r, g, b, drawA);
    ScreenDraw::DrawColoredRect(x, y + h - t, w, t, r, g, b, drawA);
    ScreenDraw::DrawColoredRect(x, y, t, h, r, g, b, drawA);
    ScreenDraw::DrawColoredRect(x + w - t, y, t, h, r, g, b, drawA);
}

static void DrawGradientRect(f32 x, f32 y, f32 w, f32 h,
                             u8 topR, u8 topG, u8 topB, u8 topA,
                             u8 botR, u8 botG, u8 botB, u8 botA) {
    const f32 x0 = HudX(x);
    const f32 y0 = HudY(y);
    const f32 x1 = HudX(x + w);
    const f32 y1 = HudY(y + h);

    ScreenDraw::DrawGouraudQuad(
        x0, y0, topR, topG, topB, ApplyHudAlphaU8(topA),
        x1, y0, topR, topG, topB, ApplyHudAlphaU8(topA),
        x0, y1, botR, botG, botB, ApplyHudAlphaU8(botA),
        x1, y1, botR, botG, botB, ApplyHudAlphaU8(botA));
}

static void DrawOutlineRect(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a, f32 thick);

static void DrawMenuDestinationBannerFrame(f32 x, f32 y, f32 w, f32 h) {
    DrawFilledRect(x, y, w, h, 0, 0, 0, kDestBannerFillA);
    DrawOutlineRect(x, y, w, h, kMenuFrameR, kMenuFrameG, kMenuFrameB, kMenuFrameA, 1.0f);
}

static void DrawOutlineRect(f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a, f32 thick) {
    const f32 x0 = HudX(x);
    const f32 y0 = HudY(y);
    const f32 drawW = HudW(w);
    const f32 drawH = HudH(h);
    const f32 t = SCREEN_SCALE_Y(thick);
    const u8 drawA = ApplyHudAlphaU8(a);

    ScreenDraw::DrawColoredRect(x0, y0, drawW, t, r, g, b, drawA);
    ScreenDraw::DrawColoredRect(x0, y0 + drawH - t, drawW, t, r, g, b, drawA);
    ScreenDraw::DrawColoredRect(x0, y0, t, drawH, r, g, b, drawA);
    ScreenDraw::DrawColoredRect(x0 + drawW - t, y0, t, drawH, r, g, b, drawA);
}

static void DrawPanelFrame(f32 x, f32 y, f32 w, f32 h) {
    DrawGradientRect(x, y, w, h,
                     8, 12, 18, 200,
                     16, 20, 30, 220);

    DrawFilledRect(x + 1.0f, y + 1.0f, w - 2.0f, 2.0f, 95, 140, 180, 120);
    DrawFilledRect(x + 1.0f, y + h - 3.0f, w - 2.0f, 2.0f, 14, 20, 30, 140);

    DrawOutlineRect(x, y, w, h, 220, 160, 80, 220, 1.0f);
    DrawOutlineRect(x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, 24, 34, 46, 255, 1.0f);
}

static void DrawIconQuad(tTexture* tex, f32 x, f32 y, f32 size,
                         u8 r = 255, u8 g = 255, u8 b = 255, u8 a = 255) {
    const f32 drawX = HudX(x);
    const f32 drawY = HudY(y);
    const f32 drawSize = SCREEN_SCALE_Y(size);
    const u8 drawA = ApplyHudAlphaU8(a);

    if (!tex) {
        ScreenDraw::DrawColoredRect(drawX, drawY, drawSize, drawSize, 35, 45, 55, ApplyHudAlphaU8(170));
        ScreenDraw::DrawColoredRect(drawX, drawY, drawSize, SCREEN_SCALE_Y(1.0f), 130, 90, 50, ApplyHudAlphaU8(220));
        ScreenDraw::DrawColoredRect(drawX, drawY + drawSize - SCREEN_SCALE_Y(1.0f), drawSize, SCREEN_SCALE_Y(1.0f), 130, 90, 50, ApplyHudAlphaU8(220));
        ScreenDraw::DrawColoredRect(drawX, drawY, SCREEN_SCALE_Y(1.0f), drawSize, 130, 90, 50, ApplyHudAlphaU8(220));
        ScreenDraw::DrawColoredRect(drawX + drawSize - SCREEN_SCALE_Y(1.0f), drawY, SCREEN_SCALE_Y(1.0f), drawSize, 130, 90, 50, ApplyHudAlphaU8(220));
        return;
    }

    ScreenDraw::DrawQuad(tex,
                         drawX,
                         drawY,
                         drawSize,
                         drawSize,
                         0.0f, 0.0f, 1.0f, 1.0f,
                         r, g, b, drawA);
}

static void DrawPulsingIconQuad(tTexture* tex,
                                f32 x,
                                f32 y,
                                f32 baseSize,
                                f32 pulseAmount,
                                f32 shakeX = 0.0f,
                                f32 shakeY = 0.0f) {
    const f32 scale = 1.0f + pulseAmount * kInventoryPulseIconScale;
    const f32 drawSize = baseSize * scale;
    const f32 offset = (baseSize - drawSize) * 0.5f;
    const f32 lift = pulseAmount * kInventoryPulseIconLift;

    DrawIconQuad(tex, x + offset + shakeX, y + offset - lift + shakeY, drawSize);
}

static void DrawProgressBar(f32 x, f32 y, f32 w, f32 h, f32 progress) {
    const f32 ratio = Clamp01(progress);
    DrawScreenFilledRect(x, y, w, h, kHealthBarBgR, kHealthBarBgG, kHealthBarBgB, kHealthBarBgA);

    const f32 fillW = w * ratio;
    if (fillW > 0.0f) {
        const f32 x0 = x;
        const f32 y0 = y;
        const f32 x1 = x + fillW;
        const f32 y1 = y + h;

        ScreenDraw::DrawGouraudQuad(
            x0, y0, kHealthBarFillTopR, kHealthBarFillTopG, kHealthBarFillTopB, ApplyHudAlphaU8(kHealthBarFillTopA),
            x1, y0, kHealthBarFillTopR, kHealthBarFillTopG, kHealthBarFillTopB, ApplyHudAlphaU8(kHealthBarFillTopA),
            x0, y1, kHealthBarFillR, kHealthBarFillG, kHealthBarFillB, ApplyHudAlphaU8(kHealthBarFillA),
            x1, y1, kHealthBarFillR, kHealthBarFillG, kHealthBarFillB, ApplyHudAlphaU8(kHealthBarFillA));
    }

    DrawScreenOutlineRect(x,
                          y,
                          w,
                          h,
                          kHealthBarOutlineR,
                          kHealthBarOutlineG,
                          kHealthBarOutlineB,
                          kHealthBarOutlineA,
                          kHealthBarOutlineThicknessPx);
}

static bool ProjectThingToScreen(const Thing* thing, s32 yOffset, f32* outX, f32* outY) {
    if (!thing || !outX || !outY || !g_display) {
        return false;
    }

    Camera* camera = g_display->GetCamera();
    if (!camera) {
        return false;
    }

    LVector world = thing->pos;
    world.y += yOffset;

    if (!camera->WorldToScreen(world, outX, outY)) {
        return false;
    }

    return true;
}

static void CopyUpperAscii(char* dst, s32 dstSize, const char* src) {
    if (!dst || dstSize <= 0) {
        return;
    }

    dst[0] = 0;
    if (!src) {
        return;
    }

    s32 i = 0;
    for (; i < dstSize - 1 && src[i] != 0; i++) {
        const unsigned char c = (unsigned char)src[i];
        dst[i] = (char)std::toupper(c);
    }
    dst[i] = 0;
}

static const char* GradeToLabel(u8 grade) {
    switch (grade) {
        case 0:
            return "E";
        case 1:
            return "D";
        case 2:
            return "C";
        case 3:
            return "B";
        case 4:
            return "A";
        case 5:
            return "A+";
        default:
            return "?";
    }
}

static void DrawTallyRow(f32 x,
                         f32 y,
                         f32 w,
                         const char* label,
                         const char* value,
                         tTexture* icon,
                         u8 valueR,
                         u8 valueG,
                         u8 valueB) {
    const f32 valueBoxX = x + w - kTallyValueBoxW;

    DrawFilledRect(valueBoxX, y, kTallyValueBoxW, kTallyRowH,
                   kTallyRowFillR, kTallyRowFillG, kTallyRowFillB, kTallyRowFillA);
    DrawOutlineRect(valueBoxX, y, kTallyValueBoxW, kTallyRowH,
                    kTallyRowOutlineR, kTallyRowOutlineG, kTallyRowOutlineB, kTallyRowOutlineA, 1.0f);

    if (icon) {
        DrawIconQuad(icon, x + 2.0f, y + 1.0f, 14.0f);
    }

    const f32 labelX = x + (icon ? 20.0f : 2.0f);

    if (BeginHudText(kHudBodyFontName,
                     kTallyLabelScale,
                     TextAlign_Left,
                     kTallyLabelR,
                     kTallyLabelG,
                     kTallyLabelB,
                     255,
                     false,
                     false)) {
        g_textManager->PrintString(label ? label : "",
                                   HudX(labelX),
                                   HudY(y + 3.0f));
    }

    if (BeginHudText(GetHudTitleFontName(), kTallyValueScale, TextAlign_Right,
                     valueR, valueG, valueB, 255, true, false)) {
        g_textManager->PrintString(value ? value : "0",
                                   HudX(valueBoxX + kTallyValueBoxW - 8.0f),
                                   HudY(y + 3.0f));
    }
}

CustomHudMgr g_customHudMgr;

CustomHudMgr::CustomHudMgr() {
}

CustomHudMgr::~CustomHudMgr() {
    Shutdown();
}

void CustomHudMgr::SetDebugTallyPreviewEnabled(bool enabled) {
    if (m_debugTallyEnabled != enabled) {
        m_debugTallyEnabled = enabled;
        m_debugTallyRestartRequested = enabled;
    }
}

void CustomHudMgr::SetDebugTallyPreviewValues(s32 fightScore,
                                              s32 comboScore,
                                              s32 styleScore,
                                              s32 redDragons,
                                              s32 goldDragons,
                                              s32 grade,
                                              bool showMovieBonus) {
    m_debugTallyFightTarget = (fightScore > 0) ? fightScore : 0;
    m_debugTallyComboTarget = (comboScore > 0) ? comboScore : 0;
    m_debugTallyStyleTarget = (styleScore > 0) ? styleScore : 0;
    m_debugTallyRDragonTarget = (redDragons > 0) ? redDragons : 0;
    m_debugTallyGDragonTarget = (goldDragons > 0) ? goldDragons : 0;

    if (grade < 0) {
        grade = 0;
    }
    if (grade > 5) {
        grade = 5;
    }
    m_debugTallyGrade = grade;
    m_debugTallyShowMovieBonus = showMovieBonus;
}

void CustomHudMgr::RestartDebugTallyPreview() {
    if (m_debugTallyEnabled) {
        m_debugTallyRestartRequested = true;
    }
}

void CustomHudMgr::EnsureAssetsLoaded() {
    if (m_assetsLoadTried) {
        return;
    }

    m_assetsLoadTried = true;

    m_takeTex = tTexture::LoadFromImagePath(kTakeTexturePath);
    m_redDragonTex = tTexture::LoadFromImagePath(kRedDragonTexturePath);
    m_goldDragonTex = tTexture::LoadFromImagePath(kGoldDragonTexturePath);
    m_greyDragonTex = tTexture::LoadFromImagePath(kGreyDragonTexturePath);
    m_bowlTex = tTexture::LoadFromImagePath(kBowlTexturePath);
    m_milkTex = tTexture::LoadFromImagePath(kMilkTexturePath);
    m_noodleTex = tTexture::LoadFromImagePath(kNoodleTexturePath);
    m_ornamentTex = tTexture::LoadFromImagePath(kOrnamentTexturePath);

    if (m_takeTex && m_takeTex->GetTexture()) {
        m_takeTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (m_redDragonTex && m_redDragonTex->GetTexture()) {
        m_redDragonTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (m_goldDragonTex && m_goldDragonTex->GetTexture()) {
        m_goldDragonTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (m_greyDragonTex && m_greyDragonTex->GetTexture()) {
        m_greyDragonTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (m_bowlTex && m_bowlTex->GetTexture()) {
        m_bowlTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (m_milkTex && m_milkTex->GetTexture()) {
        m_milkTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (m_noodleTex && m_noodleTex->GetTexture()) {
        m_noodleTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
    if (m_ornamentTex && m_ornamentTex->GetTexture()) {
        m_ornamentTex->GetTexture()->SetFilterMode(PDDI_FILTER_BILINEAR);
    }
}

void CustomHudMgr::EnsureFontsLoaded() {
    if (m_fontLoadTried) {
        return;
    }

    m_fontLoadTried = true;

    if (!g_textManager) {
        return;
    }

    if (!g_textManager->FindFont(kHudTitleFontName)) {
        TextFontDesc desc = {};
        desc.name = kHudTitleFontName;
        desc.path = kHudTitleFontPath;
        desc.pixelHeight = 52;
        g_textManager->LoadFont(desc);
    }
}

void CustomHudMgr::Shutdown() {
    if (m_takeTex) {
        m_takeTex->Release();
        m_takeTex = nullptr;
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
    if (m_bowlTex) {
        m_bowlTex->Release();
        m_bowlTex = nullptr;
    }
    if (m_milkTex) {
        m_milkTex->Release();
        m_milkTex = nullptr;
    }
    if (m_noodleTex) {
        m_noodleTex->Release();
        m_noodleTex = nullptr;
    }
    if (m_ornamentTex) {
        m_ornamentTex->Release();
        m_ornamentTex = nullptr;
    }

    m_playerHealthRatio = -1.0f;
    m_playerHealthDamageRatio = -1.0f;
    m_playerDamageHoldTimer = 0.0f;
    m_playerHealthShakeTimer = 0.0f;
    m_enemyHealthRatio = -1.0f;
    m_enemyHealthDamageRatio = -1.0f;
    m_enemyDamageHoldTimer = 0.0f;
    m_enemyHealthShakeTimer = 0.0f;
    m_enemyAnimTarget = nullptr;
    m_gameplayHudAlpha = 1.0f;

    m_prevRedDragonCount = -1;
    m_prevGoldDragonCount = -1;
    m_prevLivesCount = -1;
    m_redPulseTimer = 0.0f;
    m_goldPulseTimer = 0.0f;
    m_livesPulseTimer = 0.0f;

    m_prevHitCount = 0;
    m_hitTextShakeTimer = 0.0f;

    m_tallyVisible = false;
    m_tallyFightDisplay = 0;
    m_tallyComboDisplay = 0;
    m_tallyStyleDisplay = 0;
    m_tallyRDragonDisplay = 0;
    m_tallyGDragonDisplay = 0;
    m_tallyGradeDelayTimer = -1.0f;
    m_tallyGradeVisible = false;
    m_tallyGoldBonusAnimTimer = 0.0f;
    m_tallyGoldLeadInTimer = -1.0f;
    m_tallyGoldBonusTriggered = false;
    m_tallyMovieBonusAnimTimer = 0.0f;
    m_tallyMovieBonusWasVisible = false;
    m_tallyMovieBonusShakePhase = 0.0f;
    m_tallyPromptPulse.Start();

    m_debugTallyEnabled = false;
    m_debugTallyRestartRequested = false;
    m_debugTallyFightTarget = 12000;
    m_debugTallyComboTarget = 8500;
    m_debugTallyStyleTarget = 6200;
    m_debugTallyRDragonTarget = 18;
    m_debugTallyGDragonTarget = 9;
    m_debugTallyGrade = 3;
    m_debugTallyShowMovieBonus = false;
    m_debugTallySoundStage = 0;

    m_assetsLoadTried = false;
    m_fontLoadTried = false;
}

void CustomHudMgr::Render(const HUD& hud) {
    EnsureAssetsLoaded();
    EnsureFontsLoaded();

    const f32 dt = GetHudDeltaSeconds();
    const f32 targetHudAlpha = (hud.visible != 0) ? 1.0f : 0.0f;
    const f32 fadeStep = kHudGlobalFadeSpeed * dt;

    if (m_gameplayHudAlpha < targetHudAlpha) {
        m_gameplayHudAlpha += fadeStep;
        if (m_gameplayHudAlpha > targetHudAlpha) {
            m_gameplayHudAlpha = targetHudAlpha;
        }
    }
    else if (m_gameplayHudAlpha > targetHudAlpha) {
        m_gameplayHudAlpha -= fadeStep;
        if (m_gameplayHudAlpha < targetHudAlpha) {
            m_gameplayHudAlpha = targetHudAlpha;
        }
    }

    SetHudDrawAlpha(1.0f);

    if (g_textManager) {
        g_textManager->PushState();
    }

    const bool showTally = m_debugTallyEnabled || (hud.tally.state != 9);
    if (showTally) {
        if (!m_tallyVisible || m_debugTallyRestartRequested) {
            m_tallyFightDisplay = 0;
            m_tallyComboDisplay = 0;
            m_tallyStyleDisplay = 0;
            m_tallyRDragonDisplay = 0;
            m_tallyGDragonDisplay = 0;
            m_tallyGradeDelayTimer = -1.0f;
            m_tallyGradeVisible = false;
            m_tallyGoldBonusAnimTimer = 0.0f;
            m_tallyGoldLeadInTimer = -1.0f;
            m_tallyGoldBonusTriggered = false;
            m_tallyMovieBonusAnimTimer = 0.0f;
            m_tallyMovieBonusWasVisible = false;
            m_tallyMovieBonusShakePhase = 0.0f;
            m_tallyPromptPulse.Start();
            m_debugTallySoundStage = 0;
            m_debugTallyRestartRequested = false;
        }
        m_tallyVisible = true;
        SetHudDrawAlpha(1.0f);
        DrawTallyOverlay(hud);
    }
    else {
        m_tallyVisible = false;
        SetHudDrawAlpha(m_gameplayHudAlpha);
        DrawGameplayHud(hud);
        SetHudDrawAlpha(1.0f);
        DrawDestinationBanner(hud);
    }

    SetHudDrawAlpha(1.0f);

    if (g_textManager) {
        g_textManager->PopState();
    }
}

void CustomHudMgr::DrawGameplayHud(const HUD& hud) {
    if (hud.playerHealth.IsVisible()) {
        DrawPlayerHealthCard(hud);
    }
    else {
        m_playerHealthRatio = -1.0f;
        m_playerHealthDamageRatio = -1.0f;
        m_playerDamageHoldTimer = 0.0f;
        m_playerHealthShakeTimer = 0.0f;
    }

    const bool enemyVisible = hud.foeHealth.IsVisible() || hud.bossHealth.IsVisible();
    const bool enemyHasData =
        ((hud.currentFoe != nullptr) && (hud.foeHealth.maxHealth > 0) && (hud.foeHealth.lastValue > 0)) ||
        ((hud.bossHandle != nullptr) && (hud.bossHealth.maxHealth > 0) && (hud.bossHealth.lastValue > 0));
    if (enemyVisible || enemyHasData) {
        DrawEnemyHealthCards(hud);
    }
    else {
        m_enemyHealthRatio = -1.0f;
        m_enemyHealthDamageRatio = -1.0f;
        m_enemyDamageHoldTimer = 0.0f;
        m_enemyHealthShakeTimer = 0.0f;
        m_enemyAnimTarget = nullptr;
    }

    if (hud.visible || hud.takes.isPlaying || hud.dragon.isPlaying) {
        DrawInventoryCard(hud);
    }

    if (hud.hits.hitCount >= 2) {
        DrawHitsCard(hud);
    }
    else {
        m_hitTextShakeTimer = 0.0f;
        m_prevHitCount = (hud.hits.hitCount > 0) ? hud.hits.hitCount : 0;
    }
}

void CustomHudMgr::DrawPlayerHealthCard(const HUD& hud) {
    const char* playerNameText = g_customText.GetString(kHudPlayerNameToken);
    if (!playerNameText || playerNameText[0] == 0) {
        playerNameText = "";
    }

    if (BeginHudText(GetHudTitleFontName(),
                     kPlayerHealthNameScale,
                     TextAlign_Left,
                     kPlayerHealthNameR,
                     kPlayerHealthNameG,
                     kPlayerHealthNameB,
                     kPlayerHealthNameA,
                     true,
                     false)) {
        g_textManager->PrintString(playerNameText, HudX(kPlayerHealthNameX), HudY(kPlayerHealthNameY));
    }

    const s32 maxHealth = (hud.playerHealth.maxHealth > 0) ? hud.playerHealth.maxHealth : 1;
    const f32 ratio = Clamp01((f32)hud.playerHealth.lastValue / (f32)maxHealth);
    const f32 dt = GetHudDeltaSeconds();

    bool damageTriggered = false;
    UpdateDelayedHealthRatio(ratio,
                             dt,
                             &m_playerHealthRatio,
                             &m_playerHealthDamageRatio,
                             &m_playerDamageHoldTimer,
                             &damageTriggered);

    UpdateDamageShakeTimer(dt, damageTriggered, &m_playerHealthShakeTimer);
    const f32 shakeX = GetDamageShakeOffset(m_playerHealthShakeTimer, kPlayerHealthShakeAmpX, 0.0f);
    const f32 shakeY = GetDamageShakeOffset(m_playerHealthShakeTimer, kPlayerHealthShakeAmpY, kPlayerHealthShakePhaseY);

    const f32 drawX = HudX(kPlayerHealthBarX) + shakeX;
    const f32 drawY = HudY(kPlayerHealthBarY) + shakeY;
    const f32 drawW = HudW(kPlayerHealthBarW);
    const f32 drawH = HudH(kPlayerHealthBarH);

    DrawProgressBar(drawX, drawY, drawW, drawH, m_playerHealthRatio);
    DrawHealthDamageDelta(drawX,
                          drawY,
                          drawW,
                          drawH,
                          m_playerHealthRatio,
                          m_playerHealthDamageRatio,
                          kHealthDamageDeltaR,
                          kHealthDamageDeltaG,
                          kHealthDamageDeltaB,
                          kHealthDamageDeltaA,
                          kHealthDamageDeltaTopR,
                          kHealthDamageDeltaTopG,
                          kHealthDamageDeltaTopB,
                          kHealthDamageDeltaTopA);

    if (hud.playerHealth.flashAlpha > 0) {
        const f32 fillW = drawW * Clamp01(m_playerHealthRatio);
        if (fillW > 0.0f) {
            const u8 alpha = (hud.playerHealth.flashAlpha > 255) ? 255 : (u8)hud.playerHealth.flashAlpha;
            DrawScreenFilledRect(drawX, drawY, fillW, drawH, 255, 255, 255, alpha / 2);
        }
    }

}

void CustomHudMgr::DrawEnemyHealthCards(const HUD& hud) {
    const hdHealth* health = nullptr;
    const char* name = nullptr;
    const Thing* target = nullptr;
    bool showBossLabel = false;

    const bool bossHasData =
        (hud.bossHandle != nullptr) && (hud.bossHealth.maxHealth > 0) && (hud.bossHealth.lastValue > 0);
    const bool foeHasData =
        (hud.currentFoe != nullptr) && (hud.foeHealth.maxHealth > 0) && (hud.foeHealth.lastValue > 0);

    if (hud.bossHealth.IsVisible() || bossHasData) {
        const char* bossLabelText = g_customText.GetString(kHudBossLabelToken);
        if (!bossLabelText || bossLabelText[0] == 0) {
            bossLabelText = "";
        }

        health = &hud.bossHealth;
        name = (HUD::szBossStatic[0] != 0) ? HUD::szBossStatic : bossLabelText;
        showBossLabel = true;
        if (hud.bossHandle) {
            target = hud.bossHandle->owner;
        }
    }
    else if (hud.foeHealth.IsVisible() || foeHasData) {
        health = &hud.foeHealth;
        name = nullptr;
        target = static_cast<const Thing*>(hud.currentFoe);
    }

    if (!health) {
        return;
    }

    if (!target) {
        return;
    }

    f32 screenX = 0.0f;
    f32 screenY = 0.0f;
    if (!ProjectThingToScreen(target, 1100, &screenX, &screenY)) {
        return;
    }

    const s32 maxHealth = (health->maxHealth > 0) ? health->maxHealth : 1;
    const s32 curHealth = health->lastValue;
    const f32 ratio = Clamp01((f32)curHealth / (f32)maxHealth);

    if (target != m_enemyAnimTarget) {
        m_enemyAnimTarget = target;
        m_enemyHealthRatio = ratio;

        const bool initFromFull = (health->flashAlpha > 0) && (ratio < (1.0f - kHealthDamageTriggerEpsilon));
        m_enemyHealthDamageRatio = initFromFull ? 1.0f : ratio;
        m_enemyDamageHoldTimer = initFromFull ? kHealthDamageHoldSeconds : 0.0f;
    }

    const f32 dt = GetHudDeltaSeconds();

    bool damageTriggered = false;
    UpdateDelayedHealthRatio(ratio,
                             dt,
                             &m_enemyHealthRatio,
                             &m_enemyHealthDamageRatio,
                             &m_enemyDamageHoldTimer,
                             &damageTriggered);

    UpdateDamageShakeTimer(dt, damageTriggered, &m_enemyHealthShakeTimer);
    const f32 shakeX = GetDamageShakeOffset(m_enemyHealthShakeTimer, kEnemyHealthShakeAmpX, 0.0f);
    const f32 shakeY = GetDamageShakeOffset(m_enemyHealthShakeTimer, kEnemyHealthShakeAmpY, kEnemyHealthShakePhaseY);

    const f32 barW = SCREEN_HEIGHT * kEnemyHealthBarWFromScreenH;
    const f32 barH = SCREEN_HEIGHT * kEnemyHealthBarHFromScreenH;
    const f32 barX = screenX - barW * 0.5f + shakeX;
    const f32 barY = screenY - SCREEN_HEIGHT * kEnemyHealthBarYOffsetFromScreenH + shakeY;

    if (showBossLabel && name && name[0]) {
        char displayName[48] = {};
        CopyUpperAscii(displayName, (s32)sizeof(displayName), name);
        if (BeginHudText(kHudBodyFontName,
                         kEnemyHealthLabelScale,
                         TextAlign_Center,
                         kBossHealthLabelR,
                         kBossHealthLabelG,
                         kBossHealthLabelB,
                         kBossHealthLabelA,
                         true,
                         false)) {
            g_textManager->PrintString(displayName,
                                       barX + barW * 0.5f,
                                       barY - SCREEN_HEIGHT * kEnemyHealthLabelYOffsetFromScreenH);
        }
    }

    DrawProgressBar(barX, barY, barW, barH, m_enemyHealthRatio);
    DrawHealthDamageDelta(barX,
                          barY,
                          barW,
                          barH,
                          m_enemyHealthRatio,
                          m_enemyHealthDamageRatio,
                          kHealthDamageDeltaR,
                          kHealthDamageDeltaG,
                          kHealthDamageDeltaB,
                          kHealthDamageDeltaA,
                          kHealthDamageDeltaTopR,
                          kHealthDamageDeltaTopG,
                          kHealthDamageDeltaTopB,
                          kHealthDamageDeltaTopA);

    if (health->flashAlpha > 0) {
        const f32 fillW = barW * Clamp01(m_enemyHealthRatio);
        if (fillW > 0.0f) {
            const u8 alpha = (health->flashAlpha > 255) ? 255 : (u8)health->flashAlpha;
            DrawScreenFilledRect(barX, barY, fillW, barH, 255, 255, 255, alpha / 2);
        }
    }
}

void CustomHudMgr::DrawInventoryCard(const HUD& hud) {
    const bool showRedDragonCounter = !IsHubLevelActive();
    const f32 iconSize = 16.0f;
    const f32 topY = 7.0f;
    const f32 slotW = 31.0f;
    const f32 rightEdge = DEFAULT_SCREEN_WIDTH - 6.0f;
    const f32 panelPadX = 2.0f;
    const s32 slotCount = showRedDragonCounter ? 3 : 2;
    const f32 panelW = slotW * (f32)slotCount + panelPadX * 2.0f;
    const f32 panelX = rightEdge - panelW;
    const f32 dt = GetHudDeltaSeconds();

    const f32 livesX = panelX + panelPadX + slotW * (f32)(slotCount - 1);
    const f32 goldX = livesX - slotW;
    const f32 redX = goldX - slotW;

    s32 redDragons = hud.dragon.dragonCount;
    if (redDragons < 0) {
        redDragons = 0;
    }

    s32 goldDragons = hud.dragon.goldDragonFlag ? 1 : 0;
    if (g_scoreManager) {
        goldDragons = g_scoreManager->currentGoldDragons;
    }
    if (goldDragons < 0) {
        goldDragons = 0;
    }

    const char* livesText = TrimLeftOrZero(hud.takes.numberBuf);
    s32 livesCount = std::atoi(livesText);
    if (livesCount < 0) {
        livesCount = 0;
    }

    UpdateIncreasePulse(redDragons, &m_prevRedDragonCount, &m_redPulseTimer, dt);
    UpdateIncreasePulse(goldDragons, &m_prevGoldDragonCount, &m_goldPulseTimer, dt);
    UpdateIncreasePulse(livesCount, &m_prevLivesCount, &m_livesPulseTimer, dt);

    const f32 redPulse = PulseAmountFromTimer(m_redPulseTimer);
    const f32 goldPulse = PulseAmountFromTimer(m_goldPulseTimer);
    const f32 livesPulse = PulseAmountFromTimer(m_livesPulseTimer);

    const f32 redIconShakeX = GetInventoryPulseShakeOffset(m_redPulseTimer, kInventoryIconShakeAmpX, kInventoryRedShakePhaseX);
    const f32 redIconShakeY = GetInventoryPulseShakeOffset(m_redPulseTimer, kInventoryIconShakeAmpY, kInventoryRedShakePhaseY);
    const f32 goldIconShakeX = GetInventoryPulseShakeOffset(m_goldPulseTimer, kInventoryIconShakeAmpX, kInventoryGoldShakePhaseX);
    const f32 goldIconShakeY = GetInventoryPulseShakeOffset(m_goldPulseTimer, kInventoryIconShakeAmpY, kInventoryGoldShakePhaseY);
    const f32 livesIconShakeX = GetInventoryPulseShakeOffset(m_livesPulseTimer, kInventoryIconShakeAmpX, kInventoryLivesShakePhaseX);
    const f32 livesIconShakeY = GetInventoryPulseShakeOffset(m_livesPulseTimer, kInventoryIconShakeAmpY, kInventoryLivesShakePhaseY);

    const f32 redTextShakeX = GetInventoryPulseShakeOffset(m_redPulseTimer,
                                                           kInventoryTextShakeAmpX,
                                                           kInventoryRedShakePhaseX + kInventoryTextShakePhaseOffsetX);
    const f32 goldTextShakeX = GetInventoryPulseShakeOffset(m_goldPulseTimer,
                                                            kInventoryTextShakeAmpX,
                                                            kInventoryGoldShakePhaseX + kInventoryTextShakePhaseOffsetX);
    const f32 livesTextShakeX = GetInventoryPulseShakeOffset(m_livesPulseTimer,
                                                             kInventoryTextShakeAmpX,
                                                             kInventoryLivesShakePhaseX + kInventoryTextShakePhaseOffsetX);

    const f32 redCenterY = topY + iconSize * 0.5f - redPulse * kInventoryPulseIconLift + redIconShakeY;
    const f32 goldCenterY = topY + iconSize * 0.5f - goldPulse * kInventoryPulseIconLift + goldIconShakeY;
    const f32 livesCenterY = topY + iconSize * 0.5f - livesPulse * kInventoryPulseIconLift + livesIconShakeY;

    char redBuf[12] = {};
    char goldBuf[12] = {};
    std::snprintf(redBuf, sizeof(redBuf), "%d", redDragons);
    std::snprintf(goldBuf, sizeof(goldBuf), "%d", goldDragons);

    if (showRedDragonCounter) {
        DrawPulsingIconQuad(m_redDragonTex ? m_redDragonTex : m_greyDragonTex,
                            redX,
                            topY,
                            iconSize,
                            redPulse,
                            redIconShakeX,
                            redIconShakeY);
    }
    DrawPulsingIconQuad(m_goldDragonTex ? m_goldDragonTex : m_redDragonTex,
                        goldX,
                        topY,
                        iconSize,
                        goldPulse,
                        goldIconShakeX,
                        goldIconShakeY);
    DrawPulsingIconQuad(m_takeTex,
                        livesX,
                        topY,
                        iconSize,
                        livesPulse,
                        livesIconShakeX,
                        livesIconShakeY);

    if (showRedDragonCounter) {
        const s32 redBoost = (s32)(redPulse * kInventoryPulseTextColorBoost);
        const f32 redTextScale = 1.0f + redPulse * kInventoryPulseTextScale;
        if (BeginHudText(GetHudTitleFontName(),
                         0.30f * redTextScale,
                         TextAlign_Left,
                         255,
                         SaturatingAddU8(244, redBoost),
                         SaturatingAddU8(214, redBoost),
                         255,
                         true,
                         false)) {
            g_textManager->PrintString(redBuf,
                                       HudX(redX + iconSize + kInventoryTextGapFromIcon + redTextShakeX),
                                       HudY(redCenterY - kInventoryTextCenterToPrintYOffset * redTextScale));
        }
    }

    const s32 goldBoost = (s32)(goldPulse * kInventoryPulseTextColorBoost);
    const f32 goldTextScale = 1.0f + goldPulse * kInventoryPulseTextScale;
    if (BeginHudText(GetHudTitleFontName(),
                     0.30f * goldTextScale,
                     TextAlign_Left,
                     255,
                     SaturatingAddU8(244, goldBoost),
                     SaturatingAddU8(214, goldBoost),
                     255,
                     true,
                     false)) {
        g_textManager->PrintString(goldBuf,
                                   HudX(goldX + iconSize + kInventoryTextGapFromIcon + goldTextShakeX),
                                   HudY(goldCenterY - kInventoryTextCenterToPrintYOffset * goldTextScale));
    }

    const s32 livesBoost = (s32)(livesPulse * kInventoryPulseTextColorBoost);
    const f32 livesTextScale = 1.0f + livesPulse * kInventoryPulseTextScale;
    if (BeginHudText(GetHudTitleFontName(),
                     0.30f * livesTextScale,
                     TextAlign_Left,
                     255,
                     SaturatingAddU8(244, livesBoost),
                     SaturatingAddU8(214, livesBoost),
                     255,
                     true,
                     false)) {
        g_textManager->PrintString(livesText,
                                   HudX(livesX + iconSize + kInventoryTextGapFromIcon + livesTextShakeX),
                                   HudY(livesCenterY - kInventoryTextCenterToPrintYOffset * livesTextScale));
    }
}

void CustomHudMgr::DrawHitsCard(const HUD& hud) {
    s32 hitCount = (hud.hits.hitCount > 0) ? hud.hits.hitCount : 0;
    const s32 parsedHits = ParseHudCounterValue(hud.hits.hitsBuf);
    if (parsedHits > hitCount) {
        hitCount = parsedHits;
    }

    const f32 dt = GetHudDeltaSeconds();

    if (hitCount > m_prevHitCount) {
        m_hitTextShakeTimer = kHitTextShakeDurationSeconds;
    }

    const Thing* anchor = nullptr;
    if (Player::s_player) {
        anchor = static_cast<const Thing*>(Player::s_player);
    }
    else if (hud.currentFoe) {
        anchor = static_cast<const Thing*>(hud.currentFoe);
    }

    f32 screenX = 0.0f;
    f32 screenY = 0.0f;
    if (!ProjectThingToScreen(anchor, 950, &screenX, &screenY)) {
        m_prevHitCount = hitCount;
        return;
    }
    screenY -= SCREEN_HEIGHT * 0.060f;

    const f32 shakeScale = (SCREEN_HEIGHT > 0.0f) ? (SCREEN_HEIGHT / DEFAULT_SCREEN_HEIGHT) : 1.0f;
    const f32 shakeX = GetHitTextShakeOffset(m_hitTextShakeTimer, kHitTextShakeAmpX * shakeScale, 0.0f);
    const f32 shakeY = GetHitTextShakeOffset(m_hitTextShakeTimer, kHitTextShakeAmpY * shakeScale, kHitTextShakePhaseY);

    const char* comboText = TrimLeftOrZero(hud.hits.hitsBuf);
    if (BeginHudText(GetHudTitleFontName(), 0.28f, TextAlign_Center,
                     (u8)hud.hits.colorR,
                     (u8)hud.hits.colorG,
                     (u8)hud.hits.colorB,
                     255,
                     true,
                     false)) {
        g_textManager->PrintString(comboText, screenX + shakeX, screenY + shakeY);
    }

    if (m_hitTextShakeTimer > 0.0f) {
        m_hitTextShakeTimer -= dt;
        if (m_hitTextShakeTimer < 0.0f) {
            m_hitTextShakeTimer = 0.0f;
        }
    }

    m_prevHitCount = hitCount;
}

void CustomHudMgr::DrawDestinationBanner(const HUD& hud) const {
    const bool titleVisible =
        (hud.destSelect.titleOvl1 && hud.destSelect.titleOvl1->visibility != 0) ||
        (hud.destSelect.titleOvl2 && hud.destSelect.titleOvl2->visibility != 0);

    if (!titleVisible && !hud.destSelect.ttlive.IsVisible()) {
        return;
    }

    const f32 w = 214.0f;
    const f32 h = 32.0f;
    const f32 x = (DEFAULT_SCREEN_WIDTH - w) * 0.5f;
    const f32 y = 8.0f;

    const char* destSelectTitleText = g_customText.GetString(kDestSelectTitleToken);
    const char* destLevelFormatText = g_customText.GetString(kDestLevelFormatToken);
    const char* destTotalGoldFormatText = g_customText.GetString(kDestTotalGoldFormatToken);
    const char* destTravelToText = g_customText.GetString(kDestTravelToToken);
    const char* destCompletedText = g_customText.GetString(kDestCompletedToken);
    const char* destNotCompletedText = g_customText.GetString(kDestNotCompletedToken);

    if (!destSelectTitleText || destSelectTitleText[0] == 0) {
        destSelectTitleText = "";
    }
    if (!destLevelFormatText || destLevelFormatText[0] == 0) {
        destLevelFormatText = "";
    }
    if (!destTotalGoldFormatText || destTotalGoldFormatText[0] == 0) {
        destTotalGoldFormatText = "";
    }
    if (!destTravelToText || destTravelToText[0] == 0) {
        destTravelToText = "";
    }
    if (!destCompletedText || destCompletedText[0] == 0) {
        destCompletedText = "";
    }
    if (!destNotCompletedText || destNotCompletedText[0] == 0) {
        destNotCompletedText = "";
    }

    DrawMenuDestinationBannerFrame(x, y, w, h);

    const s32 currentLevel = hud.destSelect.currentLevel;
    if (currentLevel <= 0) {
        if (BeginHudText(GetHudTitleFontName(), 0.35f, TextAlign_Center,
                         kMenuTitleTextR, kMenuTitleTextG, kMenuTitleTextB, 255, true, false)) {
            g_textManager->PrintString(destSelectTitleText, HudX(x + w * 0.5f), HudY(y + 5.0f));
        }

        s32 totalGold = 0;
        if (g_scoreManager) {
            totalGold = g_scoreManager->GetTotalGoldDragon();
        }

        char line[64];
        std::snprintf(line, sizeof(line), destTotalGoldFormatText, totalGold);
        if (BeginHudText(kHudBodyFontName, 0.20f, TextAlign_Center,
                         kMenuTextNormR, kMenuTextNormG, kMenuTextNormB, 255, false, false)) {
            g_textManager->PrintString(line, HudX(x + w * 0.5f), HudY(y + 20.0f));
        }
        return;
    }

    World* world = g_game ? g_game->GetWorld() : nullptr;
    s32 levelIndex = -1;
    const char* levelName = nullptr;
    if (world) {
        const s32 candidate = world->LevelIDToIndex(currentLevel);
        if (candidate >= 0 && world->GetLevelIDFromIndex((u32)candidate) == currentLevel) {
            levelIndex = candidate;
            levelName = world->GetLevelNameFromIndex((u32)levelIndex);
        }
    }

    char levelBuf[64];
    if (levelName && levelName[0]) {
        CopyUpperAscii(levelBuf, (s32)sizeof(levelBuf), levelName);
    }
    else {
        std::snprintf(levelBuf, sizeof(levelBuf), destLevelFormatText, currentLevel);
    }

    if (BeginHudText(kHudBodyFontName, 0.20f, TextAlign_Center,
                     kMenuTextNormR, kMenuTextNormG, kMenuTextNormB, 255, false, false)) {
        g_textManager->PrintString(destTravelToText, HudX(x + w * 0.5f), HudY(y + 4.0f));
    }

    if (BeginHudText(GetHudTitleFontName(), 0.37f, TextAlign_Center,
                     kMenuTitleTextR, kMenuTitleTextG, kMenuTitleTextB, 255, true, false)) {
        g_textManager->PrintString(levelBuf, HudX(x + w * 0.5f), HudY(y + 12.0f));
    }

    bool completed = false;
    if (levelIndex >= 0 && g_scoreManager) {
        PetalStats* ps = &g_scoreManager->petalStats[levelIndex * 3];
        completed = (ps[0].goldDragons != 0) || (ps[1].goldDragons != 0) || (ps[2].goldDragons != 0);
    }

    if (BeginHudText(kHudBodyFontName, 0.20f, TextAlign_Center,
                     kMenuTextNormR,
                     kMenuTextNormG,
                     kMenuTextNormB,
                     255,
                     false,
                     false)) {
        g_textManager->PrintString(completed ? destCompletedText : destNotCompletedText,
                                   HudX(x + w * 0.5f),
                                   HudY(y + 24.0f));
    }
}

void CustomHudMgr::DrawTallyOverlay(const HUD& hud) {
    const bool useDebugPreview = m_debugTallyEnabled;
    const s32 tallyState = hud.tally.state;
    const s32 tallyFrameCounter = hud.tally.frameCounter;
    const bool nonDebugRDragonStarted = (tallyState > 4) || (tallyState == 4 && tallyFrameCounter == 0);
    const bool nonDebugGoldBonusStarted = (tallyState > 5) || (tallyState == 5 && tallyFrameCounter == 0);
    const bool nonDebugGDragonStarted = (tallyState > 6) || (tallyState == 6 && tallyFrameCounter == 0);
    const bool showRDragons = useDebugPreview || nonDebugRDragonStarted;
    const bool showGoldBonusStage = useDebugPreview || nonDebugGoldBonusStarted;
    const bool showGDragonRow = useDebugPreview || nonDebugGDragonStarted;

    const f32 x = kTallyPanelX;
    const f32 y = kTallyPanelY;
    const f32 w = kTallyPanelW;
    const f32 h = kTallyPanelH;

    const char* tallyHeaderText = g_customText.GetString(kTallyHeaderToken);
    const char* fightLabelText = g_customText.GetString(kTallyFightLabelToken);
    const char* comboLabelText = g_customText.GetString(kTallyComboLabelToken);
    const char* styleLabelText = g_customText.GetString(kTallyStyleLabelToken);
    const char* gradeLabelText = g_customText.GetString(kTallyGradeLabelToken);
    const char* redDragonsLabelText = g_customText.GetString(kTallyRedDragonsLabelToken);
    const char* goldDragonsLabelText = g_customText.GetString(kTallyGoldDragonsLabelToken);
    const char* bonusGoldText = g_customText.GetString(kTallyBonusGoldTextToken);
    const char* dragonBonusText = g_customText.GetString(kTallyDragonBonusTextToken);
    const char* movieBonusText = g_customText.GetString(kTallyMovieBonusTextToken);

    if (!tallyHeaderText || tallyHeaderText[0] == 0) {
        tallyHeaderText = "";
    }
    if (!fightLabelText || fightLabelText[0] == 0) {
        fightLabelText = "";
    }
    if (!comboLabelText || comboLabelText[0] == 0) {
        comboLabelText = "";
    }
    if (!styleLabelText || styleLabelText[0] == 0) {
        styleLabelText = "";
    }
    if (!gradeLabelText || gradeLabelText[0] == 0) {
        gradeLabelText = "";
    }
    if (!redDragonsLabelText || redDragonsLabelText[0] == 0) {
        redDragonsLabelText = "";
    }
    if (!goldDragonsLabelText || goldDragonsLabelText[0] == 0) {
        goldDragonsLabelText = "";
    }
    if (!bonusGoldText || bonusGoldText[0] == 0) {
        bonusGoldText = "";
    }
    if (!dragonBonusText || dragonBonusText[0] == 0) {
        dragonBonusText = "";
    }
    if (!movieBonusText || movieBonusText[0] == 0) {
        movieBonusText = "";
    }

    DrawOutlineRect(x, y, w, h,
                    kTallyPanelOutlineR, kTallyPanelOutlineG, kTallyPanelOutlineB, kTallyPanelOutlineA, 1.0f);

    const f32 panelInnerX = x + 1.0f;
    const f32 panelInnerW = w - 2.0f;
    const f32 bodyY0 = y + kTallyPanelTopBarH;
    const f32 bodyY1 = y + h - 1.0f;

    const f32 topInnerH = kTallyPanelTopBarH - 1.0f;
    const f32 topHalfH = topInnerH * 0.5f;
    DrawGradientRect(panelInnerX, y + 1.0f, panelInnerW, topHalfH,
                     kTallyPanelBarEdgeR, kTallyPanelBarEdgeG, kTallyPanelBarEdgeB, kTallyPanelBarA,
                     kTallyPanelBarMidR, kTallyPanelBarMidG, kTallyPanelBarMidB, kTallyPanelBarA);
    DrawGradientRect(panelInnerX, y + 1.0f + topHalfH, panelInnerW, topInnerH - topHalfH,
                     kTallyPanelBarMidR, kTallyPanelBarMidG, kTallyPanelBarMidB, kTallyPanelBarA,
                     kTallyPanelBarEdgeR, kTallyPanelBarEdgeG, kTallyPanelBarEdgeB, kTallyPanelBarA);
    
    DrawFilledRect(panelInnerX, bodyY0, panelInnerW, bodyY1 - bodyY0,
                   kTallyPanelBodyR, kTallyPanelBodyG, kTallyPanelBodyB, kTallyPanelBodyA);
    DrawFilledRect(panelInnerX, bodyY0, panelInnerW, 1.0f,
                   kTallyPanelOutlineR, kTallyPanelOutlineG, kTallyPanelOutlineB, kTallyPanelOutlineA);
    
    DrawFilledRect(x + 10.0f, y + 5.0f, w - 20.0f, 18.0f, 0, 0, 0, 255);
    DrawOutlineRect(x + 10.0f, y + 5.0f, w - 20.0f, 18.0f,
                    kTallyPanelOutlineR, kTallyPanelOutlineG, kTallyPanelOutlineB, 255, 1.0f);

    if (BeginHudText(kHudBodyFontName, kTallyHeaderTextScale, TextAlign_Center,
                     kTallyTitleR, kTallyTitleG, kTallyTitleB, 255, true, false)) {
        g_textManager->PrintString(tallyHeaderText, HudX(x + w * 0.5f), HudY(y + 9.0f));
    }

    const f32 dt = GetHudDeltaSeconds();
    s32 fightTarget = 0;
    s32 comboTarget = 0;
    s32 styleTarget = 0;
    s32 rdragonTarget = 0;
    s32 gdragonTarget = 0;

    if (useDebugPreview) {
        fightTarget = m_debugTallyFightTarget;
        comboTarget = m_debugTallyComboTarget;
        styleTarget = m_debugTallyStyleTarget;
        rdragonTarget = m_debugTallyRDragonTarget;
        gdragonTarget = m_debugTallyGDragonTarget;
    }
    else if (g_scoreManager) {
        const s32 collectCount = g_scoreManager->currentCollectCount;
        fightTarget = g_scoreManager->currentFightScore;
        comboTarget = g_scoreManager->currentComboScore;
        styleTarget = g_scoreManager->currentStyleScore;
        rdragonTarget = collectCount;
        gdragonTarget = g_scoreManager->CalcGDrags(collectCount) ? 1 : 0;
    }
    else {
        fightTarget = ParseHudCounterValue(hud.tally.fightScoreBuf);
        comboTarget = ParseHudCounterValue(hud.tally.comboScoreBuf);
        styleTarget = ParseHudCounterValue(hud.tally.styleScoreBuf);
        rdragonTarget = ParseHudCounterValue(hud.tally.rdragonBuf);
        gdragonTarget = (rdragonTarget >= 10) ? 1 : 0;
    }

    const bool showGoldBonus = showGoldBonusStage && (gdragonTarget > 0);

    if (useDebugPreview && m_debugTallySoundStage == 0 && g_frontEndSound) {
        g_frontEndSound->ProcessSoundEvent(kTallySoundStart);
        m_debugTallySoundStage = 1;
    }

    m_tallyFightDisplay = StepDisplayedCounter(m_tallyFightDisplay, fightTarget, dt, kTallyScoreCountRate, kTallyScoreMinStep);
    if (m_tallyFightDisplay >= fightTarget) {
        m_tallyComboDisplay = StepDisplayedCounter(m_tallyComboDisplay, comboTarget, dt, kTallyScoreCountRate, kTallyScoreMinStep);
    }
    else {
        m_tallyComboDisplay = 0;
    }

    if (m_tallyFightDisplay >= fightTarget && m_tallyComboDisplay >= comboTarget) {
        m_tallyStyleDisplay = StepDisplayedCounter(m_tallyStyleDisplay, styleTarget, dt, kTallyScoreCountRate, kTallyScoreMinStep);
    }
    else {
        m_tallyStyleDisplay = 0;
    }

    const bool scoreRowsFilled =
        (m_tallyFightDisplay >= fightTarget) &&
        (m_tallyComboDisplay >= comboTarget) &&
        (m_tallyStyleDisplay >= styleTarget);

    if (useDebugPreview) {
        if (scoreRowsFilled) {
            m_tallyGradeDelayTimer = 0.0f;
            m_tallyGradeVisible = true;
        }
        else {
            m_tallyGradeDelayTimer = -1.0f;
            m_tallyGradeVisible = false;
        }
    }
    else {
        m_tallyGradeDelayTimer = (tallyState >= 3) ? 0.0f : -1.0f;
        m_tallyGradeVisible = tallyState >= 3;
    }

    const bool gradeRevealed = m_tallyGradeVisible;
    const bool canCountRDragons = useDebugPreview
        ? (showRDragons && scoreRowsFilled && gradeRevealed)
        : (showRDragons && gradeRevealed);

    if (canCountRDragons) {
        m_tallyRDragonDisplay = StepDisplayedCounter(m_tallyRDragonDisplay, rdragonTarget, dt, kTallyDragonCountRate, kTallyDragonMinStep);
    }
    else {
        m_tallyRDragonDisplay = 0;
    }

    const bool redDragonsFinished =
        showGoldBonus &&
        gradeRevealed &&
        (m_tallyRDragonDisplay >= rdragonTarget);

    if (redDragonsFinished) {
        if (m_tallyGoldLeadInTimer < 0.0f) {
            m_tallyGoldLeadInTimer = kTallyGoldLeadInSeconds;
        }
        else if (m_tallyGoldLeadInTimer > 0.0f) {
            m_tallyGoldLeadInTimer -= dt;
            if (m_tallyGoldLeadInTimer < 0.0f) {
                m_tallyGoldLeadInTimer = 0.0f;
            }
        }
    }
    else {
        m_tallyGoldLeadInTimer = -1.0f;
    }

    const bool canTriggerGoldBonus =
        showGoldBonus &&
        redDragonsFinished &&
        (m_tallyGoldLeadInTimer <= 0.0f);

    if (canTriggerGoldBonus) {
        if (!m_tallyGoldBonusTriggered) {
            m_tallyGoldBonusTriggered = true;
            m_tallyGoldBonusAnimTimer = kTallyGoldBonusAnimSeconds;
        }
    }
    else if (!showGoldBonus) {
        m_tallyGoldBonusTriggered = false;
        m_tallyGoldBonusAnimTimer = 0.0f;
    }

    const bool canCountGoldBonus = showGDragonRow && canTriggerGoldBonus;

    if (canCountGoldBonus) {
        m_tallyGDragonDisplay = StepDisplayedCounter(m_tallyGDragonDisplay, gdragonTarget, dt, kTallyDragonCountRate, kTallyDragonMinStep);
    }
    else {
        m_tallyGDragonDisplay = 0;
    }

    if (m_tallyGoldBonusAnimTimer > 0.0f) {
        m_tallyGoldBonusAnimTimer -= dt;
        if (m_tallyGoldBonusAnimTimer < 0.0f) {
            m_tallyGoldBonusAnimTimer = 0.0f;
        }
    }

    const bool goldDragonCountDone =
        (gdragonTarget > 0) &&
        canCountGoldBonus &&
        (m_tallyGDragonDisplay >= gdragonTarget);
    const bool goldBonusAnimDone = (m_tallyGoldBonusAnimTimer <= 0.0f);
    const bool movieBonusTimingReady =
        (gdragonTarget > 0)
            ? (goldDragonCountDone && goldBonusAnimDone)
            : (showRDragons && (m_tallyRDragonDisplay >= rdragonTarget));
    const bool rawMovieBonusVisible = useDebugPreview ? m_debugTallyShowMovieBonus : hud.tally.movieBonusOvl.IsVisible();
    const bool movieBonusVisible = rawMovieBonusVisible && movieBonusTimingReady;
    if (movieBonusVisible) {
        if (!m_tallyMovieBonusWasVisible) {
            m_tallyMovieBonusWasVisible = true;
            m_tallyMovieBonusAnimTimer = kTallyMovieBonusAnimSeconds;
        }
        else if (m_tallyMovieBonusAnimTimer > 0.0f) {
            m_tallyMovieBonusAnimTimer -= dt;
            if (m_tallyMovieBonusAnimTimer < 0.0f) {
                m_tallyMovieBonusAnimTimer = 0.0f;
            }
        }
    }
    else {
        m_tallyMovieBonusWasVisible = false;
        m_tallyMovieBonusAnimTimer = 0.0f;
    }

    if (useDebugPreview && g_frontEndSound) {
        if (m_debugTallySoundStage == 1 && m_tallyFightDisplay >= fightTarget) {
            g_frontEndSound->ProcessSoundEvent(kTallySoundFightDone);
            m_debugTallySoundStage = 2;
        }
        if (m_debugTallySoundStage == 2 && m_tallyComboDisplay >= comboTarget) {
            g_frontEndSound->ProcessSoundEvent(kTallySoundComboDone);
            m_debugTallySoundStage = 3;
        }
        if (m_debugTallySoundStage == 3 && m_tallyStyleDisplay >= styleTarget) {
            g_frontEndSound->ProcessSoundEvent(kTallySoundStyleDone);
            m_debugTallySoundStage = 4;
        }
        if (m_debugTallySoundStage == 4 && gradeRevealed) {
            g_frontEndSound->ProcessSoundEvent(kTallySoundGradeDone);
            m_debugTallySoundStage = 5;
        }
        if (m_debugTallySoundStage == 5 && showRDragons && scoreRowsFilled && gradeRevealed) {
            g_frontEndSound->ProcessSoundEvent(kTallySoundRDragonDone);
            m_debugTallySoundStage = showGoldBonus ? 6 : 9;
        }

        if (m_debugTallySoundStage == 6 && showGoldBonus && m_tallyGoldBonusTriggered) {
            g_frontEndSound->ProcessSoundEvent(kTallySoundGoldBonus);
            m_debugTallySoundStage = 7;
        }

        if (m_debugTallySoundStage == 7 && showGoldBonus && m_tallyGDragonDisplay >= gdragonTarget) {
            g_frontEndSound->ProcessSoundEvent(kTallySoundGDragonDone);
            m_debugTallySoundStage = 9;
        }
    }

    char fightBuf[16] = {};
    char comboBuf[16] = {};
    char styleBuf[16] = {};
    char rdragonBuf[16] = {};
    char gdragonBuf[16] = {};
    std::snprintf(fightBuf, sizeof(fightBuf), "%d", m_tallyFightDisplay);
    std::snprintf(comboBuf, sizeof(comboBuf), "%d", m_tallyComboDisplay);
    std::snprintf(styleBuf, sizeof(styleBuf), "%d", m_tallyStyleDisplay);
    std::snprintf(rdragonBuf, sizeof(rdragonBuf), "%d", m_tallyRDragonDisplay);
    std::snprintf(gdragonBuf, sizeof(gdragonBuf), "%d", m_tallyGDragonDisplay);

    f32 rowY = y + 32.0f;
    DrawTallyRow(x + 10.0f, rowY, w - 20.0f, fightLabelText, fightBuf, nullptr,
                 kTallyValueGoldR, kTallyValueGoldG, kTallyValueGoldB);
    rowY += kTallyRowSpacing;
    DrawTallyRow(x + 10.0f, rowY, w - 20.0f, comboLabelText, comboBuf, nullptr,
                 kTallyValueGoldR, kTallyValueGoldG, kTallyValueGoldB);
    rowY += kTallyRowSpacing;
    DrawTallyRow(x + 10.0f, rowY, w - 20.0f, styleLabelText, styleBuf, nullptr,
                 kTallyValueGoldR, kTallyValueGoldG, kTallyValueGoldB);
    rowY += kTallyRowSpacing;

    const u8 grade = useDebugPreview ? (u8)m_debugTallyGrade : (g_scoreManager ? g_scoreManager->CalcGrade() : 0);
    const char* gradeText = gradeRevealed ? GradeToLabel(grade) : "--";
    DrawTallyRow(x + 10.0f, rowY, w - 20.0f, gradeLabelText, gradeText, nullptr,
                 kTallyValueGoldR, kTallyValueGoldG, kTallyValueGoldB);
    rowY += kTallyRowSpacing;

    if (showRDragons || showGoldBonus) {
        rowY += kTallyDragonRowsYOffset;
    }

    if (showRDragons) {
        DrawTallyRow(x + 10.0f, rowY, w - 20.0f, redDragonsLabelText, rdragonBuf, m_redDragonTex,
                     kTallyValueGoldR, kTallyValueGoldG, kTallyValueGoldB);
        rowY += kTallyRowSpacing;
    }

    if (showRDragons && gdragonTarget > 0) {
        DrawTallyRow(x + 10.0f, rowY, w - 20.0f, goldDragonsLabelText, gdragonBuf, m_goldDragonTex,
                     kTallyValueGoldR, kTallyValueGoldG, kTallyValueGoldB);
        rowY += kTallyRowSpacing;
    }

    if (showGoldBonus && m_tallyGoldBonusAnimTimer > 0.0f) {
        f32 t = m_tallyGoldBonusAnimTimer / kTallyGoldBonusAnimSeconds;
        if (t < 0.0f) {
            t = 0.0f;
        }
        if (t > 1.0f) {
            t = 1.0f;
        }

        const f32 progress = 1.0f - t;
        const f32 holdEnd = 1.0f - kTallyGoldBonusFadeOutPortion;

        f32 scaleMul = 1.0f;
        f32 fade = 1.0f;
        f32 lift = 0.0f;

        if (progress < kTallyGoldBonusFadeInPortion) {
            f32 u = progress / kTallyGoldBonusFadeInPortion;
            if (u < 0.0f) {
                u = 0.0f;
            }
            if (u > 1.0f) {
                u = 1.0f;
            }

            const f32 eased = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
            scaleMul = kTallyGoldBonusStartScaleMul - (kTallyGoldBonusStartScaleMul - 1.08f) * eased;
            fade = eased;
            lift = kTallyGoldBonusSettleLiftPx * (1.0f - eased);
        }
        else if (progress < holdEnd) {
            const f32 holdSpan = holdEnd - kTallyGoldBonusFadeInPortion;
            f32 u = (holdSpan > 0.0f) ? ((progress - kTallyGoldBonusFadeInPortion) / holdSpan) : 1.0f;
            if (u < 0.0f) {
                u = 0.0f;
            }
            if (u > 1.0f) {
                u = 1.0f;
            }

            scaleMul = 1.0f + 0.025f * (f32)std::sin(u * 9.4247779f) * (1.0f - u);
            fade = 1.0f;
            lift = 0.0f;
        }
        else {
            f32 u = (progress - holdEnd) / kTallyGoldBonusFadeOutPortion;
            if (u < 0.0f) {
                u = 0.0f;
            }
            if (u > 1.0f) {
                u = 1.0f;
            }

            const f32 eased = u * u;
            scaleMul = 1.0f + 0.08f * u;
            fade = 1.0f - eased;
            lift = kTallyGoldBonusSettleLiftPx * u;
        }

        if (fade < 0.0f) {
            fade = 0.0f;
        }
        if (fade > 1.0f) {
            fade = 1.0f;
        }

        f32 widthU = progress;
        if (widthU < 0.0f) {
            widthU = 0.0f;
        }
        if (widthU > 1.0f) {
            widthU = 1.0f;
        }
        widthU = widthU * widthU * (3.0f - 2.0f * widthU);
        const f32 backWidthMul = 1.0f + (kTallyGoldBonusWidthExpandMul - 1.0f) * widthU;

        const u8 alpha = (u8)(255.0f * fade);
        if (alpha > 0) {
            const f32 drawX = HudX(kTallyGoldBonusScreenX);
            const f32 drawY = HudY(kTallyGoldBonusScreenY - lift);
            const u8 glowAlpha = (u8)((f32)alpha * kTallyGoldBonusGlowAlphaMul);
            const f32 glowScale = kTallyGoldBonusTextScale * scaleMul * kTallyGoldBonusGlowScaleMul;
            const f32 mainScale = kTallyGoldBonusTextScale * scaleMul;

            if (glowAlpha > 0 &&
                BeginHudText(GetHudTitleFontName(),
                             glowScale,
                             TextAlign_Center,
                             255,
                             186,
                             72,
                             glowAlpha,
                             false,
                             false)) {
                const f32 glowScaleY = SCREEN_SCALE_Y(glowScale);
                g_textManager->SetScale(glowScaleY * backWidthMul, glowScaleY);
                g_textManager->PrintString(bonusGoldText, drawX, drawY);
            }

            if (BeginHudText(GetHudTitleFontName(),
                             mainScale,
                             TextAlign_Center,
                             255,
                             244,
                             176,
                             alpha,
                             true,
                             false)) {
                g_textManager->PrintString(bonusGoldText, drawX, drawY);
            }
        }
    }

    if (!useDebugPreview && hud.tally.rdragonBonusOvl.IsVisible()) {
        if (BeginHudText(kHudBodyFontName, 0.30f, TextAlign_Center, 0, 0, 0, 255, false, false)) {
            g_textManager->PrintString(dragonBonusText, HudX(x + w * 0.5f), HudY(y + h - 30.0f));
        }
    }

    if (movieBonusVisible) {
        m_tallyMovieBonusShakePhase += dt;
        if (m_tallyMovieBonusShakePhase > 1024.0f) {
            m_tallyMovieBonusShakePhase -= 1024.0f;
        }

        f32 drawScale = 0.30f;
        f32 lift = 0.0f;
        u8 drawAlpha = 255;

        if (m_tallyMovieBonusAnimTimer > 0.0f) {
            f32 t = m_tallyMovieBonusAnimTimer / kTallyMovieBonusAnimSeconds;
            if (t < 0.0f) {
                t = 0.0f;
            }
            if (t > 1.0f) {
                t = 1.0f;
            }

            const f32 progress = 1.0f - t;
            const f32 easeOut = 1.0f - (t * t * t);
            f32 scaleMul = 1.0f + (kTallyMovieBonusStartScaleMul - 1.0f) * (1.0f - easeOut);
            scaleMul += 0.05f * (f32)std::sin(progress * kTallyGoldBonusAnimFrequency) * t;

            f32 fade = 1.0f;
            if (progress < kTallyMovieBonusFadeInPortion) {
                fade = progress / kTallyMovieBonusFadeInPortion;
            }

            if (fade < 0.0f) {
                fade = 0.0f;
            }
            if (fade > 1.0f) {
                fade = 1.0f;
            }

            drawScale *= scaleMul;
            drawAlpha = (u8)(255.0f * fade);
            lift = kTallyMovieBonusSettleLiftPx * t;
        }

        const f32 shakeX = (f32)std::sin(m_tallyMovieBonusShakePhase * kTallyMovieBonusShakeFreqX) * kTallyMovieBonusShakeAmpX;
        const f32 shakeY = (f32)std::sin(m_tallyMovieBonusShakePhase * kTallyMovieBonusShakeFreqY + 1.5707963f) * kTallyMovieBonusShakeAmpY;

        if (drawAlpha > 0 &&
            BeginHudText(GetHudTitleFontName(), drawScale, TextAlign_Center, 255, 240, 160, drawAlpha, true, false)) {
            g_textManager->PrintString(movieBonusText,
                                       HudX(kTallyMovieBonusScreenX + shakeX),
                                       HudY(kTallyMovieBonusScreenY - lift + shakeY));
        }
    }
    else {
        m_tallyMovieBonusShakePhase = 0.0f;
    }

    const bool debugPromptReady =
        useDebugPreview &&
        (m_debugTallySoundStage >= 9) &&
        (m_tallyGoldBonusAnimTimer <= 0.0f) &&
        (!m_debugTallyShowMovieBonus || (m_tallyMovieBonusAnimTimer <= 0.0f));

    if ((useDebugPreview && debugPromptReady) || (!useDebugPreview && tallyState >= 8)) {
        const char* continuePromptText = g_customText.GetString(kTallyContinuePromptToken);
        if (continuePromptText && continuePromptText[0] != 0) {
            m_tallyPromptPulse.Update();
            const xcColour1555 pulseColor = m_tallyPromptPulse.GetColor();
            if (BeginHudText(kHudBodyFontName,
                             kTallyContinuePromptScale,
                             TextAlign_Center,
                             pulseColor.GetRed8(),
                             pulseColor.GetGreen8(),
                             pulseColor.GetBlue8(),
                             255,
                             true,
                             false)) {
                g_textManager->PrintString(continuePromptText,
                                           HudX(kTallyContinuePromptX),
                                           HudY(kTallyContinuePromptY));
            }
        }
    }
}
