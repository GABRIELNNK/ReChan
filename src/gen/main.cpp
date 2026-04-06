// main.cpp
#include "common.h"
#include "p3d/context.h"
#include "p3d/input.h"
#include "p3d/inventory.h"
#include "p3d/loadmanager.h"
#include "p3d/texture.h"
#include "p3d/shader.h"
#include "p3d/stream.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "pc/audio.h"
#include "pc/debugui.h"
#include "gen/game.h"
#include "gen/time.h"

#include <vector>
#include <algorithm>

static f32 sSavedMasterVolume = 1.0f;

static bool OnWndProc(const pddiWndMessage& msg) {
    switch (msg.event) {
    case PDDI_WND_FOCUS:
        if (msg.param1) {
            // Regained focus
            if (p3d::input)
                p3d::input->SetEnabled(true);
            if (AudioEngine::IsInitialized())
                AudioEngine::SetMasterVolume(sSavedMasterVolume);
        } else {
            // Lost focus
            if (p3d::input)
                p3d::input->SetEnabled(false);
            if (AudioEngine::IsInitialized()) {
                sSavedMasterVolume = AudioEngine::GetMasterVolume();
                AudioEngine::SetMasterVolume(0.0f);
            }
        }
        break;
    default:
        break;
    }
    return false;
}

int main() {
    Log::Get().Init();

    tPlatform* platform = tPlatform::Create();

    tContextInitData init;
    init.xSize = JCSM_TARGET_WIDTH;
    init.ySize = JCSM_TARGET_HEIGHT;
    init.title = JCSM_TITLE;

    tContext* ctx = platform->CreateContext(init);
    if (!ctx)
        return 1;

    p3d::display->ShowCursor(false);
    p3d::display->SetWndProc(OnWndProc);
    p3d::display->SetOverlayCallback(DebugUI::Draw);

    Game game;
    game.Open();
    game.SetState(GameState::Intro);

    p3d::context->SetClearColour(pddiColour(30, 30, 35));

    MARKFUNCTION(0x8002635C);

    f64 prevTime = Time::GetTimeInSeconds();

    while (!p3d::display->ShouldClose()) {
        f64 frameStart = Time::GetTimeInSeconds();

        f32 realDt = (f32)(frameStart - prevTime);
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
            f32 elapsed = (f32)(Time::GetTimeInSeconds() - frameStart);
            if (elapsed < targetDt) {
                Time::Sleep(targetDt - elapsed);
            }
        }
    }

    platform->DestroyContext(ctx);
    tPlatform::Destroy();

    LOG("Clean shutdown");
    Log::Get().Shutdown();

    return 0;
}
