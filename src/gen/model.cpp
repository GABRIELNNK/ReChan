#include "gen/model.h"
#include "gen/animmat.h"
#include "gen/animstruct.h"
#include "gen/charmgr.h"
#include "gen/paramanim.h"
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

static const u8 kStreeMirrorSwapPairs[] = {
    2, 6,
    3, 7,
    4, 8,
    5, 9,
    0, 0,
};

static DrawableSTree* GetDrawableSTree(Model* model) {
    if (!model || model->drawableType != 2 || !model->drawable) {
        return nullptr;
    }
    return static_cast<DrawableSTree*>(model->drawable);
}

static void UpdateFlipMirrorState(Model* model, AnimStructure* anim) {
    if (!anim || !anim->flip) {
        return;
    }

    DrawableSTree* drawable = GetDrawableSTree(model);
    anim->flip->mirrored = drawable && ((drawable->mirrorFlags & 1) != 0);
    anim->flip->mirroredJointOrderMap = drawable ? drawable->mirroredJointOrderMap : nullptr;
}

static TransformAnim* GetTransformAnimForModel(void* animation) {
    if (!animation || IsCameraParamAnim(animation)) {
        return nullptr;
    }

    return static_cast<TransformAnim*>(animation);
}

static OriginalSTree* CloneActiveSTree(const OriginalSTree* source) {
    if (!source || !source->skeleton) {
        return nullptr;
    }

    OriginalSTree* clone = new OriginalSTree();
    clone->nameCRC = source->nameCRC;
    clone->SetStoreID(source->GetStoreID());
    clone->meshVertCount = source->meshVertCount;
    clone->meshTriCount = source->meshTriCount;
    clone->skeleton = CloneSTreeData(source->skeleton);
    if (!clone->skeleton) {
        delete clone;
        return nullptr;
    }

    return clone;
}

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
    if (orig && orig->skeleton && orig->activeTreeInUse) {
        alternate = CloneActiveSTree(orig);
    }

    if (!alternate) {
        alternate = orig;
        if (orig) {
            orig->activeTreeInUse = true;
        }
    }

    mirrorFlags = 0;
    mirroredJointOrderMap = nullptr;
}

DrawableSTree::~DrawableSTree() {
    if (alternate && alternate != original) {
        delete alternate;
    }
    else if (original) {
        original->activeTreeInUse = false;
    }

    original = nullptr;
    alternate = nullptr;
    delete[] mirroredJointOrderMap;
    mirroredJointOrderMap = nullptr;
}

// PSX: Display dispatches through vtable to OriginalSTree::Draw -> tPrimGeom::Display
// PC: draws the skeleton mesh (per-joint transforms baked in) or flat fallback
void DrawableSTree::Display(u32 /*flags*/) {
    OriginalSTree* active = GetActiveSTree(this);
    OriginalSTree* renderSource = original ? original : active;
    if (!active || !renderSource) {
        return;
    }

    STreeData* skel = active->skeleton;
    SkinData* skin = renderSource->skinData;
    STreeData* renderSkeleton = renderSource->skeleton;
    pddiPrimBuffer* skinnedBuffer = (renderSkeleton && renderSkeleton->joints && renderSkeleton->numJoints > 0)
        ? renderSkeleton->joints[0].meshBuffer
        : nullptr;

    // Per-frame CPU skinning
    if (skel && skin && skin->numVerts > 0 && skel->joints && skinnedBuffer) {
        Mat4* jointMatrices = new Mat4[skel->numJoints];
        skel->ComputeWorldMatricesWithCallbacks(jointMatrices);

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

        skinnedBuffer->SetVertexData(vertData.data(), skin->numVerts);
        p3d::context->DrawPrimBuffer(skinnedBuffer);
        delete[] jointMatrices;
        return;
    }

    // Fallback to flat mesh
    STreeData* fallbackSkel = renderSource->skeleton;
    if (fallbackSkel && fallbackSkel->joints && fallbackSkel->joints[0].meshBuffer) {
        p3d::context->DrawPrimBuffer(fallbackSkel->joints[0].meshBuffer);
    }
    else if (renderSource->meshBuffer) {
        p3d::context->DrawPrimBuffer(renderSource->meshBuffer);
    }
}

