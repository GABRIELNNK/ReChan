// config.h - PC recompilation feature flags
// Toggle non-OG / custom PC features here.
// The goal is to keep reversed PSX code faithful; any PC-specific
// convenience features should be gated behind macros in this file.
#pragma once
// Replace the OG PSX DebugCam (pad-only) with a PC-friendly camera:
// WASD to move, LMB+drag to look, Shift for speed boost, Q/E for up/down.
// When disabled, DebugCam uses the original PSX pad-button controls.
#define IMPROVED_DEBUG_CAM 1

// Skip initial intros and goes straight into title screen
#define SKIP_INTRO 0

// Adds support to any aspect ratio, fixes ui and camera.
#define FIX_ASPECT_RATIO 1

// Director cutscene bars: 0 = PSX-like alpha fade, 1 = slide in/out using alpha as progress.
#define DIRECTOR_WIDESCREEN_SLIDE_BARS 0

// When ESC is bound to both menu back and menu open/close,
// let it back out of submenu pages before closing the whole menu.
#define ESC_BACKS_OUT_SUBMENUS_FIRST 1

// Menu back button shape policy:
// 0 = PSX-original Triangle, 1 = Circle (Xbox B)
#define MENU_BACK_USES_CIRCLE 1

// Custom text files with localization support, parsed from game directory (requires .txt files for each language).
#define CUSTOM_TEXT 1

// Custom reimplementation of the game menus, allowing mouse support and custom settings. (Requires CUSTOM_TEXT for the text strings)
#define CUSTOM_MENU 1

// Experimental: Keep simulation fixed at 30 fps while rendering at higher frame rates, with interpolation for smoothness.
#define HIGH_FPS_PLAY_PRESENTATION 1

// Mod loader support: when enabled, the game checks the ~mods/ directory for
// replacement assets (PNG textures, GLB models, WAV sounds, JSON data) before
// falling back to original PSX-format assets. Set to 0 to keep original code
// path fully intact with zero mod-related overhead.
#define MOD_LOADER 1

// Adds a persistent Cheats page and its optional gameplay changes.
#define NEW_CHEATS 1

// Real-texture rendering: adds a second, runtime-switchable rendering path
// (toggle via the debug UI) that samples a real full-resolution 2D texture
// per material instead of the shared PSX VRAM/CLUT atlas. Off by default at
// runtime even when compiled in. Every named texture chunk (character and
// world geometry) still gets decoded at native resolution either way, so
// toggling never changes vanilla (unmodded) visuals -- it only lifts the
// resolution/palette cap for ModLoader texture overrides.
#define REAL_TEXTURE_RENDERING 1

#define AUTO_UPDATER (CUSTOM_MENU && 1)
