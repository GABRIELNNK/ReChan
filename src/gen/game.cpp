// game.cpp
#include "common.h"
#include "gen/game.h"
#include <chrono>
#include "gen/world.h"
#include "gen/block.h"
#include "gen/charmgr.h"
#include "gen/database.h"
#include "gen/cammgr.h"
#include "gen/levelmgr.h"
#include "gen/time.h"
#include "gen/ai.h"
#include "gen/director.h"
#include "gen/scoremgr.h"
#include "snd/sound.h"
#include "snd/rsevent.h"
#include "snd/fesnd.h"
#include "ai/player.h"
#include "fe/femenumgr.h"
#include "fe/gamemenu.h"
#include "fe/titlescreen.h"
#include "fe/gameoverscreen.h"
#include "fe/xcfont.h"
#include "radmovie/movieplayer.h"
#include "pc/tim.h"
#include "config.h"
#include "p3d/context.h"
#include "p3d/input.h"
#include "p3d/texture.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"

// Global game pointer
Game* g_game = nullptr;

// PSX globals
s16 g_selectedLevel = -1;   // gp+44: queued level ID (-1 = none)
s32 g_directorActive = 0;   // gp+20: director intro script active
s32 g_feInitialized = 0;    // gp+88: FE memory puddle initialized

static constexpr u32 kControlStartAction = PsxPad::Start;
static constexpr u32 kControlConfirmAction = PsxPad::Cross;

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

    controlVal[0] = 0;
    controlVal[1] = 0;
    field136 = 0;

    // PSX: Game constructor creates 4 handlers in handlerSet2
    // BeginFrameHandler (pri=64), DrawEverythingHandler (pri=-16),
    // AnimateEverythingHandler (pri=-48), EndFrameHandler (pri=-64)
    // For now we register the ones we have:
    handlerSet2.AddHandler(BeginFrameHandler, 64);
    handlerSet2.AddHandler(DrawEverythingHandlerCB, -16);
    handlerSet2.AddHandler(EndFrameHandler, -64);

    // PSX: handlerSet1 gets AI and Director handler callbacks
    // aiPrivHandler (pri=0) - runs AI::MoveThings per frame
    handlerSet1.AddHandler(aiPrivHandler, 0);
    // runDirector (pri=-32) - runs Director::Process per frame
    handlerSet1.AddHandler(runDirector, -32);
    // DrawDirectorOverlays (pri=-64) - draws widescreen bars etc.
    handlerSet2.AddHandler(DrawDirectorOverlays, -48);

    SetState(GameState::Null);
    g_game = this;
    LOG("[Game] Created");
}

Game::~Game() {
    // Clean up intro/title resources
    if (introTexture) { introTexture->Release(); introTexture = nullptr; }
    if (titleScreen) { delete titleScreen; titleScreen = nullptr; }
    if (gameOverScreen) { delete gameOverScreen; gameOverScreen = nullptr; }
    FreeXconFE();
    if (g_oxFontFile) { delete g_oxFontFile; g_oxFontFile = nullptr; }
    ScreenDraw::Shutdown();

    // Close all managers
    for (ccMinNode* n = managerList.head; n; ) {
        ccMinNode* next = n->next;
        Manager* mgr = static_cast<Manager*>(n);
        mgr->Close();
        n = next;
    }
    g_game = nullptr;
}

// PSX: InternalOpen__4Game (GAME.CPP:2888, 0x800C9D08)
// Creates all game managers and adds them to managerList, then calls Open() on each.
void Game::InternalOpen() {
    MARKFUNCTION(0x800C9D08);

    // PSX creates managers in this order:
    // 1. tCellAlligator (8204) - memory allocator (not needed on PC)
    // 2. oxScreenManager (48) + FontInit - screen/font (TODO)

    // 3. Time (PSX: 40 bytes)
    g_time = new Time();
    g_time->SetName("Time", 0);
    managerList.AddNodePri(g_time);

    // 4. AI (116) - AI master
    g_ai = new AI();
    g_ai->SetName("AI", 0);
    managerList.AddNodePri(g_ai);

    // 5. World (PSX: 160 bytes)
    World* world = new World();
    world->SetName("World", 0);
    managerList.AddNodePri(world);

    // 6. EnvironmentManager (140) - environment effects (TODO)
    // 7. Display (32) - PSX display (not needed on PC)

    // 8. Director (212) - scripting/cutscenes
    g_director = new Director();
    g_director->SetName("Director", 0);
    managerList.AddNodePri(g_director);

    // 9. InputManager (PSX: 1492 bytes)
    if (!g_inputManager) {
        g_inputManager = new InputManager();
        g_inputManager->SetName("InputManager", 0);
        managerList.AddNodePri(g_inputManager);
    }

    // 10. LevelManager (PSX: 136 bytes)
    LevelManager* levelMgr = new LevelManager();
    levelMgr->SetName("LevelManager", 0);
    managerList.AddNodePri(levelMgr);

    // 11. Database (PSX: 120 bytes)
    Database* database = new Database();
    database->SetName("Database", 0);
    managerList.AddNodePri(database);

    // 12. Sound (PSX: 44 bytes)
    g_sound = new Sound();
    g_sound->SetName("Sound", 0);
    managerList.AddNodePri(g_sound);

    // 13. CameraManager (PSX: 60 bytes)
    CameraManager* camMgr = new CameraManager();
    camMgr->SetName("CameraManager", 0);
    managerList.AddNodePri(camMgr);

    // 14. BlockManager (PSX: 168 bytes)
    // BlockManager is currently owned by World, so we don't create a separate instance.
    // TODO: extract from World when block loading is decoupled

    // 15. AnimationManager (40) - animation playback (TODO)

    // 16. CharacterManager (PSX: 3004 bytes)
    g_characterManager = new CharacterManager();
    g_characterManager->SetName("CharacterManager", 0);
    managerList.AddNodePri(g_characterManager);

    // 17. ScoreManager (504) - score/collectibles
    g_scoreManager = new ScoreManager();
    g_scoreManager->SetName("ScoreManager", 0);
    managerList.AddNodePri(g_scoreManager);

    // Open all managers in list
    for (ccMinNode* n = managerList.head; n; n = n->next) {
        Manager* mgr = static_cast<Manager*>(n);
        mgr->Open();
    }

    LOG("[Game] InternalOpen: managers created");
}

