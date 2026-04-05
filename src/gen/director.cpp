// director.cpp - Director class reversed from PSX DIRECTOR.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\DIRECTOR.CPP
#include "gen/director.h"

Director* g_director = nullptr;

// PSX: __8Director (DIRECTOR.CPP:2658, 0x800CA1B8)
Director::Director() {
    MARKFUNCTION(0x800CA1B8);
    scriptPtr = nullptr;
    scriptBase = nullptr;
    codeSnipPtr = nullptr;
    codeSnipThing = nullptr;
    processState = 0;
    timerValue = 0;
    loopCount = 0;
    dirFlags = 0;
    wideScreenDesired = 0;
    wideScreenCurrent = 0;
}

// PSX: _._8Director (DIRECTOR.CPP:2681, 0x8003BE10)
Director::~Director() {
    MARKFUNCTION(0x8003BE10);
}

// PSX: InternalOpen__8Director (DIRECTOR.CPP:2704, 0x800CA2AC)
void Director::InternalOpen() {
    MARKFUNCTION(0x800CA2AC);
    // PSX: registers runDirector and DrawDirectorOverlays handlers
    // into Game's handlerSets. Handlers wired in Game::InternalOpen instead.
}

// PSX: InternalClose__8Director (DIRECTOR.CPP:2757, 0x8003C11C)
void Director::InternalClose() {
    MARKFUNCTION(0x8003C11C);
    PurgeAnims();
    cleanUpTexAnim();
}

// PSX: InternalReset__8Director (DIRECTOR.CPP:2736, 0x8003C04C)
void Director::InternalReset() {
    MARKFUNCTION(0x8003C04C);
    scriptPtr = nullptr;
    scriptBase = nullptr;
    codeSnipPtr = nullptr;
    codeSnipThing = nullptr;
    processState = 0;
    timerValue = 0;
    loopCount = 0;
    dirFlags = 0;
    wideScreenDesired = 0;
    wideScreenCurrent = 0;
    PurgeAnims();
    cleanUpTexAnim();
}

// PSX: LevelReset__8Director (DIRECTOR.CPP:2731, 0x8003C044)
void Director::LevelReset() {
    MARKFUNCTION(0x8003C044);
    // PSX: NOP stub (0 bytes)
}

// PSX: SetScript__8Director (DIRECTOR.CPP:2765, 0x8003C234)
void Director::SetScript() {
    MARKFUNCTION(0x8003C234);
    // PSX: sets scriptPtr/scriptBase from level data
    // TODO: wire to level script data when loaded
}

// PSX: SetCodeSnip__8DirectorPlP5Thing (DIRECTOR.CPP:2782, 0x8003C268)
void Director::SetCodeSnip(s32* snip, Thing* thing) {
    MARKFUNCTION(0x8003C268);
    codeSnipPtr = snip;
    codeSnipThing = thing;
}

// PSX: Process__8Director (DIRECTOR.CPP:2806, 0x8003C298)
// Main script interpreter - 96 bytes, dispatches opcodes.
// This is the core of the Director: reads script commands and executes them.
void Director::Process() {
    MARKFUNCTION(0x8003C298);

    // Process code snippet if active
    if (codeSnipPtr) {
        // TODO: interpret code snippet opcodes
        codeSnipPtr = nullptr;
        codeSnipThing = nullptr;
    }

    // Process main script if active
    if (!scriptPtr) {
        // PSX: script opcode 0 (end) clears g_directorActive.
        // Since we have no script loaded, immediately clear it.
        extern s32 g_directorActive;
        g_directorActive = 0;
        return;
    }

    // TODO: full script interpreter
    // PSX dispatches on opcode at *scriptPtr:
    //   0 = end, 1 = Timer, 2 = Loop, 3 = ProcessCameraFunc,
    //   4 = ProcessHumanoidFunc, 5 = ProcessDoorFunc,
    //   6 = ProcessLadderFunc, 7 = ProcessEdison,
    //   8 = ProcessModelFunc, 9 = ProcessHudFunc,
    //   10 = ProcessSoundScript, 11 = ProcessDynamicAnimFunc,
    //   12 = SetDesiredWideScreen
}

// PSX: ProcessSoundScript__8Director (DIRECTOR.CPP:3576, 0x8003D5A4)
void Director::ProcessSoundScript() {
    MARKFUNCTION(0x8003D5A4);
}

// PSX: Timer__8Director (DIRECTOR.CPP:3611, 0x8003D634)
void Director::Timer() {
    MARKFUNCTION(0x8003D634);
    if (timerValue > 0) {
        timerValue--;
    }
}

// PSX: Loop__8Director (DIRECTOR.CPP:3637, 0x8003D6CC)
void Director::Loop() {
    MARKFUNCTION(0x8003D6CC);
}

