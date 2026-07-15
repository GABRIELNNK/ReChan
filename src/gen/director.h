#pragma once
#include "gen/manager.h"
#include "gen/handler.h"

// Director is the cutscene/NIS scripting manager.
// Processes level scripts, controls camera/humanoid/door/ladder funcs.

class CDirectorSound;
class Thing;

enum class DirectorOpcode : s32 {
    End = 0,
    ResetTimeout = 1,
    EndScript = 2,
    Call = 3,
    Return = 4,
    Yield = 5,
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

static inline const char* CmdToString(DirectorOpcode op) {
    switch (op) {
        case DirectorOpcode::End: return "End";
        case DirectorOpcode::ResetTimeout: return "ResetTimeout";
        case DirectorOpcode::EndScript: return "EndScript";
        case DirectorOpcode::Call: return "Call";
        case DirectorOpcode::Return: return "Return";
        case DirectorOpcode::Yield: return "Yield";
        case DirectorOpcode::Timer: return "Timer";
        case DirectorOpcode::Loop: return "Loop";
        case DirectorOpcode::EnablePlayerInput: return "EnablePlayerInput";
        case DirectorOpcode::DisablePlayerInput: return "DisablePlayerInput";
        case DirectorOpcode::DetermineLevelIntro: return "DetermineLevelIntro";
        case DirectorOpcode::FaceThing: return "FaceThing";
        case DirectorOpcode::SetHumanoidAction: return "SetHumanoidAction";
        case DirectorOpcode::DynamicAnimLoad: return "DynamicAnimLoad";
        case DirectorOpcode::DynamicAnimWaitLoaded: return "DynamicAnimWaitLoaded";
        case DirectorOpcode::WaitAnimationDone: return "WaitAnimationDone";
        case DirectorOpcode::DynamicAnimWaitCamera: return "DynamicAnimWaitCamera";
        case DirectorOpcode::WaitForNisControl: return "WaitForNisControl";
        case DirectorOpcode::RestorePlayerControl: return "RestorePlayerControl";
        case DirectorOpcode::PlayThingDynamicAnim: return "PlayThingDynamicAnim";
        case DirectorOpcode::DynamicAnimUnload: return "DynamicAnimUnload";
        case DirectorOpcode::SetupFaceTextureAnim: return "SetupFaceTextureAnim";
        case DirectorOpcode::CleanupFaceTextureAnim: return "CleanupFaceTextureAnim";
        case DirectorOpcode::QueueDetermineNextState: return "QueueDetermineNextState";
        case DirectorOpcode::SetGameState: return "SetGameState";
        case DirectorOpcode::SetCheckpoint: return "SetCheckpoint";
        case DirectorOpcode::SetCheckpointByUid: return "SetCheckpointByUid";
        case DirectorOpcode::SetCheckpointData: return "SetCheckpointData";
        case DirectorOpcode::TriggerCheckpoint: return "TriggerCheckpoint";
        case DirectorOpcode::ClearCheckpointValid: return "ClearCheckpointValid";
        case DirectorOpcode::SpawnEffectFromMatrix: return "SpawnEffectFromMatrix";
        case DirectorOpcode::SpawnEffectFromAttack: return "SpawnEffectFromAttack";
        case DirectorOpcode::SpawnEffectAtPosA: return "SpawnEffectAtPosA";
        case DirectorOpcode::SpawnEffectAtPosB: return "SpawnEffectAtPosB";
        case DirectorOpcode::SpawnEffectByUidAtPos: return "SpawnEffectByUidAtPos";
        case DirectorOpcode::SpawnFwEffectAtPos: return "SpawnFwEffectAtPos";
        case DirectorOpcode::DestroyDestructible: return "DestroyDestructible";
        case DirectorOpcode::RemoveNisEffect: return "RemoveNisEffect";
        case DirectorOpcode::SetPlayerFlag: return "SetPlayerFlag";
        case DirectorOpcode::ClearGlobalEffectRef: return "ClearGlobalEffectRef";
        case DirectorOpcode::DropPickup: return "DropPickup";
        case DirectorOpcode::CameraFunc: return "CameraFunc";
        case DirectorOpcode::DoorFunc: return "DoorFunc";
        case DirectorOpcode::FacePointAndNisControl: return "FacePointAndNisControl";
        case DirectorOpcode::LadderFunc: return "LadderFunc";
        case DirectorOpcode::ModelFunc: return "ModelFunc";
        case DirectorOpcode::HudFunc: return "HudFunc";
        case DirectorOpcode::SetThingFlag08: return "SetThingFlag08";
        case DirectorOpcode::SetThingFlag28: return "SetThingFlag28";
        case DirectorOpcode::ClearThingFlagsAndKill: return "ClearThingFlagsAndKill";
        case DirectorOpcode::KillThingType52: return "KillThingType52";
        case DirectorOpcode::KillThingType88: return "KillThingType88";
        case DirectorOpcode::KillThingsIfCombat: return "KillThingsIfCombat";
        case DirectorOpcode::HumanoidFunc: return "HumanoidFunc";
        case DirectorOpcode::ResetCameraManager: return "ResetCameraManager";
        case DirectorOpcode::SetDesiredWideScreen: return "SetDesiredWideScreen";
        case DirectorOpcode::ResetWideScreenDefaults: return "ResetWideScreenDefaults";
        case DirectorOpcode::SetSoundScript: return "SetSoundScript";
        case DirectorOpcode::EdisonFunc: return "EdisonFunc";
        case DirectorOpcode::SoundCdYield: return "SoundCdYield";
        case DirectorOpcode::SoundCdAccess: return "SoundCdAccess";
        case DirectorOpcode::LoadDialogA: return "LoadDialogA";
        case DirectorOpcode::LoadDialogB: return "LoadDialogB";
        case DirectorOpcode::WaitDialogPlayable: return "WaitDialogPlayable";
        case DirectorOpcode::PlayDialogNear: return "PlayDialogNear";
        case DirectorOpcode::PlayDialogFar: return "PlayDialogFar";
        case DirectorOpcode::PlayPriorityDialog: return "PlayPriorityDialog";
        case DirectorOpcode::SetDialogTimeout: return "SetDialogTimeout";
        case DirectorOpcode::SetNisPoint: return "SetNisPoint";
        case DirectorOpcode::SetGotoCheckpoint: return "SetGotoCheckpoint";
        case DirectorOpcode::SetFallbackCheckpoint: return "SetFallbackCheckpoint";
        case DirectorOpcode::SetBlockCheckpoint: return "SetBlockCheckpoint";
        case DirectorOpcode::DetermineVictory: return "DetermineVictory";
        case DirectorOpcode::DetermineDeath: return "DetermineDeath";
        default: return "UnknownDirectorOpcode";
    }

    return "UnknownDirectorOpcode";
}

enum class DirectorWideScreenCmd : s32 {
    End = 5,
    SetAlphaTarget = 'c',
    SetAlphaCurrent = 'd',
    SetAlphaStep = 'e',
    SetBarTarget = 'f',
    SetBarStep = 'g',
    SetMode = 'h',
};

static inline const char* DirectorWideScreenCmdToString(DirectorWideScreenCmd cmd) {
    switch (cmd) {
        case DirectorWideScreenCmd::End: return "End";
        case DirectorWideScreenCmd::SetAlphaTarget: return "SetAlphaTarget";
        case DirectorWideScreenCmd::SetAlphaCurrent: return "SetAlphaCurrent";
        case DirectorWideScreenCmd::SetAlphaStep: return "SetAlphaStep";
        case DirectorWideScreenCmd::SetBarTarget: return "SetBarTarget";
        case DirectorWideScreenCmd::SetBarStep: return "SetBarStep";
        case DirectorWideScreenCmd::SetMode: return "SetMode";
        default: return "UnknownWideScreenCmd";
    }

    return "UnknownWideScreenCmd";
}

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

static inline const char* DirectorCameraCmdToString(DirectorCameraCmd cmd) {
    switch (cmd) {
        case DirectorCameraCmd::EnableNisCamera: return "EnableNisCamera";
        case DirectorCameraCmd::ClearCameraFlag: return "ClearCameraFlag";
        case DirectorCameraCmd::SetCameraFlag: return "SetCameraFlag";
        case DirectorCameraCmd::CopyP3DFov: return "CopyP3DFov";
        case DirectorCameraCmd::ResetCameraFov: return "ResetCameraFov";
        case DirectorCameraCmd::SetCameraMode: return "SetCameraMode";
        case DirectorCameraCmd::LoadAsyncAnim: return "LoadAsyncAnim";
        case DirectorCameraCmd::DeleteAsyncAnim: return "DeleteAsyncAnim";
        case DirectorCameraCmd::PlayAsyncAnim: return "PlayAsyncAnim";
        case DirectorCameraCmd::ShakeCamera: return "ShakeCamera";
        case DirectorCameraCmd::LookAtNisPoint: return "LookAtNisPoint";
        case DirectorCameraCmd::SetCameraAndLookAt: return "SetCameraAndLookAt";
        default: return "UnknownCameraCmd";
    }

    return "UnknownCameraCmd";
}

enum class DirectorHudCmd : s32 {
    HideHud = 'J',
    ShowHud = 'K',
    DisplayTally = 'L',
    ShowBossHealth = 'M',
};

static inline const char* DirectorHudCmdToString(DirectorHudCmd cmd) {
    switch (cmd) {
        case DirectorHudCmd::HideHud: return "HideHud";
        case DirectorHudCmd::ShowHud: return "ShowHud";
        case DirectorHudCmd::DisplayTally: return "DisplayTally";
        case DirectorHudCmd::ShowBossHealth: return "ShowBossHealth";
        default: return "UnknownHudCmd";
    }

    return "UnknownHudCmd";
}

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

static inline const char* DirectorHumanoidCmdToString(DirectorHumanoidCmd cmd) {
    switch (cmd) {
        case DirectorHumanoidCmd::End: return "End";
        case DirectorHumanoidCmd::EnterNis: return "EnterNis";
        case DirectorHumanoidCmd::EnterNisMove: return "EnterNisMove";
        case DirectorHumanoidCmd::ExitNis: return "ExitNis";
        case DirectorHumanoidCmd::FaceAngleDegrees: return "FaceAngleDegrees";
        case DirectorHumanoidCmd::StandFacingZero: return "StandFacingZero";
        case DirectorHumanoidCmd::SetPosition: return "SetPosition";
        case DirectorHumanoidCmd::PlayDynamicAnim: return "PlayDynamicAnim";
        case DirectorHumanoidCmd::SetStandState: return "SetStandState";
        case DirectorHumanoidCmd::SetPositionByCurrent: return "SetPositionByCurrent";
        default: return "UnknownHumanoidCmd";
    }

    return "UnknownHumanoidCmd";
}

enum class DirectorLadderCmd : s32 {
    End = 5,
    FaceLadderPoint = 'B',
    TeleportPlayer = 'C',
    CameraLookAtHatch = 'D',
    CloseHatch = 'E',
    ClearNis = 'F',
};

static inline const char* DirectorLadderCmdToString(DirectorLadderCmd cmd) {
    switch (cmd) {
        case DirectorLadderCmd::End: return "End";
        case DirectorLadderCmd::FaceLadderPoint: return "FaceLadderPoint";
        case DirectorLadderCmd::TeleportPlayer: return "TeleportPlayer";
        case DirectorLadderCmd::CameraLookAtHatch: return "CameraLookAtHatch";
        case DirectorLadderCmd::CloseHatch: return "CloseHatch";
        case DirectorLadderCmd::ClearNis: return "ClearNis";
        default: return "UnknownLadderCmd";
    }

    return "UnknownLadderCmd";
}

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

static inline const char* DirectorDoorCmdToString(DirectorDoorCmd cmd) {
    switch (cmd) {
        case DirectorDoorCmd::End: return "End";
        case DirectorDoorCmd::SetDoor: return "SetDoor";
        case DirectorDoorCmd::OpenDoor: return "OpenDoor";
        case DirectorDoorCmd::SetDoorState: return "SetDoorState";
        case DirectorDoorCmd::FaceDoorPoint: return "FaceDoorPoint";
        case DirectorDoorCmd::FaceDoorAngle: return "FaceDoorAngle";
        case DirectorDoorCmd::AttachToDoor: return "AttachToDoor";
        case DirectorDoorCmd::TeleportThroughDoor: return "TeleportThroughDoor";
        default: return "UnknownDoorCmd";
    }

    return "UnknownDoorCmd";
}

enum class DirectorEdisonCmd : s32 {
    PlayTransient = 108,
    StopMusic = 109,
};

static inline const char* DirectorEdisonCmdToString(DirectorEdisonCmd cmd) {
    switch (cmd) {
        case DirectorEdisonCmd::PlayTransient: return "PlayTransient";
        case DirectorEdisonCmd::StopMusic: return "StopMusic";
        default: return "UnknownEdisonCmd";
    }

    return "UnknownEdisonCmd";
}

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
//   +76:  handlerSetA (HandlerSet)  - internal director handler set
//   +112: handlerSetB (HandlerSet)  - internal director handler set
//   +148: runDirectorHandler (ptr)  - Game handlerSet1 node
//   +152: overlayHandler (ptr)      - Game handlerSet2 node
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