// PSX: InternalClose__4Game (GAME.CPP:3003)
void Game::InternalClose() {
    MARKFUNCTION(0x8002A184);

    // Destroy player entity
    if (Player::s_player) {
        delete Player::s_player;
    }

    // Close all managers in reverse order
    for (ccMinNode* n = managerList.tail; n; ) {
        ccMinNode* prev = n->prev;
        Manager* mgr = static_cast<Manager*>(n);
        mgr->Close();
        n = prev;
    }
}

// PSX: InternalReset__4Game
void Game::InternalReset() {
    for (ccMinNode* n = managerList.head; n; n = n->next) {
        Manager* mgr = static_cast<Manager*>(n);
        mgr->Reset();
    }
}

// PSX: ProcessHandlers__4Game (GAME.CPP:2756, 0x8002B4F0)
// Iterates both handler sets and calls each handler's funcPtr
void Game::ProcessHandlers() {
    MARKFUNCTION(0x8002B4F0);

    // Process handlerSet1 (think/logic handlers)
    for (ccMinNode* n = handlerSet1.handlerList.head; n; ) {
        ccMinNode* next = n->next;
        Handler* h = static_cast<Handler*>(n);
        if (h->funcPtr) h->funcPtr(h);
        n = next;
    }

    // Process handlerSet2 (draw/render handlers)
    for (ccMinNode* n = handlerSet2.handlerList.head; n; ) {
        ccMinNode* next = n->next;
        Handler* h = static_cast<Handler*>(n);
        if (h->funcPtr) h->funcPtr(h);
        n = next;
    }
}

// Helper: get World from manager list
World* Game::GetWorld() const {
    // World is the first manager in the list (for now)
    for (ccMinNode* n = managerList.head; n; n = n->next) {
        World* w = dynamic_cast<World*>(static_cast<Manager*>(n));
        if (w) return w;
    }
    return nullptr;
}

// Handler callbacks
void Game::BeginFrameHandler(Handler*) {
    MARKFUNCTION(0x8002B408);
    // PSX: BeginFrame - P3D context begin frame
    // Now handled by main.cpp BeginFrame/EndFrame
}

// PSX: DrawEverythingHandler__FP7Handler (GAME.CPP:2211, 0x8002A98C)
// 1532 bytes, 91 blocks. Sorts blocks by distance, renders world geometry,
// characters, items, shadows per-block. Uses tView layers for ordering.
void Game::DrawEverythingHandlerCB(Handler*) {
    MARKFUNCTION(0x8002A98C);
    if (!g_game) return;
    World* world = g_game->GetWorld();
    if (!world) return;

    // PSX: sorts blocks by distance from camera, then iterates:
    //   EnterLayer(view, 0) -> DrawBG -> ExitLayer
    //   per-block: Render geometry, LookAt camera, render items/characters/shadows

    // PC: Camera think + view render
    g_game->gameCamera.Think();
    g_game->gameCamera.Update();

    p3d::context->EnableZBuffer(true);
    p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    p3d::context->SetCullMode(PDDI_CULL_NONE);

    g_game->view.BeginRender();

    const LVector& camPos = g_game->gameCamera.GetPosition();
    world->Render(&camPos);

    if (Player::s_player) {
        Player::s_player->Draw();
    }

    g_game->view.EndRender();
}

// PSX: MenuRender__FP7MenuMgr (GAME.CPP:1714, 0x80029D68)
// Renders the game world behind a menu overlay, then the menu itself.
static void MenuRender(MenuMgr* menuMgr) {
    MARKFUNCTION(0x80029D68);
    // PSX: DrawEverythingHandler(null)
    Game::DrawEverythingHandlerCB(nullptr);
    // PSX: DrawDirectorOverlays(null)
    DrawDirectorOverlays(nullptr);
    // PSX: HUD::Display()
    // TODO: g_hud->Display();
    // PSX: if menuMgr: menuMgr->Render() (via oxScreenManager)
    if (menuMgr) {
        menuMgr->Render();
    }
}

// PSX: MenuDraw__FP7MenuMgr (GAME.CPP:1735, 0x80029DB8)
// Calls Invoke on the menu, renders game + menu, returns Invoke result.
static s32 MenuDraw(MenuMgr* menuMgr) {
    MARKFUNCTION(0x80029DB8);
    s32 result;
    if (menuMgr) {
        result = menuMgr->Invoke();
    } else {
        result = 1;
    }
    // PSX: Display::BeginFrame, MenuRender, Display::EndFrame
    // PC: main.cpp handles BeginFrame/EndFrame
    MenuRender(menuMgr);
    return result;
}

void Game::EndFrameHandler(Handler*) {
    MARKFUNCTION(0x8002B420);
    // PSX: EndFrame - P3D context end frame
    // Now handled by main.cpp BeginFrame/EndFrame
}

