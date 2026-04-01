// game.cpp
#include "gen/game.h"
#include "gen/block.h"
#include "config.h"
#include "p3d/context.h"
#include "p3d/input.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"

const Game::StateFunc Game::sStateTable[static_cast<int>(GameState::COUNT)] = {
    gsNullState,
    gsIntroState,
    gsTitleState,
    gsTitleLoopState,
    gsInitState,
    gsOpenFEState,
    gsFEState,
    gsPrePlayState,
    gsPlayState,
    gsEndLevelState,
    gsEndLevelLoopState,
    gsEndLevelExitState,
    gsPlayMovieCredits,
    gsDbgMenuState,
    gsMenuState,
    gsErrorState,
    gsErrorLoopState,
    gsErrorExitState,
    gsLocationMenuState,
    gsOpenLocationState,
    gsQueueLevelLoad,
    gsQueuePetalLoad,
    gsQueueLevelPetalLoad,
    gsDetermineNextGameState,
    gsDetermineGameOverState,
    gsEndGameState,
    gsEndGameLoopState,
    gsEndState,
};

Game::Game() {
    MARKFUNCTION(0x800C9AEC); // __4Game

    // PSX: InputManager created during game init, stored at 0x800DD69C
    if (!g_inputManager) {
        g_inputManager = new InputManager();
    }

    SetState(GameState::Null);
    RC_LOG("[Game] Created");
}

bool Game::Step() {
    MARKFUNCTION(0x8002B65C); // Step__4Game
    if (stateFunc)
        return stateFunc(this);
    return false;
}

void Game::SetState(GameState s) {
    MARKFUNCTION(0x8002C5AC); // SetState
    if (s == state)
        return;

    prevState = state;
    s32 idx = static_cast<s32>(s);
    if (idx >= 0 && idx < static_cast<s32>(GameState::COUNT))
        stateFunc = sStateTable[idx];
    else
        stateFunc = nullptr;
    state = s;

    RC_LOG("[Game] State: %d -> %d", static_cast<int>(prevState), static_cast<int>(state));
}

bool Game::gsNullState(Game*) {
    MARKFUNCTION(0x80029328); // gsNullState
    return true;
}

bool Game::gsIntroState(Game* game) {
    MARKFUNCTION(0x800C99A0); // gsIntroState
    RC_LOG("[Game] Intro (skipped) -> Init");
    game->SetState(GameState::Init);
    return true;
}

bool Game::gsTitleState(Game* game) {
    MARKFUNCTION(0x80029450); // gsTitleState
    RC_LOG("[Game] Title -> TitleLoop");
    game->SetState(GameState::TitleLoop);
    return true;
}

bool Game::gsTitleLoopState(Game* game) {
    MARKFUNCTION(0x8002BE0C); // gsTitleLoopState
    // Skip title screen, go straight to level loading
    game->SetState(GameState::QueueLevelLoad);
    return true;
}

bool Game::gsInitState(Game* game) {
    MARKFUNCTION(0x80029460); // gsInitState
    RC_LOG("[Game] Init -> Title");
    game->SetState(GameState::Title);
    return true;
}

bool Game::gsOpenFEState(Game* game) {
    MARKFUNCTION(0x800299B8); // gsOpenFEState
    RC_LOG("[Game] OpenFE -> FE");
    game->SetState(GameState::FE);
    return true;
}

bool Game::gsFEState(Game*) {
    MARKFUNCTION(0x80029A48); // gsFEState
    return true;
}

bool Game::gsPrePlayState(Game* game) {
    MARKFUNCTION(0x80029AC0); // gsPrePlayState
    RC_LOG("[Game] PrePlay -> Play");
    game->SetState(GameState::Play);
    return true;
}

bool Game::gsPlayState(Game* game) {
    MARKFUNCTION(0x80029C6C); // gsPlayState

    // --- PSX per-frame input pipeline ---
    // PSX: ReadSonyPads() -> ServiceInput(buttons, 0) -> Step()
    // PC: keyboard -> button bits -> ServiceInput(buttons, 0) -> Step()
#if RC_FEATURE_PAD_KEYBOARD_EMULATION
    g_inputManager->UpdateFromKeyboard(p3d::input, 0);
#endif
    g_inputManager->Step();

    // Run the reversed PSX camera pipeline
    game->gameCamera.Think();
    game->gameCamera.Update();

    const LVector& camPos = game->gameCamera.GetPosition();

    p3d::context->EnableZBuffer(true);
    p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    p3d::context->SetCullMode(PDDI_CULL_NONE);

    game->view.BeginRender();
    game->world.Render(&camPos);
    game->view.EndRender();

    return true;
}

bool Game::gsEndLevelState(Game* game) {
    MARKFUNCTION(0x8002B688); // gsEndLevelState
    game->SetState(GameState::EndLevelLoop);
    return true;
}

