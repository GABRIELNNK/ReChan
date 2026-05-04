#include "xclib/xccolour.h"

#define USE_DT_FOR_COLOR_CYCLE 1

// PSX: the menu color cycling globals are stored via $gp.
// Start color, end color, cycle length, and per-frame deltas in 16.16 fixed point.
// On PC: we replicate the same behavior with simpler globals.

// PSX globals (HDMENU.CPP data section)
// Menu pulse endpoints: red -> yellow (PSX style)
// Keep blue at 0 and raise green over time.
static s32 g_menuColorStartR = 66;
static s32 g_menuColorEndR = 112;
static s32 g_menuColorStartG = 0;
static s32 g_menuColorEndG = 73;
static s32 g_menuColorStartB = 0;
static s32 g_menuColorEndB = 0;

#if !USE_DT_FOR_COLOR_CYCLE
// PSX: frame-based cycle (30 frames per half-cycle at 60Hz = 0.5s)
static s32 g_menuColorCycleLen = 30; // frames per half-cycle

// 16.16 fixed-point state
static s32 g_menuColorCurR = 0;
static s32 g_menuColorCurG = 0;
static s32 g_menuColorCurB = 0;
static s32 g_menuColorDeltaR = 0;
static s32 g_menuColorDeltaG = 0;
static s32 g_menuColorDeltaB = 0;

// PSX: rmDiv16i(a, b) = (a << 16) / b
static s32 rmDiv16i(s32 a, s32 b) {
    if (b == 0) return 0;
    return (s32)(((s64)a << 16) / b);
}
#else
#include "gen/time.h"

static f32 g_menuColorCycleDuration = 0.5f; // half-cycle time in seconds
static f32 g_menuColorElapsed = 0.0f;       // elapsed time in current half-cycle
#endif

// PSX: MenuColorStart(xcColour1555&) (HDMENU.CPP:112, 0x8005CB4C)
void MenuColorStart(xcColour1555& col) {
    MARKFUNCTION(0x8005CB4C);
#if !USE_DT_FOR_COLOR_CYCLE
    s32 len = g_menuColorCycleLen;
    g_menuColorDeltaR = rmDiv16i(g_menuColorEndR - g_menuColorStartR, len);
    g_menuColorDeltaG = rmDiv16i(g_menuColorEndG - g_menuColorStartG, len);
    g_menuColorDeltaB = rmDiv16i(g_menuColorEndB - g_menuColorStartB, len);
    g_menuColorCurR = g_menuColorStartR << 16;
    g_menuColorCurG = g_menuColorStartG << 16;
    g_menuColorCurB = g_menuColorStartB << 16;
#else
    g_menuColorElapsed = 0.0f;
#endif

    u8 r = (u8)g_menuColorStartR;
    u8 g = (u8)g_menuColorStartG;
    u8 b = (u8)g_menuColorStartB;
    col.Set8(r, g, b);
}

#if !USE_DT_FOR_COLOR_CYCLE
// PSX: CalcNextColor(xcColour1555&) (HDMENU.CPP:125, 0x8005CC44)
static void CalcNextColor(xcColour1555& col) {
    g_menuColorCurR += g_menuColorDeltaR;
    g_menuColorCurG += g_menuColorDeltaG;
    g_menuColorCurB += g_menuColorDeltaB;

    s32 r = g_menuColorCurR >> 16;
    s32 g = g_menuColorCurG >> 16;
    s32 b = g_menuColorCurB >> 16;

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    col.Set8((u8)r, (u8)g, (u8)b);
}
#else
static void UpdateColorFromTime(xcColour1555& col) {
    f32 duration = g_menuColorCycleDuration;
    if (duration <= 0.0f) duration = 0.5f;

    f32 progress = g_menuColorElapsed / duration;
    if (progress > 1.0f) progress = 1.0f;

    s32 r = g_menuColorStartR + (s32)((g_menuColorEndR - g_menuColorStartR) * progress);
    s32 g = g_menuColorStartG + (s32)((g_menuColorEndG - g_menuColorStartG) * progress);
    s32 b = g_menuColorStartB + (s32)((g_menuColorEndB - g_menuColorStartB) * progress);

    col.Set8((u8)r, (u8)g, (u8)b);
}
#endif

// PSX: MenuColorNext(xcColour1555&) (HDMENU.CPP:143, 0x8005CD10)
void MenuColorNext(xcColour1555& col) {
    MARKFUNCTION(0x8005CD10);
#if !USE_DT_FOR_COLOR_CYCLE
    CalcNextColor(col);

    // Check if we've gone past the end — if so, restart
    u8 curR = col.GetRed8();
    if (g_menuColorStartR < g_menuColorEndR) {
        if (curR >= (u8)g_menuColorEndR)
            MenuColorStart(col);
    }
    else {
        if (curR <= (u8)g_menuColorEndR)
            MenuColorStart(col);
    }
#else
    if (!g_time) {
        col.Set8((u8)g_menuColorStartR, (u8)g_menuColorStartG, (u8)g_menuColorStartB);
        return;
    }

    g_menuColorElapsed += g_time->GetDeltaTime();
    UpdateColorFromTime(col);

    if (g_menuColorElapsed >= g_menuColorCycleDuration) {
        while (g_menuColorElapsed >= g_menuColorCycleDuration) {
            g_menuColorElapsed -= g_menuColorCycleDuration;
        }
        col.Set8((u8)g_menuColorStartR, (u8)g_menuColorStartG, (u8)g_menuColorStartB);
    }
#endif
}