bool Game::Step() {
    MARKFUNCTION(0x8002B65C); // Step__4Game

#if PAD_KEYBOARD_EMULATION
    if (g_inputManager) {
        // PSX services pad input from VBlank every frame. On PC we mirror that
        // behavior by feeding keyboard state once per game Step.
        g_inputManager->UpdateFromKeyboard(p3d::input, 0);
    }
#endif

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

    // PSX: for states that require input (TitleLoop=3, Play=8, EndGameLoop=26),
    // check if a pad is connected. If not, redirect to Error state.
    if (s == GameState::TitleLoop || s == GameState::Play || s == GameState::EndGameLoop) {
        if (g_inputManager) {
            // PSX: checks inputManager+56 (control flags) bit 0 = connected
            // On PC with keyboard, always connected
        }
    }

    LOG("[Game] State: %d -> %d", static_cast<int>(prevState), static_cast<int>(state));
}

bool Game::gsNullState(Game*) {
    MARKFUNCTION(0x80029328); // gsNullState
    return true;
}

bool Game::gsIntroState(Game* game) {
    MARKFUNCTION(0x800C99A0); // gsIntroState

#if SKIP_INTRO
    game->SetState(GameState::Init);
#endif

    // Phase 0: first entry - start 300-frame wait with LICENSE.TIM
    if (game->introPhase == 0) {
        game->introTimer = 0;
        TimImage* img = Tim::LoadFromFile("LICENSE.TIM");
        if (img) {
            game->introTexture = Tim::CreateTexture(img);
            delete img;
        }
        game->introPhase = 1;
    }

    // Phase 1: 300-frame wait (any button breaks)
    if (game->introPhase == 1) {
        if (game->introTexture) {
            ScreenDraw::DrawFullscreen(game->introTexture);
        }
        game->introTimer++;

        g_inputManager->Step();
        u32 buttons = g_inputManager->GetControlVal(0);

        // 300 on psx
        if (game->introTimer >= 100 || buttons != 0) {
            if (game->introTexture) {
                game->introTexture->Release();
                game->introTexture = nullptr;
            }

            game->PlayMovie("Mdwy320m.str", 1, 0);
            game->PlayMovie("radi.str", 1, 0);
            game->PlayMovie("dolby.str", 0, 0);

            game->introPhase = 0;
            game->introTimer = 0;

            // PSX: setup display RECT, GTE stereo, SetupEnv
            game->SetState(GameState::Init);
        }
        return true;
    }

    return true;
}

bool Game::gsTitleState(Game* game) {
    MARKFUNCTION(0x8002C474); // gsTitleState

    // PSX: FreeXconFE, InitXconFSImage
    FreeXconFE();
    InitXconFSImage();

    // PSX: destroy old screen manager, create TitleScreen(56)
    if (game->titleScreen) {
        delete game->titleScreen;
        game->titleScreen = nullptr;
    }
    game->titleScreen = new TitleScreen();
    game->titleScreen->Init("XC/TITLE.1", g_oxFontFile);

    // PSX: poll input to clear buffer
    g_inputManager->Step();
    g_inputManager->GetControlVal(0);

    // PSX: setup title control mode for both pads
    if (g_inputManager) {
        for (s16 pad = 0; pad < 2; pad++) {
            g_inputManager->SetControlModeArray(pad, TitleControlModeArray());
        }
    }

    // PSX: rsEvent(4, 22, 0, 0) - set sound location to title music
    rsEvent(RS_SET_LOCATION, 22, 0, 0);
    // PSX: rsEvent(5, 0, 0, 0) - start music
    rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);

    // PSX: reset idle timers
    game->titleIdleTimer = 0;
    game->titleIdleBase = 0;

    // PSX: ClearEasterEggs()

    p3d::context->SetClearColour(pddiColour(0, 0, 0));
    game->titleFadeType = 0;

    game->SetState(GameState::TitleLoop);
    return true;
}

bool Game::gsTitleLoopState(Game* game) {
    MARKFUNCTION(0x8002BE0C); // gsTitleLoopState

    // PSX: 1056 bytes, 73 blocks.
    // BeginFrame -> oxScreenManager Update+Render -> EndFrame
    // InputStep -> GetControlVal(0) -> idle timer check (900 frames)
    // Attract mode: rsEvent(6) -> FadeBegin -> fade loop -> FadeEnd -> rsEvent(3)
    //   -> PlayMovie("demo.str") -> ReloadFont -> gsTitleState
    // Normal: Start check -> PrintEasterEggs -> easter egg dispatch
    // Start press: ProcessSoundEvent(gp[72], 8) -> rsEvent(6) -> FadeBegin
    //   -> fade loop -> FadeEnd -> rsEvent(3) -> gp[28]=1 -> destroy screenMgr
    //   -> FreeXconFSImage -> LoadXconFE -> PlayMovie("prolog.str")
    //   -> LoadCharTexture(0) -> SetState(OpenFE=5)

    // PSX uses blocking inline fade loops. PC animates one step per frame.
    if (game->titleFadeType != 0) {
        // Continue rendering title screen behind the fade overlay
        if (game->titleScreen) {
            game->titleScreen->Update();
            game->titleScreen->Render();
        }

        s32 stillFading = FadeUpdate();
        FadeRender();

        if (!stillFading) {
            FadeEnd();

            if (game->titleFadeType == 2) {
                rsEvent(RS_UNLOAD_LEVEL, 0, 0, 0);
                game->PlayMovie("demo.str", 1, 0);
                LOG("[Game] TitleLoop: attract fade complete");
                game->titleIdleBase = 0;
                game->titleIdleTimer = 0;
                game->titleFadeType = 0;
                if (!g_oxFontFile) g_oxFontFile = new oxFontFile();
                g_oxFontFile->ReloadFont("XC/FONTS.1");
                gsTitleState(game);
            } else {
                rsEvent(RS_UNLOAD_LEVEL, 0, 0, 0);
                game->field136 = 1;
                if (game->titleScreen) {
                    delete game->titleScreen;
                    game->titleScreen = nullptr;
                }
                FreeXconFSImage();
                LoadXconFE();
                game->PlayMovie("prolog.str", 1, 0);
                LOG("[Game] TitleLoop: fade complete -> OpenFE");
                game->titleFadeType = 0;
                game->SetState(GameState::OpenFE);
            }
        }
        return true;
    }

    if (game->titleScreen) {
        game->titleScreen->Update();
        game->titleScreen->Render();
    }

    // PSX: InputManager::Step, GetControlVal(0)
    g_inputManager->Step();
    u32 buttons = g_inputManager->GetControlVal(0);
    game->controlVal[0] = (s32)buttons;

    // PSX: attract mode timer check (gp+128 - gp+124) >= 900
    s32 elapsed = game->titleIdleTimer - game->titleIdleBase;
    if (elapsed >= 900) {
        rsEvent(RS_STOP_MUSIC, 0, 0, 0);
        FadeBegin();
        game->titleFadeType = 2;
        return true;
    }

    if (buttons & kControlStartAction) {
        // PSX: ProcessSoundEvent(gp[72], 8)
        if (g_frontEndSound) {
            g_frontEndSound->ProcessSoundEvent(FE_SND_MENU_OPEN);
        }
        // PSX: rsEvent(6, 0, 0, 0) - stop music
        rsEvent(RS_STOP_MUSIC, 0, 0, 0);
        FadeBegin();
        game->titleFadeType = 1;
    } else {
        game->titleIdleTimer++;
    }

    return true;
}

