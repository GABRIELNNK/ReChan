#include "gen/model.h"
#include "gen/animmat.h"
#include "gen/animstruct.h"
#include "gen/charmgr.h"
#include "ai/player.h"
#include "gen/skeleton.h"
#include "p3d/context.h"
#include "p3d/p3dmath.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "snd/hmndsnd.h"
#include "pc/log.h"
#include <cstdlib>
#include <vector>

// OriginalSTree
OriginalSTree::OriginalSTree() {
    SetType(1); // STree type
}

OriginalSTree::~OriginalSTree() {
    if (meshBuffer) {
        meshBuffer->Release();
        meshBuffer = nullptr;
    }
    if (skeleton) {
        delete skeleton;
        skeleton = nullptr;
    }
    if (skinData) {
        delete skinData;
        skinData = nullptr;
    }
}

OriginalGeo::OriginalGeo() {
    SetType(0); // Geo type
}

OriginalGeo::~OriginalGeo() {
    if (meshBuffer) {
        meshBuffer->Release();
        meshBuffer = nullptr;
    }
}

OriginalETree::OriginalETree() {
    SetType(2); // ETree type
}

OriginalETree::~OriginalETree() {
    if (meshBuffer) {
        meshBuffer->Release();
        meshBuffer = nullptr;
    }
}

DrawableBasic::~DrawableBasic() = default;

// DrawableSTree
DrawableSTree::DrawableSTree(OriginalSTree* orig) {
    original = orig;
    alternate = nullptr;
}

DrawableSTree::~DrawableSTree() {
    original = nullptr;
    alternate = nullptr;
}

// PSX: Display dispatches through vtable to OriginalSTree::Draw -> tPrimGeom::Display
// PC: draws the skeleton mesh (per-joint transforms baked in) or flat fallback
void DrawableSTree::Display(u32 /*flags*/) {
    if (!original)
        return;

    STreeData* skel = original->skeleton;
    SkinData* skin = original->skinData;

    // Per-frame CPU skinning
    if (skel && skin && skin->numVerts > 0 && skel->joints &&
        skel->joints[0].meshBuffer) {
        Mat4* jointMatrices = new Mat4[skel->numJoints];
        skel->ComputeWorldMatrices(jointMatrices);

        std::vector<f32> vertData(skin->numVerts * 10);
        for (u32 i = 0; i < skin->numVerts; i++) {
            const SkinVertex& sv = skin->verts[i];
            const Mat4& m = jointMatrices[sv.jointIdx];
            f32 wx, wy, wz;
            Mat4TransformPoint(m, sv.lx, sv.ly, sv.lz, wx, wy, wz);
            vertData[i * 10 + 0] = wx;
            vertData[i * 10 + 1] = wy;
            vertData[i * 10 + 2] = wz;
            vertData[i * 10 + 3] = sv.r;
            vertData[i * 10 + 4] = sv.g;
            vertData[i * 10 + 5] = sv.b;
            vertData[i * 10 + 6] = sv.u;
            vertData[i * 10 + 7] = sv.v;
            vertData[i * 10 + 8] = sv.tpage;
            vertData[i * 10 + 9] = sv.cba;
        }

        skel->joints[0].meshBuffer->SetVertexData(vertData.data(), skin->numVerts);
        p3d::context->DrawPrimBuffer(skel->joints[0].meshBuffer);
        delete[] jointMatrices;
        return;
    }

    // Fallback to flat mesh
    STreeData* fallbackSkel = original->skeleton;
    if (fallbackSkel && fallbackSkel->joints && fallbackSkel->joints[0].meshBuffer) {
        p3d::context->DrawPrimBuffer(fallbackSkel->joints[0].meshBuffer);
    }
    else if (original->meshBuffer) {
        p3d::context->DrawPrimBuffer(original->meshBuffer);
    }
}

DrawableGeo::DrawableGeo(OriginalGeo* orig) {
    original = orig;
}

DrawableGeo::~DrawableGeo() {
    original = nullptr;
}

