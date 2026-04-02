// game.h - Game class reversed from PSX GAME.CPP
// PSX source: C:\CHAN\GAME\SRC\GEN\GAME.CPP
// Game is the top-level Manager that owns all other managers via a ccList,
// and two HandlerSets for per-frame think and draw callbacks.
#pragma once

#include "core.h"
#include "gen/manager.h"
#include "gen/handler.h"
#include "gen/camera.h"
#include "gen/control.h"
#include "p3d/view.h"

// Forward declarations - managers created in InternalOpen
class World;

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
    Camera& GetCamera() { return gameCamera; }
    tView& GetView() { return view; }
    HandlerSet& GetHandlerSet1() { return handlerSet1; }
    HandlerSet& GetHandlerSet2() { return handlerSet2; }
    s32 GetControlVal(s32 pad) const { return controlVal[pad]; }

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

    // PC: non-PSX members for current rendering setup
    Camera gameCamera;
    tView view;

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

    // PSX handler callbacks (registered in Game constructor)
    static void BeginFrameHandler(Handler* h);
    static void DrawEverythingHandlerCB(Handler* h);
    static void EndFrameHandler(Handler* h);
};

// Global game pointer (PSX: accessible through gp-relative)
extern Game* g_game;

