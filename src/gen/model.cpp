#include "gen/model.h"
#include "gen/animmgr.h"
#include "gen/animmat.h"
#include "gen/animstruct.h"
#include "gen/charmgr.h"
#include "gen/envmgr.h"
#include "gen/geometry.h"
#include "gen/lights.h"
#include "gen/paramanim.h"
#include "ai/player.h"
#include "gen/skeleton.h"
#include "p3d/byteread.h"
#include "p3d/context.h"
#include "p3d/p3dmath.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "snd/hmndsnd.h"
#include "pc/log.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>

static const u8 kStreeMirrorSwapPairs[] = {
    2, 6,
    3, 7,
    4, 8,
    5, 9,
    0, 0,
};

static std::vector<const OriginalSTree*> s_liveOriginalSTrees;
static u32 s_streePacketLitTraceBudget = 24;
static u32 s_streeShowLightTraceBudget = 16;
static u32 s_streeSuitTraceBudget = 96;

static void RegisterLiveOriginalSTree(const OriginalSTree* tree) {
    if (!tree) {
        return;
    }

    s_liveOriginalSTrees.push_back(tree);
}

static void UnregisterLiveOriginalSTree(const OriginalSTree* tree) {
    if (!tree) {
        return;
    }

    const auto it = std::find(s_liveOriginalSTrees.begin(), s_liveOriginalSTrees.end(), tree);
    if (it != s_liveOriginalSTrees.end()) {
        s_liveOriginalSTrees.erase(it);
    }
}

static bool IsLiveOriginalSTree(const OriginalSTree* tree) {
    if (!tree) {
        return false;
    }

    return std::find(s_liveOriginalSTrees.begin(), s_liveOriginalSTrees.end(), tree) != s_liveOriginalSTrees.end();
}

static void BuildMat4FromPsxPackedMatrix(const s32* packedMatrix, Mat4& out) {
    out = Mat4();
    if (!packedMatrix) {
        return;
    }

    static constexpr f32 kInvQ12 = 1.0f / 4096.0f;
    const s16* rot = reinterpret_cast<const s16*>(packedMatrix);
    out.m[0] = (f32)rot[0] * kInvQ12;
    out.m[1] = (f32)rot[1] * kInvQ12;
    out.m[2] = (f32)rot[2] * kInvQ12;
    out.m[4] = (f32)rot[3] * kInvQ12;
    out.m[5] = (f32)rot[4] * kInvQ12;
    out.m[6] = (f32)rot[5] * kInvQ12;
    out.m[8] = (f32)rot[6] * kInvQ12;
    out.m[9] = (f32)rot[7] * kInvQ12;
    out.m[10] = (f32)rot[8] * kInvQ12;
}

static bool BuildLitPacketScratch(const OriginalSTree* renderSource, std::vector<u8>* outPackets) {
    if (!renderSource || !outPackets || !renderSource->primGeom || !renderSource->fixUpPolysCallback) {
        return false;
    }

    const tPrimGeom* geom = renderSource->primGeom;
    if (!geom->ownedRawData || !geom->primList || !geom->loopPrimData || !geom->ownedRawSize) {
        return false;
    }

    const u32 primStartOffset = static_cast<u32>(geom->primList - geom->ownedRawData);
    if (primStartOffset >= geom->ownedRawSize) {
        return false;
    }

    const u32 primRegionSize = geom->ownedRawSize - primStartOffset;
    outPackets->assign(geom->primList, geom->primList + primRegionSize);
    u8* cursor = outPackets->data();
    u32 polyIndex = 0;
    for (u32 loopIndex = 0; loopIndex < geom->numLoops; loopIndex++) {
        cursor = renderSource->fixUpPolysCallback(renderSource->primGeom, cursor, loopIndex, polyIndex);
        if (!cursor) {
            return false;
        }

        const u8* loopPrim = geom->loopPrimData + loopIndex * 8u;
        polyIndex += static_cast<u32>(p3dReadU16LE(loopPrim + 0));
        polyIndex += static_cast<u32>(p3dReadU16LE(loopPrim + 2));
        polyIndex += static_cast<u32>(p3dReadU16LE(loopPrim + 4));
        polyIndex += static_cast<u32>(p3dReadU16LE(loopPrim + 6));
    }

    return true;
}

static void ApplyLitPacketColoursToSkin(const SkinData* skin,
                                        const std::vector<u8>& litPackets,
                                        std::vector<f32>* vertData) {
    if (!skin || !vertData || litPackets.empty()) {
        return;
    }

    const u8* cursor = litPackets.data();
    const u8* end = litPackets.data() + litPackets.size();
    for (u32 primIndex = 0; primIndex < skin->numPrims; primIndex++) {
        if (cursor + 8 > end) {
            break;
        }

        const u8 cmdBase = static_cast<u8>(cursor[7] & 0xFCu);
        u32 primitiveSize = 0;
        u32 colourOffsets[4] = {};
        u32 colourCount = 0;

        if (cmdBase == 0x3Cu) {
            primitiveSize = 52u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 16u;
            colourOffsets[2] = 28u;
            colourOffsets[3] = 40u;
            colourCount = 4u;
        }
        else if (cmdBase == 0x2Cu) {
            primitiveSize = 40u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 4u;
            colourOffsets[2] = 4u;
            colourOffsets[3] = 4u;
            colourCount = 4u;
        }
        else if (cmdBase == 0x38u) {
            primitiveSize = 36u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 12u;
            colourOffsets[2] = 20u;
            colourOffsets[3] = 28u;
            colourCount = 4u;
        }
        else if (cmdBase == 0x28u) {
            primitiveSize = 36u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 4u;
            colourOffsets[2] = 4u;
            colourOffsets[3] = 4u;
            colourCount = 4u;
        }
        else if (cmdBase == 0x34u) {
            primitiveSize = 40u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 16u;
            colourOffsets[2] = 28u;
            colourCount = 3u;
        }
        else if (cmdBase == 0x24u) {
            primitiveSize = 32u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 4u;
            colourOffsets[2] = 4u;
            colourCount = 3u;
        }
        else if (cmdBase == 0x30u) {
            primitiveSize = 28u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 12u;
            colourOffsets[2] = 20u;
            colourCount = 3u;
        }
        else if (cmdBase == 0x20u) {
            primitiveSize = 28u;
            colourOffsets[0] = 4u;
            colourOffsets[1] = 4u;
            colourOffsets[2] = 4u;
            colourCount = 3u;
        }
        else {
            break;
        }

        if (cursor + primitiveSize > end) {
            break;
        }

        const u32 base = skin->primStart[primIndex];
        for (u32 colourIndex = 0; colourIndex < colourCount; colourIndex++) {
            const u32 vertexIndex = base + colourIndex;
            if (vertexIndex >= skin->numVerts) {
                continue;
            }

            const u32 offset = colourOffsets[colourIndex];
            (*vertData)[vertexIndex * 10u + 3u] = static_cast<f32>(cursor[offset + 0u]) / 128.0f;
            (*vertData)[vertexIndex * 10u + 4u] = static_cast<f32>(cursor[offset + 1u]) / 128.0f;
            (*vertData)[vertexIndex * 10u + 5u] = static_cast<f32>(cursor[offset + 2u]) / 128.0f;
        }

        cursor += primitiveSize;
    }
}