bool Game::gsInitState(Game* game) {
    MARKFUNCTION(0x80029460); // gsInitState

    // PSX: DisplayTIM(gp[24]) - show RUNFIRST.TIM loading screen
    // PSX: VBlankLogo::StartLogo(0x000A0000), FillMeter(100)

    // PSX: if firstBoot (gp[80]): LoadPermanent(), clear firstBoot
    if (game->firstBoot) {
        // PSX: MEMSTAT(24, 1)
        // PSX: world->LoadPermanent()
        // PSX: MEMSTAT(24, 2)
        game->firstBoot = 0;
    }

    // PSX: setup input control mode arrays for both pads (loop i=0,1)
    if (g_inputManager) {
        for (s16 pad = 0; pad < 2; pad++) {
            g_inputManager->SetControlModeArray(pad, GameControlModeArray());
            g_inputManager->SetControlMapArray(pad, g_inputManager->PlayerMapArray());
        }
    }

    // PSX: VBlankLogo::StopLogo

    game->SetState(GameState::Title);
    return true;
}

bool Game::gsOpenFEState(Game* game) {
    MARKFUNCTION(0x800299B8); // gsOpenFEState

    // PSX: first-time init - rMakePuddle for FE memory
    if (!g_feInitialized) {
        // PSX: rMakePuddle(cellAlligator, overlayAddr, 5836, 0)
        g_feInitialized = 1;
    }

    // PSX: check gp[44] (selectedLevel). If a level was previously
    // selected (e.g. returning from game over), go straight to loading.
    if (g_selectedLevel != -1) {
        World* world = game->GetWorld();
        if (world) {
            world->SetTargetLevelPetal((u32)g_selectedLevel, 0);
        }
        if (g_time) {
            g_time->frameCounter = 0;
        }
        game->SetState(GameState::QueueLevelLoad);
    } else {
        // No level selected - show FE menu
        game->SetState(GameState::FE);
    }
    return true;
}

bool Game::gsFEState(Game* game) {
    MARKFUNCTION(0x80029A48); // gsFEState

    // PSX 0x80029A48: immediate transition to OpenLocationMenu.
    // FE menu interaction happens in LocationMenuState when applicable.
    game->SetState(GameState::OpenLocationMenu);
    return true;
}

bool Game::gsPrePlayState(Game* game) {
    MARKFUNCTION(0x80029AC0); // gsPrePlayState

    // PSX: 432 bytes, 27 blocks.
    // Loads overlay based on level type, calls Director::DetermineLevelIntro,
    // sets up rsEvent for level audio, checks checkpoint, resets HUD,
    // sets up input control modes, then SetState(Play=8).

    // PSX: GetCurLevelID(world)
    // PSX: if level 7 (boss): LoadOverlay(1), feMenuMgr->Activate, feMenuMgr->Render
    // PSX: else: gameMenu->ShowPauseMenu(), LoadOverlay(0)
    if (g_gameMenu) {
        g_gameMenu->ShowPauseMenu();
    }

    // PSX: Director->DetermineLevelIntro() via vtable
    if (g_director) {
        g_director->DetermineLevelIntro();
    }

    // PSX: rsEvent(21, player+28, cameraManager+384, 0) - init level audio

    // PSX: CheckpointInfo::IsValid(player+636) -> if valid, cameraMgr+364 = 1

    game->SetState(GameState::Play);

    // PSX: g_directorActive = 1 (gp+20)
    g_directorActive = 1;

    // PSX: HUD->InternalReset()

    // PSX: clear controlVal
    game->controlVal[0] = 0;
    game->controlVal[1] = 0;

    // PSX: loop i=0..1: SetControlModeArray, PlayerMapArray, SetControlMapArray
    if (g_inputManager) {
        for (s16 pad = 0; pad < 2; pad++) {
            g_inputManager->SetControlModeArray(pad, GameControlModeArray());
            g_inputManager->SetControlMapArray(pad, g_inputManager->PlayerMapArray());
        }
    }

    // PSX: if level != 7: HUD->SetHUDVisible(1, 1)

    return true;
}

