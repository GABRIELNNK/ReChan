// config.h - PC recompilation feature flags
// Toggle non-OG / custom PC features here.
// The goal is to keep reversed PSX code faithful; any PC-specific
// convenience features should be gated behind macros in this file.
#pragma once

// ---------------------------------------------------------------------------
// RC_FEATURE_PAD_KEYBOARD_EMULATION
//   Emulate PSX gamepad input from keyboard+mouse.
//   Default key bindings map WASD/arrows/face keys to PSX pad buttons.
//   Disable this if using a real gamepad passthrough.
// ---------------------------------------------------------------------------
#define RC_FEATURE_PAD_KEYBOARD_EMULATION 1

// ---------------------------------------------------------------------------
// RC_FEATURE_IMPROVED_DEBUG_CAM
//   Replace the OG PSX DebugCam (pad-only) with a PC-friendly camera:
//   WASD to move, LMB+drag to look, Shift for speed boost, Q/E for up/down.
//   When disabled, DebugCam uses the original PSX pad-button controls.
// ---------------------------------------------------------------------------
#define RC_FEATURE_IMPROVED_DEBUG_CAM 1

// ---------------------------------------------------------------------------
// RC_FEATURE_COLLISION_DEBUG
//   Draw collision walls/floors as colored lines.
//   Toggle at runtime with F3. Green=walls, cyan=floors, magenta=ceilings.
// ---------------------------------------------------------------------------
#define RC_FEATURE_COLLISION_DEBUG 1
