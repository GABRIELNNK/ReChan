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