void DrawableGeo::Display(u32 /*flags*/) {
    if (original && original->meshBuffer) {
        p3d::context->DrawPrimBuffer(original->meshBuffer);
    }
}

DrawableETree::DrawableETree(OriginalETree* orig) {
    original = orig;
}

DrawableETree::~DrawableETree() {
    original = nullptr;
}

void DrawableETree::Display(u32 /*flags*/) {
    if (original && original->meshBuffer) {
        p3d::context->DrawPrimBuffer(original->meshBuffer);
    }
}

// Model
// PSX: _5Model (MODEL.CPP:671, 0x8006E654)
Model::Model() {
    MARKFUNCTION(0x8006E654);
    drawable = nullptr;
    drawableType = 0;
    animStructure = nullptr;
    field36 = new ModelFloorHeightState();
    ambientLight = nullptr;
    hwLights = nullptr;
    hwLightCount = 0;
    rotX = 0;
    rotY = 0;
    rotZ = 0;
    posX = 0;
    posY = 0;
    posZ = 0;
    backPtr = nullptr;
    modelFlags = 0;
    Reset();
}

// PSX: __5Model (MODEL.CPP:697, 0x8006E6CC)
Model::~Model() {
    MARKFUNCTION(0x8006E6CC);
    if (field36) {
        delete static_cast<ModelFloorHeightState*>(field36);
        field36 = nullptr;
    }
    DeleteDrawable();
}

// PSX: Reset__5Model (MODEL.CPP:777, 0x8006E86C)
void Model::Reset() {
    MARKFUNCTION(0x8006E86C);
    shadowAngle = 0;
}

void Model::Show(u32 /*flags*/) {
    // Base does nothing - overridden by SModel/GModel
}

void Model::Animate() {
    // Base does nothing - overridden by SModel
}

void Model::ApplyAnimToModel(s32 /*thingType*/, s32 /*animEnum*/, s32 /*p3*/, s32 /*p4*/, s32 /*p5*/) {
    // Base does nothing - overridden by SModel
}

void Model::DeleteDrawable() {
    if (drawable) {
        delete drawable;
        drawable = nullptr;
    }
    drawableType = 0;
}

// PSX: SetAnim__5Modelllil (MODEL.CPP:1907)
// Base implementation is a no-op. Overridden by HumanoidModel/PlayerModel.
void Model::SetAnim(s32 /*animEnum*/, s32 /*loopType*/, s32 /*flag*/, s32 /*extra*/) {}

// PSX: Model animation boundary handlers (MODEL.CPP:1916-2009)
// Base implementations are trampolines to AnimStructure methods.
// PSX signature: void _Handler(Model* this, AnimStructure* anim)

void Model::HandleLoop(AnimStructure* anim) { anim->Loop(); }
void Model::HandleLoopReverse(AnimStructure* anim) { anim->LoopReverse(); }
void Model::HandleHoldFirst(AnimStructure* anim) { anim->HoldFirst(); }
void Model::HandleHoldLast(AnimStructure* anim) { anim->HoldLast(); }
void Model::HandleRunToLast(AnimStructure* anim) { anim->RunToLast(); }
void Model::HandleHoldFrame(AnimStructure* /*anim*/) { /* PSX: empty stub */ }
void Model::HandleRunToFrame(AnimStructure* /*anim*/) { /* PSX: empty stub */ }
void Model::HandleIncFrame(AnimStructure* /*anim*/) { /* PSX: empty stub */ }
void Model::HandleDecFrame(AnimStructure* anim) { anim->DecFrame(); }
void Model::HandleLoopDesired(AnimStructure* /*anim*/) { /* PSX: empty stub */ }
void Model::HandleRunToLastBlend(AnimStructure* anim) { anim->RunToLastBlend(); }

// SModel
// PSX: _6SModel (MODEL.CPP:1013, 0x8006ED68)
SModel::SModel() {
    MARKFUNCTION(0x8006ED68);
    scale = FIX16_ONE;
    field92 = 0;
}