bool Game::gsPlayState(Game* game) {
    MARKFUNCTION(0x80029C6C); // gsPlayState

    // PSX: check g_directorActive (gp+20). If active, Director runs the
    // intro script and we skip normal gameplay.
    if (g_directorActive) {
        if (g_director) {
            g_director->Process();
        }
        return true;
    }

    // PSX: InputManager::Step, then loop 2 pads storing GetControlVal
    g_inputManager->Step();

    for (s32 pad = 0; pad < 2; pad++) {
        game->controlVal[pad] = (s32)g_inputManager->GetControlVal((u16)pad);
    }

    // PSX: ProcessHandlers(game) - runs handlerSet1 (think) + handlerSet2 (draw)
    game->ProcessHandlers();

    // PSX: if state changed during handlers, return early
    if (game->state != GameState::Play) return true;

    // PSX: check if director is not busy (field168 < 1) AND Start pressed
    // PSX uses 0x0800 (remapped Start in gameplay control mode)
    s32 canPause = 0;
    if (g_director && g_director->field168 < 1) {
        canPause = 1;
    } else if (!g_director) {
        canPause = 1;
    }

    if (canPause && (game->controlVal[0] & kControlStartAction)) {
        game->SetState(GameState::Menu);
    }

    return true;
}

bool Game::gsEndLevelState(Game* game) {
    MARKFUNCTION(0x8002B688); // gsEndLevelState
    game->SetState(GameState::EndLevelLoop);
    return true;
}

bool Game::gsEndLevelLoopState(Game* game) {
    MARKFUNCTION(0x8002B6B0); // gsEndLevelLoopState

    // PSX: Time::Step, animLoopDSTACK, MenuDraw(null)
    // PSX: checks HUD fields (108, 344, 472) to determine if animations done
    // PSX: when all done, SetState(EndLevelExit=11)

    // For now, transition immediately since animations aren't implemented
    game->SetState(GameState::EndLevelExit);
    return true;
}