static DrawableSTree* GetDrawableSTree(Model* model) {
    if (!model || model->drawableType != 2 || !model->drawable) {
        return nullptr;
    }
    return static_cast<DrawableSTree*>(model->drawable);
}

static void SyncFlipTreeWithDrawable(Model* model, AnimStructure* anim) {
    if (!model || !anim || !anim->flip) {
        return;
    }

    DrawableBasic* drawable = model->drawable;
    OriginalSTree* active = GetActiveSTree(drawable);
    STreeData* skeleton = active ? active->skeleton : nullptr;
    if (!skeleton) {
        return;
    }

    if (anim->flip->tree != skeleton) {
        anim->flip->tree = skeleton;
        anim->flip->dirty = 1;
    }
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

static constexpr u32 kSuitEntityTypeCompositeAnim = 0x00010001u;
static constexpr u32 kSuitEntityTypeTexList = 0x00010007u;
static constexpr u32 kSuitEntityTypeClutList = 0x00010008u;

static CompositeAnimData* ResolveCompositeAnimRootForSuit(CompositeAnimData* compositeAnim) {
    CompositeAnimData* resolved = compositeAnim;
    for (u32 depth = 0; depth < 8; depth++) {
        if (!resolved || resolved->numParts != 0 || !g_animMgr) {
            break;
        }

        MiscAnimNode* node = g_animMgr->GetMiscAnim(resolved->nameUID);
        if (!node || !node->compositeAnim || node->compositeAnim == resolved) {
            break;
        }

        resolved = node->compositeAnim;
    }

    return resolved;
}

static u32 ResolveCompositePartEntityTypeForSuit(CompositeAnimPartData* part) {
    if (!part) {
        return 0;
    }

    if (part->compositeAnim) {
        return kSuitEntityTypeCompositeAnim;
    }

    if (part->clutAnim) {
        return kSuitEntityTypeClutList;
    }

    if (part->animNameUID == 0 || !g_animMgr) {
        return 0;
    }

    MiscAnimNode* liveNode = g_animMgr->GetMiscAnim(part->animNameUID);
    if (liveNode && liveNode != part->animNode) {
        part->animNode = liveNode;
    }

    if (!part->animNode) {
        return 0;
    }

    if (part->animNode->compositeAnim) {
        part->compositeAnim = part->animNode->compositeAnim;
        return kSuitEntityTypeCompositeAnim;
    }

    if (part->animNode->clutAnim) {
        part->clutAnim = part->animNode->clutAnim;
        return kSuitEntityTypeClutList;
    }

    return 0;
}

// PSX: RedirectCompositeSuitAnimation__FP14tCompositeAnimPC9tPrimGeom (MODEL.CPP:3521)
static s32 RedirectCompositeSuitAnimation(CompositeAnimData* compositeAnim, const tPrimGeom* primGeom) {
    MARKFUNCTION(0x800721B0);

    compositeAnim = ResolveCompositeAnimRootForSuit(compositeAnim);

    if (!compositeAnim || !compositeAnim->parts) {
        return 0;
    }

    for (u32 partIndex = 0; partIndex < compositeAnim->numParts; partIndex++) {
        CompositeAnimPartData& part = compositeAnim->parts[partIndex];
        const u32 entityType = ResolveCompositePartEntityTypeForSuit(&part);
        if (entityType == kSuitEntityTypeCompositeAnim && part.compositeAnim) {
            RedirectCompositeSuitAnimation(part.compositeAnim, primGeom);
        }
        else if (entityType == kSuitEntityTypeTexList || entityType == kSuitEntityTypeClutList) {
            // PSX writes prim geom targets into tex/clut list objects at redirect time.
            part.targetPrimGeom = primGeom;
        }
    }

    return 0;
}

static s32 ResolveSuitPartFrame(const CompositeAnimPartData* part, s32 frame, s32 frameCount) {
    if (!part || frameCount <= 0) {
        return 0;
    }

    const u32 shift = static_cast<u32>(part->field1) & 31u;
    s32 partFrame = frame >> shift;
    if (partFrame >= frameCount) {
        if (part->field0 != 0) {
            partFrame %= frameCount;
        }
        else {
            partFrame = frameCount - 1;
        }
    }

    return partFrame;
}

static void ApplyClutListSuitFrameToVerts(const ClutAnimData* clutAnim,
                                          s32 frame,
                                          const SkinData* skin,
                                          std::vector<f32>* vertData) {
    if (!clutAnim || !skin || !vertData || !clutAnim->frames || clutAnim->numFrames <= 0) {
        return;
    }

    const f32 clutWord = static_cast<f32>(clutAnim->GetFrameValue(frame));

    if (clutAnim->mode != 0
        && clutAnim->offsets
        && clutAnim->numOffsets > 0
        && skin->primPacketOffset
        && skin->primPacketSize
        && skin->primStart
        && skin->primVertCount) {
        u32 matchedOffsets = 0;
        for (s32 offsetIndex = 0; offsetIndex < clutAnim->numOffsets; offsetIndex++) {
            const u32 packetOffset = static_cast<u32>(clutAnim->offsets[offsetIndex]);
            for (u32 primIndex = 0; primIndex < skin->numPrims; primIndex++) {
                const u32 packetBase = skin->primPacketOffset[primIndex];
                const u32 packetSize = static_cast<u32>(skin->primPacketSize[primIndex]);
                if (packetSize == 0 || packetOffset < packetBase || packetOffset >= (packetBase + packetSize)) {
                    continue;
                }

                const u32 start = skin->primStart[primIndex];
                const u32 count = static_cast<u32>(skin->primVertCount[primIndex]);
                for (u32 corner = 0; corner < count; corner++) {
                    const u32 vertexIndex = start + corner;
                    if (vertexIndex >= skin->numVerts) {
                        break;
                    }

                    (*vertData)[vertexIndex * 10u + 9u] = clutWord;
                }

                matchedOffsets++;
                break;
            }
        }

        if (s_streeSuitTraceBudget > 0 && (frame > 0 || matchedOffsets == 0)) {
            LOG("[SuitClut] mode=%u frame=%d clut=0x%X offsets=%d matched=%u prims=%u verts=%u",
                static_cast<u32>(clutAnim->mode),
                frame,
                static_cast<u32>(clutAnim->GetFrameValue(frame)),
                clutAnim->numOffsets,
                matchedOffsets,
                skin->numPrims,
                skin->numVerts);
            s_streeSuitTraceBudget--;
        }

        return;
    }

    if (s_streeSuitTraceBudget > 0 && frame > 0) {
        LOG("[SuitClut] mode=%u frame=%d clut=0x%X fallback=full-mesh verts=%u",
            static_cast<u32>(clutAnim->mode),
            frame,
            static_cast<u32>(clutAnim->GetFrameValue(frame)),
            skin->numVerts);
        s_streeSuitTraceBudget--;
    }

    for (u32 vertexIndex = 0; vertexIndex < skin->numVerts; vertexIndex++) {
        (*vertData)[vertexIndex * 10u + 9u] = clutWord;
    }
}

static void ApplyCompositeSuitFrameToVerts(CompositeAnimData* compositeAnim,
                                           s32 frame,
                                           const SkinData* skin,
                                           std::vector<f32>* vertData) {
    compositeAnim = ResolveCompositeAnimRootForSuit(compositeAnim);

    if (!compositeAnim || !compositeAnim->parts || !skin || !vertData) {
        return;
    }

    u32 resolvedCompositeParts = 0;
    u32 resolvedClutParts = 0;
    u32 unresolvedParts = 0;

    for (u32 partIndex = 0; partIndex < compositeAnim->numParts; partIndex++) {
        CompositeAnimPartData& part = compositeAnim->parts[partIndex];
        const u32 entityType = ResolveCompositePartEntityTypeForSuit(&part);

        if (entityType == kSuitEntityTypeCompositeAnim && part.compositeAnim) {
            resolvedCompositeParts++;
            const s32 partFrame = ResolveSuitPartFrame(&part, frame, static_cast<s32>(part.compositeAnim->field12));
            ApplyCompositeSuitFrameToVerts(part.compositeAnim, partFrame, skin, vertData);
        }
        else if (entityType == kSuitEntityTypeClutList && part.clutAnim) {
            resolvedClutParts++;
            const s32 partFrame = ResolveSuitPartFrame(&part, frame, part.clutAnim->numFrames);
            ApplyClutListSuitFrameToVerts(part.clutAnim, partFrame, skin, vertData);
        }
        else {
            unresolvedParts++;
        }
    }

    if (s_streeSuitTraceBudget > 0 && (frame > 0 || unresolvedParts != 0 || resolvedClutParts == 0)) {
        LOG("[SuitComposite] frame=%d root=0x%08X parts=%u clut=%u comp=%u unresolved=%u field12=%u",
            frame,
            compositeAnim->nameUID,
            compositeAnim->numParts,
            resolvedClutParts,
            resolvedCompositeParts,
            unresolvedParts,
            compositeAnim->field12);
        s_streeSuitTraceBudget--;
    }
}


static const Mat4* FindJointWorldMatrixByHash(const STreeData* skeleton,
                                              const Mat4* jointMatrices,
                                              u32 jointHash) {
    if (!skeleton || !jointMatrices || !skeleton->joints || jointHash == 0) {
        return nullptr;
    }

    for (u32 jointIndex = 0; jointIndex < skeleton->numJoints; jointIndex++) {
        if (skeleton->joints[jointIndex].nameUID == jointHash) {
            return &jointMatrices[jointIndex];
        }
    }

    return nullptr;
}

static void DrawGeoPartMesh(OriginalGeo* geo) {
    if (!geo || !geo->meshBuffer) {
        return;
    }

    if (geo->usesSemiTrans) {
        pddiBlendMode blendMode = PDDI_BLEND_ALPHA;
        switch (geo->semiTransMode & 3u) {
            case 1: blendMode = PDDI_BLEND_ADD; break;
            case 2: blendMode = PDDI_BLEND_SUBTRACT; break;
            case 3: blendMode = PDDI_BLEND_PSX_QUARTER; break;
            default: break;
        }
        p3d::context->SetBlendMode(blendMode);
    }

    p3d::context->DrawPrimBuffer(geo->meshBuffer);

    if (geo->usesSemiTrans) {
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    }
}

CompositeAnimData::~CompositeAnimData() {
    delete[] parts;
    if (compositeAnims) {
        for (u32 i = 0; i < numCompositeAnims; i++) {
            delete compositeAnims[i];
        }
        delete[] compositeAnims;
    }
    if (clutAnims) {
        for (u32 i = 0; i < numClutAnims; i++) {
            delete clutAnims[i];
        }
        delete[] clutAnims;
    }
}

// OriginalSTree
OriginalSTree::OriginalSTree() {
    SetType(1); // STree type
    RegisterLiveOriginalSTree(this);
}

OriginalSTree::~OriginalSTree() {
    UnregisterLiveOriginalSTree(this);

    if (primGeom) {
        delete primGeom;
        primGeom = nullptr;
    }

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
    if (compositeAnim) {
        delete compositeAnim;
        compositeAnim = nullptr;
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

    delete[] dynamicVerts;
    dynamicVerts = nullptr;
    dynamicVertCount = 0;

    delete[] dynamicPrimStart;
    dynamicPrimStart = nullptr;

    delete[] dynamicPrimVertCount;
    dynamicPrimVertCount = nullptr;

    delete[] dynamicPrimMaterialUID;
    dynamicPrimMaterialUID = nullptr;

    delete[] dynamicPrimCmd;
    dynamicPrimCmd = nullptr;

    delete[] dynamicPrimPacketOffset;
    dynamicPrimPacketOffset = nullptr;

    dynamicPrimCount = 0;
}

OriginalETree::OriginalETree() {
    SetType(2); // ETree type
}

OriginalETree::~OriginalETree() {
    if (meshBuffer) {
        meshBuffer->Release();
        meshBuffer = nullptr;
    }

    if (skeleton) {
        delete skeleton;
        skeleton = nullptr;
    }

    delete[] geoParts;
    geoParts = nullptr;

    delete[] geoPartJointHashes;
    geoPartJointHashes = nullptr;

    delete[] geoPartHashes;
    geoPartHashes = nullptr;

    geoPartCount = 0;
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
    suitIndex = 0;
}

DrawableSTree::~DrawableSTree() {
    if (alternate && alternate != original) {
        delete alternate;
    }
    else if (original && IsLiveOriginalSTree(original)) {
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

    auto resolveSemiTransBlendMode = [](u8 semiTransMode) -> pddiBlendMode {
        switch (semiTransMode & 3u) {
            case 1: return PDDI_BLEND_ADD;
            case 2: return PDDI_BLEND_SUBTRACT;
            case 3: return PDDI_BLEND_PSX_QUARTER;
            default: return PDDI_BLEND_ALPHA;
        }
    };

    auto applySkinBlendState = [&](const SkinData* skinData) -> bool {
        if (!skinData || !skinData->usesSemiTrans) {
            return false;
        }

        p3d::context->SetBlendMode(resolveSemiTransBlendMode(skinData->semiTransMode));
        return true;
    };

    // Per-frame CPU skinning
    if (skel && skin && skin->numVerts > 0 && skel->joints && skinnedBuffer) {
        Mat4* jointMatrices = new Mat4[skel->numJoints];
        skel->ComputeWorldMatricesWithCallbacks(jointMatrices);
        const Mat4 modelWorld = p3d::context->GetWorldMatrix();

        std::vector<u16> litScratch;
        std::vector<u8> litWritten;
        std::vector<u8> litPackets;
        bool hasLitScratch = false;
        bool hasLitPackets = false;
        u8 scratchMinR = 255;
        u8 scratchMinG = 255;
        u8 scratchMinB = 255;
        u8 scratchMaxR = 0;
        u8 scratchMaxG = 0;
        u8 scratchMaxB = 0;
        bool haveScratchRange = false;
        u8 normalIndexMin = 255;
        u8 normalIndexMax = 0;
        bool haveNormalIndexRange = false;
        if (renderSource->xformVertsCallback && renderSource->primGeom && renderSource->primGeom->numVerts > 0) {
            const u32 sourceVertCount = renderSource->primGeom->numVerts;
            const u8* sourceVertexList = renderSource->primGeom->GetVertexList();
            litScratch.resize(sourceVertCount * 2u, 0u);
            litWritten.resize(sourceVertCount, 0u);

            for (u32 jointIndex = 0; jointIndex < skel->numJoints; jointIndex++) {
                STreeJoint* joint = &skel->joints[jointIndex];
                if (joint->primGeomCount == 0) {
                    continue;
                }

                const u32 startIndex = static_cast<u32>(joint->primGeomStartIdx);
                const u32 endIndex = startIndex + static_cast<u32>(joint->primGeomCount);
                if (startIndex >= sourceVertCount) {
                    continue;
                }
                const u32 clampedEnd = (endIndex > sourceVertCount) ? sourceVertCount : endIndex;
                if (clampedEnd <= startIndex) {
                    continue;
                }
                for (u32 sourceIndex = startIndex; sourceIndex < clampedEnd; sourceIndex++) {
                    litWritten[sourceIndex] = 1u;
                    if (sourceVertexList) {
                        const u8 normalIndex = sourceVertexList[sourceIndex * 8u + 6u];
                        if (!haveNormalIndexRange) {
                            normalIndexMin = normalIndex;
                            normalIndexMax = normalIndex;
                            haveNormalIndexRange = true;
                        }
                        else {
                            if (normalIndex < normalIndexMin) normalIndexMin = normalIndex;
                            if (normalIndex > normalIndexMax) normalIndexMax = normalIndex;
                        }
                    }
                }

                STreeJoint clampedJoint = *joint;
                clampedJoint.primGeomStartIdx = static_cast<u16>(startIndex);
                clampedJoint.primGeomCount = static_cast<u16>(clampedEnd - startIndex);
                Mat4 lightingMatrix = modelWorld * jointMatrices[jointIndex];
                renderSource->xformVertsCallback(renderSource->primGeom,
                                                 &clampedJoint,
                                                 reinterpret_cast<u32*>(&lightingMatrix),
                                                 litScratch.data());
            }

            hasLitScratch = true;
            for (u32 sourceIndex = 0; sourceIndex < sourceVertCount; sourceIndex++) {
                if (litWritten[sourceIndex] == 0u) {
                    continue;
                }

                const u16 rg = litScratch[sourceIndex * 2u + 0u];
                const u16 b = litScratch[sourceIndex * 2u + 1u];
                const u8 litR = static_cast<u8>(rg & 0xFFu);
                const u8 litG = static_cast<u8>((rg >> 8) & 0xFFu);
                const u8 litB = static_cast<u8>(b & 0xFFu);

                if (!haveScratchRange) {
                    scratchMinR = litR;
                    scratchMinG = litG;
                    scratchMinB = litB;
                    scratchMaxR = litR;
                    scratchMaxG = litG;
                    scratchMaxB = litB;
                    haveScratchRange = true;
                }
                else {
                    if (litR < scratchMinR) scratchMinR = litR;
                    if (litG < scratchMinG) scratchMinG = litG;
                    if (litB < scratchMinB) scratchMinB = litB;
                    if (litR > scratchMaxR) scratchMaxR = litR;
                    if (litG > scratchMaxG) scratchMaxG = litG;
                    if (litB > scratchMaxB) scratchMaxB = litB;
                }
            }

            hasLitPackets = BuildLitPacketScratch(renderSource, &litPackets);
        }

        std::vector<f32> vertData(skin->numVerts * 10);
        for (u32 i = 0; i < skin->numVerts; i++) {
            const SkinVertex& sv = skin->verts[i];
            const Mat4& m = jointMatrices[sv.jointIdx];
            f32 wx, wy, wz;
            Mat4TransformPoint(m, sv.lx, sv.ly, sv.lz, wx, wy, wz);

            f32 litR = sv.r;
            f32 litG = sv.g;
            f32 litB = sv.b;
            if (hasLitScratch && renderSource->primGeom && sv.sourceIndex < renderSource->primGeom->numVerts
                && litWritten[sv.sourceIndex] != 0u) {
                const u32 sourceIndex = static_cast<u32>(sv.sourceIndex);
                const u16 rg = litScratch[sourceIndex * 2u + 0u];
                const u16 b = litScratch[sourceIndex * 2u + 1u];
                litR = static_cast<f32>(rg & 0xFFu) / 128.0f;
                litG = static_cast<f32>((rg >> 8) & 0xFFu) / 128.0f;
                litB = static_cast<f32>(b & 0xFFu) / 128.0f;
            }

            vertData[i * 10 + 0] = wx;
            vertData[i * 10 + 1] = wy;
            vertData[i * 10 + 2] = wz;
            vertData[i * 10 + 3] = litR;
            vertData[i * 10 + 4] = litG;
            vertData[i * 10 + 5] = litB;
            vertData[i * 10 + 6] = sv.u;
            vertData[i * 10 + 7] = sv.v;
            vertData[i * 10 + 8] = sv.tpage;
            vertData[i * 10 + 9] = sv.cba;
        }

        ApplyCompositeSuitFrameToVerts(renderSource ? renderSource->compositeAnim : nullptr,
                           static_cast<s32>(suitIndex),
                           skin,
                           &vertData);

        if (hasLitPackets) {
            ApplyLitPacketColoursToSkin(skin, litPackets, &vertData);
            if (s_streePacketLitTraceBudget > 0) {
                LOG("[STreeLit] packet fixup applied: verts=%u prims=%u packetBytes=%u scratchRange=R[%u..%u] G[%u..%u] B[%u..%u] normalIdx[%u..%u]",
                    skin->numVerts,
                    skin->numPrims,
                    static_cast<u32>(litPackets.size()),
                    haveScratchRange ? static_cast<u32>(scratchMinR) : 0u,
                    haveScratchRange ? static_cast<u32>(scratchMaxR) : 0u,
                    haveScratchRange ? static_cast<u32>(scratchMinG) : 0u,
                    haveScratchRange ? static_cast<u32>(scratchMaxG) : 0u,
                    haveScratchRange ? static_cast<u32>(scratchMinB) : 0u,
                    haveScratchRange ? static_cast<u32>(scratchMaxB) : 0u,
                    haveNormalIndexRange ? static_cast<u32>(normalIndexMin) : 0u,
                    haveNormalIndexRange ? static_cast<u32>(normalIndexMax) : 0u);
                s_streePacketLitTraceBudget--;
            }
        }
        else if (hasLitScratch && s_streePacketLitTraceBudget > 0) {
            LOG("[STreeLit] fallback vertex scratch only: verts=%u prims=%u scratchRange=R[%u..%u] G[%u..%u] B[%u..%u] normalIdx[%u..%u]",
                skin->numVerts,
                skin->numPrims,
                haveScratchRange ? static_cast<u32>(scratchMinR) : 0u,
                haveScratchRange ? static_cast<u32>(scratchMaxR) : 0u,
                haveScratchRange ? static_cast<u32>(scratchMinG) : 0u,
                haveScratchRange ? static_cast<u32>(scratchMaxG) : 0u,
                haveScratchRange ? static_cast<u32>(scratchMinB) : 0u,
                haveScratchRange ? static_cast<u32>(scratchMaxB) : 0u,
                haveNormalIndexRange ? static_cast<u32>(normalIndexMin) : 0u,
                haveNormalIndexRange ? static_cast<u32>(normalIndexMax) : 0u);
            s_streePacketLitTraceBudget--;
        }

        skinnedBuffer->SetVertexData(vertData.data(), skin->numVerts);
        const bool skinBlendApplied = applySkinBlendState(skin);
        p3d::context->DrawPrimBuffer(skinnedBuffer);
        if (skinBlendApplied) {
            p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        }
        delete[] jointMatrices;
        return;
    }

    // Fallback to flat mesh
    STreeData* fallbackSkel = renderSource->skeleton;
    if (fallbackSkel && fallbackSkel->joints && fallbackSkel->joints[0].meshBuffer) {
        const bool skinBlendApplied = applySkinBlendState(renderSource->skinData);
        p3d::context->DrawPrimBuffer(fallbackSkel->joints[0].meshBuffer);
        if (skinBlendApplied) {
            p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        }
    }
    else if (renderSource->meshBuffer) {
        const bool skinBlendApplied = applySkinBlendState(renderSource->skinData);
        p3d::context->DrawPrimBuffer(renderSource->meshBuffer);
        if (skinBlendApplied) {
            p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        }
    }
}

// PSX: ChangeSuit__13OriginalSTreeP13DrawableSTrees (MODEL.CPP:3588)
s32 OriginalSTree::ChangeSuit(DrawableSTree* drawable, s16 suitIndexValue) {
    MARKFUNCTION(0x800722A8);

    if (!drawable || !compositeAnim) {
        return 0;
    }

    OriginalSTree* target = drawable->GetAlternateSTree();
    if (!target) {
        target = drawable->GetOriginalSTree();
    }
    RedirectCompositeSuitAnimation(compositeAnim, target ? target->primGeom : nullptr);

    // PSX calls tCompositeFlip::SetFrame(suit) then Update().
    // Host stores suit frame on drawable and applies during skinning.
    drawable->suitIndex = suitIndexValue;

    if (s_streeSuitTraceBudget > 0) {
        LOG("[SuitChange] drawable=%p suit=%d comp=0x%08X parts=%u field12=%u skin=%p",
            drawable,
            static_cast<s32>(suitIndexValue),
            compositeAnim->nameUID,
            compositeAnim->numParts,
            compositeAnim->field12,
            skinData);
        s_streeSuitTraceBudget--;
    }

    return RedirectCompositeSuitAnimation(compositeAnim, primGeom);
}

// PSX: ChangeSuit__13DrawableSTrees (MODEL.CPP:3630)
s32 DrawableSTree::ChangeSuit(s16 suitIndexValue) {
    MARKFUNCTION(0x80072350);

    if (!original) {
        return 0;
    }

    return original->ChangeSuit(this, suitIndexValue);
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
    if (!original || !original->meshBuffer) {
        return;
    }

    DrawGeoPartMesh(original);
}

DrawableETree::DrawableETree(OriginalETree* orig) {
    original = orig;
    ownerModel = nullptr;
    if (original && original->geoPartCount > 0) {
        geoPartVisible = new u8[original->geoPartCount];
        if (geoPartVisible) {
            for (u16 i = 0; i < original->geoPartCount; i++) {
                geoPartVisible[i] = 1;
            }
        }
    }
}

DrawableETree::~DrawableETree() {
    delete[] geoPartVisible;
    geoPartVisible = nullptr;
    ownerModel = nullptr;
    original = nullptr;
}

void DrawableETree::Display(u32 /*flags*/) {
    if (!original) {
        return;
    }

    if (original->geoParts && original->geoPartHashes && original->geoPartCount > 0) {
        const Mat4 baseWorld = p3d::context->GetWorldMatrix();

        STreeData* skeleton = nullptr;
        bool invokeCallbacks = false;
        std::vector<Mat4> partJointMatrices;
        if (ownerModel && ownerModel->animStructure && original->geoPartJointHashes) {
            AnimStructure* anim = static_cast<AnimStructure*>(ownerModel->animStructure);
            TransformFlip* flip = anim ? anim->GetFlip() : nullptr;
            if (flip && flip->tree && flip->tree->joints && flip->tree->numJoints > 0) {
                skeleton = flip->tree;
                invokeCallbacks = true;
            }
        }

        if (!skeleton && original->skeleton && original->skeleton->joints && original->skeleton->numJoints > 0) {
            skeleton = original->skeleton;
        }

        if (skeleton) {
            partJointMatrices.resize(skeleton->numJoints);
            if (invokeCallbacks) {
                skeleton->ComputeWorldMatricesWithCallbacks(partJointMatrices.data());
            }
            else {
                skeleton->ComputeWorldMatrices(partJointMatrices.data());
            }
        }

        for (u16 i = 0; i < original->geoPartCount; i++) {
            if (geoPartVisible && geoPartVisible[i] == 0) {
                continue;
            }

            OriginalGeo* geo = original->geoParts[i];
            if (!geo || !geo->meshBuffer) {
                continue;
            }

            const Mat4* partMatrix = nullptr;
            if (skeleton && partJointMatrices.data() && original->geoPartJointHashes) {
                partMatrix = FindJointWorldMatrixByHash(skeleton,
                                                        partJointMatrices.data(),
                                                        original->geoPartJointHashes[i]);
            }

            if (partMatrix) {
                Mat4 partWorld = baseWorld * (*partMatrix);
                p3d::context->SetWorldMatrix(partWorld);
            }
            else {
                p3d::context->SetWorldMatrix(baseWorld);
            }

            DrawGeoPartMesh(geo);
        }

        p3d::context->SetWorldMatrix(baseWorld);
        return;
    }

    if (original->meshBuffer) {
        if (original->usesSemiTrans) {
            pddiBlendMode blendMode = PDDI_BLEND_ALPHA;
            switch (original->semiTransMode & 3u) {
                case 1: blendMode = PDDI_BLEND_ADD; break;
                case 2: blendMode = PDDI_BLEND_SUBTRACT; break;
                case 3: blendMode = PDDI_BLEND_PSX_QUARTER; break;
                default: break;
            }
            p3d::context->SetBlendMode(blendMode);
        }

        p3d::context->DrawPrimBuffer(original->meshBuffer);

        if (original->usesSemiTrans) {
            p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        }
    }
}

void DrawableETree::SetGeoPartVisibleByHash(u32 hash, bool visible) {
    if (!original || !original->geoPartHashes || original->geoPartCount == 0 || !geoPartVisible) {
        return;
    }

    for (u16 i = 0; i < original->geoPartCount; i++) {
        if (original->geoPartHashes[i] == hash) {
            geoPartVisible[i] = visible ? 1 : 0;
        }
    }
}

void DrawableETree::ResetGeoPartVisibility() {
    if (!original || !geoPartVisible) {
        return;
    }

    for (u16 i = 0; i < original->geoPartCount; i++) {
        geoPartVisible[i] = 1;
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

    DeleteAmbientLight();
    DeleteHardwareLights();

    if (field36) {
        delete static_cast<ModelFloorHeightState*>(field36);
        field36 = nullptr;
    }

    DeleteAnimStructures();
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
void Model::HandleHoldFrame(AnimStructure* /*anim*/) { /* PSX: no-op */ }
void Model::HandleRunToFrame(AnimStructure* /*anim*/) { /* PSX: no-op */ }
void Model::HandleIncFrame(AnimStructure* /*anim*/) { /* PSX: no-op */ }
void Model::HandleDecFrame(AnimStructure* anim) { anim->DecFrame(); }
void Model::HandleLoopDesired(AnimStructure* /*anim*/) { /* PSX: no-op */ }
void Model::HandleRunToLastBlend(AnimStructure* anim) { anim->RunToLastBlend(); }

// PSX: DeleteAnimStructures__5Model (MODEL.CPP:768, 0x8006E83C)
void Model::DeleteAnimStructures() {
    MARKFUNCTION(0x8006E83C);

    if (!animStructure) {
        return;
    }

    delete static_cast<AnimStructure*>(animStructure);
    animStructure = nullptr;
}

// PSX: AllocateAmbientLight__5Model (MODEL.CPP:2024, 0x8007013C)
AmbientLight* Model::AllocateAmbientLight() {
    MARKFUNCTION(0x8007013C);

    DeleteAmbientLight();
    ambientLight = new AmbientLight();
    return static_cast<AmbientLight*>(ambientLight);
}

// PSX: DeleteAmbientLight__5Model (MODEL.CPP:2030, 0x80070170)
void Model::DeleteAmbientLight() {
    MARKFUNCTION(0x80070170);

    if (!ambientLight) {
        return;
    }

    delete static_cast<AmbientLight*>(ambientLight);
    ambientLight = nullptr;
}

// PSX: AllocateHardwareLights__5ModelUl (MODEL.CPP:2039, 0x800701BC)
HardwareLight* Model::AllocateHardwareLights(u32 count) {
    MARKFUNCTION(0x800701BC);

    DeleteHardwareLights();

    if (count == 0) {
        return nullptr;
    }

    HardwareLight* lights = new HardwareLight[count]();
    hwLights = lights;
    hwLightCount = static_cast<s32>(count);
    return lights;
}

// PSX: DeleteHardwareLights__5Model (MODEL.CPP:2046, 0x80070254)
void Model::DeleteHardwareLights() {
    MARKFUNCTION(0x80070254);

    if (!hwLights) {
        hwLightCount = 0;
        return;
    }

    delete[] static_cast<HardwareLight*>(hwLights);
    hwLights = nullptr;
    hwLightCount = 0;
}

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

    if (g_environmentManager) {
        g_environmentManager->lighting.DoModelLighting(backPtr);
    }

    if (s_streeShowLightTraceBudget > 0 && drawableType == 2) {
        const HardwareLight* modelLightsProbe = static_cast<const HardwareLight*>(hwLights);
        const HardwareLight* portLight0 = GetCurrentPortHardwareLight(0);
        LOG("[SModelLight] model=%p ownerModel=%p hwCount=%d m0(active=%u color=0x%06X dir=(%d,%d,%d)) port0(color=0x%06X dir=(%d,%d,%d))",
            this,
            backPtr ? backPtr->model : nullptr,
            hwLightCount,
            (modelLightsProbe && hwLightCount > 0) ? static_cast<u32>(modelLightsProbe[0].active) : 0u,
            (modelLightsProbe && hwLightCount > 0) ? modelLightsProbe[0].colour : 0u,
            (modelLightsProbe && hwLightCount > 0) ? modelLightsProbe[0].directionX : 0,
            (modelLightsProbe && hwLightCount > 0) ? modelLightsProbe[0].directionY : 0,
            (modelLightsProbe && hwLightCount > 0) ? modelLightsProbe[0].directionZ : 0,
            portLight0 ? portLight0->colour : 0u,
            portLight0 ? portLight0->directionX : 0,
            portLight0 ? portLight0->directionY : 0,
            portLight0 ? portLight0->directionZ : 0);
    }

    bool addedHwLights[3] = { false, false, false };
    if (g_environmentManager && hwLights && hwLightCount > 0) {
        HardwareLight* modelLights = static_cast<HardwareLight*>(hwLights);
        const s32 lightCount = (hwLightCount < 3) ? hwLightCount : 3;

        for (s32 slot = 0; slot < lightCount; slot++) {
            if (!modelLights[slot].active) {
                continue;
            }

            const LVector lightDir = {
                modelLights[slot].directionX,
                modelLights[slot].directionY,
                modelLights[slot].directionZ,
            };
            g_environmentManager->lighting.AddLightToPort(slot, &lightDir, modelLights[slot].colour);
            addedHwLights[slot] = true;
        }
    }

    if (s_streeShowLightTraceBudget > 0 && drawableType == 2) {
        const HardwareLight* portLight0 = GetCurrentPortHardwareLight(0);
        LOG("[SModelLight] after AddLightToPort: port0(color=0x%06X dir=(%d,%d,%d))",
            portLight0 ? portLight0->colour : 0u,
            portLight0 ? portLight0->directionX : 0,
            portLight0 ? portLight0->directionY : 0,
            portLight0 ? portLight0->directionZ : 0);
        s_streeShowLightTraceBudget--;
    }

    if (ambientLight) {
        static_cast<AmbientLight*>(ambientLight)->SetPortToLight();
    }

    const u32 ownerFlags = backPtr->flags;
    const bool ownerDeadWindow = (ownerFlags & 0x80u) != 0;
    const bool ownerSemiFlag = (ownerFlags & 0x100u) != 0;
    const bool useSemiTrans = ownerDeadWindow && ownerSemiFlag;

    // PSX: if owner is no longer in death draw state, clear the transient semi bit.
    if (!ownerDeadWindow && ownerSemiFlag) {
        backPtr->flags &= ~0x100u;
    }

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

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    p3d::context->SetWorldMatrix(world);

    if (useSemiTrans) {
        p3d::context->SetBlendMode(PDDI_BLEND_ALPHA);
    }

    // PSX: calls drawable->Display(flags) through vtable
    drawable->Display(flags);

    if (useSemiTrans) {
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    }

    if (ambientLight) {
        RestoreWorldAmbientLightToPort();
    }

    if (g_environmentManager) {
        for (s32 slot = 0; slot < 3; slot++) {
            if (addedHwLights[slot]) {
                g_environmentManager->lighting.RemoveLightFromPort(slot);
            }
        }
    }

    p3d::context->SetWorldMatrix(savedWorld);
}

// PSX: Animate__6SModel (MODEL.CPP:1416, 0x8006F640)
void SModel::Animate() {
    MARKFUNCTION(0x8006F640);
    if (animStructure) {
        AnimStructure* anim = (AnimStructure*)animStructure;
        anim->ExecuteHandler(1);
    }
}

// PSX: InitBlendPose__6SModel (MODEL.CPP:1328, 0x8006F438)
BlendPoseState* SModel::InitBlendPose() {
    MARKFUNCTION(0x8006F438);

    AnimStructure* anim = static_cast<AnimStructure*>(animStructure);
    if (!anim || !anim->flip || !anim->flip->tree) {
        return nullptr;
    }

    const u32 jointCount = anim->flip->tree->numJoints;

    if (!anim->blendPose) {
        anim->blendPose = new BlendPoseState();
    }

    if (!anim->blendPose) {
        return nullptr;
    }

    if (anim->blendPose->jointCount != jointCount) {
        delete[] anim->blendPose->joints;
        anim->blendPose->joints = nullptr;
        anim->blendPose->jointCount = jointCount;

        if (jointCount > 0) {
            anim->blendPose->joints = new BlendJointPose[jointCount];
        }
    }

    return anim->blendPose;
}

// PSX: ApplyBlending__6SModelP10tAnimationll (MODEL.CPP:1346, 0x8006F4A0)
AnimStructure* SModel::ApplyBlending(TransformAnim* animation, s32 blendFrames, s32 startFrame) {
    MARKFUNCTION(0x8006F4A0);

    AnimStructure* anim = static_cast<AnimStructure*>(animStructure);
    if (!animation || !anim || anim->mode != 0 || !anim->flip || !anim->flip->tree) {
        return nullptr;
    }

    BlendPoseState* blendPose = InitBlendPose();
    if (!blendPose || !blendPose->joints) {
        return nullptr;
    }

    STreeData* skeleton = anim->flip->tree;
    for (u32 jointIndex = 0; jointIndex < blendPose->jointCount; jointIndex++) {
        const STreeJoint& joint = skeleton->joints[jointIndex];
        BlendJointPose& pose = blendPose->joints[jointIndex];
        pose.translationX = joint.translationX;
        pose.translationY = joint.translationY;
        pose.translationZ = joint.translationZ;
        pose.rotationX = joint.rotationX;
        pose.rotationY = joint.rotationY;
        pose.rotationZ = joint.rotationZ;
    }

    ApplyAnimToModelBasic(animation);

    anim = static_cast<AnimStructure*>(animStructure);
    if (!anim) {
        return nullptr;
    }

    anim->endFrame = blendFrames << 16;
    anim->startFrame = 0;
    anim->currentFrame = 0;
    anim->prevFrame = 0;
    anim->loopCount = 0;
    anim->SetLoopType(ANIM_BLEND2, 0);
    anim->field56 = startFrame;
    return anim;
}

// PSX: IsAnimationLoaded__6SModell (MODEL.CPP:1062, 0x8006EE4C)
s32 SModel::IsAnimationLoaded(s32 animEnum) {
    MARKFUNCTION(0x8006EE4C);

    if (!g_characterManager) {
        return 0;
    }

    const u32 thingType = backPtr ? static_cast<u32>(backPtr->thingType) : 0;
    if (g_characterManager->GetAnimation(thingType, animEnum)) {
        return 1;
    }

    return g_characterManager->GetAnimation(0, animEnum) ? 1 : 0;
}

// PSX: ApplyAnimToModel__6SModellllll (MODEL.CPP:1098, 0x8006EEAC)
void SModel::ApplyAnimToModel(s32 thingType, s32 animEnum, s32 loopType, s32 p4, s32 p5) {
    MARKFUNCTION(0x8006EEAC);
    if (!g_characterManager) {
        return;
    }

    void* rawAnimation = g_characterManager->GetAnimation((u32)thingType, animEnum);
    if (!rawAnimation) {
        rawAnimation = g_characterManager->GetAnimation(0, animEnum);
        if (!rawAnimation) {
            LOG("[ApplyAnim] FALLBACK animEnum=%d not found, using idle(22)", animEnum);
            rawAnimation = g_characterManager->GetAnimation(0, 22);
            animEnum = 22;
        }
    }

    TransformAnim* animation = GetTransformAnimForModel(rawAnimation);
    if (!animation) {
        LOG("[ApplyAnimToModel] GetTransformAnimForModel NULL for type=%d animEnum=%d (CameraParamAnim or null)", thingType, animEnum);
        return;
    }

    if (!animStructure) {
        animStructure = new AnimStructure(0, animation, loopType, this, drawable);
    }

    AnimStructure* as = (AnimStructure*)animStructure;
    if (!as) {
        return;
    }

    as->animEnum = animEnum;

    if (p4 != 0) {
        as = ApplyBlending(animation, p4, p5 << 16);
        if (as && as->loopTypeField == ANIM_BLEND2) {
            as->animEnum = animEnum;
            if (as->blendPose) {
                as->blendPose->loopType = loopType;
            }
            as->humanoidCB = {};
            return;
        }
    }

    ApplyAnimToModelBasic(animation);

    as = (AnimStructure*)animStructure;
    if (!as) {
        return;
    }

    as->animEnum = animEnum;
    if (as->flip) {
        as->flip->Reset();
    }
    as->SetLoopType(loopType, 1);
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
    SyncFlipTreeWithDrawable(this, anim);
    anim->flip->anim = animation;
    UpdateFlipMirrorState(this, anim);
    anim->flip->dirty = 1;

    anim->startFrame = 0;
    anim->currentFrame = 0;
    anim->endFrame = (animation->numFrames > 0) ? ((animation->numFrames - 1) << 16) : 0;
    anim->prevFrame = 0;
    anim->loopCount = 0;
    anim->speed = FIX16_ONE;

    if (anim->animEnum == 361 || animation->numFrames <= 1) {
        LOG("[ApplyAnimBasic] animEnum=%d numFrames=%d endFrame=0x%X loopCount=%d",
            anim->animEnum, animation->numFrames, anim->endFrame, anim->loopCount);
    }
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

    std::memset(attachedMatrix, 0, sizeof(attachedMatrix));
    s16* rot = reinterpret_cast<s16*>(attachedMatrix);
    rot[0] = 0x1000;
    rot[4] = 0x1000;
    rot[8] = 0x1000;
    attachedMatrixActive = 0;
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

    if (g_environmentManager) {
        const u16 thingType = backPtr->thingType;
        const bool inGatedTypeRange = (thingType >= 400u && thingType < 474u);
        const bool forceLighting = (*(reinterpret_cast<const u8*>(backPtr) + 113u) != 0u);
        if (!inGatedTypeRange || forceLighting) {
            g_environmentManager->lighting.DoModelLighting(backPtr);
        }
    }

    bool addedHwLights[3] = { false, false, false };
    if (g_environmentManager && hwLights && hwLightCount > 0) {
        HardwareLight* modelLights = static_cast<HardwareLight*>(hwLights);
        const s32 lightCount = (hwLightCount < 3) ? hwLightCount : 3;

        for (s32 slot = 0; slot < lightCount; slot++) {
            if (!modelLights[slot].active) {
                continue;
            }

            const LVector lightDir = {
                modelLights[slot].directionX,
                modelLights[slot].directionY,
                modelLights[slot].directionZ,
            };
            g_environmentManager->lighting.AddLightToPort(slot, &lightDir, modelLights[slot].colour);
            addedHwLights[slot] = true;
        }
    }

    if (ambientLight) {
        static_cast<AmbientLight*>(ambientLight)->SetPortToLight();
    }

    modelFlags |= 0x50;

    Mat4 world;
    if ((modelFlags & 1u) != 0 && attachedMatrixActive != 0) {
        BuildMat4FromPsxPackedMatrix(attachedMatrix, world);
    }
    else {
        p3dBuildRotMatrixZYX(rotX, rotY, rotZ, world);
    }
    world.SetTranslation((f32)posX, (f32)posY, (f32)posZ);

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    p3d::context->SetWorldMatrix(world);
    drawable->Display(flags);

    if (ambientLight) {
        RestoreWorldAmbientLightToPort();
    }

    if (g_environmentManager) {
        for (s32 slot = 0; slot < 3; slot++) {
            if (addedHwLights[slot]) {
                g_environmentManager->lighting.RemoveLightFromPort(slot);
            }
        }
    }

    p3d::context->SetWorldMatrix(savedWorld);
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
    DrawableETree* drawableETree = new DrawableETree(original);
    drawableETree->SetOwnerModel(this);
    drawable = drawableETree;
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

    ApplyAnimToModel(anim, loopType);
}

AnimStructure* EModel::ApplyAnimToModel(TransformAnim* animation, s32 loopType) {
    MARKFUNCTION(0x8006FCAC);

    AnimStructure* animStruct = new AnimStructure(2, animation, loopType, this, drawable);
    animStructure = animStruct;
    return animStruct;
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

    if (g_environmentManager) {
        g_environmentManager->lighting.DoModelLighting(backPtr);
    }

    bool addedHwLights[3] = { false, false, false };
    if (g_environmentManager && hwLights && hwLightCount > 0) {
        HardwareLight* modelLights = static_cast<HardwareLight*>(hwLights);
        const s32 lightCount = (hwLightCount < 3) ? hwLightCount : 3;

        for (s32 slot = 0; slot < lightCount; slot++) {
            if (!modelLights[slot].active) {
                continue;
            }

            const LVector lightDir = {
                modelLights[slot].directionX,
                modelLights[slot].directionY,
                modelLights[slot].directionZ,
            };
            g_environmentManager->lighting.AddLightToPort(slot, &lightDir, modelLights[slot].colour);
            addedHwLights[slot] = true;
        }
    }

    if (ambientLight) {
        static_cast<AmbientLight*>(ambientLight)->SetPortToLight();
    }

    modelFlags |= 0x50;

    Mat4 world;
    p3dBuildRotMatrixZYX(rotX, rotY, rotZ, world);
    world.SetTranslation((f32)posX, (f32)posY, (f32)posZ);

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    p3d::context->SetWorldMatrix(world);
    drawable->Display(flags);

    if (ambientLight) {
        RestoreWorldAmbientLightToPort();
    }

    if (g_environmentManager) {
        for (s32 slot = 0; slot < 3; slot++) {
            if (addedHwLights[slot]) {
                g_environmentManager->lighting.RemoveLightFromPort(slot);
            }
        }
    }

    p3d::context->SetWorldMatrix(savedWorld);
}

// HumanoidModel

// PSX: SetAnim__13HumanoidModelllil (MHUMAN.CPP:119, 0x8006E1B0)
// Routes anims to ApplyAnimToModel. Transition anims (37-38) get
// special blend-from-current-frame handling. High enum anims default to
// RunToLast unless explicitly routed to Loop/HoldFirst by the PSX tree.
void HumanoidModel::SetAnim(s32 animEnum, s32 a3, s32 force, s32 extra) {
    MARKFUNCTION(0x8006E1B0);

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
    AllocateHardwareLights(3);
    AllocateAmbientLight();

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
    if (backPtr == Player::s_player) {
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
