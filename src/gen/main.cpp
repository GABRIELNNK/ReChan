// main.cpp
#include "common.h"
#include "p3d/context.h"
#include "p3d/inventory.h"
#include "p3d/loadmanager.h"
#include "p3d/texture.h"
#include "p3d/shader.h"
#include "p3d/stream.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "gen/game.h"
#include "gen/time.h"

#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

int main() {
    Log::Get().Init();

    tPlatform* platform = tPlatform::Create();

    tContextInitData init;
    init.xSize = 1280;
    init.ySize = 720;
    init.title = "Jackie Chan Stuntmaster";

    tContext* ctx = platform->CreateContext(init);
    if (!ctx)
        return 1;

    Game game;
    game.Open();
    game.SetState(GameState::Intro);

    p3d::context->SetClearColour(pddiColour(30, 30, 35));

    MARKFUNCTION(0x8002635C);

    using Clock = std::chrono::steady_clock;

    auto prevTime = Clock::now();

    // PSX main (MAIN.CPP:519, 0x8002635C):
    //   while (!quit) {
    //       while (Step(game) && !quit) rDoTaskList();
    //       SetLivesLeft(player, 4); SetState(game, QueueLevelLoad);
    //   }
    // PSX main does NOT call BeginFrame/EndFrame - that is the
    // responsibility of the game states via Display handler callbacks
    // in ProcessHandlers, or inline in menu states like MenuDraw.
    // PC mirrors this: main.cpp only does timing, events, and swap.

    while (!p3d::display->ShouldClose()) {
        auto frameStart = Clock::now();

        // Measure real elapsed time since last frame
        f32 realDt = std::chrono::duration<f32>(frameStart - prevTime).count();
        prevTime = frameStart;
        g_time->Tick(realDt);
        g_time->Step();

        p3d::display->PollEvents();

        bool running = game.Step();
        if (!running) {
            // PSX: when Step returns false (from gsDetermineGameOverState or gsEndState),
            // the main loop resets lives via SetLivesLeft and re-enters via QueueLevelLoad.
            // PSX: SetLivesLeft(g_player, savedLives)
            game.SetState(GameState::QueueLevelLoad);
        }

        // Sleep to maintain target framerate (0 = uncapped)
        f32 targetDt = g_time->GetTargetDt();
        if (targetDt > 0.0f) {
            auto frameEnd = Clock::now();
            f32 elapsed = std::chrono::duration<f32>(frameEnd - frameStart).count();
            if (elapsed < targetDt) {
                std::this_thread::sleep_for(std::chrono::duration<f32>(targetDt - elapsed));
            }
        }
    }

    platform->DestroyContext(ctx);
    tPlatform::Destroy();

    LOG("Clean shutdown");
    Log::Get().Shutdown();

    return 0;
}
