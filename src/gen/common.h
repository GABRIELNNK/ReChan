#pragma once
#include "pc/log.h"
#include "gen/display.h"

// Game title
#define JCSM_TITLE "Jackie Chan Stuntmaster"
#define JCSM_VERSION "1.0.0"

#define JCSM_TARGET_WIDTH 1280
#define JCSM_TARGET_HEIGHT 720

// Native resolution (overlay coordinate space)
#define DEFAULT_SCREEN_WIDTH 512
#define DEFAULT_SCREEN_HEIGHT 240
#define DEFAULT_ASPECT_RATIO (DEFAULT_SCREEN_WIDTH / (f32)DEFAULT_SCREEN_HEIGHT)

// Font/text coordinate space
#define DEFAULT_FONT_WIDTH 320
#define DEFAULT_FONT_HEIGHT 240

#define SCREEN_WIDTH (g_display ? g_display->GetScreenWidth() : DEFAULT_SCREEN_WIDTH)
#define SCREEN_HEIGHT (g_display ? g_display->GetScreenHeight() : DEFAULT_SCREEN_HEIGHT)
#define ASPECT_RATIO (g_display ? g_display->GetAspectRatio() : DEFAULT_ASPECT_RATIO)

// Normalize overlay coordinates (512x240) to 0..1
#define SCALE_NORM_X(x)  ((f32)(x) / (f32)DEFAULT_SCREEN_WIDTH)
#define SCALE_NORM_Y(y)  ((f32)(y) / (f32)DEFAULT_SCREEN_HEIGHT)

// Normalize font coordinates (320x240) to 0..1
#define FONT_NORM_X(x) ((f32)(x) / (f32)DEFAULT_FONT_WIDTH)
#define FONT_NORM_Y(y) ((f32)(y) / (f32)DEFAULT_FONT_HEIGHT)

template <typename T>
static inline T Clamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
