#include "pc/debugui.h"
#include "imgui.h"
#include "gen/game.h"
#include "gen/camera.h"
#include "gen/animstruct.h"
#include "gen/model.h"
#include "gen/charmgr.h"
#include "gen/scoremgr.h"
#include "gen/time.h"
#include "ai/player.h"
#include "ai/humanoid.h"
#include "ai/thing.h"
#include "snd/sound.h"
#include "pc/audio.h"
#include "p3d/lvector.h"
#include <cstdio>

static bool sShowPlayer = false;
static bool sShowCamera = false;
static bool sShowAudio = false;
static bool sShowAnimation = false;
static bool sShowGame = false;
static bool sShowImGuiDemo = false;
static s32 sAnimSelectedEnum = 0;
static s32 sAnimSelectedLoopType = ANIM_LOOP;

static const char* GameStateName(GameState s) {
    switch (s) {
    case GameState::Null: return "Null";
    case GameState::Intro: return "Intro";
    case GameState::Title: return "Title";
    case GameState::TitleLoop: return "TitleLoop";
    case GameState::Init: return "Init";
    case GameState::OpenFE: return "OpenFE";
    case GameState::FE: return "FE";
    case GameState::PrePlay: return "PrePlay";
    case GameState::Play: return "Play";
    case GameState::EndLevel: return "EndLevel";
    case GameState::EndLevelLoop: return "EndLevelLoop";
    case GameState::EndLevelExit: return "EndLevelExit";
    case GameState::PlayMovieCredits: return "PlayMovieCredits";
    case GameState::DbgMenu: return "DbgMenu";
    case GameState::Menu: return "Menu";
    case GameState::Error: return "Error";
    case GameState::ErrorLoop: return "ErrorLoop";
    case GameState::ErrorExit: return "ErrorExit";
    case GameState::LocationMenu: return "LocationMenu";
    case GameState::OpenLocationMenu: return "OpenLocationMenu";
    case GameState::QueueLevelLoad: return "QueueLevelLoad";
    case GameState::QueuePetalLoad: return "QueuePetalLoad";
    case GameState::QueueLevelPetalLoad: return "QueueLevelPetalLoad";
    case GameState::DetermineNextGameState: return "DetermineNextGameState";
    case GameState::DetermineGameOverState: return "DetermineGameOverState";
    case GameState::EndGame: return "EndGame";
    case GameState::EndGameLoop: return "EndGameLoop";
    case GameState::End: return "End";
    default: return "Unknown";
    }
}

static const char* ActionStateName(s32 s) {
    switch (s) {
    case AS_INACTIVE_IDLE: return "InactiveIdle";
    case AS_STAND: return "Stand";
    case AS_STAND_ANIM: return "StandAnim";
    case AS_WALL_JUMP_TAUNT: return "WallJumpTaunt";
    case AS_DIVE_ROLL: return "DiveRoll";
    case AS_PAUSE: return "Pause/RunJump";
    case AS_JUMP: return "Jump";
    case AS_LEDGE_LATCH: return "LedgeLatch";
    case AS_RUN: return "Run";
    case AS_BACKFLIP: return "Backflip";
    case AS_STRAFE: return "Strafe";
    case AS_FALL: return "Fall";
    case AS_HARDFALL: return "HardFall";
    case AS_HARDLAND: return "HardLand";
    case AS_FLIP: return "Flip";
    case AS_FLIP_VARIANT: return "FlipVariant";
    case AS_POLE_IDLE: return "PoleIdle";
    case AS_PUSH_OBJECT: return "PushObject";
    case AS_SLOPE_SLIDE: return "SlopeSlide";
    case AS_TABLE_ROLL: return "TableRoll";
    case AS_POLE_SWING: return "PoleSwing";
    case AS_LEDGE_PULLUP: return "LedgePullup";
    case AS_PUNCH_ATTACK: return "PunchAttack";
    case AS_KICK_ATTACK: return "KickAttack";
    case AS_COMBAT_IDLE: return "CombatIdle";
    case AS_PICKUP: return "Pickup";
    case AS_THROW_PICKUP: return "ThrowPickup";
    case AS_FLYING_BACK_LAND: return "FlyingBackLand";
    case AS_BACK_GRAB_RECOVER: return "BackGrabRecover";
    case AS_GET_UP: return "GetUp";
    case AS_FLYING_BACK_CHECK: return "FlyingBackCheck";
    case AS_SPIN_BACK_RECOVER: return "SpinBackRecover";
    case AS_DEAD: return "Dead";
    case AS_HIT_EXPLOSION: return "HitExplosion";
    case AS_HIT_ENVIRONMENT: return "HitEnvironment";
    default: return "Unknown";
    }
}

