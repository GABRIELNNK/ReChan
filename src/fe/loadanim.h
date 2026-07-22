#pragma once
#include "core.h"

// PSX: VBlankLogo class
// On PSX this uses a VBlank interrupt to animate a gradient tile bar
// into VRAM while the main thread blocks on disc reads.
// On PC, loading is synchronous: we present the loading screen before
// the blocking load (frame persists), then animate the bar to 100%
// after the load completes in StopLogo.

// Ends the current main-loop frame, loads RUNFIRST.TIM and LOADANIM.CON,
// presents the initial loading screen with an empty bar.
void StartLogo(const char* timFile);
void FillMeter(u8 target);

void PumpLoadingScreen();

// Animates bar to target over several frames, cleans up textures,
// then starts a new frame for the main loop's EndFrame balance.
void StopLogo();

// Show a TIM image fullscreen without a loading bar (e.g. LICENSE.TIM).
void DisplayTIM(const char* filename);
