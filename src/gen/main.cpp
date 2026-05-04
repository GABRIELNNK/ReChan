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
#include "pc/inputaction.h"
#include "gen/game.h"
#include "gen/time.h"
#include "gen/display.h"
#include "extra/fecustommenumgr.h"
#include "radlib/rtask.h"
#include "ai/player.h"

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
                // Re-capture mouse only when the menu is not open
                if (g_display && (!g_feCustomMenuMgr || !g_feCustomMenuMgr->IsActive()))
                    g_display->SetCursorCaptured(true);
            }
            else {
                // Lost focus
                if (p3d::input)
                    p3d::input->SetEnabled(false);
                if (AudioEngine::IsInitialized()) {
                    sSavedMasterVolume = AudioEngine::GetMasterVolume();
                    AudioEngine::SetMasterVolume(0.0f);
                }
                // Always release mouse when losing focus
                if (g_display)
                    g_display->SetCursorCaptured(false);
            }
            break;
        case PDDI_WND_SCROLL:
            if (g_actionInput && g_feCustomMenuMgr && g_feCustomMenuMgr->IsActive()) {
                const s32 dir = (msg.fparam2 > 0.0) ? 1 : -1;
                g_actionInput->AddScrollDelta(dir);
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

    p3d::display->SetWndProc(OnWndProc);
    p3d::display->SetOverlayCallback(DebugUI::Draw);

    rTaskInit(100);
    rInitTaskList(&rMainTaskList);
    rInitTaskList(&rFrameTaskList);
    rFrameCount = 0;
    rFrameCount60 = 0;

    Game game;
    game.Open();

    if (!g_actionInput) {
        g_actionInput = new ActionInput();
    }

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

        rFrameCount = 1;
        rFrameCount60 = 1;
        rDoTaskList(&rFrameTaskList, 0);

        p3d::display->PollEvents();

        if (p3d::input) {
            p3d::input->ServiceInput();
        }
        if (g_actionInput) {
            g_actionInput->Update(p3d::input);
        }
        if (g_inputManager) {
            const ActionInput* actionInputForGame = DebugUI::ShouldBlockGameInput()
                ? nullptr
                : g_actionInput;
            g_inputManager->ServiceHostPads(actionInputForGame);
        }

        bool running = game.Step();
        if (!running) {
#if QUIT_GAME_CLOSES_GAME
            if (game.GetState() == GameState::End) {
                break;
            }
#endif
            // PSX: SetLivesLeft(g_player, savedLives)
            Player::s_player->SetLivesLeft(4);
            game.SetState(GameState::QueueLevelLoad);
        }
        else {
            rDoTaskList(&rMainTaskList, 0);
        }

        g_time->WaitForFrameEnd(frameStart);

        // Update title bar with FPS every 30 frames
        static u32 titleCounter = 0;
        if (++titleCounter >= 30) {
            char titleBuf[128];
            snprintf(titleBuf, sizeof(titleBuf), "%s - %.1f fps (%.2f ms)", JCSM_TITLE, g_time->fps, g_time->deltaTime * 1000.0f);
            p3d::display->SetTitle(titleBuf);
            titleCounter = 0;
        }
    }

    platform->DestroyContext(ctx);
    tPlatform::Destroy();

    delete g_actionInput;
    g_actionInput = nullptr;

    LOG("Clean shutdown");
    Log::Get().Shutdown();

    return 0;
}
