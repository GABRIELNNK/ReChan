// main.cpp
#include "core.h"
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

#include <cstdio>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);

    tPlatform* platform = tPlatform::Create();

    tContextInitData init;
    init.xSize = 1280;
    init.ySize = 720;
    init.title = "Jackie Chan Stuntmaster";

    tContext* ctx = platform->CreateContext(init);
    if (!ctx) return 1;

    Game game;
    game.Open(); // PSX: Manager::Open() -> InternalOpen() creates all managers
    game.SetState(GameState::Intro);

    p3d::context->SetClearColour(pddiColour(30, 30, 35));

    MARKFUNCTION(0x8002635C); // psx_main

    // --- Simple frame-limited game loop ---
    // Runs at g_time->targetFPS (default 30). One logic tick + one render per frame.
    // Matches PSX behaviour: all physics values assume 30 fps per-frame execution.
    using Clock = std::chrono::steady_clock;

    auto prevTime = Clock::now();

    while (!p3d::display->ShouldClose()) {
        auto frameStart = Clock::now();

        // Measure real elapsed time since last frame
        f32 realDt = std::chrono::duration<f32>(frameStart - prevTime).count();
        prevTime = frameStart;
        g_time->Tick(realDt);
        g_time->Step();

        ctx->BeginFrame();
        p3d::context->Clear(PDDI_BUFFER_ALL);

        bool running = game.Step();
        if (!running)
            game.SetState(GameState::QueueLevelLoad);

        ctx->EndFrame();

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

    RC_LOG("Clean shutdown");
    return 0;
}
