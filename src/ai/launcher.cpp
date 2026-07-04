#include "gen/common.h"
#include "ai/launcher.h"
#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "ai/player.h"
#include "gen/animmgr.h"
#include "gen/animstruct.h"
#include "gen/control.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/game.h"
#include "gen/model.h"
#include "gen/time.h"
#include "p3d/flip.h"
#include "p3d/p3dmath.h"
#include "snd/snddrct.h"
#include "extra/shadowcsm.h"

static constexpr s32 LAUNCHER_AWNING_LAUNCH_FRAME = 8;       // gp+0xc28
static constexpr s32 LAUNCHER_JUMP_HOLD_MAX_FRAMES = 3;      // gp+0xc2c
static constexpr s32 LAUNCHER_FORCE_VECTOR = 12000;           // gp+0xc30
static constexpr s32 LAUNCHER_LAST_FUDGE_FRAME = 20;          // gp+0xc34
static constexpr s32 LAUNCHER_BOUNCE_RESET_FRAME = 0;         // gp+0xc38
static constexpr s32 LAUNCHER_BOX_YMAX = 300;                 // gp+0xc3c

// PSX: __ct__8LauncherPC10tagLVectorUs (0x8001FE34)
Launcher::Launcher(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x8001FE34);
    forceX = 0;
    forceY = 0;
    forceZ = 0;
    animStruct = nullptr;
}

// PSX: __dt__8Launcher (0x8001FE7C)
Launcher::~Launcher() {
    MARKFUNCTION(0x8001FE7C);
}

// PSX: AnalyzeMesh__8LauncherP6DBRoot (0x8001FEA4)
void Launcher::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x8001FEA4);

    Obstacle::AnalyzeMesh(root);

    // PSX: DBRoot +0x28/+0x2C/+0x30 hold launcher draw rotation.
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    // Attrib 3 → collision radius
    u32 tmp = 0;
    root->FindAttribValue(3, &tmp);
    collisionRadius = (u16)tmp;

    // Attrib 6,7,8 → force vector
    root->FindAttribValue(6, (u32*)&forceX);
    root->FindAttribValue(7, (u32*)&forceY);
    root->FindAttribValue(8, (u32*)&forceZ);

    // Attrib 15 → blockNum (PSX stores at +0x54 = blockNum)
    root->FindAttribValue(15, &tmp);
    blockNum = (u16)tmp;

    // Attrib 21 → animation index
    root->FindAttribValue(21, (u32*)&animIndex);

    // Fill collision box from DBVolume bounds
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    FillCollisionBox(localBox, *static_cast<const DBVolume*>(root));
    SetCollisionBox(localBox);
}

// PSX: CreateModel__8LauncherPCc (0x8001FFB0)
void Launcher::CreateModel(const char* name) {
    MARKFUNCTION(0x8001FFB0);

    if (!modelHash) {
        flags |= TF_MODEL_CREATED;
        return;
    }

    Thing::CreateModel(nullptr);

    animation = Obstacle_GetAnimation(animIndex);
    if (!animation) return;

    Model* mdl = static_cast<Model*>(model);
    if (!mdl) return;

    s32 drawableType = mdl->drawableType;

    if (drawableType == 2) {
        // SModel/STree: create AnimStructure(mode=0, TransformAnim*, ANIM_HOLD_LAST, model, drawable)
        TransformAnim* ta = animation->anim;
        AnimStructure* as = new AnimStructure(0, ta, ANIM_HOLD_LAST, mdl, mdl->drawable);
        if (!as) return;
        animStruct = as;
        mdl->animStructure = as;
        animFrame = 0;
        // Set flip to frame 0
        if (as->flip) {
            as->flip->SetFrame(0);
        }
        animPlaying = 0;
    }
    else if (drawableType == 3) {
        // EModel: create AnimStructure(mode=2, TransformAnim*, ANIM_HOLD_LAST, model, drawable)
        // PSX: EModel::ApplyAnimToModel (0x8006FCAC)
        TransformAnim* ta = animation->anim;
        AnimStructure* as = new AnimStructure(2, ta, ANIM_HOLD_LAST, mdl, mdl->drawable);
        mdl->animStructure = as;
        animStruct = as;
    }
}