SModel::~SModel() {
    MARKFUNCTION(0x8006EDAC);
}

// PSX: Show__6SModelUl (MODEL.CPP:1454, 0x8006F68C)
// PC adaptation: sets world matrix from position/rotation/scale, then draws mesh
void SModel::Show(u32 flags) {
    MARKFUNCTION(0x8006F68C);

    // PSX: clear visible/drawn bits
    modelFlags &= ~0x30;

    if (!drawable || !backPtr)
        return;

    // Mark as visible + drawn
    modelFlags |= 0x50;

    AnimStructure* anim = static_cast<AnimStructure*>(animStructure);
    if (anim && anim->flip && anim->flip->dirty) {
        s32 frame = anim->currentFrame;
        if (frame < 0) {
            frame = 0;
        }
        anim->flip->SetFrameReal(frame);
        anim->flip->UpdateJoints();
    }

    // Build world matrix from position + rotation + scale
    // PSX: TransMatrix, RotMatrixZYXAndLights, ScaleMatrix (from MIPS LST)
    Mat4 world;
    p3dBuildRotMatrixZYX(rotX, rotY, rotZ, world);

    // Scale: PSX FIX16_ONE = 1.0
    f32 s = FIX16_TO_FLOAT(scale);
    world.ScaleRotation(s);

    // Translation
    world.SetTranslation((f32)posX, (f32)posY, (f32)posZ);

    Humanoid* humanoid = dynamic_cast<Humanoid*>(backPtr);
    const bool compensateLadderRoot = humanoid && (humanoid->flags2 & TF2_NIS_ENTER) != 0;

    if (compensateLadderRoot) {
        OriginalSTree* original = drawable->GetOriginalSTree();
        STreeData* skeleton = original ? original->skeleton : nullptr;
        if (skeleton && skeleton->joints && skeleton->jointOrderMap && skeleton->numMapEntries > 0) {
            u32 jointIndex = skeleton->jointOrderMap[0];
            if (jointIndex < skeleton->numJoints) {
                const STreeJoint& joint = skeleton->joints[jointIndex];
                f32 rootWorldX = 0.0f;
                f32 rootWorldY = 0.0f;
                f32 rootWorldZ = 0.0f;
                Mat4TransformDir(world,
                    (f32)joint.translationX,
                    (f32)joint.translationY,
                    (f32)joint.translationZ,
                    rootWorldX,
                    rootWorldY,
                    rootWorldZ);
                world.SetTranslation(
                    (f32)posX - rootWorldX,
                    (f32)posY - rootWorldY,
                    (f32)posZ - rootWorldZ);
            }
        }
    }

    p3d::context->SetWorldMatrix(world);

    // PSX: calls drawable->Display(flags) through vtable
    drawable->Display(flags);
}

// PSX: Animate__6SModel (MODEL.CPP:1416, 0x8006F640)
void SModel::Animate() {
    MARKFUNCTION(0x8006F640);
    if (animStructure) {
        AnimStructure* anim = (AnimStructure*)animStructure;
        anim->ExecuteHandler(1);
    }
}

// PSX: ApplyAnimToModel__6SModellllll (MODEL.CPP:1098, 0x8006EEAC)
void SModel::ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 /*p4*/, s32 /*p5*/) {
    MARKFUNCTION(0x8006EEAC);
    if (!g_characterManager) return;

    void* anim = g_characterManager->GetAnimation((u32)thingType, animEnum);
    if (!anim) {
        anim = g_characterManager->GetAnimation(0, animEnum);
        if (!anim) {
            LOG("[Model] ApplyAnimToModel: anim %d not found for type %d, lazy-loading", animEnum, thingType);
            g_characterManager->LoadAnimationBatch(0, animEnum, nullptr);
            anim = g_characterManager->GetAnimation(0, animEnum);
            if (!anim) {
                LOG("[Model] ApplyAnimToModel: anim %d STILL not found after load, falling back to 22", animEnum);
                anim = g_characterManager->GetAnimation(0, 22);
                animEnum = 22;
            }
        }
    }

    if (!animStructure) {
        animStructure = new AnimStructure(0, anim, loopType, this, drawable);
    }

    AnimStructure* as = (AnimStructure*)animStructure;
    as->animEnum = animEnum;

    // PSX: p4 == 0 path (normal apply)
    as->animation = (TransformAnim*)anim;
    if (as->flip) {
        as->flip->anim = (TransformAnim*)anim;
        as->flip->additiveTranslation = false;
        as->flip->dirty = 1;
    }
    as->ResetCountsToAnim();
    as->SetLoopType(loopType, 1);
    as->humanoidCB = {};
}