    // PC-only: while set, the next widescreen bar/alpha ramp-in snaps instantly
    // instead of animating (used to avoid a redundant slide/fade-in right after
    // a loading screen). Expires on its own after a short window so it never
    // suppresses an unrelated later cutscene's entrance.
    f64 wsSkipEnterAnimUntil = -1.0;

    s32 field68 = 0;                // +68: script loop blocked flag
    s32 enableInput = 0;            // +72: player input enabled (1=yes, 0=NIS disabled)

    HandlerSet handlerSetA;         // +76: internal director handler set A
    HandlerSet handlerSetB;         // +112: internal director handler set B
    Handler* runDirectorHandler = nullptr;      // +148: Game think-list handler
    Handler* overlayHandler = nullptr;          // +152: Game draw-list handler

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

    Director();
    ~Director() override;
    void InternalOpen() override;
    void InternalClose() override;
    void InternalReset() override;
    void LevelReset();
    void SetScript();
    void SetCodeSnip(s32* snip, Thing* thing);
    bool TriggerDeathVolume(s32 deathType);
    void TriggerGotoPoint(s32 x, s32 y, s32 z, Thing* thing);

    // Main script interpreter - dispatches script opcodes.
    void Process();
    void ProcessSoundScript();

    // Returns 1 when done/reset, 0 when blocked.
    s32 TimerStep();
    void Loop();
    void SetDesiredWideScreen();
    void SkipNextWideScreenEnterAnim();
    void ProcessEdison();
    void ProcessModelFunc();
    void ProcessCameraFunc();
    void ProcessHudFunc();
    void ProcessHumanoidFunc();
    void ProcessLadderFunc();
    void ProcessDoorFunc();
    void DetermineVictoryIdle();
    void DetermineLevelIntro();
    void DetermineDeath();
    // Returns 1 when done, 0 when still waiting.
    s32 WaitAnimationDoneStep();
    void ProcessDynamicAnimFunc();
    void HandleWideScreen();
    void DrawWideScreenPolys();
    void PurgeAnims();
    bool DoesLevelHaveExtraMem(s32 level);
    void updateVramAnims();
    void cleanUpTexAnim();

    // Script accessor methods for obstacle cutscenes
    static s32* GetNISDoor1Script();
    static s32* GetNISDoor1WithDialogScript();
    static s32* GetNISLadder1Script();
    static s32* GetDeathScript();
    static s32* GetLevelEndScript();
    static s32* GetGlobalScriptByIndex(s32 index);
};

void runDirector(Handler* h);
void DrawDirectorOverlays(Handler* h);

// Global Director pointer
extern Director* g_director;