bool Game::gsEndLevelExitState(Game* game) {
    MARKFUNCTION(0x8002B744); // gsEndLevelExitState

    // PSX: 660 bytes - handles level progression
    // PSX: ScoreManager::HandleLevelEnd()
    if (g_scoreManager) {
        g_scoreManager->HandleLevelEnd();
    }
    // PSX: checks if next petal exists, determines QueuePetalLoad vs QueueLevelLoad
    // PSX: handles boss level win, level complete movies, etc.

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

bool Game::gsMenuState(Game* game) {
    MARKFUNCTION(0x80029EF8); // gsMenuState

    // PSX: GetCurLevelID(world)
    // PSX: level 7 (boss): menuMgr = gp[48] (feMenuMgr)
    // PSX: else: menuMgr = gp[52] (gameMenu)
    MenuMgr* menuMgr = g_gameMenu;

    // PSX: result = MenuDraw(menuMgr)
    s32 result = MenuDraw(menuMgr);

    // PSX: result 8 = resume game (back to Play)
    if (result == 8) {
        game->SetState(GameState::Play);
    }
    // PSX: result 4 = quit game (callback already set state via SetState)

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

bool Game::gsLocationMenuState(Game* game) {
    MARKFUNCTION(0x8002A128); // gsLocationMenuState

    // PSX 0x8002A128: result = MenuDraw(feMenuMgr); if result==8, return to Play.
    if (MenuDraw(g_feMenuMgr) == 8) {
        if (g_feMenuMgr) {
            g_feMenuMgr->ShowNewGameMenu();
        }
        game->SetState(GameState::Play);
    }

    return true;
}

bool Game::gsOpenLocationState(Game* game) {
    MARKFUNCTION(0x80029A70); // gsOpenLocationState

    // PSX: world target = (level 6, petal 0), time->frameCounter = 0,
    // then QueueLevelLoad and ResetLevel.
    World* world = game->GetWorld();
    if (world) {
        world->SetTargetLevelPetal(6, 0);
        world->ResetLevel();
    }
    if (g_time) {
        g_time->frameCounter = 0;
    }

    g_selectedLevel = 6;
    game->SetState(GameState::QueueLevelLoad);
    return true;
}

bool Game::gsQueueLevelLoad(Game* game) {
    MARKFUNCTION(0x80029574); // gsQueueLevelLoad

    // PSX: 520 bytes, 33 blocks.
    // rsEvent(6,0,0,0) - stop music
    rsEvent(RS_STOP_MUSIC, 0, 0, 0);

    // PSX: Shock(18), MenuFade() - blocking fade
    // PSX: HUD->SetHUDVisible(0, 1)
    // PSX: SetMemoryState(0), MEMSTAT_CLEAR, MEMSTAT_MIN_CLEAR, MEMSTAT_NEW_RESET
    // PSX: world->UnloadLevel(), world->UnloadLevelPart2()
    // PSX: DeletePlayerBlendAndAnimData(), FreeDynamicPrimBuffers()
    // PSX: AllocateDynamicPrimBuffers(0)

    World* world = game->GetWorld();
    if (!world) {
        game->SetState(GameState::Error);
        return true;
    }

    world->Unload();

    // PSX: DisplayTIM(gp[24]) - show loading screen
    // PSX: gameMenu->ShowLoadingScreenText(world->levelId, world->petalLoadTarget)
    // PSX: determine overlay based on level type (7=boss->overlay 1, else overlay 0)
    // PSX: LoadOverlay(overlayId)

    // PSX: LoadLevel(world, world->targetLevelIndex)
    if (!world->LoadLevelIndex(world->GetTargetLevelIndex())) {
        game->SetState(GameState::Error);
        return true;
    }

    // PSX: camera setup done via CameraManager paths loaded from WDB
    // Camera init for rendering until CameraManager is reversed
    if (world) {
        const LVector& lo = world->GetLevelMin();
        const LVector& hi = world->GetLevelMax();
        s32 cx = (lo.x + hi.x) / 2;
        s32 cy = (lo.y + hi.y) / 2;
        s32 cz = lo.z - (hi.z - lo.z) / 2;

        game->gameCamera.Reset();
        game->gameCamera.SetMode(CAM_MODE_FOLLOW);
        game->gameCamera.SetPosition(cx, cy, cz);

        tCamera* cam = game->gameCamera.GetP3DCamera();
        cam->SetFOV(0.7f, 4.0f / 3.0f);
        cam->SetNearPlane(100.0f);
        cam->SetFarPlane(500000.0f);

        game->view.SetCamera(game->gameCamera.GetP3DCamera());
        game->view.SetBackgroundColour(pddiColour(30, 30, 35));
        game->view.SetClearMask(PDDI_BUFFER_ALL);
    }

    // PSX: creates Player (492+216 bytes), sets gp+3432 = player
    // PSX: CharacterManager::LoadChar(0) -> loads player model
    if (!Player::s_player && world) {
        const LVector& lo = world->GetLevelMin();
        const LVector& hi = world->GetLevelMax();
        LVector spawnPos;
        spawnPos.x = (lo.x + hi.x) / 2;
        spawnPos.y = (lo.y + hi.y) / 2;
        spawnPos.z = (lo.z + hi.z) / 2;
        Player* player = new Player(&spawnPos);
        player->Reset();
    }

    // PSX 0x80029574 transitions to PrePlay (state 7) after load completes.
    game->SetState(GameState::PrePlay);

    // PSX: jcsStartDialog(), InputManager::Step(), GetControlVal(0)
    g_inputManager->Step();
    g_inputManager->GetControlVal(0);

    return true;
}

bool Game::gsQueuePetalLoad(Game* game) {
    MARKFUNCTION(0x8002977C); // gsQueuePetalLoad

    // PSX: rsEvent(6,0,0,0)
    rsEvent(RS_STOP_MUSIC, 0, 0, 0);

    World* world = game->GetWorld();
    if (!world) {
        game->SetState(GameState::Error);
        return true;
    }

    world->UnloadPetal();
    world->LoadPetal(world->GetTargetPetalIndex());

    // PSX 0x8002977C transitions to PrePlay (state 7).
    game->SetState(GameState::PrePlay);

    // PSX: InputManager::Step(), GetControlVal(0)
    g_inputManager->Step();
    g_inputManager->GetControlVal(0);

    return true;
}

bool Game::gsQueueLevelPetalLoad(Game* game) {
    MARKFUNCTION(0x8002986C); // gsQueueLevelPetalLoad

    // PSX chooses QueueLevelLoad (20) vs QueuePetalLoad (21) from
    // current/target world level+petal indices. ResetLevel is called
    // when any target differs from current.
    World* world = game->GetWorld();
    if (!world) {
        game->SetState(GameState::QueueLevelLoad);
        return true;
    }

    if (world->GetCurrentLevelIndex() != world->GetTargetLevelIndex()) {
        game->SetState(GameState::QueueLevelLoad);
        world->ResetLevel();
    } else if (world->GetCurrentPetalIndex() != world->GetTargetPetalIndex()) {
        game->SetState(GameState::QueuePetalLoad);
        world->ResetLevel();
    } else {
        game->SetState(GameState::QueuePetalLoad);
    }

    if (g_inputManager) {
        g_inputManager->Step();
        g_inputManager->GetControlVal(0);
    }

    return true;
}

bool Game::gsDetermineNextGameState(Game* game) {
    MARKFUNCTION(0x80029924); // gsDetermineNextGameState

    // PSX: decrement lives and branch to EndGame if depleted, else QueuePetalLoad.
    if (Player::s_player) {
        s32 lives = Player::s_player->GetLivesLeft() - 1;
        Player::s_player->SetLivesLeft(lives);

        if (lives <= 0) {
            Player::s_player->SetLivesLeft(Player::kMaxLives);
            game->SetState(GameState::EndGame);
        } else {
            game->SetState(GameState::QueuePetalLoad);
        }
    } else {
        game->SetState(GameState::QueuePetalLoad);
    }

    return true;
}

bool Game::gsDetermineGameOverState(Game* game) {
    MARKFUNCTION(0x800299B0); // gsDetermineGameOverState
    // PSX: returns 0 (false) - stops the game loop
    // On PC, main.cpp catches false return and handles it
    return false;
}

bool Game::gsEndGameState(Game* game) {
    MARKFUNCTION(0x8002C3B4); // gsEndGameState

    // PSX: SetHUDVisible(hud, 0, 1), UnloadLevel(world), LoadOverlay(1)
    // PSX: FreeXconFE(), InitXconFSImage()
    FreeXconFE();
    InitXconFSImage();

    // PSX: new oxScreenManager(56) -> Init("xc/gameover.1", screenMgr)
    if (game->gameOverScreen) {
        delete game->gameOverScreen;
        game->gameOverScreen = nullptr;
    }
    game->gameOverScreen = new GameOverScreen();
    game->gameOverScreen->Init("XC/GAMEOVER.1", g_oxFontFile);

    // PSX: rsEvent(4, 23, 0, 0) - set sound location to 23 (game over music)
    rsEvent(RS_SET_LOCATION, 23, 0, 0);
    // PSX: rsEvent(5, 0, 0, 0) - start music
    rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);

    game->gameOverFadeType = 0;

    game->SetState(GameState::EndGameLoop);
    return true;
}

bool Game::gsEndGameLoopState(Game* game) {
    MARKFUNCTION(0x8002C22C); // gsEndGameLoopState

    // PSX: BeginFrame -> oxScreenManager::Update + Render -> EndFrame
    // PC: render game over screen
    if (game->gameOverScreen) {
        game->gameOverScreen->Update();
        game->gameOverScreen->Render();
    }

    if (game->gameOverFadeType != 0) {
        s32 stillFading = FadeUpdate();
        FadeRender();

        if (!stillFading) {
            FadeEnd();
            // PSX: rsEvent(3,0,0,0), destroy screenMgr, FreeXconFSImage,
            // DeletePlayerBlendAndAnimData, LoadXconFE, SetState(OpenLocationMenu=19)
            rsEvent(RS_UNLOAD_LEVEL, 0, 0, 0);
            game->field136 = 1;
            if (game->gameOverScreen) {
                delete game->gameOverScreen;
                game->gameOverScreen = nullptr;
            }
            FreeXconFSImage();
            LoadXconFE();
            // PSX transitions to OpenLocationMenu (state 19) which goes to FE
            game->SetState(GameState::OpenFE);
        }
        return true;
    }

    g_inputManager->Step();
    u32 buttons = g_inputManager->GetControlVal(0);
    game->controlVal[0] = (s32)buttons;

    // PSX: check start-action (0x0800) or confirm-action (0x0040): 0x0840
    if (buttons & (kControlStartAction | kControlConfirmAction)) {
        // PSX: ProcessSoundEvent(frontEndSound, 19) = FE_SND_JT_0
        if (g_frontEndSound) {
            g_frontEndSound->ProcessSoundEvent(FE_SND_JT_0);
        }
        rsEvent(RS_STOP_MUSIC, 0, 0, 0);
        FadeBegin();
        game->gameOverFadeType = 1;
    }

    return true;
}

bool Game::gsEndState(Game*) {
    MARKFUNCTION(0x8002A17C); // gsEndState
    return false;
}

// PSX: PlayMovie__4GamePcii (GAME.CPP:3309, 0x8002BBF0)
// Plays an STR movie file. Blocks until movie finishes or is skipped.
// PSX: func_800366E8 x4, func_80026C04 x2, func_80027638, then
//   if unloadLevel: UnloadLevel, LoadOverlay(1)
//   rsEvent(4,24,0,0), rsEvent(5,0,0,0), rsEvent(12,7,0,0)
//   new(348) MoviePlayer, SetPath, Play(callback)
//   if unloadLevel after: LoadOverlay(0), ReloadFont, LoadCharTexture
//   rsEvent(13,7,0,0), rsEvent(6,0,0,0), rsEvent(3,0,0,0)
void Game::PlayMovie(const char* name, s32 skippable, s32 unloadLevel) {
    MARKFUNCTION(0x8002BBF0);

    LOG("[Game] PlayMovie(\"%s\", skip=%d, unload=%d)", name, skippable, unloadLevel);

    // PSX: display sync / GTE setup calls (func_800366E8, func_80026C04, func_80027638)
    // PC: not needed

    // PSX: if (unloadLevel) { world->UnloadLevel(); LoadOverlay(1); }
    // PC: overlay system not used, but world unload may be relevant later

    // PSX: rsEvent(4, 24, 0, 0) -- SetSFXVol(24)
    rsEvent(4, 24, 0, 0);
    // PSX: rsEvent(5, 0, 0, 0) -- LevelBegin/StopMusic
    rsEvent(5, 0, 0, 0);
    // PSX: rsEvent(12, 7, 0, 0)
    rsEvent(12, 7, 0, 0);

    // PSX: new(348) MoviePlayer -> constructor 0x80014338
    MoviePlayer* player = new MoviePlayer();

    // PSX: SetPath(0x800DB43C = "fe\\movies") then AddPlayMovie(name)
    // MoviePlayer builds full path as "fe\\movies\\<name>"
    char moviePath[128];
    std::snprintf(moviePath, sizeof(moviePath), "fe\\movies\\%s", name);

    if (!player->Open(moviePath)) {
        LOG("[Game] PlayMovie: cannot open %s, skipping", moviePath);
        delete player;
        goto cleanup;
    }

    // PSX: Play(0x80014534) with skip callback (0x80039330) if skippable
    // PC: blocking frame loop
    {
        using Clock = std::chrono::steady_clock;
        auto prevFrame = Clock::now();
        f32 targetDt = 1.0f / 15.0f; // STR movies are typically 15fps

        while (!player->IsFinished() && !p3d::display->ShouldClose()) {
            auto now = Clock::now();
            f32 elapsed = std::chrono::duration<f32>(now - prevFrame).count();
            if (elapsed < targetDt) continue;
            prevFrame = now;

            p3d::display->PollEvents();

            // PSX: skip callback checks Start button
            // PC: Enter (Start), Escape, or Space to skip
            if (skippable) {
                if (p3d::display->IsKeyDown(257) ||  // GLFW_KEY_ENTER
                    p3d::display->IsKeyDown(256) ||  // GLFW_KEY_ESCAPE
                    p3d::display->IsKeyDown(32)) {   // GLFW_KEY_SPACE
                    LOG("[Game] PlayMovie: skipped by user");
                    break;
                }
            }

            player->AdvanceFrame();

            p3d::context->BeginFrame();
            p3d::context->Clear(PDDI_BUFFER_ALL);
            player->Render();
            p3d::context->EndFrame();
            p3d::display->SwapBuffers();
        }
    }

    // PSX: destructor 0x8001434C with param 3
    delete player;

cleanup:
    // PSX: if (unloadLevel) { LoadOverlay(0); ReloadFont("xc/fonts.1"); LoadCharTexture(0); }
    if (unloadLevel) {
        if (g_oxFontFile) {
            g_oxFontFile->ReloadFont("XC/FONTS.1");
        }
        // PSX: LoadCharTexture(g_charMgr, 0) -- TODO when charMgr is reversed
    }

    // PSX: rsEvent(13, 7, 0, 0) -- cleanup
    rsEvent(13, 7, 0, 0);
    // PSX: rsEvent(6, 0, 0, 0) -- StopMusic
    rsEvent(6, 0, 0, 0);
    // PSX: rsEvent(3, 0, 0, 0) -- UnloadLevel sound
    rsEvent(3, 0, 0, 0);
}

// PSX: FreeXconFE__4Game (GAME.CPP:3812, 0x8002C7A4)
// Destroys feMenuMgr, gameMenu, and HUD via virtual destructor.
void Game::FreeXconFE() {
    MARKFUNCTION(0x8002C7A4);
    if (g_feMenuMgr) {
        delete g_feMenuMgr;
        g_feMenuMgr = nullptr;
    }
    if (g_gameMenu) {
        delete g_gameMenu;
        g_gameMenu = nullptr;
    }
    // PSX: also destroys HUD (g_hud)
}

// PSX: InitXconFSImage__4Game (GAME.CPP:3826, 0x8002C838)
void Game::InitXconFSImage() {
    MARKFUNCTION(0x8002C838);
    // PSX: sets up VRAM cell/palette areas for fullscreen images,
    // reloads font from "xc/fonts.1", creates CFrontEndSound (gp+72)
    if (!g_oxFontFile) {
        g_oxFontFile = new oxFontFile();
    }
    g_oxFontFile->ReloadFont("XC/FONTS.1");

    // PSX: new CFrontEndSound stored at gp+72
    if (!g_frontEndSound) {
        g_frontEndSound = new CFrontEndSound();
    }
}

// PSX: FreeXconFSImage__4Game (GAME.CPP:3859, 0x8002C998)
void Game::FreeXconFSImage() {
    MARKFUNCTION(0x8002C998);
    // PSX: NOP (empty function)
}

// PSX: LoadXconFE__4Game (GAME.CPP:3789, 0x8002C648)
// Creates feMenuMgr, gameMenu, and HUD. Sets up VRAM, loads overlay 1.
void Game::LoadXconFE() {
    MARKFUNCTION(0x8002C648);
    if (g_feMenuMgr) return; // already loaded

    // PSX: DeleteAllocators(cellAlligator)
    // PSX: InitCellArea({960,64,64,56}), InitPal4Area({960,120,64,4}), InitPal8Area({960,124,64,4})
    // PSX: LoadOverlay(1) - loads FE overlay

    // PSX: ReloadFont(gp[56], "xc/fonts.1")
    if (!g_oxFontFile) {
        g_oxFontFile = new oxFontFile();
    }
    g_oxFontFile->ReloadFont("XC/FONTS.1");

    // PSX: new(100) feMenuMgr -> gp[48]
    g_feMenuMgr = new feMenuMgr();

    // PSX: new(92) gameMenu -> gp[52]
    g_gameMenu = new gameMenu();

    // PSX: new(712) HUD -> g_hud
    // HUD not yet reversed

    // PSX: feMenuMgr->Init("xc/fe.1", gp[56])  -- gp[56] is oxFontFile
    g_feMenuMgr->Init("XC/FE.1", g_oxFontFile);

    // PSX: gameMenu->Init("xc/gamemenu.1", gp[56])
    g_gameMenu->Init("XC/GAMEMENU.1", g_oxFontFile);

    // PSX: hud->Init("xc/hud.1", gp[56])
}

// PSX: fade globals (gp+3388, gp+3392)
u8 Game::s_fadeStep = 17;
u8 Game::s_fadeCounter = 0;

// PSX: FadeBegin__4Game (GAME.CPP:3869, 0x8002C9A0)
void Game::FadeBegin() {
    MARKFUNCTION(0x8002C9A0);
    // PSX: fadeStep = 17, fadeCounter = 0
    s_fadeStep = 17;
    s_fadeCounter = 0;
}

// PSX: FadeEnd__4Game (GAME.CPP:3875, 0x8002C9B4)
void Game::FadeEnd() {
    MARKFUNCTION(0x8002C9B4);
    // PSX: NOP (empty function)
}

// PSX: FadeUpdate__4Game (GAME.CPP:3879, 0x8002C9BC)
// Returns 1 if fade still in progress, 0 when complete (counter >= 255).
s32 Game::FadeUpdate() {
    MARKFUNCTION(0x8002C9BC);
    // PSX: fadeCounter += fadeStep; clamp to 255; return (fadeCounter < 255)
    s32 newVal = (s32)s_fadeCounter + (s32)s_fadeStep;
    if (newVal < 255) {
        s_fadeCounter = (u8)newVal;
    } else {
        s_fadeCounter = 255;
    }
    return (s_fadeCounter < 255) ? 1 : 0;
}

// PSX: FadeRender__4Game (GAME.CPP:3893, 0x8002C9F8)
// Draws a fullscreen black POLY_F4 with alpha=fadeCounter at layer 5.
void Game::FadeRender() {
    MARKFUNCTION(0x8002C9F8);
    // PSX: EnterLayer(view, 5), setup POLY_F4 (512x240),
    // set RGB to fadeCounter, render primitive, ExitLayer(view, 5)
    // PC: draw a fullscreen colored quad with alpha blending
    ScreenDraw::DrawColoredQuad(0, 0, 0, s_fadeCounter);
}
