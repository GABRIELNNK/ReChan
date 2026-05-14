#pragma once

#include "ai/obstacle.h"

class AnimStructure;
struct TransformAnim;

class Collectible : public Obstacle {
public:
    AnimStructure* mAnim = nullptr;
    MiscAnimNode* mAnimB = nullptr;
    TransformAnim* mBoundTransformAnim = nullptr;
    s32 mCurrentFrame = 0;
    s32 mModelIndex = -1;
    s32 mFloatAngle = 0;
    s32 mFloatRadius = 0;
    s32 mFlipAngle = 1;
    LVector mInitialPos = {};
    s32 mCollectible = 0;
    s32 mPointsToAdd = 0;
    s32 mHealthToAdd = 0;
    s32 mLivesToAdd = 0;
    s32 mTimer = 20;

    Collectible(const LVector* pos, u16 type);
    ~Collectible() override;

    void AnalyzeMesh(DBRoot* root) override;
    void CreateModel(const char* name) override;
    void DeleteModel() override;
    void Reset() override;
    void Think() override;
    void UpdatePosition() override;
    void HandlePickupCollision(Thing* pickup) override;
    void HandleHumanoidCollision(Humanoid* hum) override;
};