// PSX: SetOriginalSTree__6SModelP13OriginalSTreeP10tAnimation (MODEL.CPP:1026, 0x8006EDD4)
void SModel::SetOriginalSTree(OriginalSTree* original) {
    MARKFUNCTION(0x8006EDD4);
    DeleteDrawable();
    drawable = new DrawableSTree(original);
    drawableType = 2; // STree type
}

// PSX: InitSemiTransMode__6SModel (MODEL.CPP:1045, 0x8006EE20)
void SModel::InitSemiTransMode() {
    MARKFUNCTION(0x8006EE20);
    // PSX: calls SetSemiMode on the OriginalSTree - no-op on PC for now
}

// PSX: PlayDynamicAnim__6SModeli (MODEL.CPP:1710, 0x8006FAFC)
void SModel::PlayDynamicAnim(s32 animEnumVal) {
    MARKFUNCTION(0x8006FAFC);
    SetAnim(animEnumVal, 0, 0, 0);
    AnimStructure* anim = static_cast<AnimStructure*>(animStructure);
    if (anim) {
        anim->animEnum = animEnumVal;
    }
}

GModel::GModel() {
    MARKFUNCTION(0x8006E8C8);
}

GModel::~GModel() {
    MARKFUNCTION(0x8006E908);
}

void GModel::Show(u32 flags) {
    MARKFUNCTION(0x8006EA7C);

    modelFlags &= ~0x30;

    if (!drawable || !backPtr) {
        return;
    }

    modelFlags |= 0x50;

    Mat4 world;
    p3dBuildRotMatrixZYX(rotX, rotY, rotZ, world);
    world.SetTranslation((f32)posX, (f32)posY, (f32)posZ);

    p3d::context->SetWorldMatrix(world);
    drawable->Display(flags);
}

void GModel::SetOriginalGeo(OriginalGeo* original) {
    DeleteDrawable();
    drawable = new DrawableGeo(original);
    drawableType = 1;
}

EModel::EModel() {
    MARKFUNCTION(0x8006FB50);
}

EModel::~EModel() {
    MARKFUNCTION(0x8006FB84);
}

void EModel::SetOriginalETree(OriginalETree* original, TransformAnim* animation) {
    MARKFUNCTION(0x8006FBAC);
    DeleteDrawable();
    drawable = new DrawableETree(original);
    drawableType = 3;

    if (animation) {
        if (animStructure) {
            delete static_cast<AnimStructure*>(animStructure);
            animStructure = nullptr;
        }
        animStructure = new AnimStructure(1, animation, ANIM_LOOP, this, drawable);
    }
}

void EModel::ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 /*p4*/, s32 /*p5*/) {
    MARKFUNCTION(0x8006FC34);
    if (!g_characterManager) {
        return;
    }

    TransformAnim* anim = (TransformAnim*)g_characterManager->GetAnimation((u32)thingType, animEnum);
    if (!anim) {
        anim = (TransformAnim*)g_characterManager->GetAnimation(0, animEnum);
        if (!anim) {
            return;
        }
    }

    if (animStructure) {
        delete static_cast<AnimStructure*>(animStructure);
        animStructure = nullptr;
    }
    animStructure = new AnimStructure(2, anim, loopType, this, drawable);
}