static const char* StateDispatchName(u16 d) {
    switch (d) {
    case SD_NONE: return "None";
    case SD_STAND: return "Stand";
    case SD_DIVE_ROLL: return "DiveRoll";
    case SD_PAUSE: return "Pause";
    case SD_RUN: return "Run";
    case SD_BACKFLIP: return "Backflip";
    case SD_STRAFE: return "Strafe";
    case SD_JUMP: return "Jump";
    case SD_FALL: return "Fall";
    case SD_GOT_HIT_HIGH: return "GotHitHigh";
    case SD_GOT_HIT_MED: return "GotHitMed";
    case SD_GOT_HIT_LOW: return "GotHitLow";
    case SD_WALLJUMP: return "WallJump";
    case SD_COLLAPSE: return "Collapse";
    case SD_DEAD: return "Dead";
    case SD_SPIN_BACK: return "SpinBack";
    case SD_FLYING_BACK: return "FlyingBack";
    case SD_STUNNED: return "Stunned";
    case SD_THROW: return "Throw";
    case SD_PICKUP: return "Pickup";
    case SD_GET_UP: return "GetUp";
    case SD_POLE_IDLE: return "PoleIdle";
    case SD_POLE_SWING: return "PoleSwing";
    case SD_SLOPE_SLIDE: return "SlopeSlide";
    case SD_DEAD_PLAYER: return "DeadPlayer";
    case SD_HARDFALL: return "HardFall";
    case SD_HARDLAND: return "HardLand";
    case SD_FLIP: return "Flip";
    case SD_INACTIVE_IDLE: return "InactiveIdle";
    case SD_PUSH_OBJECT: return "PushObject";
    case SD_TABLE_ROLL: return "TableRoll";
    case SD_LEDGE_LATCH: return "LedgeLatch";
    case SD_LEDGE_PULLUP: return "LedgePullup";
    case SD_DO_STAND: return "DoStand";
    default: return "Unknown";
    }
}

static const char* AnimLoadStateName(s32 s) {
    switch (s) {
    case 0: return "Stopped";
    case 1: return "Playing";
    case 2: return "Paused";
    default: return "Unknown";
    }
}

static const char* AnimLoopTypeName(s32 t) {
    switch (t) {
    case ANIM_LOOP: return "Loop";
    case ANIM_LOOP_REVERSE: return "LoopReverse";
    case ANIM_RUN_TO_LAST: return "RunToLast";
    case ANIM_HOLD_FIRST: return "HoldFirst";
    case ANIM_HOLD_LAST: return "HoldLast";
    case ANIM_BLEND: return "Blend";
    case ANIM_DEC_FRAME: return "DecFrame";
    case ANIM_BLEND2: return "Blend2";
    case ANIM_STOP: return "Stop";
    default: return "Unknown";
    }
}

static const char* CameraModeName(CameraMode m) {
    switch (m) {
    case CAM_MODE_DEFAULT: return "Debug";
    case CAM_MODE_FOLLOW: return "Follow";
    case CAM_MODE_RIGID: return "Rigid";
    default: return "Unknown";
    }
}

static void LVectorText(const char* label, const LVector& v) {
    ImGui::Text("%s: %d, %d, %d (%.2f, %.2f, %.2f)",
        label, v.x, v.y, v.z,
        v.x / 4096.0f, v.y / 4096.0f, v.z / 4096.0f);
}

void DebugUI::Init() {
}

