// game.h
#pragma once

#include "core.h"
#include "gen/world.h"
#include "gen/camera.h"
#include "gen/control.h"
#include "p3d/view.h"

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

class Game {
public:
    Game();

    bool Step();
    void SetState(GameState state);

    GameState GetState() const { return state; }
    GameState GetPrevState() const { return prevState; }

private:
    using StateFunc = bool(*)(Game*);

    GameState state = GameState::Null;
    GameState prevState = GameState::Null;
    StateFunc stateFunc = nullptr;

    World world;
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
};