void EModel::Animate() {
    MARKFUNCTION(0x8006FD10);
    if (!animStructure) {
        return;
    }

    s32 doFlip = ((modelFlags >> 6) & 1) ? 1 : 0;
    static_cast<AnimStructure*>(animStructure)->ExecuteHandler(doFlip);
}

void EModel::Show(u32 flags) {
    MARKFUNCTION(0x8006FD44);

    modelFlags &= ~0x30;

    if (!drawable || !backPtr) {
        return;
    }

    modelFlags |= 0x50;

    Mat4 world;
    p3dBuildRotMatrixZYX(rotX, rotY, rotZ, world);
    world.SetTranslation((f32)posX, (f32)posY, (f32)posZ);

    p3d::context->SetWorldMatrix(world);
    drawable->Display(flags);
}

// HumanoidModel

// PSX: SetAnim__13HumanoidModelllil (MHUMAN.CPP:166, 0x8006E248)
// Routes anims to ApplyAnimToModel. Transition anims (37-38) get
// special blend-from-current-frame handling. High enum anims default to
// RunToLast unless explicitly routed to Loop/HoldFirst by the PSX tree.
void HumanoidModel::SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) {
    MARKFUNCTION(0x8006E248);

    AnimStructure* as = (AnimStructure*)animStructure;
    // Early exit: if not forcing and anim already matches, no-op
    if (!force && as && as->animEnum == animEnum) {
        return;
    }

    // PSX: reads thingType from backPtr + 24 (Thing::thingType)
    s32 thingType = backPtr->thingType;

    if (animEnum == 0) {
        ApplyAnimToModel(thingType, animEnum, ANIM_HOLD_FIRST, a3, extra);
        return;
    }

    if (animEnum == 1 || animEnum == 2 || animEnum == 4 || animEnum == 15 || animEnum == 22
        || animEnum == 43 || animEnum == 45 || animEnum == 49 || animEnum == 50
        || animEnum == 314 || animEnum == 315) {
        ApplyAnimToModel(thingType, animEnum, ANIM_LOOP, a3, extra);
        return;
    }

    // PSX: anims 37-38 are transition anims with blend from current frame
    if (animEnum >= 37 && animEnum <= 38) {
        // PSX: ApplyAnimToModel using current frame as start, loopType=2
        ApplyAnimToModel(thingType, animEnum, 2, a3, extra);
        // PSX: sets humanoidCB blend data {4063232, 8}
        as = (AnimStructure*)animStructure;
        if (as) {
            as->humanoidCB.offsetLo = (s16)(4063232 & 0xFFFF);
            as->humanoidCB.offsetHi = (s16)(4063232 >> 16);
            as->humanoidCB.funcPtr = reinterpret_cast<void*>(static_cast<intptr_t>(8));
        }
        return;
    }

    // PSX default path uses RunToLast for all remaining humanoid anims.
    ApplyAnimToModel(thingType, animEnum, ANIM_RUN_TO_LAST, a3, extra);
}

// PSX: _13HumanoidModel (MHUMAN.CPP:45, 0x8006E020)
HumanoidModel::HumanoidModel() {
    MARKFUNCTION(0x8006E020);
    // PSX allocates AnimationMatrices (660 bytes on PSX layout).
    animMatrices = new AnimationMatrices();
    attackHandRadius = 100;
    attackFootRadius = 100;
    field108 = 400;
    field112 = 0;
    field116 = 0;
    field120 = 0;
    field124 = 0;
    field128 = 0;
    field132 = INVALID_HANDLE;
}

// PSX: __13HumanoidModel (MHUMAN.CPP:56, 0x8006E0C8)
HumanoidModel::~HumanoidModel() {
    MARKFUNCTION(0x8006E0C8);
    if (animMatrices) {
        delete animMatrices;
        animMatrices = nullptr;
    }
}

