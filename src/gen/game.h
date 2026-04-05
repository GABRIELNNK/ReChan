// game.h - Game class reversed from PSX GAME.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\GAME.CPP
// Game is the top-level Manager that owns all other managers via a ccList,
// and two HandlerSets for per-frame think and draw callbacks.
#pragma once

#include "common.h"
#include "gen/manager.h"
#include "gen/handler.h"
#include "gen/control.h"

class Camera;
class tView;

// Forward declarations - managers created in InternalOpen
class World;
class TitleScreen;
class GameOverScreen;
class tTexture;

enum class GameState : s32 {
    Null = 0,
    Intro,
    Title,
    TitleLoop,
    Init,
    OpenFE,
    FE,
    PrePlay,
    Play,
    EndLevel,
    EndLevelLoop,
    EndLevelExit,
    PlayMovieCredits,
    DbgMenu,
    Menu,
    Error,
    ErrorLoop,
    ErrorExit,
    LocationMenu,
    OpenLocationMenu,
    QueueLevelLoad,
    QueuePetalLoad,
    QueueLevelPetalLoad,
    DetermineNextGameState,
    DetermineGameOverState,
    EndGame,
    EndGameLoop,
    End,
    COUNT
};

// Game (140 bytes on PSX) - extends Manager
// PSX layout:
//   +0:  Manager base (28 bytes: ccNode(24) + isOpen(s16))
//   +28: gameState (s32)
//   +32: prevState (s32)
//   +36: stateFunc (ptr)
//   +40: managerList (ccList, 12 bytes)
//   +52: padding / reserved
//   +56: handlerSet1 (HandlerSet, 36 bytes) - think/logic handlers
//   +92: handlerSet2 (HandlerSet, 36 bytes) - draw/render handlers
//   +128: controlVal[2] (s32[2]) - per-pad button bitmask
//   +136: field136 (s32)
class Game : public Manager {
public:
    Game();
    ~Game() override;

    bool Step();
    void SetState(GameState state);

    GameState GetState() const { return state; }
    GameState GetPrevState() const { return prevState; }

    // PSX: ProcessHandlers__4Game (GAME.CPP:2756, 0x8002B4F0)
    void ProcessHandlers();

    // Manager overrides
    void InternalOpen() override;
    void InternalClose() override;
    void InternalReset() override;

    // Access
    World* GetWorld() const;
    Camera& GetCamera();
    tView& GetView();
    HandlerSet& GetHandlerSet1() { return handlerSet1; }
    HandlerSet& GetHandlerSet2() { return handlerSet2; }
    s32 GetControlVal(s32 pad) const { return controlVal[pad]; }

    // PSX handler callbacks (registered in Game constructor)
    // On PSX these are free functions; public so MenuRender/MenuDraw can call them.
    static void BeginFrameHandler(Handler* h);
    static void DrawEverythingHandlerCB(Handler* h);
    static void EndFrameHandler(Handler* h);

private:
    using StateFunc = bool(*)(Game*);

    GameState state = GameState::Null;          // +28
    GameState prevState = GameState::Null;      // +32
    StateFunc stateFunc = nullptr;              // +36

    ccList managerList;                         // +40: all managers
    HandlerSet handlerSet1;                     // +56: think handlers
    HandlerSet handlerSet2;                     // +92: draw handlers
    s32 controlVal[2] = {};                     // +128: per-pad button state
    s32 field136 = 0;                           // +136

    // PSX globals stored per-game instance
    s32 introTimer = 0;                         // frame counter for intro screens
    s32 introPhase = 0;                         // sub-phase within intro state
    tTexture* introTexture = nullptr;           // current intro screen texture (PSX: gp+24)
    TitleScreen* titleScreen = nullptr;         // PSX: gp+60
    GameOverScreen* gameOverScreen = nullptr;    // PSX: gp+64
    s32 titleIdleTimer = 0;                     // PSX: gp+128
    s32 titleIdleBase = 0;                      // PSX: gp+124
    s32 firstBoot = 1;                          // PSX: gp+80

    // PC: per-frame fade state (PSX uses blocking inline loops)
    s32 titleFadeType = 0;
    s32 gameOverFadeType = 0;

    // PSX: PlayMovie__4GamePcii (GAME.CPP:3309, 0x8002BBF0)
    void PlayMovie(const char* name, s32 skippable, s32 unloadLevel);

    // PSX: FadeBegin__4Game (GAME.CPP:3869, 0x8002C9A0)
    static void FadeBegin();
    // PSX: FadeUpdate__4Game (GAME.CPP:3879, 0x8002C9BC)
    // Returns true (1) if fade is still in progress, false (0) when complete.
    static s32 FadeUpdate();
    // PSX: FadeEnd__4Game (GAME.CPP:3875, 0x8002C9B4)
    static void FadeEnd();
    // PSX: FadeRender__4Game (GAME.CPP:3893, 0x8002C9F8)
    // Draws a fullscreen black quad with alpha=fadeCounter.
    static void FadeRender();

    // PSX: fade globals (gp+3388, gp+3392)
    static u8 s_fadeStep;     // gp+3388: step per frame (init=17)
    static u8 s_fadeCounter;  // gp+3392: accumulated alpha (0-255)

    // PSX: FreeXconFE__4Game (GAME.CPP:3812, 0x8002C7A4)
    static void FreeXconFE();
    // PSX: InitXconFSImage__4Game (GAME.CPP:3826, 0x8002C838)
    static void InitXconFSImage();
    // PSX: FreeXconFSImage__4Game (GAME.CPP:3859, 0x8002C998)
    static void FreeXconFSImage();
    // PSX: LoadXconFE__4Game (GAME.CPP:3789, 0x8002C648)
    static void LoadXconFE();

    static const StateFunc sStateTable[static_cast<int>(GameState::COUNT)];

    static bool gsNullState(Game* game);
    static bool gsIntroState(Game* game);
    static bool gsTitleState(Game* game);
    static bool gsTitleLoopState(Game* game);
    static bool gsInitState(Game* game);
    static bool gsOpenFEState(Game* game);
    static bool gsFEState(Game* game);
    static bool gsPrePlayState(Game* game);
    static bool gsPlayState(Game* game);
    static bool gsEndLevelState(Game* game);
    static bool gsEndLevelLoopState(Game* game);
    static bool gsEndLevelExitState(Game* game);
    static bool gsPlayMovieCredits(Game* game);
    static bool gsDbgMenuState(Game* game);
    static bool gsMenuState(Game* game);
    static bool gsErrorState(Game* game);
    static bool gsErrorLoopState(Game* game);
    static bool gsErrorExitState(Game* game);
    static bool gsLocationMenuState(Game* game);
    static bool gsOpenLocationState(Game* game);
    static bool gsQueueLevelLoad(Game* game);
    static bool gsQueuePetalLoad(Game* game);
    static bool gsQueueLevelPetalLoad(Game* game);
    static bool gsDetermineNextGameState(Game* game);
    static bool gsDetermineGameOverState(Game* game);
    static bool gsEndGameState(Game* game);
    static bool gsEndGameLoopState(Game* game);
    static bool gsEndState(Game* game);
};

// Global game pointer (PSX: accessible through gp-relative)
extern Game* g_game;

