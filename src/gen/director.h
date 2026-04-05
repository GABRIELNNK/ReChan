// director.h - Director class reversed from PSX DIRECTOR.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\DIRECTOR.CPP
// Director is the cutscene/NIS scripting manager.
// Processes level scripts, controls camera/humanoid/door/ladder funcs.
#pragma once

#include "gen/manager.h"
#include "gen/handler.h"

class CDirectorSound;
class Thing;

enum class DirectorOpcode : s32 {
    End = 0,
    ResetTimeout = 1,
    EndScript = 2,
    Call = 3,
    Return = 4,
    Timer = 6,
    Loop = 7,
    EnablePlayerInput = 8,
    DisablePlayerInput = 9,
    DetermineLevelIntro = 0x0A,
    FaceThing = 0x0C,
    SetHumanoidAction = 0x0E,
    DynamicAnimLoad = 0x0F,
    DynamicAnimWaitLoaded = 0x10,
    WaitAnimationDone = 0x11,
    DynamicAnimWaitCamera = 0x12,
    WaitForNisControl = 0x13,
    RestorePlayerControl = 0x14,
    PlayThingDynamicAnim = 0x15,
    DynamicAnimUnload = 0x16,
    SetupFaceTextureAnim = 0x17,
    CleanupFaceTextureAnim = 0x18,
    QueueDetermineNextState = 0x19,
    SetGameState = 0x1A,
    SetCheckpoint = 0x1B,
    SetCheckpointByUid = 0x1C,
    SetCheckpointData = 0x1D,
    TriggerCheckpoint = 0x1E,
    ClearCheckpointValid = 0x1F,
    SpawnEffectFromMatrix = 0x20,
    SpawnEffectFromAttack = 0x21,
    SpawnEffectAtPosA = 0x22,
    SpawnEffectAtPosB = 0x23,
    SpawnEffectByUidAtPos = 0x24,
    SpawnFwEffectAtPos = 0x25,
    DestroyDestructible = 0x26,
    RemoveNisEffect = 0x27,
    SetPlayerFlag = 0x28,
    ClearGlobalEffectRef = 0x29,
    DropPickup = 0x2A,
    CameraFunc = 0x2B,
    DoorFunc = 0x38,
    FacePointAndNisControl = 0x40,
    LadderFunc = 0x41,
    ModelFunc = 0x47,
    HudFunc = 0x49,
    SetThingFlag08 = 0x4E,
    SetThingFlag28 = 0x4F,
    ClearThingFlagsAndKill = 0x50,
    KillThingType52 = 0x51,
    KillThingType88 = 0x52,
    KillThingsIfCombat = 0x53,
    HumanoidFunc = 0x54,
    ResetCameraManager = 0x5E,
    SetDesiredWideScreen = 0x62,
    ResetWideScreenDefaults = 0x69,
    SetSoundScript = 0x6A,
    EdisonFunc = 0x6B,
    SoundCdYield = 0x71,
    SoundCdAccess = 0x72,
    LoadDialogA = 0x73,
    LoadDialogB = 0x74,
    WaitDialogPlayable = 0x75,
    PlayDialogNear = 0x76,
    PlayDialogFar = 0x77,
    PlayPriorityDialog = 0x78,
    SetDialogTimeout = 0x79,
    SetNisPoint = 0x7A,
    SetGotoCheckpoint = 0x7B,
    SetFallbackCheckpoint = 0x7C,
    SetBlockCheckpoint = 0x7D,
    DetermineVictory = 0x7E,
    DetermineDeath = 0x7F,
};

enum class DirectorWideScreenCmd : s32 {
    End = 5,
    SetAlphaTarget = 'c',
    SetAlphaCurrent = 'd',
    SetAlphaStep = 'e',
    SetBarTarget = 'f',
    SetBarStep = 'g',
    SetMode = 'h',
};

enum class DirectorCameraCmd : s32 {
    EnableNisCamera = ',',
    ClearCameraFlag = '-',
    SetCameraFlag = '.',
    CopyP3DFov = '/',
    ResetCameraFov = '0',
    SetCameraMode = '1',
    LoadAsyncAnim = '2',
    DeleteAsyncAnim = '3',
    PlayAsyncAnim = '4',
    ShakeCamera = '5',
    LookAtNisPoint = '6',
    SetCameraAndLookAt = '7',
};

enum class DirectorHudCmd : s32 {
    HideHud = 'J',
    ShowHud = 'K',
    DisplayTally = 'L',
    ShowBossHealth = 'M',
};

enum class DirectorHumanoidCmd : s32 {
    End = 5,
    EnterNis = 'U',
    EnterNisMove = 'V',
    ExitNis = 'W',
    FaceAngleDegrees = 'X',
    StandFacingZero = 'Y',
    SetPosition = 'Z',
    PlayDynamicAnim = '[',
    SetStandState = '\\',
    SetPositionByCurrent = ']',
};

enum class DirectorLadderCmd : s32 {
    End = 5,
    FaceLadderPoint = 'B',
    TeleportPlayer = 'C',
    CameraLookAtHatch = 'D',
    CloseHatch = 'E',
    ClearNis = 'F',
};