// PSX: Animate__13HumanoidModel (MHUMAN.CPP:207, 0x8006E418)
void HumanoidModel::Animate() {
    MARKFUNCTION(0x8006E418);
    SModel::Animate();

    Humanoid* owner = static_cast<Humanoid*>(backPtr);
    if (owner && owner->humanoidSound && animStructure) {
        AnimStructure* anim = static_cast<AnimStructure*>(animStructure);
        owner->humanoidSound->ProcessSoundScript(
            (u32)anim->animEnum,
            (u32)(s16)((u32)anim->currentFrame >> 16)
        );
    }
}

// PSX: _Loop__13HumanoidModelP13AnimStructure (MHUMAN.CPP:196)
// Only calls base Loop when mode == 0 (normal). For reverse/runToLast/camera
// modes, the loop handler intentionally does nothing.
void HumanoidModel::HandleLoop(AnimStructure* anim) {
    if (anim->mode == 0) {
        Model::HandleLoop(anim);
    }
}

// PlayerModel
PlayerModel::PlayerModel() {
    // Same as HumanoidModel on PSX
}

PlayerModel::~PlayerModel() {}

void PlayerModel::ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) {
    Player* owner = (backPtr && backPtr->thingType == AITypes::TT_PLAYER)
        ? static_cast<Player*>(backPtr)
        : nullptr;

    if (owner && owner->Debug_IsAnimationOverrideActive() && !owner->Debug_IsAnimationOverrideApplying()) {
        return;
    }

    SModel::ApplyAnimToModel(thingType, animEnum, loopType, p4, p5);
}

// PSX: SetAnim__11PlayerModelllil (MPLAYER.CPP:293, 0x80077B20)
// Routes player-specific animation enums to correct loop types.
// Some anims trigger sound effects. Falls back to HumanoidModel::SetAnim
// for unrecognized enums.
void PlayerModel::SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) {
    MARKFUNCTION(0x80077B20);

    Player* owner = (backPtr && backPtr->thingType == AITypes::TT_PLAYER)
        ? static_cast<Player*>(backPtr)
        : nullptr;
    if (owner && owner->Debug_IsAnimationOverrideActive() && !owner->Debug_IsAnimationOverrideApplying()) {
        return;
    }

    AnimStructure* as = (AnimStructure*)animStructure;
    // Early exit: if not forcing and anim already matches, no-op
    if (!force && as && as->animEnum == animEnum) {
        return;
    }

    s32 loopType = ANIM_RUN_TO_LAST; // default v11=2
    bool handled = true;
    bool setBlendData = false;

    // PSX anim routing - traced from binary comparison tree
    if (animEnum == 1 || animEnum == 22) {
        // PSX: anim 1 + 22 redirect to 22 with Loop
        animEnum = 22;
        loopType = ANIM_LOOP;
    }
    else if (animEnum == 2 || animEnum == 31 || animEnum == 41 ||
             animEnum == 47 || animEnum == 48 || animEnum == 189 ||
             animEnum == 206 || animEnum == 220 || animEnum == 231 ||
             animEnum == 244 || animEnum == 254 || animEnum == 261) {
        // PSX: these anims use Loop (v11=0)
        loopType = ANIM_LOOP;
    }
    else if (animEnum == 3 || animEnum == 17 || animEnum == 21 ||
             animEnum == 152 || animEnum == 295) {
        // PSX: RunToLast, no blend data
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum == 24) {
        // PSX: strafe - plays DiveRoll sound if frame < 29
        if (backPtr && backPtr->thingType < 29) {
            Humanoid* owner = static_cast<Humanoid*>(backPtr);
            if (owner->humanoidSound) {
                owner->humanoidSound->DiveRoll(SMAT_CONCRETE);
            }
        }
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum >= 27 && animEnum <= 30) {
        // PSX: 27 gets blend data, 28 goes to LABEL_58, 29-30 are RunToLast
        if (animEnum == 27) {
            setBlendData = true;
        }
        else if (animEnum == 28) {
            HumanoidModel::SetAnim(animEnum, a3, force, extra);
            return;
        }
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum >= 32 && animEnum <= 36) {
        // PSX: RunToLast, no blend data
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum == 42) {
        // PSX: plays HitWorldStructure sound if frame < 29
        if (backPtr && backPtr->thingType < 29) {
            Humanoid* owner = static_cast<Humanoid*>(backPtr);
            if (owner->humanoidSound) {
                owner->humanoidSound->HitWorldStructure(SMAT_CONCRETE);
            }
        }
        setBlendData = true;
        loopType = ANIM_RUN_TO_LAST;
    }
    else if (animEnum == 46) {
        // PSX: RunToLast with blend data (turn animation)
        setBlendData = true;
        loopType = ANIM_RUN_TO_LAST;
    }
    else {
        // Fallback to HumanoidModel::SetAnim
        HumanoidModel::SetAnim(animEnum, a3, force, extra);
        handled = false;
    }

    if (handled) {
        ApplyAnimToModel(0, animEnum, loopType, a3, extra);
        // PSX: LABEL_59 always writes v18[0]/v18[1] into animStructure+96/+100.
        // v18 starts as {0,0} and is set to {3997696, 8} only for blend anims.
        as = (AnimStructure*)animStructure;
        if (as) {
            if (setBlendData) {
                // PSX: v18[0] = 3997696 (0x003D0000), LOWORD(v18[1]) = 8
                as->humanoidCB.offsetLo = 0;
                as->humanoidCB.offsetHi = 61;
                as->humanoidCB.funcPtr = reinterpret_cast<void*>(static_cast<intptr_t>(8));
            }
            else {
                as->humanoidCB.offsetLo = 0;
                as->humanoidCB.offsetHi = 0;
                as->humanoidCB.funcPtr = nullptr;
            }
        }
    }
}