// PSX: Draw__8Launcher (0x800200F0)
void Launcher::Draw() {
    MARKFUNCTION(0x800200F0);

    Model* mdl = static_cast<Model*>(model);
    if (!mdl) return;

    LVector drawPos = pos;
    LVector drawOrient = orientation;

#if MODERN_GRAPHICS
    // See Obstacle::Draw: skip the HIGH_FPS smoothing update during the
    // shadow caster prepass, since this frame's main pass Draw() call
    // still runs afterwards and would double-advance it.
    if (!ShadowCSM::IsCasterPrepass()) {
        ObstacleBuildRenderTransform(this, pos, orientation, drawPos, drawOrient);
    }
#else
    ObstacleBuildRenderTransform(this, pos, orientation, drawPos, drawOrient);
#endif

    // Copy position and orientation to model
    mdl->posX = drawPos.x;
    mdl->posY = drawPos.y;
    mdl->posZ = drawPos.z;
    // PSX copies 12 bytes from orientation to model rotation area (+0x34)
    // Model rotation is u16 x3 starting at +52, but PSX writes full s32 words
    mdl->rotX = (u16)(drawOrient.x & 0xFFFF);
    mdl->rotY = (u16)(drawOrient.y & 0xFFFF);
    mdl->rotZ = (u16)(drawOrient.z & 0xFFFF);

#if HIGH_FPS_PLAY_PRESENTATION
    const bool launcherInPlay = (g_time && g_game && g_game->GetState() == GameState::Play);
    if (launcherInPlay && animPlaying && animStruct && animStruct->flip) {
        f32 alpha = g_time->GetPlayPresentationAlpha();
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        const s32 prevFrameReal = renderPrevFrame << 16;
        const s32 curFrameReal = animFrame << 16;
        const s32 interpFrameReal = prevFrameReal + (s32)((f32)(curFrameReal - prevFrameReal) * alpha);

        animStruct->flip->SetFrameReal(interpFrameReal);
        animStruct->flip->UpdateJoints();
    }
#endif

    mdl->Show(0);
}

// PSX: DeleteModel__8Launcher (0x80020164)
void Launcher::DeleteModel() {
    MARKFUNCTION(0x80020164);

    Thing::DeleteModel();
    animStruct = nullptr;
}

// PSX: Reset__8Launcher (0x8002018C)
void Launcher::Reset() {
    MARKFUNCTION(0x8002018C);

    // PSX: animFrame = (s16)(animStruct->endFrame >> 16)
    // endFrame is stored as 16.16 fixed point, high half = integer frame index
    if (animStruct) {
        animFrame = (s16)(animStruct->endFrame >> 16);
    }
    animPlaying = 0;
}

// PSX: Think__8Launcher (0x800201A4)
void Launcher::Think() {
    MARKFUNCTION(0x800201A4);

    // Override collBox maxY with LAUNCHER_BOX_YMAX each frame
    tagCollisionBox localBox = collBox;
    localBox.maxY = (s16)LAUNCHER_BOX_YMAX;
    SetCollisionBox(localBox);

    if (!animPlaying) {
        animFrame = 0;
        animPlaying = 0;
#if HIGH_FPS_PLAY_PRESENTATION
        renderPrevFrame = 0;
#endif
        return;
    }

    // Animation is playing - advance frame
    if (!animStruct) {
        animFrame = 0;
        animPlaying = 0;
        return;
    }

    TransformFlip* flip = animStruct->flip;
    if (!flip || !flip->anim) {
        animFrame = 0;
        animPlaying = 0;
        return;
    }

    s32 numFrames = flip->anim->numFrames;
    if (animFrame < numFrames) {
        s32 oldFrame = animFrame;
        animFrame = oldFrame + 1;
        flip->SetFrame(oldFrame);
        flip->UpdateJoints();
#if HIGH_FPS_PLAY_PRESENTATION
        renderPrevFrame = oldFrame;
#endif
    }
    else {
        animFrame = 0;
        animPlaying = 0;
    }
}

// PSX: UpdatePosition__8Launcher (0x80020290) - empty
void Launcher::UpdatePosition() {
    MARKFUNCTION(0x80020290);
}

