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
#include "gen/assets.h"
#include "gen/game.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <vector>
#include <algorithm>

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

    Assets::SetRoot(".");

    Game game;
    game.SetState(GameState::Intro);

    p3d::context->SetClearColour(pddiColour(30, 30, 35));
    p3d::context->SetBlendMode(PDDI_BLEND_ALPHA);

    MARKFUNCTION(0x8002635C); // psx_main

    while (!p3d::display->ShouldClose()) {
        ctx->BeginFrame();
        p3d::context->Clear(PDDI_BUFFER_ALL);

        bool running = game.Step();
        if (!running)
            game.SetState(GameState::QueueLevelLoad);

        ctx->EndFrame();
    }

    platform->DestroyContext(ctx);
    tPlatform::Destroy();

    RC_LOG("Clean shutdown");
    return 0;
}