// PSX: _Loop__11PlayerModelP13AnimStructure (MPLAYER.CPP:573)
// Simple trampoline to base Model::HandleLoop on PSX.
void PlayerModel::HandleLoop(AnimStructure* anim) {
    Model::HandleLoop(anim);
}

// PSX: _RunToLast__11PlayerModelP13AnimStructure (MPLAYER.CPP:374)
// Checks specific anim enums for animation chaining on completion.
void PlayerModel::HandleRunToLast(AnimStructure* anim) {
    s32 curAnim = anim->animEnum;

    if (curAnim == 281) {
        // PSX: anim 281 (0x119) chains to anim 282 (0x11A) on completion
        Model::HandleRunToLast(anim);
        if (anim->loopCount > 0) {
            SetAnim(282, 0, 0, 0);
        }
        return;
    }

    if (curAnim == 32) {
        // PSX: walljump anim (0x20) - on completion, calls DoWallJump then chains to anim 33
        Model::HandleRunToLast(anim);
        s16 currentFrameHi = (s16)((u32)anim->currentFrame >> 16);
        s16 endFrameHi = (s16)((u32)anim->endFrame >> 16);
        if (currentFrameHi >= endFrameHi) {
            Player* player = dynamic_cast<Player*>(backPtr);
            if (player) {
                player->DoWallJump();
            }
            SetAnim(33, 0, 0, 0);
        }
        return;
    }

    if (curAnim == 295) {
        // PSX: anim 295 (0x127) - on completion, sets action state to fall (13, 3)
        Model::HandleRunToLast(anim);
        if (anim->loopCount > 0 && backPtr) {
            Humanoid* humanoid = dynamic_cast<Humanoid*>(backPtr);
            if (humanoid) {
                humanoid->SetActionState(AS_FALL, 3);
            }
        }
        return;
    }

    // Default: just run the base handler
    Model::HandleRunToLast(anim);
}

// PSX: _IncFrame__11PlayerModelP13AnimStructure (MPLAYER.CPP:578)
// Simple trampoline to base Model::HandleIncFrame on PSX (which is a no-op).
void PlayerModel::HandleIncFrame(AnimStructure* anim) {
    Model::HandleIncFrame(anim);
}