enum class DirectorDoorCmd : s32 {
    End = 5,
    SetDoor = '9',
    OpenDoor = ':',
    SetDoorState = ';',
    FaceDoorPoint = '<',
    FaceDoorAngle = '=',
    AttachToDoor = '>',
    TeleportThroughDoor = '?',
};

enum class DirectorEdisonCmd : s32 {
    PlayTransient = 108,
    StopMusic = 109,
};

// Director (212 bytes on PSX) - inherits Manager
// PSX layout:
//   +0:   Manager base (28 bytes)
//   +28:  scriptPtr (s32*)          - current script instruction pointer
//   +32:  scriptBase (s32*)         - sound script base address
//   +36:  codeSnipPtr (s32*)        - trigger code snippet pointer
//   +40:  wsBarCurrent (s32)        - widescreen bar current height
//   +44:  wsBarTarget (s32)         - widescreen bar target height
//   +48:  wsBarStep (s32)           - widescreen bar step (256=instant)
//   +52:  wsMode (u16)              - widescreen blend mode (0/1/2)
//   +56:  wsAlphaStep (s32)         - widescreen alpha step
//   +60:  wsAlphaCurrent (s32)      - widescreen alpha current
//   +64:  wsAlphaTarget (s32)       - widescreen alpha target
//   +68:  blocked (s32)             - script loop blocked flag
//   +72:  enableInput (s32)         - player input enabled during NIS
//   +76..+155: handler sets, internal lists
//   +156: timerTarget (s32)         - timer end frame
//   +160: timerStart (s32)          - timer start frame
//   +164: visitedLevels (u32)       - bitmask of visited levels
//   +168: scriptState (s32)         - 0=idle, 534=SetScript, 537=SetCodeSnip
//   +172: deathType (s32)           - death cause for DetermineDeath
//   +176: victoryBossCRC (u32)      - boss CRC for DetermineVictory
//   +180: nisPointX (s32)           - NIS reference point X
//   +184: nisPointY (s32)           - NIS reference point Y
//   +188: nisPointZ (s32)           - NIS reference point Z
//   +192: directorSound (ptr)       - CDirectorSound*
//   +196: texAnimA (ptr)            - tAnimKeyFrame* for face anim A
//   +200: flipbookA (ptr)           - tFlipbook* for face anim A
//   +204: texAnimB (ptr)            - tAnimKeyFrame* for face anim B
//   +208: flipbookB (ptr)           - tFlipbook* for face anim B
class Director : public Manager {
public:
    s32* scriptPtr = nullptr;       // +28: current script instruction pointer
    s32* scriptBase = nullptr;      // +32: sound script base address
    s32* codeSnipPtr = nullptr;     // +36: trigger code snippet

    // +40..+64: widescreen letterbox state
    s32 wsBarCurrent = 0;           // +40: bar height in PSX scanlines
    s32 wsBarTarget = 0;            // +44: target bar height
    s32 wsBarStep = 0;              // +48: step per frame (256=instant)
    u16 wsMode = 0;                 // +52: blend mode (2=black, other=white)
    s32 wsAlphaStep = 0;            // +56: alpha step per frame
    s32 wsAlphaCurrent = 0;         // +60: current alpha (0..255)
    s32 wsAlphaTarget = 0;          // +64: target alpha

    s32 field68 = 0;                // +68: script loop blocked flag
    s32 enableInput = 0;            // +72: player input enabled (1=yes, 0=NIS disabled)

    // +76..+155: handler sets, internal NIS animation lists
    // PSX has two HandlerSets and supporting ccLists here.
    // On PC these are currently unused placeholders.
    s32 _pad76[20] = {};

    s32 timerTarget = 0;            // +156: timer target frame (0 = inactive)
    s32 timerStart = 0;             // +160: timer start frame
    s32 visitedLevels = 0;          // +164: bitmask of previously visited levels
    s32 scriptState = 0;            // +168: 0=idle, 534=SetScript, 537=SetCodeSnip
    s32 deathType = 0;              // +172: death cause (0=fall, 1=pavement, 2=water, 4=goo)
    u32 victoryBossCRC = 0;         // +176: boss CRC for DetermineVictory dispatch
    s32 nisPointX = 0;              // +180: NIS reference point X
    s32 nisPointY = 0;              // +184: NIS reference point Y
    s32 nisPointZ = 0;              // +188: NIS reference point Z
    CDirectorSound* directorSound = nullptr; // +192: m_pDirectorSound
    uintptr_t texAnimA = 0;         // +196: tAnimKeyFrame* for face anim A
    uintptr_t flipbookA = 0;        // +200: tFlipbook* for face anim A
    uintptr_t texAnimB = 0;         // +204: tAnimKeyFrame* for face anim B
    uintptr_t flipbookB = 0;        // +208: tFlipbook* for face anim B

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
    // Returns 1 when done/reset, 0 when blocked.
    s32 TimerStep();

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
    // Returns 1 when done, 0 when still waiting.
    s32 WaitAnimationDoneStep();

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
