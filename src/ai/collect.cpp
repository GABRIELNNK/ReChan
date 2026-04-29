#include "ai/collect.h"

#include "ai/humanoid.h"
#include "ai/obstacle_shared.h"
#include "ai/player.h"
#include "fe/hud.h"
#include "gen/animmgr.h"
#include "gen/animstruct.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "gen/model.h"
#include "gen/scoremgr.h"
#include "snd/snddrct.h"

static constexpr s32 COLLECT_FLOAT_STEP = 1092;

Collectible::Collectible(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80012744);

    mAnim = nullptr;
    mAnimB = nullptr;
    mCurrentFrame = 0;
    mModelIndex = -1;
    mFloatAngle = 0;
    mFlipAngle = 1;
    mTimer = 20;
}

Collectible::~Collectible() {
    MARKFUNCTION(0x800127A4);
}

void Collectible::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800127CC);

    Obstacle::AnalyzeMesh(root);

    root->FindAttribValue(21, reinterpret_cast<u32*>(&mModelIndex));

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };

    if (root->FindAttribValue(21, reinterpret_cast<u32*>(&mModelIndex))) {
        FillCollisionBox(localBox, *static_cast<const DBVolume*>(root));
    }
    else {
        mModelIndex = -1;
        ObstacleFillCollisionBox(localBox, root, 5);
    }

    SetCollisionBox(localBox);

    const DBAttrib* attrib = root->FindAttrib(6);
    mCollectible = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 0;

    attrib = root->FindAttrib(7);
    mPointsToAdd = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 0;

    attrib = root->FindAttrib(8);
    mHealthToAdd = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 0;

    attrib = root->FindAttrib(9);
    mLivesToAdd = (attrib && attrib->type == 1) ? static_cast<s32>(attrib->value) : 0;

    if (mCollectible != 0) {
        if (g_scoreManager && g_scoreManager->RegisterCollectible(this, mCollectible) != 0) {
            Kill();
        }
    }
}

void Collectible::CreateModel(const char* name) {
    MARKFUNCTION(0x80012970);

    if (mModelIndex != -1 && modelHash != 0) {
        Thing::CreateModel(nullptr);

        Model* mdl = static_cast<Model*>(model);
        if (mdl) {
            mdl->modelFlags |= 5u;
        }

        mAnimB = Obstacle_GetAnimation(mModelIndex);
        if (mAnimB && mdl && mAnimB->anim) {
            if (mdl->drawableType == 2) {
                AnimStructure* as = new AnimStructure(2, mAnimB->anim, ANIM_HOLD_LAST, mdl, mdl->drawable);
                mAnim = as;
                mdl->animStructure = as;

                if (as) {
                    SModel* sm = static_cast<SModel*>(mdl);
                    sm->ApplyAnimToModelBasic(mAnimB->anim);
                    mAnim = static_cast<AnimStructure*>(sm->animStructure);
                    mCurrentFrame = 0;
                    if (mAnim && mAnim->flip) {
                        mAnim->flip->SetFrame(0);
                    }
                }
            }
            else if (mdl->drawableType == 3) {
                delete static_cast<AnimStructure*>(mdl->animStructure);
                mdl->animStructure = new AnimStructure(2, mAnimB->anim, ANIM_HOLD_LAST, mdl, mdl->drawable);
                mAnim = static_cast<AnimStructure*>(mdl->animStructure);
            }
        }
    }
    else {
        if (!model) {
            GModel* gm = new GModel();
            gm->backPtr = this;
            gm->modelFlags |= 5u;
            model = gm;
        }

        Thing::CreateModel(name);
    }

    mInitialPos = pos;
    AllocateAndCreateShadow();
}

void Collectible::DeleteModel() {
    MARKFUNCTION(0x80012C5C);
    Thing::DeleteModel();
}

void Collectible::Reset() {
    MARKFUNCTION(0x80012C7C);
}

void Collectible::Think() {
    MARKFUNCTION(0x80012C84);

    if (mTimer > 0) {
        mTimer--;
    }

    if (mAnim && mAnim->flip && mAnim->flip->anim) {
        TransformFlip* flip = mAnim->flip;
        if (mCurrentFrame < flip->anim->numFrames) {
            s32 oldFrame = mCurrentFrame;
            mCurrentFrame = oldFrame + 1;
            flip->SetFrame(oldFrame);
            flip->UpdateJoints();
        }
        else {
            mCurrentFrame = 0;
        }
    }
    else {
        mCurrentFrame = 0;
    }

    const s32 bobOffset = MulShift16(rmSin16(mFloatAngle + 0x4000), 16);
    const s32 waveRadius = bobOffset + 32;

    if (mFlipAngle != 0) {
        pos.x = mInitialPos.x + MulShift16(rmSin16(mFloatAngle + 0x4000), waveRadius);
        pos.y = mInitialPos.y + MulShift16(rmSin16(mFloatAngle), waveRadius);

        if (mFloatAngle > 0xFFFF) {
            mFloatAngle = -0x8000;
            mFlipAngle ^= 1;
        }
        else {
            mFloatAngle += COLLECT_FLOAT_STEP;
        }
    }
    else {
        pos.x = mInitialPos.x + MulShift16(rmSin16(mFloatAngle + 0x4000), 32) + 64;
        pos.y = mInitialPos.y + MulShift16(rmSin16(mFloatAngle), waveRadius);

        if (-mFloatAngle > 0x17FFF) {
            mFloatAngle = 0;
            mFlipAngle ^= 1;
        }
        else {
            mFloatAngle -= COLLECT_FLOAT_STEP;
        }
    }

    pos.z = mInitialPos.z;
    UpdateShadowFloorHeight();
}

void Collectible::UpdatePosition() {
    MARKFUNCTION(0x80012EC8);
}

void Collectible::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80012ED0);
    (void)pickup;
}

void Collectible::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80012ED8);

    if (mTimer > 0 || !hum) {
        return;
    }

    if (hum->thingType != AITypes::TT_PLAYER) {
        return;
    }

    if (g_scoreManager) {
        g_scoreManager->RegisterGotCollectible(this, mCollectible);
    }

    if (mCollectible == 1 || mCollectible == 2) {
        const u16 soundID = (mCollectible == 1) ? 254 : 255;
        CSoundDirect::PlayTransient(soundID, &pos, 0, 0);
        hum->LoadDialog(14, 99);
        hum->PlayDialog(14, 60);
    }
    else if (mLivesToAdd != 0) {
        CSoundDirect::PlayTransient(257, &pos, 0, 0);
    }

    if (mHealthToAdd != 0) {
        const s32 newHealth = static_cast<s32>(hum->health) + mHealthToAdd;
        if (newHealth < static_cast<s32>(hum->maxHealth)) {
            hum->health = static_cast<u16>(newHealth);
        }
        else {
            hum->health = hum->maxHealth;
        }
        CSoundDirect::PlayTransient(256, &pos, 0, 0);
    }

    if (mLivesToAdd != 0) {
        if (Player::s_player) {
            Player::s_player->SetLivesLeft(Player::s_player->livesLeft + mLivesToAdd);
        }

        if (g_hud) {
            g_hud->DisplayExtraTake();
        }
    }

    Kill();
}