void DebugUI::Draw() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Game", nullptr, &sShowGame);
            ImGui::MenuItem("Player", nullptr, &sShowPlayer);
            ImGui::MenuItem("Camera", nullptr, &sShowCamera);
            ImGui::MenuItem("Animation", nullptr, &sShowAnimation);
            ImGui::MenuItem("Audio", nullptr, &sShowAudio);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &sShowImGuiDemo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (sShowGame && g_game) {
        if (ImGui::Begin("Game", &sShowGame)) {
            ImGui::Text("State: %s (%d)", GameStateName(g_game->GetState()), (s32)g_game->GetState());
            ImGui::Text("Prev State: %s (%d)", GameStateName(g_game->GetPrevState()), (s32)g_game->GetPrevState());
            if (g_time) {
                ImGui::Separator();
                ImGui::Text("Target Dt: %.4f", g_time->GetTargetDt());
            }
            if (g_scoreManager) {
                ImGui::Separator();
                ImGui::Text("Fight Score: %d", g_scoreManager->currentFightScore);
                ImGui::Text("Combo Score: %d", g_scoreManager->currentComboScore);
                ImGui::Text("Style Score: %d", g_scoreManager->currentStyleScore);
                ImGui::Text("Grade: %d", g_scoreManager->currentGrade);
                ImGui::Text("Gold Dragons: %d", g_scoreManager->currentGoldDragons);
                ImGui::Text("Collectibles: %d", g_scoreManager->collectibleCount);
            }
        }
        ImGui::End();
    }

    if (sShowPlayer) {
        Player* p = Player::s_player;
        if (ImGui::Begin("Player", &sShowPlayer)) {
            if (p) {
                Model* m = p->model ? static_cast<Model*>(p->model) : nullptr;
                AnimStructure* anim = m ? static_cast<AnimStructure*>(m->animStructure) : nullptr;
                u32 cb = (u32)p->commandBits;

                ImGui::SeparatorText("Position");
                LVectorText("Pos", p->pos);
                LVectorText("Orientation", p->orientation);
                LVectorText("Velocity", p->velocity);

                ImGui::SeparatorText("State");
                ImGui::Text("Action State: %s (%d)", ActionStateName(p->actionState), p->actionState);
                ImGui::Text("Attack Joint: %d", p->attackJointIndex);
                ImGui::Text("Prev Joint: %d", p->prevAttackJointIndex);
                ImGui::Text("Dispatch: %s (%u)", StateDispatchName(p->stateDispatch), (u32)p->stateDispatch);
                ImGui::Text("Face Angle: %d", p->faceAngle);
                ImGui::Text("Orientation Y: %d", p->orientation.y);
                ImGui::Text("On Ground: %s", (p->flags & TF_ON_GROUND) ? "yes" : "no");

                ImGui::SeparatorText("Input Bits");
                ImGui::Text("Command Bits: 0x%08X", cb);
                ImGui::Text("Run:%d Jump:%d Guard:%d Strafe:%d", (cb >> 2) & 1, (cb >> 3) & 1, (cb >> 4) & 1, (cb >> 5) & 1);
                ImGui::Text("Backflip:%d Attack:%d Pickup:%d", (cb >> 6) & 1, (cb >> 7) & 1, (cb >> 15) & 1);

                ImGui::SeparatorText("Stats");
                ImGui::Text("Health: %d", p->health);
                ImGui::Text("Lives: %d", p->livesLeft);
                ImGui::Text("Combo: %d (timer: %d)", p->hitCombo, p->comboTimer);
                ImGui::Text("Anim Requested: %d", p->currentAnimEnum);
                ImGui::Text("Anim Load State: %s (%d)", AnimLoadStateName(p->animLoadState), p->animLoadState);
                ImGui::Text("Jump Phase: %d", p->field700);
                ImGui::Text("Force Accum: %d", p->forceAccum);
                ImGui::Text("Idle Timer: %d", p->idleTimer);
                ImGui::Text("Turn Flag/Timer: %d / %d", p->turnAroundFlag, p->turnAroundTimer);
                ImGui::Text("Flags: 0x%04X", p->flags);
                ImGui::Text("Flags2: 0x%04X", p->flags2);
                ImGui::Text("Player Flags: 0x%08X", p->playerFlags);

                ImGui::SeparatorText("Animation Runtime");
                if (anim) {
                    ImGui::Text("Anim Current: %d", anim->animEnum);
                    ImGui::Text("Loop Type: %s (%d)", AnimLoopTypeName(anim->loopTypeField), anim->loopTypeField);
                    ImGui::Text("Loop Count: %d", anim->loopCount);
                    ImGui::Text("Frame: %d / %d", anim->currentFrame >> 16, anim->endFrame >> 16);
                    ImGui::Text("Raw Frame: %d / %d", anim->currentFrame, anim->endFrame);
                    ImGui::Text("Speed: %.3f (raw: %d)", anim->speed / 65536.0f, anim->speed);
                    ImGui::Text("Ticks prev/cur: %d / %d", anim->prevTick, anim->currentTick);
                } else {
                    ImGui::Text("AnimStructure: null");
                }
            } else {
                ImGui::Text("No player");
            }
        }
        ImGui::End();
    }

    if (sShowCamera && g_game) {
        if (ImGui::Begin("Camera", &sShowCamera)) {
            Camera& cam = g_game->GetCamera();
            ImGui::Text("Mode: %s", CameraModeName(cam.GetMode()));
            LVectorText("Position", cam.GetPosition());
            LVectorText("Target", cam.GetTargetPos());
            ImGui::Text("FOV: %d (desired: %d)", cam.GetCurFOV(), cam.GetDesiredFOV());
        }
        ImGui::End();
    }

    if (sShowAnimation) {
        Player* p = Player::s_player;
        if (ImGui::Begin("Animation", &sShowAnimation)) {
            if (p && p->model) {
                Model* m = (Model*)p->model;
                AnimStructure* anim = (AnimStructure*)m->animStructure;
                if (sAnimSelectedEnum < 0) {
                    sAnimSelectedEnum = 0;
                }
                if (sAnimSelectedEnum >= (s32)CharSlot::ANIM_TABLE_SIZE) {
                    sAnimSelectedEnum = (s32)CharSlot::ANIM_TABLE_SIZE - 1;
                }
                if (sAnimSelectedLoopType < ANIM_LOOP || sAnimSelectedLoopType > ANIM_STOP) {
                    sAnimSelectedLoopType = ANIM_LOOP;
                }

                ImGui::SeparatorText("Controls");
                ImGui::InputInt("Anim Enum", &sAnimSelectedEnum, 1, 10);
                if (sAnimSelectedEnum < 0) {
                    sAnimSelectedEnum = 0;
                }
                if (sAnimSelectedEnum >= (s32)CharSlot::ANIM_TABLE_SIZE) {
                    sAnimSelectedEnum = (s32)CharSlot::ANIM_TABLE_SIZE - 1;
                }

                if (ImGui::BeginCombo("Loop Type", AnimLoopTypeName(sAnimSelectedLoopType))) {
                    for (s32 lt = ANIM_LOOP; lt <= ANIM_STOP; lt++) {
                        bool selected = (sAnimSelectedLoopType == lt);
                        if (ImGui::Selectable(AnimLoopTypeName(lt), selected)) {
                            sAnimSelectedLoopType = lt;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::Button("Load")) {
                    if (g_characterManager) {
                        g_characterManager->LoadAnimationBatch(0, sAnimSelectedEnum, nullptr);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Play")) {
                    p->PlayAnimation(sAnimSelectedEnum, sAnimSelectedLoopType);
                }
                ImGui::SameLine();
                if (ImGui::Button("Pause")) {
                    p->PauseAnimation();
                }
                ImGui::SameLine();
                if (ImGui::Button("Resume")) {
                    p->ResumeAnimation();
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    p->StopAnimation();
                }

                CharSlot* playerSlot = nullptr;
                if (g_characterManager) {
                    for (s32 i = 0; i < CHAR_MAX_SLOTS; i++) {
                        if (g_characterManager->slots[i].thingType == 0) {
                            playerSlot = &g_characterManager->slots[i];
                            break;
                        }
                    }
                }

                s32 maxAnimEnum = (s32)CharSlot::ANIM_TABLE_SIZE - 1;
                if (playerSlot && playerSlot->charFile && playerSlot->charFile->rrHeaderEntries >= 10) {
                    s32 rrMax = (playerSlot->charFile->rrHeaderEntries - 10) / 2;
                    if (rrMax < maxAnimEnum) {
                        maxAnimEnum = rrMax;
                    }
                }
                if (maxAnimEnum < 0) {
                    maxAnimEnum = 0;
                }

                s32 loadedCount = 0;
                if (playerSlot && g_characterManager) {
                    for (s32 e = 0; e <= maxAnimEnum; e++) {
                        u8 handle = playerSlot->animIndexTable[e];
                        if (handle != 0xFF && handle < CHAR_MAX_ANIMS && g_characterManager->animPtrs[handle]) {
                            loadedCount++;
                        }
                    }
                }

                ImGui::SeparatorText("Available Animations");
                ImGui::Text("Available enums: 0..%d", maxAnimEnum);
                ImGui::Text("Loaded animations: %d", loadedCount);

                if (ImGui::BeginChild("AnimList", ImVec2(0, 220), true)) {
                    ImGuiListClipper clipper;
                    clipper.Begin(maxAnimEnum + 1);
                    while (clipper.Step()) {
                        for (s32 e = clipper.DisplayStart; e < clipper.DisplayEnd; e++) {
                            bool loaded = false;
                            u8 handle = 0xFF;
                            if (playerSlot && g_characterManager) {
                                handle = playerSlot->animIndexTable[e];
                                loaded = (handle != 0xFF && handle < CHAR_MAX_ANIMS && g_characterManager->animPtrs[handle] != nullptr);
                            }

                            char label[64];
                            std::snprintf(label, sizeof(label), "%03d %s", e, loaded ? "[loaded]" : "");

                            if (ImGui::Selectable(label, sAnimSelectedEnum == e)) {
                                sAnimSelectedEnum = e;
                            }
                        }
                    }
                    ImGui::EndChild();
                }

                ImGui::SeparatorText("Current Playback");
                ImGui::Text("Current Anim Enum: %d", p->currentAnimEnum);
                ImGui::Text("Anim State: %s", p->IsAnimationPaused() ? "Paused" : "Playing");

                if (anim) {
                    ImGui::Text("Frame: %d (0x%X)", anim->currentFrame >> 16, anim->currentFrame);
                    ImGui::Text("Start/End: %d / %d", anim->startFrame >> 16, anim->endFrame >> 16);
                    ImGui::Text("Prev Frame: %d", anim->prevFrame >> 16);
                    ImGui::Text("Speed: %d (%.2fx)", anim->speed, anim->speed / 65536.0f);
                    ImGui::Text("Mode: %d", anim->mode);
                    ImGui::Text("Loop Type: %s (%d)", AnimLoopTypeName(anim->loopTypeField), anim->loopTypeField);
                    ImGui::Text("Loop Count: %d", anim->loopCount);
                    ImGui::Text("Has Flip: %s", anim->flip ? "yes" : "no");
                    ImGui::Text("Has Anim: %s", anim->animation ? "yes" : "no");

                    if (anim->endFrame > anim->startFrame) {
                        f32 progress = (f32)(anim->currentFrame - anim->startFrame) /
                                       (f32)(anim->endFrame - anim->startFrame);
                        if (progress < 0.0f) progress = 0.0f;
                        if (progress > 1.0f) progress = 1.0f;
                        ImGui::ProgressBar(progress, ImVec2(-1, 0), "");
                    }
                } else {
                    ImGui::Text("No AnimStructure");
                }
            } else {
                ImGui::Text("No player model");
            }
        }
        ImGui::End();
    }

    if (sShowAudio) {
        if (ImGui::Begin("Audio", &sShowAudio)) {
            if (AudioEngine::IsInitialized()) {
                f32 master = AudioEngine::GetMasterVolume();
                if (ImGui::SliderFloat("Master Volume", &master, 0.0f, 1.0f)) {
                    AudioEngine::SetMasterVolume(master);
                }
                ImGui::Text("Music Playing: %s", AudioEngine::IsMusicPlaying() ? "yes" : "no");
            } else {
                ImGui::Text("Audio not initialized");
            }
            if (g_sound) {
                ImGui::Separator();
                ImGui::Text("Active SFX Bank: %d", g_sound->activeSfxBank);
                ImGui::Text("WAX Banks: %d", g_sound->numWaxBanks);
                ImGui::Text("Music: %s", g_sound->musicPlaying ? "yes" : "no");
                ImGui::Text("Active: %d", g_sound->activeFlag);
            }
        }
        ImGui::End();
    }

    if (sShowImGuiDemo) {
        ImGui::ShowDemoWindow(&sShowImGuiDemo);
    }
}
