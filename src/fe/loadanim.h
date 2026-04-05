// loadanim.h - PSX VBlankLogo loading bar (LOADANIM.CPP)
#pragma once

#include "gen/common.h"

// PSX: VBlankLogo class from C:\CHAN\GAME\SRC\FE\LOADANIM.CPP
// On PSX this uses a VBlank interrupt to animate a gradient tile bar
// into VRAM while the main thread blocks on disc reads.
// On PC, loading is synchronous: we present the loading screen before
// the blocking load (frame persists), then animate the bar to 100%
// after the load completes in StopLogo.

// PSX: StartLogo__10VBlankLogol (LOADANIM.CPP:167, 0x80047968)
// Ends the current main-loop frame, loads RUNFIRST.TIM and LOADANIM.CON,
// presents the initial loading screen with an empty bar.
void StartLogo(const char* timFile);

// PSX: FillMeter__10VBlankLogoUc (LOADANIM.CPP:207, 0x80047A68)
void FillMeter(u8 target);

// PSX: StopLogo__10VBlankLogo (LOADANIM.CPP:183, 0x800479BC)
// Animates bar to target over several frames, cleans up textures,
// then starts a new frame for the main loop's EndFrame balance.
void StopLogo();

// PSX: DisplayTIM__FPCc (GAME.CPP:862, 0x80029200)
// Show a TIM image fullscreen without a loading bar (e.g. LICENSE.TIM).
void DisplayTIM(const char* filename);