// PSX: SetDesiredWideScreen__8Director (DIRECTOR.CPP:3642, 0x8003D6D4)
void Director::SetDesiredWideScreen() {
    MARKFUNCTION(0x8003D6D4);
}

// PSX: ProcessEdison__8Director (DIRECTOR.CPP:3689, 0x8003D800)
void Director::ProcessEdison() {
    MARKFUNCTION(0x8003D800);
}

// PSX: ProcessModelFunc__8Director (DIRECTOR.CPP:3711, 0x8003D87C)
void Director::ProcessModelFunc() {
    MARKFUNCTION(0x8003D87C);
}

// PSX: ProcessCameraFunc__8Director (DIRECTOR.CPP:3716, 0x8003D884)
void Director::ProcessCameraFunc() {
    MARKFUNCTION(0x8003D884);
}

// PSX: ProcessHudFunc__8Director (DIRECTOR.CPP:3845, 0x8003DC44)
void Director::ProcessHudFunc() {
    MARKFUNCTION(0x8003DC44);
}

// PSX: ProcessHumanoidFunc__8Director (DIRECTOR.CPP:3894, 0x8003DD10)
void Director::ProcessHumanoidFunc() {
    MARKFUNCTION(0x8003DD10);
}

// PSX: ProcessLadderFunc__8Director (DIRECTOR.CPP:4003, 0x8003E0D4)
void Director::ProcessLadderFunc() {
    MARKFUNCTION(0x8003E0D4);
}

// PSX: ProcessDoorFunc__8Director (DIRECTOR.CPP:4069, 0x8003E378)
void Director::ProcessDoorFunc() {
    MARKFUNCTION(0x8003E378);
}

// PSX: DetermineVictoryIdle__8Director (DIRECTOR.CPP:4170, 0x8003E71C)
void Director::DetermineVictoryIdle() {
    MARKFUNCTION(0x8003E71C);
}

// PSX: DetermineLevelIntro__8Director (DIRECTOR.CPP:4260, 0x8003E864)
void Director::DetermineLevelIntro() {
    MARKFUNCTION(0x8003E864);
    // PSX: sets up the level intro script sequence (camera fly-in, etc.)
    // TODO: reverse the intro script setup
}

// PSX: DetermineDeath__8Director (DIRECTOR.CPP:4382, 0x8003EA4C)
void Director::DetermineDeath() {
    MARKFUNCTION(0x8003EA4C);
}

// PSX: WaitAnimationDone__8Director (DIRECTOR.CPP:4443, 0x8003EB14)
void Director::WaitAnimationDone() {
    MARKFUNCTION(0x8003EB14);
}

// PSX: ProcessDynamicAnimFunc__8Director (DIRECTOR.CPP:4469, 0x8003EB88)
void Director::ProcessDynamicAnimFunc() {
    MARKFUNCTION(0x8003EB88);
}

// PSX: HandleWideScreen__8Director (DIRECTOR.CPP:4539, 0x8003ECD4)
void Director::HandleWideScreen() {
    MARKFUNCTION(0x8003ECD4);
    // PSX: NOP
}

// PSX: DrawWideScreenPolys__8Director (DIRECTOR.CPP:4582, 0x8003ED90)
void Director::DrawWideScreenPolys() {
    MARKFUNCTION(0x8003ED90);
    // TODO: draw letterbox polys
}

// PSX: PurgeAnims__8Director (DIRECTOR.CPP:4662, 0x8003F0E4)
void Director::PurgeAnims() {
    MARKFUNCTION(0x8003F0E4);
    // TODO: clean up loaded animation resources
}

// PSX: DoesLevelHaveExtraMem__8Directorl (DIRECTOR.CPP:4669, 0x8003F104)
bool Director::DoesLevelHaveExtraMem(s32 level) {
    MARKFUNCTION(0x8003F104);
    return false;
}

// PSX: updateVramAnims__8Director (DIRECTOR.CPP:2597, 0x8003BB90)
void Director::updateVramAnims() {
    MARKFUNCTION(0x8003BB90);
}

// PSX: cleanUpTexAnim__8Director (DIRECTOR.CPP:2611, 0x8003BBE0)
void Director::cleanUpTexAnim() {
    MARKFUNCTION(0x8003BBE0);
}

// PSX: runDirector (DIRECTOR.CPP:2570, 0x8003BB0C) - handler callback
void runDirector(Handler* h) {
    MARKFUNCTION(0x8003BB0C);
    if (g_director) {
        g_director->Process();
    }
}

// PSX: DrawDirectorOverlays (DIRECTOR.CPP:2578, 0x8003BB34) - handler callback
void DrawDirectorOverlays(Handler* h) {
    MARKFUNCTION(0x8003BB34);
    if (g_director) {
        g_director->DrawWideScreenPolys();
    }
}
