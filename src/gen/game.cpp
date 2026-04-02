// game.cpp
#include "gen/game.h"
#include "gen/world.h"
#include "gen/block.h"
#include "gen/charmgr.h"
#include "gen/database.h"
#include "gen/cammgr.h"
#include "gen/levelmgr.h"
#include "gen/time.h"
#include "snd/sound.h"
#include "ai/player.h"
#include "config.h"
#include "p3d/context.h"
#include "p3d/input.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#if RC_FEATURE_COLLISION_DEBUG
#include "pc/coldebug.h"
#endif

// Global game pointer
Game* g_game = nullptr;

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

    SetState(GameState::Null);
    g_game = this;
    RC_LOG("[Game] Created");
}

Game::~Game() {
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

    // 4. AI (116) - AI master (TODO)

    // 5. World (PSX: 160 bytes)
    World* world = new World();
    world->SetName("World", 0);
    managerList.AddNodePri(world);

    // 6. EnvironmentManager (140) - environment effects (TODO)
    // 7. Display (32) - PSX display (not needed on PC)
    // 8. Director (212) - scripting/cutscenes (TODO)

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

    // 17. ScoreManager (504) - score/collectibles (TODO)

    // Open all managers in list
    for (ccMinNode* n = managerList.head; n; n = n->next) {
        Manager* mgr = static_cast<Manager*>(n);
        mgr->Open();
    }

    RC_LOG("[Game] InternalOpen: managers created");
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

void Game::DrawEverythingHandlerCB(Handler*) {
    MARKFUNCTION(0x8002A98C);
    if (!g_game) return;
    World* world = g_game->GetWorld();
    if (!world) return;

    const LVector& camPos = g_game->gameCamera.GetPosition();
    world->Render(&camPos);
}

void Game::EndFrameHandler(Handler*) {
    MARKFUNCTION(0x8002B420);
    // PSX: EndFrame - P3D context end frame
    // Now handled by main.cpp BeginFrame/EndFrame
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

    // PSX: gsPrePlayState creates the Player entity and loads character data.
    // PC: create Player at level center, skip character model loading for now.
    if (!Player::s_player) {
        World* world = game->GetWorld();
        if (world) {
            const LVector& lo = world->GetLevelMin();
            const LVector& hi = world->GetLevelMax();
            LVector spawnPos;
            spawnPos.x = (lo.x + hi.x) / 2;
            spawnPos.y = (lo.y + hi.y) / 2;
            spawnPos.z = (lo.z + hi.z) / 2;

            Player* player = new Player(&spawnPos);
            player->Reset();
            RC_LOG("[Game] Player spawned at (%d, %d, %d)", spawnPos.x, spawnPos.y, spawnPos.z);
        }
    }

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

    // PSX: loop 2 pads, store GetControlVal into game->controlVal[pad]
    for (s32 pad = 0; pad < 2; pad++) {
        game->controlVal[pad] = (s32)g_inputManager->GetControlVal((u16)pad);
    }

    // Player think/AI (PSX: called from handler pipeline)
    if (Player::s_player) {
        Player::s_player->Think();
    }

    // Camera update (before rendering)
    game->gameCamera.Think();
    game->gameCamera.Update();

    p3d::context->EnableZBuffer(true);
    p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    p3d::context->SetCullMode(PDDI_CULL_NONE);

    game->view.BeginRender();

    // PSX: ProcessHandlers - runs all registered handler callbacks
    // This calls BeginFrameHandler, DrawEverythingHandler, EndFrameHandler
    game->ProcessHandlers();

    // Draw entities (PSX: DrawLoop on per-block entity lists)
    if (Player::s_player) {
        Player::s_player->Draw();
    }

#if RC_FEATURE_COLLISION_DEBUG
    if (p3d::input->IsKeyTriggered(pddiInput::KeyF3))
        CollisionDebug::enabled = !CollisionDebug::enabled;
    {
        World* world = game->GetWorld();
        if (world)
            CollisionDebug::Draw(world->GetBlockManager());
    }
#endif

    // F1: toggle between follow camera and debug camera
    if (p3d::input->IsKeyTriggered(pddiInput::KeyF1)) {
        if (game->gameCamera.GetMode() == CAM_MODE_FOLLOW) {
            game->gameCamera.SetMode(CAM_MODE_DEFAULT);
            RC_LOG("[Game] Camera: Debug mode (WASD+mouse)");
        } else {
            game->gameCamera.SetMode(CAM_MODE_FOLLOW);
            RC_LOG("[Game] Camera: Follow mode");
        }
    }

    game->view.EndRender();

    // PSX: check if still in Play state after handlers
    if (game->state != GameState::Play) return true;

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
    RC_LOG("[Game] QueueLevelLoad: loading LEV07.LCF");

    World* world = game->GetWorld();
    if (!world) {
        RC_ERR("[Game] No World manager!");
        game->SetState(GameState::DetermineNextGameState);
        return true;
    }

    world->Load("RTARGET/LEV07.LCF");

    // Position camera at center of level
    {
        const LVector& lo = world->GetLevelMin();
        const LVector& hi = world->GetLevelMax();
        s32 cx = (lo.x + hi.x) / 2;
        s32 cy = (lo.y + hi.y) / 2;
        s32 cz = lo.z - (hi.z - lo.z) / 2;

        // Reset and configure the PSX camera
        game->gameCamera.Reset();
        game->gameCamera.SetMode(CAM_MODE_FOLLOW); // Player-follow mode

        // Set initial camera position at level center, backed off in Z
        game->gameCamera.SetPosition(cx, cy, cz);

        tCamera* cam = game->gameCamera.GetP3DCamera();
        cam->SetFOV(0.7f, 4.0f / 3.0f);
        cam->SetNearPlane(100.0f);
        cam->SetFarPlane(500000.0f);

        RC_LOG("[Game] Camera positioned at (%d, %d, %d), Follow mode", cx, cy, cz);
        RC_LOG("[Game] Controls: WASD=move, K=jump, F1=toggle debug cam");
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