s32 DrawableSTree::MirrorTree(SModel* model) {
    MARKFUNCTION(0x8007170C);

    OriginalSTree* active = GetActiveSTree(this);
    if (!model || !active || !active->skeleton) {
        return 0;
    }

    STreeData* skeleton = active->skeleton;
    if (skeleton->jointOrderMap && skeleton->numMapEntries > 0 && !mirroredJointOrderMap) {
        mirroredJointOrderMap = new u32[skeleton->numMapEntries];
        for (u32 i = 0; i < skeleton->numMapEntries; i++) {
            mirroredJointOrderMap[i] = skeleton->jointOrderMap[i];
        }

        for (u32 pairIndex = 0; kStreeMirrorSwapPairs[pairIndex] != 0; pairIndex += 2) {
            const u32 lhs = kStreeMirrorSwapPairs[pairIndex];
            const u32 rhs = kStreeMirrorSwapPairs[pairIndex + 1];
            if (lhs < skeleton->numMapEntries && rhs < skeleton->numMapEntries) {
                const u32 temp = mirroredJointOrderMap[lhs];
                mirroredJointOrderMap[lhs] = mirroredJointOrderMap[rhs];
                mirroredJointOrderMap[rhs] = temp;
            }
        }
    }

    mirrorFlags ^= 1u;

    AnimStructure* anim = static_cast<AnimStructure*>(model->animStructure);
    if (anim && anim->animation) {
        model->ApplyAnimToModelBasic(anim->animation);
        if (anim->flip) {
            anim->flip->Reset();
        }
    }

    model->SetupModelCallbacks();
    return 1;
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

    // Build world matrix from position + rotation + scale
    // PSX: TransMatrix, RotMatrixZYXAndLights, ScaleMatrix (from MIPS LST)
    Mat4 world;
    p3dBuildRotMatrixZYX(rotX, rotY, rotZ, world);

    // Scale: PSX FIX16_ONE = 1.0
    f32 s = FIX16_TO_FLOAT(scale);
    world.ScaleRotation(s);

    // Translation
    world.SetTranslation((f32)posX, (f32)posY, (f32)posZ);

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
    if (!g_characterManager) {
        return;
    }

    void* rawAnimation = g_characterManager->GetAnimation((u32)thingType, animEnum);
    if (!rawAnimation) {
        rawAnimation = g_characterManager->GetAnimation(0, animEnum);
        if (!rawAnimation) {
            g_characterManager->LoadAnimationBatch(0, animEnum, nullptr);
            rawAnimation = g_characterManager->GetAnimation(0, animEnum);
        }

        if (!rawAnimation) {
            rawAnimation = g_characterManager->GetAnimation(0, 22);
            animEnum = 22;
        }
    }

    TransformAnim* animation = GetTransformAnimForModel(rawAnimation);
    if (!animation) {
        return;
    }

    if (!animStructure) {
        animStructure = new AnimStructure(0, animation, loopType, this, drawable);
    }

    ApplyAnimToModelBasic(animation);

    AnimStructure* as = (AnimStructure*)animStructure;
    if (!as) {
        return;
    }

    as->animEnum = animEnum;
    if (as->flip) {
        as->flip->Reset();
    }
    as->SetLoopType(loopType, 1);
    if (as->flip) {
        as->ForceFrame(0);
    }
    as->humanoidCB = {};
}

void SModel::ApplyAnimToModelBasic(TransformAnim* animation) {
    MARKFUNCTION(0x8006F068);

    if (!animation) {
        return;
    }

    AnimStructure* anim = static_cast<AnimStructure*>(animStructure);
    if (!anim || anim->mode != 0 || !anim->flip) {
        delete anim;
        anim = new AnimStructure(0, animation, 0, this, drawable);
        animStructure = anim;
    }

    if (!anim || !anim->flip) {
        return;
    }

    anim->animation = animation;
    anim->flip->anim = animation;
    UpdateFlipMirrorState(this, anim);
    anim->flip->dirty = 1;

    anim->startFrame = 0;
    anim->currentFrame = 0;
    anim->endFrame = (animation->numFrames > 0) ? ((animation->numFrames - 1) << 16) : 0;
    anim->prevFrame = 0;
    anim->loopCount = 0;
    anim->speed = FIX16_ONE;
}

// PSX: SetOriginalSTree__6SModelP13OriginalSTreeP10tAnimation (MODEL.CPP:1026, 0x8006EDD4)
void SModel::SetOriginalSTree(OriginalSTree* original) {
    MARKFUNCTION(0x8006EDD4);
    DeleteDrawable();
    drawable = new DrawableSTree(original);
    drawableType = 2; // STree type
    SetupModelCallbacks();
}

// PSX: SetupModelCallbacks__6SModel (MODEL.HPP:1097, 0x80072440)
void SModel::SetupModelCallbacks() {
    MARKFUNCTION(0x80072440);
}

s32 SModel::MirrorTree() {
    MARKFUNCTION(0x8006FAD4);

    DrawableSTree* drawableStree = GetDrawableSTree(this);
    if (!drawableStree) {
        return 0;
    }

    return drawableStree->MirrorTree(this);
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
    field108 = 100;
    field112 = 400;
    field116 = 100;
    field120 = 0;
    headTrackDir = { 0, 0, 0xFFFF };
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

// PSX: SetupModelCallbacks__13HumanoidModel (MHUMAN.CPP:69, 0x8006E114)
void HumanoidModel::SetupModelCallbacks() {
    MARKFUNCTION(0x8006E114);

    if (!animMatrices) {
        return;
    }

    animMatrices->SetupCallbacks(this);
    animMatrices->SetHumanoid(backPtr);
    if (!backPtr) {
        animMatrices->SetupExtraCallbacks(this);
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
