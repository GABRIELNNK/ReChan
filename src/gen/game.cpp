// game.cpp
#include "common.h"
#include "gen/game.h"
#include "gen/display.h"
#include "gen/camera.h"
#include "gen/world.h"
#include "gen/block.h"
#include "gen/charmgr.h"
#include "gen/database.h"
#include "gen/cammgr.h"
#include "gen/levelmgr.h"
#include "gen/time.h"
#include "gen/ai.h"
#include "gen/model.h"
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
#include "p3d/keycode.h"
#include "p3d/input.h"
#include "p3d/context.h"
#include "fe/loadanim.h"
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
    // Display::InternalOpen adds dispBeginFrameHandler (62) and dispEndFrameHandler (-62)
    handlerSet2.AddHandler(BeginFrameHandler, 64);
    handlerSet2.AddHandler(AnimateEverythingHandler, -48);
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

    // Close and delete all managers
    while (ccMinNode* n = managerList.RemHead()) {
        Manager* mgr = static_cast<Manager*>(n);
        mgr->Close();
        delete mgr;
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

    // 7. Display (32) - owns tView, BeginFrame/EndFrame, frame counter
    Display* disp = new Display();
    disp->SetName("Display", 0);
    managerList.AddNodePri(disp);
    // PSX: Display::InternalOpen registers dispBeginFrameHandler (pri=62)
    // and dispEndFrameHandler (pri=-62) into handlerSet2.
    handlerSet2.AddHandler(Display::dispBeginFrameHandler, 62);
    handlerSet2.AddHandler(Display::dispEndFrameHandler, -62);

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

Camera& Game::GetCamera() {
    return *g_display->GetCamera();
}

tView& Game::GetView() {
    return g_display->GetView();
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
// PSX: BeginFrameHandler (GAME.CPP:2192, pri=64 in handlerSet2)
// Resets render counter. Frame begin/end is handled by Display handlers.
void Game::BeginFrameHandler(Handler*) {
    MARKFUNCTION(0x8002B408);
    // PSX: gp[3404] = 0 (reset render counter)
}

// PSX: AnimateEverythingHandler (GAME.CPP:2620, pri=-48)
static void AnimateLoop(ccList& list) {
    for (ccMinNode* node = list.head; node != nullptr; node = node->next) {
        Thing* thing = static_cast<Thing*>(node);
        if (thing->model) {
            Model* m = static_cast<Model*>(thing->model);
            m->Animate();
        }
    }
}

void Game::AnimateEverythingHandler(Handler*) {
    MARKFUNCTION(0x8002B2F0);
    if (!g_ai) return;
    AnimateLoop(g_ai->activeZoneList);
    AnimateLoop(g_ai->humanoidList);
    AnimateLoop(g_ai->pickupList);
    AnimateLoop(g_ai->inactivePickupList);
}

// PSX: DrawEverythingHandler__FP7Handler (GAME.CPP:2211, 0x8002A98C)
// 1532 bytes, 91 blocks. Sorts blocks by distance, renders world geometry,
// characters, items, shadows per-block. Uses tView layers for ordering.
// NOTE: tView::BeginRender/EndRender are handled by Display's handlers (pri 62/-62).
void Game::DrawEverythingHandlerCB(Handler*) {
    MARKFUNCTION(0x8002A98C);
    if (!g_game) return;
    World* world = g_game->GetWorld();
    if (!world) return;

    // PSX: sorts blocks by distance from camera, then iterates:
    //   EnterLayer(view, 0) -> DrawBG -> ExitLayer
    //   per-block: Render geometry, LookAt camera, render items/characters/shadows

    p3d::context->EnableZBuffer(true);
    p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    p3d::context->SetCullMode(PDDI_CULL_NONE);

    const LVector& camPos = g_display->GetCamera()->GetPosition();
    world->Render(&camPos);

    // PSX: entities are drawn inside DrawEverythingHandler with VRAM active.
    // PC: VRAM handle must be set for character tpage/cba texture lookup.
    if (Player::s_player) {
        p3d::context->SetVRAMHandle(world->GetVRAMHandle());
        Player::s_player->Draw();
        p3d::context->SetVRAMHandle(0);
    }
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
    g_display->BeginFrame();
    MenuRender(menuMgr);
    g_display->EndFrame();
    return result;
}

// PSX: EndFrameHandler (GAME.CPP:2205, pri=-64 in handlerSet2)
// Noop on PSX - Display's dispEndFrameHandler does the real work.
void Game::EndFrameHandler(Handler*) {
    MARKFUNCTION(0x8002B420);
}

bool Game::Step() {
    MARKFUNCTION(0x8002B65C); // Step__4Game

    // Poll platform-level keyboard/mouse state before anything reads it
    if (p3d::input) {
        p3d::input->ServiceInput();
    }

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
        g_display->BeginFrame();
        if (game->introTexture) {
            ScreenDraw::DrawFullscreen(game->introTexture);
        }
        g_display->EndFrame();
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
        g_display->BeginFrame();
        if (game->titleScreen) {
            game->titleScreen->Update();
            game->titleScreen->Render();
        }

        s32 stillFading = FadeUpdate();
        FadeRender();
        g_display->EndFrame();

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

    g_display->BeginFrame();
    if (game->titleScreen) {
        game->titleScreen->Update();
        game->titleScreen->Render();
    }
    g_display->EndFrame();

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

    // PSX: ClearImage, DisplayTIM(gp[24]), StartLogo(655360), FillMeter(100)
    DisplayTIM("RUNFIRST.TIM");
    StartLogo("RUNFIRST.TIM");
    FillMeter(100);

    // PSX: if firstBoot (gp[80]): LoadPermanent(), clear firstBoot
    if (game->firstBoot) {
        // PSX: MEMSTAT(24, 1)
        World* world = game->GetWorld();
        if (world) {
            world->LoadPermanent();
        }
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
    StopLogo();

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
    // Loads overlay, shows menu, calls DetermineLevelIntro, sets up audio
    // listener, checks checkpoint, resets HUD, input control modes,
    // then transitions to Play state.

    World* world = game->GetWorld();
    s32 levelID = (world) ? world->GetCurLevelID() : 0;

    // PSX: level 7 (hub): LoadOverlay(1), feMenuMgr->ShowNewGameMenu, feMenuMgr->OpenDoors
    // PSX: else: gameMenu->ShowPauseMenu(), LoadOverlay(0)
    if (levelID == 7) {
        // PSX: LoadOverlay(1) - load boss overlay for hub
        if (g_feMenuMgr) {
            g_feMenuMgr->ShowNewGameMenu();
            g_feMenuMgr->OpenDoors();
        }
    } else {
        if (g_gameMenu) {
            g_gameMenu->ShowPauseMenu();
        }
        // PSX: LoadOverlay(0) - load normal overlay
    }

    // PSX: Director->DetermineLevelIntro() (vtable dispatch)
    if (g_director) {
        g_director->DetermineLevelIntro();
    }

    // PSX: rsEvent(21, player+28, cameraManager+384, 0) - set 3D audio listener
    // The args are pointers to player position and camera matrix for 3D audio.
    // On PC we pass zeros - audio spatialization not yet wired.
    rsEvent(21, 0, 0, 0);

    // PSX: if CheckpointInfo::IsValid(player+636): cameraManager field364 = 1
    // TODO: CheckpointInfo not yet reversed

    // PSX: SetState(Play=8)
    game->SetState(GameState::Play);

    // PSX: g_directorActive = 1 (gp+20)
    g_directorActive = 1;

    // PSX: HUD->InternalReset()
    // TODO: HUD not yet reversed

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

    // PSX: if level != 7: SetHUDVisible(0, 1, 1)
    // TODO: HUD not yet reversed

    // PSX: MEMSTAT_NEW_PRINT, SetMemoryState(1)

    return true;
}

bool Game::gsPlayState(Game* game) {
    MARKFUNCTION(0x80029C6C); // gsPlayState

    // PSX: check g_directorActive (gp+20). If active, Director runs the
    // intro script and we skip normal gameplay processing.
    if (g_directorActive) {
        if (g_director) {
            g_director->Process();
        }
        // PSX: VBlank interrupt handles display/camera updates automatically.
        // PC: we must still run ProcessHandlers so draw handlers fire each frame.
        game->ProcessHandlers();
        return true;
    }

    // PSX: InputManager::Step, then loop 2 pads storing GetControlVal
    g_inputManager->Step();

    for (s32 pad = 0; pad < 2; pad++) {
        game->controlVal[pad] = (s32)g_inputManager->GetControlVal((u16)pad);
    }

    // PSX: ProcessHandlers(game) - runs handlerSet1 (think) + handlerSet2 (draw)
    game->ProcessHandlers();

    // PSX: check state==Play AND director scriptState==0 for pause eligibility
    if (game->state == GameState::Play) {
        s32 canPause = (!g_director || g_director->scriptState == 0);
        if (canPause && (game->controlVal[0] & kControlStartAction)) {
            game->SetState(GameState::Menu);
            // PSX: Shock(18) - controller vibration on pause
            // TODO: Shock not yet reversed
        }
    }

    // PSX: Update__7Profile() - PSX profiling system, not applicable on PC

#if IMPROVED_DEBUG_CAM
    // CTRL+B: toggle between debug camera and follow camera
    if (p3d::input->IsKeyDown(KEY_LEFT_CONTROL) && p3d::input->IsKeyTriggered(KEY_B)) {
        Camera* cam = g_display->GetCamera();

        if (cam->GetMode() == CAM_MODE_DEFAULT) {
            cam->SetMode(CAM_MODE_FOLLOW);
        } else {
            cam->SetMode(CAM_MODE_DEFAULT);
        }
    }
#endif

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

    if (g_scoreManager) {
        g_scoreManager->HandleLevelEnd();
    }

    World* world = game->GetWorld();
    if (!world) {
        game->SetState(GameState::QueueLevelLoad);
        return true;
    }

    s32 nextPetal = (s32)world->GetCurrentPetalIndex() + 1;
    if (nextPetal < world->GetCurLevelPetals()) {
        g_scoreManager->OpenPetal(world->GetCurrentLevelIndex(), nextPetal);
    }

    s32 hubIndex = world->LevelIDToIndex(7);
    world->SetTargetLevelPetal((u32)hubIndex, 0);

    game->SetState(GameState::QueueLevelLoad);
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

    // PSX: rsEvent(6,0,0,0) - stop music
    rsEvent(RS_STOP_MUSIC, 0, 0, 0);

    // PSX: Shock(18) - controller vibration pulse
    // TODO: Shock not yet reversed

    // PSX: MenuFade() - blocking fade to black
    // TODO: MenuFade not yet reversed (blocking inline loop on PSX)

    // PSX: SetHUDVisible(0, 0, 1) - hide HUD during load
    // TODO: HUD not yet reversed

    // PSX: SetMemoryState(0), MEMSTAT_CLEAR, MEMSTAT_MIN_CLEAR, MEMSTAT_NEW_RESET
    // PSX memory tracking - not applicable on PC

    World* world = game->GetWorld();
    if (!world) {
        game->SetState(GameState::Error);
        return true;
    }

    // PSX: UnloadLevel(world), UnloadLevelPart2(world)
    world->Unload();
    // TODO: UnloadLevelPart2 not yet reversed (additional cleanup)

    // PSX: DeletePlayerBlendAndAnimData() - free player blend tree memory
    // TODO: not yet reversed

    // PSX: FreeDynamicPrimBuffers(), AllocateDynamicPrimBuffers(0)
    // PSX GPU primitive buffers - not applicable on PC

    // PSX: DisplayTIM(gp[24]) - show loading screen background
    DisplayTIM("RUNFIRST.TIM");

    // PSX: ShowLoadingScreenText(gameMenu, levelId, petalTarget)
    // TODO: gameMenu::ShowLoadingScreenText not yet reversed

    // PSX: LoadOverlay(levelType==7) - load code overlay
    // PSX: BossAI overlay switch for levels 1-7 vs 8,11-14
    // Not applicable on PC - all code is statically linked

    // PSX: LoadLevel(world, targetLevelIndex) - internally calls Construct
    // which spawns AI entities, resets Director, sets level script
    if (!world->LoadLevelIndex(world->GetTargetLevelIndex())) {
        game->SetState(GameState::Error);
        return true;
    }

    // PSX: Camera setup through ExecuteLoadCallbacks chain
    // which fires cameraLoadFunc -> CameraManager::SetupPaths.
    g_display->GetCamera()->Reset();
    if (g_cameraManager) {
        g_cameraManager->SetupPaths();
        g_display->GetCamera()->SetCameraAnchor(g_cameraManager->GetAnchor());
    }

    tMatrixCamera* cam = g_display->GetCamera()->GetP3DCamera();
    cam->SetNearPlane(100.0f);
    cam->SetFarPlane(500000.0f);

    g_display->GetView().SetCamera(g_display->GetCamera()->GetP3DCamera());
    g_display->GetView().SetBackgroundColour(pddiColour(30, 30, 35));
    g_display->GetView().SetClearMask(PDDI_BUFFER_ALL);

    // PSX: SetLookAtTarget via CameraManager path system.
    // Player was created by AI::Populate inside World::LoadLevelIndex -> Construct.
    if (Player::s_player) {
        g_display->GetCamera()->SetLookAtTarget(Player::s_player, 1);
    }

    // PSX: SetState(PrePlay=7)
    game->SetState(GameState::PrePlay);

    // PSX: jcsStartDialog() - initialize dialog/subtitle system
    // TODO: jcsStartDialog not yet reversed

    // PSX: InputManager::Step(), GetControlVal(0) - flush input buffer
    g_inputManager->Step();
    g_inputManager->GetControlVal(0);

    return true;
}

bool Game::gsQueuePetalLoad(Game* game) {
    MARKFUNCTION(0x8002977C); // gsQueuePetalLoad

    // PSX: rsEvent(6,0,0,0) - stop music
    rsEvent(RS_STOP_MUSIC, 0, 0, 0);

    // PSX: Shock(18), MenuFade(), SetHUDVisible(0, 0, 1)
    // PSX: SetMemoryState(0), MEMSTAT_CLEAR()
    // TODO: not yet reversed

    World* world = game->GetWorld();
    if (!world) {
        game->SetState(GameState::Error);
        return true;
    }

    // PSX: UnloadPetal(world)
    world->UnloadPetal();

    // PSX: DisplayTIM(gp[24])
    DisplayTIM("RUNFIRST.TIM");

    // PSX: ShowLoadingScreenText(gameMenu, currentLevelIndex, targetPetalIndex)
    // TODO: not yet reversed

    // PSX: LoadPetal(world, targetPetalIndex)
    world->LoadPetal(world->GetTargetPetalIndex());

    // PSX: SetState(PrePlay=7)
    game->SetState(GameState::PrePlay);

    // PSX: jcsStartDialog()
    // TODO: not yet reversed

    // PSX: InputManager::Step(), GetControlVal(0) - flush input buffer
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

    // PSX: jcsStartDialog()
    // TODO: not yet reversed

    // PSX: InputManager::Step(), GetControlVal(0)
    if (g_inputManager) {
        g_inputManager->Step();
        g_inputManager->GetControlVal(0);
    }

    return true;
}

bool Game::gsDetermineNextGameState(Game* game) {
    MARKFUNCTION(0x80029924); // gsDetermineNextGameState

    // PSX: Shock(18) - controller vibration on death
    // TODO: Shock not yet reversed

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
    g_display->BeginFrame();
    if (game->gameOverScreen) {
        game->gameOverScreen->Update();
        game->gameOverScreen->Render();
    }

    if (game->gameOverFadeType != 0) {
        s32 stillFading = FadeUpdate();
        FadeRender();
        g_display->EndFrame();

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

    g_display->EndFrame();

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
        f64 prevFrame = Time::GetTimeInSeconds();
        f32 targetDt = 1.0f / player->GetFrameRate();

        while (!player->IsFinished() && !p3d::display->ShouldClose()) {
            f64 now = Time::GetTimeInSeconds();
            f32 elapsed = (f32)(now - prevFrame);
            if (elapsed < targetDt) continue;
            prevFrame = now;

            p3d::display->PollEvents();

            // PSX: skip callback checks Start button
            // PC: Enter (Start), Escape, or Space to skip
            if (skippable) {
                if (p3d::display->IsKeyDown(KEY_ENTER) ||
                    p3d::display->IsKeyDown(KEY_ESCAPE) ||
                    p3d::display->IsKeyDown(KEY_SPACE)) {
                    LOG("[Game] PlayMovie: skipped by user");
                    break;
                }
            }

            player->AdvanceFrame();

            g_display->BeginFrame();
            player->Render();
            g_display->EndFrame();
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
