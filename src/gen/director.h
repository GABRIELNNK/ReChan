// director.h - Director class reversed from PSX DIRECTOR.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\DIRECTOR.CPP
// Director is the cutscene/NIS scripting manager.
// Processes level scripts, controls camera/humanoid/door/ladder funcs.
#pragma once

#include "gen/manager.h"
#include "gen/handler.h"

class Thing;

// Director (212 bytes on PSX) - inherits Manager
// PSX layout:
//   +0:   Manager base (28 bytes)
//   +28:  scriptPtr (ptr)           - current script instruction pointer
//   +32:  scriptBase (ptr)          - script data base address
//   +36:  codeSnipPtr (ptr)         - trigger code snippet
//   +40:  codeSnipThing (ptr)       - Thing that triggered code snippet
//   +44:  processState (s32)        - current script interpreter state
//   +48:  timerValue (s32)          - countdown timer
//   +52:  loopCount (s32)           - loop counter
//   +56:  flags (u32)               - director flags
//   +60:  wideScreenDesired (s32)   - widescreen letterbox target
//   +64:  wideScreenCurrent (s32)   - current widescreen amount
//   +68:  field68..+208             - various state fields
class Director : public Manager {
public:
    s32* scriptPtr = nullptr;       // +28
    s32* scriptBase = nullptr;      // +32
    s32* codeSnipPtr = nullptr;     // +36
    Thing* codeSnipThing = nullptr; // +40
    s32 processState = 0;           // +44
    s32 timerValue = 0;             // +48
    s32 loopCount = 0;              // +52
    u32 dirFlags = 0;               // +56
    s32 wideScreenDesired = 0;      // +60
    s32 wideScreenCurrent = 0;      // +64

    // Additional fields to fill 212 bytes
    s32 field68 = 0;
    s32 field72 = 0;
    s32 field76 = 0;
    s32 field80 = 0;
    s32 field84 = 0;
    s32 field88 = 0;
    s32 field92 = 0;
    s32 field96 = 0;
    s32 field100 = 0;
    s32 field104 = 0;
    s32 field108 = 0;
    s32 field112 = 0;
    s32 field116 = 0;
    s32 field120 = 0;
    s32 field124 = 0;
    s32 field128 = 0;
    s32 field132 = 0;
    s32 field136 = 0;
    s32 field140 = 0;
    s32 field144 = 0;
    s32 field148 = 0;
    s32 field152 = 0;
    s32 field156 = 0;
    s32 field160 = 0;
    s32 field164 = 0;
    s32 field168 = 0;
    s32 field172 = 0;
    s32 field176 = 0;
    s32 field180 = 0;
    s32 field184 = 0;
    s32 field188 = 0;
    s32 field192 = 0;
    s32 field196 = 0;
    s32 field200 = 0;
    s32 field204 = 0;
    s32 field208 = 0;

    // PSX: __8Director (DIRECTOR.CPP:2658, 0x800CA1B8)
    Director();

    // PSX: _._8Director (DIRECTOR.CPP:2681, 0x8003BE10)
    ~Director() override;

    // PSX: InternalOpen__8Director (DIRECTOR.CPP:2704, 0x800CA2AC)
    void InternalOpen() override;

    // PSX: InternalClose__8Director (DIRECTOR.CPP:2757, 0x8003C11C)
    void InternalClose() override;

    // PSX: InternalReset__8Director (DIRECTOR.CPP:2736, 0x8003C04C)
    void InternalReset() override;

    // PSX: LevelReset__8Director (DIRECTOR.CPP:2731, 0x8003C044)
    void LevelReset();

    // PSX: SetScript__8Director (DIRECTOR.CPP:2765, 0x8003C234)
    void SetScript();

    // PSX: SetCodeSnip__8DirectorPlP5Thing (DIRECTOR.CPP:2782, 0x8003C268)
    void SetCodeSnip(s32* snip, Thing* thing);

    // PSX: Process__8Director (DIRECTOR.CPP:2806, 0x8003C298)
    // Main script interpreter - dispatches script opcodes.
    void Process();

    // PSX: ProcessSoundScript__8Director (DIRECTOR.CPP:3576, 0x8003D5A4)
    void ProcessSoundScript();

    // PSX: Timer__8Director (DIRECTOR.CPP:3611, 0x8003D634)
    void Timer();

    // PSX: Loop__8Director (DIRECTOR.CPP:3637, 0x8003D6CC)
    void Loop();

    // PSX: SetDesiredWideScreen__8Director (DIRECTOR.CPP:3642, 0x8003D6D4)
    void SetDesiredWideScreen();

    // PSX: ProcessEdison__8Director (DIRECTOR.CPP:3689, 0x8003D800)
    void ProcessEdison();

    // PSX: ProcessModelFunc__8Director (DIRECTOR.CPP:3711, 0x8003D87C)
    void ProcessModelFunc();

    // PSX: ProcessCameraFunc__8Director (DIRECTOR.CPP:3716, 0x8003D884)
    void ProcessCameraFunc();

    // PSX: ProcessHudFunc__8Director (DIRECTOR.CPP:3845, 0x8003DC44)
    void ProcessHudFunc();

    // PSX: ProcessHumanoidFunc__8Director (DIRECTOR.CPP:3894, 0x8003DD10)
    void ProcessHumanoidFunc();

    // PSX: ProcessLadderFunc__8Director (DIRECTOR.CPP:4003, 0x8003E0D4)
    void ProcessLadderFunc();

    // PSX: ProcessDoorFunc__8Director (DIRECTOR.CPP:4069, 0x8003E378)
    void ProcessDoorFunc();

    // PSX: DetermineVictoryIdle__8Director (DIRECTOR.CPP:4170, 0x8003E71C)
    void DetermineVictoryIdle();

    // PSX: DetermineLevelIntro__8Director (DIRECTOR.CPP:4260, 0x8003E864)
    void DetermineLevelIntro();

    // PSX: DetermineDeath__8Director (DIRECTOR.CPP:4382, 0x8003EA4C)
    void DetermineDeath();

    // PSX: WaitAnimationDone__8Director (DIRECTOR.CPP:4443, 0x8003EB14)
    void WaitAnimationDone();

    // PSX: ProcessDynamicAnimFunc__8Director (DIRECTOR.CPP:4469, 0x8003EB88)
    void ProcessDynamicAnimFunc();

    // PSX: HandleWideScreen__8Director (DIRECTOR.CPP:4539, 0x8003ECD4)
    void HandleWideScreen();

    // PSX: DrawWideScreenPolys__8Director (DIRECTOR.CPP:4582, 0x8003ED90)
    void DrawWideScreenPolys();

    // PSX: PurgeAnims__8Director (DIRECTOR.CPP:4662, 0x8003F0E4)
    void PurgeAnims();

    // PSX: DoesLevelHaveExtraMem__8Directorl (DIRECTOR.CPP:4669, 0x8003F104)
    bool DoesLevelHaveExtraMem(s32 level);

    // PSX: updateVramAnims__8Director (DIRECTOR.CPP:2597, 0x8003BB90)
    void updateVramAnims();

    // PSX: cleanUpTexAnim__8Director (DIRECTOR.CPP:2611, 0x8003BBE0)
    void cleanUpTexAnim();
};

// PSX: runDirector (DIRECTOR.CPP:2570, 0x8003BB0C) - handler callback
void runDirector(Handler* h);

// PSX: DrawDirectorOverlays (DIRECTOR.CPP:2578, 0x8003BB34) - handler callback
void DrawDirectorOverlays(Handler* h);

// Global Director pointer
extern Director* g_director;