bool Game::gsEndLevelLoopState(Game*) {
    MARKFUNCTION(0x8002B6B0); // gsEndLevelLoopState
    return true;
}

bool Game::gsEndLevelExitState(Game* game) {
    MARKFUNCTION(0x8002B744); // gsEndLevelExitState
    game->SetState(GameState::DetermineNextGameState);
    return true;
}

bool Game::gsPlayMovieCredits(Game* game) {
    MARKFUNCTION(0x8002CB28); // gsPlayMovieCredits
    game->SetState(GameState::End);
    return true;
}

bool Game::gsDbgMenuState(Game*) {
    MARKFUNCTION(0x8002A174); // gsDbgMenuState
    return true;
}

bool Game::gsMenuState(Game*) {
    MARKFUNCTION(0x80029EF8); // gsMenuState
    return true;
}

bool Game::gsErrorState(Game* game) {
    MARKFUNCTION(0x80029F64); // gsErrorState
    game->SetState(GameState::ErrorLoop);
    return true;
}

bool Game::gsErrorLoopState(Game*) {
    MARKFUNCTION(0x8002A064); // gsErrorLoopState
    return true;
}

bool Game::gsErrorExitState(Game* game) {
    MARKFUNCTION(0x8002A004); // gsErrorExitState
    game->SetState(GameState::Title);
    return true;
}

bool Game::gsLocationMenuState(Game*) {
    MARKFUNCTION(0x8002A128); // gsLocationMenuState
    return true;
}

bool Game::gsOpenLocationState(Game* game) {
    MARKFUNCTION(0x80029A70); // gsOpenLocationState
    game->SetState(GameState::LocationMenu);
    return true;
}

bool Game::gsQueueLevelLoad(Game* game) {
    MARKFUNCTION(0x80029574); // gsQueueLevelLoad
    RC_LOG("[Game] QueueLevelLoad: loading LEV01.LCF");
    game->world.Load("RTARGET/LEV01.LCF");

    // Position camera at center of level
    {
        const LVector& lo = game->world.GetLevelMin();
        const LVector& hi = game->world.GetLevelMax();
        s32 cx = (lo.x + hi.x) / 2;
        s32 cy = (lo.y + hi.y) / 2;
        s32 cz = lo.z - (hi.z - lo.z) / 2;

        // Reset and configure the PSX camera
        game->gameCamera.Reset();
        game->gameCamera.SetMode(CAM_MODE_DEFAULT); // DebugCam mode

        // Set initial camera position at level center, backed off in Z
        game->gameCamera.SetPosition(cx, cy, cz);

        tCamera* cam = game->gameCamera.GetP3DCamera();
        cam->SetFOV(0.7f, 4.0f / 3.0f);
        cam->SetNearPlane(100.0f);
        cam->SetFarPlane(500000.0f);

        RC_LOG("[Game] Camera positioned at (%d, %d, %d), DebugCam mode active", cx, cy, cz);
        RC_LOG("[Game] Controls: WASD=move, IJKL=rotate, Enter=Start, N=L3 (up/down)");
    }

    // Use PSX camera for view
    game->view.SetCamera(game->gameCamera.GetP3DCamera());
    game->view.SetBackgroundColour(pddiColour(30, 30, 35));
    game->view.SetClearMask(PDDI_BUFFER_ALL);

    game->SetState(GameState::DetermineNextGameState);
    return true;
}

bool Game::gsQueuePetalLoad(Game* game) {
    MARKFUNCTION(0x8002977C); // gsQueuePetalLoad
    game->SetState(GameState::DetermineNextGameState);
    return true;
}

bool Game::gsQueueLevelPetalLoad(Game* game) {
    MARKFUNCTION(0x8002986C); // gsQueueLevelPetalLoad
    game->SetState(GameState::DetermineNextGameState);
    return true;
}

bool Game::gsDetermineNextGameState(Game* game) {
    MARKFUNCTION(0x80029924); // gsDetermineNextGameState
    game->SetState(GameState::PrePlay);
    return true;
}

bool Game::gsDetermineGameOverState(Game* game) {
    MARKFUNCTION(0x800299B0); // gsDetermineGameOverState
    game->SetState(GameState::EndGame);
    return true;
}

bool Game::gsEndGameState(Game* game) {
    MARKFUNCTION(0x8002C3B4); // gsEndGameState
    game->SetState(GameState::EndGameLoop);
    return true;
}

bool Game::gsEndGameLoopState(Game*) {
    MARKFUNCTION(0x8002C22C); // gsEndGameLoopState
    return true;
}

bool Game::gsEndState(Game*) {
    MARKFUNCTION(0x8002A17C); // gsEndState
    return false;
}
