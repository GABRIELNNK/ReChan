// config.h - PC recompilation feature flags
// Toggle non-OG / custom PC features here.
// The goal is to keep reversed PSX code faithful; any PC-specific
// convenience features should be gated behind macros in this file.
#pragma once
// Emulate PSX gamepad input from keyboard+mouse.
// Default key bindings map WASD/arrows/face keys to PSX pad buttons.
// Disable this if using a real gamepad passthrough.
#define PAD_KEYBOARD_EMULATION 1

// Replace the OG PSX DebugCam (pad-only) with a PC-friendly camera:
// WASD to move, LMB+drag to look, Shift for speed boost, Q/E for up/down.
// When disabled, DebugCam uses the original PSX pad-button controls.
#define IMPROVED_DEBUG_CAM 0

// Skip initial intros and goes straight into title screen
#define SKIP_INTRO 1

// Scale 2D UI to maintain 4:3 aspect ratio instead of stretching to
// fill the window. Adds pillarbox/letterbox bars when needed.
#define CORRECT_UI_ASPECT 1