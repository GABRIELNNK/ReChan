#include "common.h"
#include "p3d/context.h"
#include "p3d/input.h"
#include "p3d/inventory.h"
#include "p3d/loadmanager.h"
#include "p3d/texture.h"
#include "p3d/shader.h"
#include "p3d/stream.h"
#include "p3d/skeleton.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "pc/audio.h"
#include "pc/debugui.h"
#include "pc/inputaction.h"
#include "pc/textmgr.h"
#include "gen/game.h"
#include "gen/time.h"
#include "gen/display.h"
#include "extra/fecustommenumgr.h"
#include "extra/assetexporter.h"
#include "extra/gltfloader.h"
#include "gen/model.h"
#include "radlib/rtask.h"
#include "ai/player.h"
#ifdef MOD_LOADER
#include "extra/modloader.h"
#endif

#if AUTO_UPDATER
#include "extra/autoupdater.h"
#endif

#include <vector>
#include <algorithm>

static f32 sSavedMasterVolume = 1.0f;
static bool sWindowedCaptureRequestedByClick = false;

static bool OnWndProc(const pddiWndMessage& msg) {
    switch (msg.event) {
        case PDDI_WND_FOCUS:
            if (msg.param1) {
                // Regained focus
                if (p3d::input)
                    p3d::input->SetEnabled(true);
                if (AudioEngine::IsInitialized())
                    AudioEngine::SetMasterVolume(sSavedMasterVolume);
                // Keep the mouse free on focus gain in windowed mode so a titlebar
                // click/drag can move the window. In-content mouse clicks will
                // recapture below via PDDI_WND_MOUSEBUTTON.
                if (g_display) {
                    if (DebugUI::IsEnabled()) {
                        g_display->SetCursorCaptured(false);
                        g_display->SetCursorVisible(true);
                    }
                    else if (!g_feCustomMenuMgr || !g_feCustomMenuMgr->IsActive()) {
                        if (g_display->GetScreenMode() == ScreenMode_Windowed) {
                            if (sWindowedCaptureRequestedByClick) {
                                g_display->SetCursorCaptured(true);
                            }
                            else {
                                g_display->SetCursorCaptured(false);
                                g_display->SetCursorVisible(true);
                            }
                            sWindowedCaptureRequestedByClick = false;
                        }
                        else {
                            g_display->SetCursorCaptured(true);
                        }
                    }
                }
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
                sWindowedCaptureRequestedByClick = false;
            }
            break;
        case PDDI_WND_MOUSEBUTTON:
            // GLFW mouse button callbacks only fire for client-area clicks, not
            // non-client titlebar drags. Use that to defer recapture in windowed
            // mode until the user clicks back into game content.
            if (msg.param2 && g_display &&
                g_display->GetScreenMode() == ScreenMode_Windowed &&
                !DebugUI::IsEnabled() &&
                (!g_feCustomMenuMgr || !g_feCustomMenuMgr->IsActive())) {
                sWindowedCaptureRequestedByClick = true;
                g_display->SetCursorCaptured(true);
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

int main(int argc, char** argv) {
    Log::Get().Init();

    // Headless verification/export path used by automated round-trip checks
    // and mod authors who want the same output as the DebugUI button.
    if (argc >= 2 && std::strcmp(argv[1], "--export-assets") == 0) {
        const char* outputDir = (argc >= 3 && argv[2] && argv[2][0])
            ? argv[2]
            : "~mods/ExportedAssets";
        AssetExporter& exporter = AssetExporter::Instance();
        exporter.BuildCatalog();
        const int exported = exporter.ExportAllCategories(outputDir);
        LOG("[AssetExporter] Headless export completed: %d asset bundle(s)", exported);
        Log::Get().Shutdown();
        return exported > 0 ? 0 : 2;
    }

#ifdef MOD_LOADER
    if (argc >= 3 && std::strcmp(argv[1], "--verify-png-baseline") == 0) {
        const bool unchanged = ModLoader::Instance().IsUnmodifiedTextureDump(argv[2]);
        LOG("[ModLoader] PNG baseline %s: %s", unchanged ? "matched" : "not matched", argv[2]);
        Log::Get().Shutdown();
        return unchanged ? 0 : 5;
    }
#endif

    tPlatform* platform = tPlatform::Create();

    tContextInitData init;
    init.xSize = JCSM_TARGET_WIDTH;
    init.ySize = JCSM_TARGET_HEIGHT;
    init.title = JCSM_TITLE;

    tContext* ctx = platform->CreateContext(init);
    if (!ctx)
        return 1;

#ifdef MOD_LOADER
    if (argc >= 3 && std::strcmp(argv[1], "--verify-glb") == 0) {
        OriginalGeo* geometry = GLTFLoader::LoadGeo(argv[2]);
        const bool valid = geometry && geometry->meshBuffer;
        delete geometry;
        platform->DestroyContext(ctx);
        tPlatform::Destroy();
        LOG("[GLTFLoader] Verification %s: %s", valid ? "passed" : "failed", argv[2]);
        Log::Get().Shutdown();
        return valid ? 0 : 3;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--verify-stree") == 0) {
        OriginalSTree* character = GLTFLoader::LoadSTree(argv[2]);
        const bool valid = character && character->skeleton && character->skinData
            && character->skeleton->numJoints > 0
            && character->skeleton->joints[0].meshBuffer;
        delete character;
        platform->DestroyContext(ctx);
        tPlatform::Destroy();
        LOG("[GLTFLoader] Skinned verification %s: %s", valid ? "passed" : "failed", argv[2]);
        Log::Get().Shutdown();
        return valid ? 0 : 4;
    }
#endif

    p3d::display->SetWndProc(OnWndProc);
    p3d::display->SetOverlayCallback(DebugUI::Draw);

    rTaskInit(100);
    rInitTaskList(&rMainTaskList);
    rInitTaskList(&rFrameTaskList);
    rFrameCount = 0;
    rFrameCount60 = 0;

    Game game;
    if (!g_actionInput) {
        g_actionInput = new ActionInput();
    }
    if (!g_textManager) {
        g_textManager = new TextManager();
        g_textManager->Init();
        TextFontDesc desc = {};
        desc.name = "Menu";
        desc.path = "pc/fonts/YIKES!__.ttf";
        desc.pixelHeight = 48;
        if (!g_textManager->LoadFont(desc)) {
            LOG("[TextManager] Failed to load menu font: %s", desc.path);
        }

        desc.name = "Legal";
        desc.path = "pc/fonts/Roboto-Regular.ttf";
        desc.pixelHeight = 48;
        if (!g_textManager->LoadFont(desc)) {
            LOG("[TextManager] Failed to load menu font: %s", desc.path);
        }
    }

    game.Open();

#ifdef MOD_LOADER
    ModLoader::Instance().Init();
#endif

    game.SetState(GameState::Intro);

    p3d::context->SetClearColour(pddiColour(30, 30, 35));

    MARKFUNCTION(0x8002635C);

    f64 prevTime = Time::GetTimeInSeconds();

    while (!p3d::display->ShouldClose()) {
        f64 frameStart = Time::GetTimeInSeconds();

        f32 realDt = (f32)(frameStart - prevTime);
        prevTime = frameStart;
        g_time->Tick(realDt);
#if HIGH_FPS_PLAY_PRESENTATION
        if (game.GetState() != GameState::Play) {
            g_time->Step();
        }
#else
        g_time->Step();
#endif

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
            bool commitInputNow = true;
#if HIGH_FPS_PLAY_PRESENTATION
            if (game.GetState() == GameState::Play) {
                commitInputNow = false;
            }
#endif
            g_inputManager->ServiceHostPads(actionInputForGame, commitInputNow);
        }

        const GameState stateBeforeStep = game.GetState();
        bool running = game.Step();
        if (!running) {
            // PC: If the game loop signals to stop, break out of the loop to shut down.
            if (game.GetState() == GameState::End) {
                break;
            }

            if (stateBeforeStep == GameState::DetermineGameOverState) {
                game.SetState(GameState::DetermineNextGameState);
            }
            else {
                // PSX: SetLivesLeft(g_player, savedLives)
                Player::s_player->SetLivesLeft(4);
                game.SetState(GameState::QueueLevelLoad);
            }
        }
        else {
            rDoTaskList(&rMainTaskList, 0);
        }

        g_time->WaitForFrameEnd(frameStart);

        // Update title bar with FPS every 30 frames
        static u32 titleCounter = 30;
        if (++titleCounter >= 30) {
            char titleBuf[128];
            snprintf(titleBuf, sizeof(titleBuf), "%s - %.1f fps (%.2f ms)", JCSM_TITLE, g_time->fps, g_time->deltaTime * 1000.0f);
            p3d::display->SetTitle(titleBuf);
            titleCounter = 0;
        }
    }

    platform->DestroyContext(ctx);
    tPlatform::Destroy();

    if (g_textManager) {
        g_textManager->Shutdown();
        delete g_textManager;
        g_textManager = nullptr;
    }

#if AUTO_UPDATER
    if (g_autoUpdater) {
        g_autoUpdater->Shutdown();
        delete g_autoUpdater;
        g_autoUpdater = nullptr;
    }
#endif

    if (g_psxDiscExtractor) {
        g_psxDiscExtractor->Shutdown();
        delete g_psxDiscExtractor;
        g_psxDiscExtractor = nullptr;
    }

    delete g_actionInput;
    g_actionInput = nullptr;

#ifdef MOD_LOADER
    ModLoader::Instance().Shutdown();
#endif

    LOG("Clean shutdown");
    Log::Get().Shutdown();

    return 0;
}
