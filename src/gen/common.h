#pragma once
#include "config.h"
#include "pc/log.h"
#include "gen/display.h"

// Game title
#define JCSM_TITLE "Jackie Chan Stuntmaster"
#define JCSM_VERSION "1.0.0"

#define JCSM_TARGET_WIDTH 1280
#define JCSM_TARGET_HEIGHT 720

// Native resolution (overlay coordinate space)
#define DEFAULT_SCREEN_WIDTH ((f32)512)
#define DEFAULT_SCREEN_HEIGHT ((f32)240)
#define DEFAULT_ASPECT_RATIO (4.0f / 3.0f)

// Font/text coordinate space
#define DEFAULT_FONT_WIDTH ((f32)320)
#define DEFAULT_FONT_HEIGHT ((f32)240)

#define SCREEN_WIDTH (g_display ? (f32)g_display->GetScreenWidth() : DEFAULT_SCREEN_WIDTH)
#define SCREEN_HEIGHT (g_display ? (f32)g_display->GetScreenHeight() : DEFAULT_SCREEN_HEIGHT)
#define ASPECT_RATIO (g_display ? g_display->GetAspectRatio() : DEFAULT_ASPECT_RATIO)

// PSX pixel coordinates to screen space.
#define SCREEN_STRETCH_X(a) ((a) * (float)SCREEN_WIDTH / DEFAULT_SCREEN_WIDTH)
#define SCREEN_STRETCH_Y(a) ((a) * (float)SCREEN_HEIGHT / DEFAULT_SCREEN_HEIGHT)
#define SCREEN_STRETCH_FROM_RIGHT(a) (SCREEN_WIDTH - SCREEN_STRETCH_X(a))
#define SCREEN_STRETCH_FROM_BOTTOM(a) (SCREEN_HEIGHT - SCREEN_STRETCH_Y(a))

#define SCREEN_SCALE_X(a) SCREEN_SCALE_AR(SCREEN_STRETCH_X(a))
#define SCREEN_SCALE_Y(a) SCREEN_STRETCH_Y(a)
#define SCREEN_SCALE_FROM_RIGHT(a) (SCREEN_WIDTH - SCREEN_SCALE_X(a))
#define SCREEN_SCALE_FROM_BOTTOM(a) (SCREEN_HEIGHT - SCREEN_SCALE_Y(a))

#define TARGET_ASPECT_RATIO (16.0f / 9.0f)

// Effective width limited to 16:9
#define SCREEN_EFFECTIVE_WIDTH \
    ((SCREEN_WIDTH > SCREEN_HEIGHT * TARGET_ASPECT_RATIO) ? \
    (SCREEN_HEIGHT * TARGET_ASPECT_RATIO) : (float)SCREEN_WIDTH)

// Offset to center the clamped area
#define SCREEN_EFFECTIVE_OFFSET_X \
    ((SCREEN_WIDTH - SCREEN_EFFECTIVE_WIDTH) * 0.5f)

#if FIX_ASPECT_RATIO
#define SCREEN_SCALE_AR(a) ((a) * DEFAULT_ASPECT_RATIO / ASPECT_RATIO)
#define SCALE_AND_CENTER_X(x) ((SCREEN_WIDTH == DEFAULT_SCREEN_WIDTH) ? (x) : (SCREEN_WIDTH - SCREEN_SCALE_X(DEFAULT_SCREEN_WIDTH)) / 2 + SCREEN_SCALE_X((x)))
#else
#define SCREEN_SCALE_AR(a) (a)
#define SCALE_AND_CENTER_X(x) SCREEN_STRETCH_X(x)
#endif

template <typename T>
static inline T Clamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