// PSX: HandleHumanoidCollision__8LauncherP8Humanoid (0x80020328)
void Launcher::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80020328);

    // Compute delta = hum->pos - hum->velocity (previous position estimate)
    LVector delta;
    delta.x = hum->pos.x - hum->velocity.x;
    delta.y = hum->pos.y - hum->velocity.y;
    delta.z = hum->pos.z - hum->velocity.z;

    bool collision = false;

    if (model) {
        // Model exists: use full box collision with correction
        LVector outPos, outNormal, outPushedPos;
        bool hit = CorrectThingPositionObstacle(
            pos, pos,
            orientation.y, orientation.y,
            collBox,
            delta,
            hum->homePos,
            hum->collBboxMin.x,
            hum->collBboxMin.y,
            hum->collBboxMin.z,
            outPos,
            outNormal,
            outPushedPos);

        if (hit) {
            // Update humanoid homePos from corrected obstacle response.
            hum->homePos.x = outPos.x;
            hum->homePos.y = outPos.y;
            hum->homePos.z = outPos.z;

            // Set floor height = this->pos.y + collBox.maxY
            hum->groundStandHeight = pos.y + collBox.maxY;

            if (outNormal.y < 0) {
                // Side/below collision
                if (hum->velocity.y > 0) {
                    // Zero velocity.y when hitting from below
                    hum->velocity.y = 0;
                }
                // Not an on-top collision, skip launch logic
            }
            else if (outNormal.y > 0) {
                // On-top collision
                if (hum->velocity.y < 0) {
                    hum->velocity.y = 0;
                }

                // Launcher-top hold uses the flip handler, not normal grounded landing.
                hum->flags &= ~TF_ON_GROUND;

                // Trigger bounce animation
                animPlaying = 1;
                s32 endFrameInt = animStruct ? (s16)(animStruct->endFrame >> 16) : 0;
                if (animFrame < endFrameInt && animFrame < LAUNCHER_LAST_FUDGE_FRAME && animFrame != 0) {
                    if (hum->thingType == AITypes::TT_PLAYER && hum->actionState != AS_FLIP_VARIANT) {
                        hum->SetActionState(AS_FLIP_VARIANT, 0);
                    }
                    // Animation in progress, within fudge window - skip reset
                    collision = true;
                    goto collision_check;
                }

                // Reset animation frame
                animFrame = 0;

                if (hum->actionState != AS_FLIP_VARIANT) {
                    hum->SetActionState(AS_FLIP_VARIANT, 0);
                }
                else {
                    // Already in launcher state: check if model anim has looped
                    Model* humModel = static_cast<Model*>(hum->model);
                    if (humModel) {
                        AnimStructure* humAnim = static_cast<AnimStructure*>(humModel->animStructure);
                        if (humAnim) {
                            humModel->SetAnim(PLAYER_ANIM_ID_296, LAUNCHER_BOUNCE_RESET_FRAME, 1, 0);
                        }
                        else {
                            collision = true;
                            goto collision_check;
                        }
                    }
                }

                // Damp velocity XZ by ~0.1 (0x1999/0x10000)
                hum->velocity.x = (s32)(((s64)hum->velocity.x * 0x1999) >> 16);
                hum->velocity.z = (s32)(((s64)hum->velocity.z * 0x1999) >> 16);

                collision = true;
            }
        }
    }
    else {
        // No model: simple point-in-box test
        bool inBox = CheckStaticHorizontalBoxPointCollision(pos, collBox, orientation.y, hum->pos);
        bool onGround = (hum->flags >> 12) & 1;
        if (onGround && inBox) {
            collision = true;
        }
    }

collision_check:
    if (!collision) {
        return;
    }

    bool doLaunch = false;

    if (collisionRadius != 0) {
        // Has collision radius: always launch
        HandleHumanoidDefaultLaunch(hum);
        if (hum == Player::s_player) {
            Shock(SHOCK_13);
        }
        return;
    }

    // collisionRadius == 0: check launch conditions
    if (!model) {
        // No model: launch immediately
        doLaunch = true;
    }
    else if (animFrame >= LAUNCHER_AWNING_LAUNCH_FRAME) {
        // Animation past launch frame: launch
        doLaunch = true;
    }
    else {
        // Check if jump was requested during the bounce window.
        if (hum->thingType == AITypes::TT_PLAYER) {
            s32 jumpDuration = 0;
            if (g_inputManager) {
                Button* jumpButton = g_inputManager->GetButtonForBit(0, 6);
                if (jumpButton) {
                    jumpDuration = (s32)(s16)jumpButton->duration;
                }
            }

            // PSX launcher timing uses Jump button hold duration window: 1..gp+0xC2C.
            if (jumpDuration > 0 && jumpDuration <= LAUNCHER_JUMP_HOLD_MAX_FRAMES) {
                flags |= TF_ON_GROUND;
                doLaunch = true;
            }
        }
        else {
            // NPC: check jump command bit (bit 3 of commandBits)
            if ((hum->commandBits >> 3) & 1) {
                flags |= TF_ON_GROUND;
                doLaunch = true;
            }
        }
    }

    if (doLaunch) {
        HandleHumanoidDefaultLaunch(hum);
        if (hum == Player::s_player) {
            Shock(SHOCK_13);
        }
    }
}

// PSX: HandleHumanoidDefaultLaunch__8LauncherP8Humanoid (0x80020730)
void Launcher::HandleHumanoidDefaultLaunch(Humanoid* hum) {
    MARKFUNCTION(0x80020730);

    // Play launch sound at launcher position
    CSoundDirect::PlayTransient(0x17, &pos, 0, 0);

    // Zero humanoid velocity
    hum->velocity.x = 0;
    hum->velocity.y = 0;
    hum->velocity.z = 0;

    // Copy force vector
    LVector force;
    force.x = forceX;
    force.y = forceY;
    force.z = forceZ;

    if (hum->thingType == 0) {
        // Player
        if (flags & TF_ON_GROUND) {
            // First launch: clear launcher's TF_ON_GROUND, set player action state
            flags &= ~TF_ON_GROUND;
            hum->SetActionState(AS_FLIP, 0);
        }
        else {
            // Subsequent launch: use constant force Y instead
            force.y = LAUNCHER_FORCE_VECTOR;
        }

        // Add force to humanoid contactForce
        hum->contactForce.x += force.x;
        hum->contactForce.y += force.y;
        hum->contactForce.z += force.z;
    }

    // Clear TF_ON_GROUND on humanoid
    hum->flags &= ~TF_ON_GROUND;
}
