#include "common.h"

#include "gen/weffect.h"

#include "gen/animmgr.h"
#include "gen/animstruct.h"
#include "gen/ai.h"
#include "gen/blockmgr.h"
#include "gen/camera.h"
#include "gen/database.h"
#include "gen/display.h"
#include "gen/geffect.h"
#include "gen/levelmgr.h"
#include "gen/model.h"
#include "gen/paldata.h"
#include "gen/path.h"
#include "gen/pweffect.h"
#include "gen/scaledata.h"
#include "gen/uvdata.h"

#include "ai/activezn.h"
#include "ai/platform.h"
#include "ai/player.h"
#include "ai/thing.h"

#include "p3d/context.h"
#include "p3d/p3dmath.h"
#include "p3d/skeleton.h"

#include "snd/esound.h"
#include "snd/sndfact.h"

static ComEffect* g_wEffectComEffects[64] = {};
static ComEffect* g_wEffectOwnedEffects[64] = {};
static s32 g_wEffectComEffectCount = 0;
static s32 g_wEffectOwnedCount = 0;

static ccList g_wEffectPool;

#define CountRenderableETreeGeos(originalPtr)                                                       \
    ([&](const OriginalETree* __etOriginal) -> u32 {                                                \
        if (!__etOriginal) {                                                                        \
            return 0;                                                                               \
        }                                                                                           \
        if (__etOriginal->geoParts && __etOriginal->geoPartCount > 0) {                            \
            u32 _count = 0;                                                                         \
            for (u16 _i = 0; _i < __etOriginal->geoPartCount; _i++) {                              \
                const OriginalGeo* _geo = __etOriginal->geoParts[_i];                              \
                if (_geo && _geo->meshBuffer) {                                                     \
                    _count++;                                                                       \
                }                                                                                   \
            }                                                                                       \
            if (_count > 0) {                                                                       \
                return _count;                                                                      \
            }                                                                                       \
        }                                                                                           \
        return __etOriginal->meshBuffer ? 1u : 0u;                                                  \
    }((originalPtr)))

#define GetRenderableETreeGeoByIndex(originalPtr, geoIndexArg, outPartIndexPtr)                    \
    ([&](OriginalETree* __etOriginal, u32 __geoIndex, u16* __outPartIndex) -> OriginalGeo* {       \
        if (!__etOriginal || !__etOriginal->geoParts || __etOriginal->geoPartCount == 0) {         \
            return nullptr;                                                                         \
        }                                                                                           \
        u32 _renderableIndex = 0;                                                                   \
        for (u16 _i = 0; _i < __etOriginal->geoPartCount; _i++) {                                  \
            OriginalGeo* _geo = __etOriginal->geoParts[_i];                                         \
            if (!_geo || !_geo->meshBuffer) {                                                       \
                continue;                                                                           \
            }                                                                                       \
            if (_renderableIndex == __geoIndex) {                                                   \
                if (__outPartIndex) {                                                               \
                    *__outPartIndex = _i;                                                           \
                }                                                                                   \
                return _geo;                                                                        \
            }                                                                                       \
            _renderableIndex++;                                                                     \
        }                                                                                           \
        return nullptr;                                                                             \
    }((originalPtr), (geoIndexArg), (outPartIndexPtr)))

#define FindETreeJointWorldMatrixByHash(skeletonPtr, jointMatricesPtr, jointHashValue)             \
    ([&](const STreeData* __skel, const Mat4* __jointMatrices, u32 __jointHash) -> const Mat4* {   \
        if (!__skel || !__jointMatrices || !__skel->joints || __jointHash == 0) {                  \
            return nullptr;                                                                         \
        }                                                                                           \
        for (u32 _jointIndex = 0; _jointIndex < __skel->numJoints; _jointIndex++) {                \
            if (__skel->joints[_jointIndex].nameUID == __jointHash) {                               \
                return &__jointMatrices[_jointIndex];                                               \
            }                                                                                       \
        }                                                                                           \
        return nullptr;                                                                             \
    }((skeletonPtr), (jointMatricesPtr), (jointHashValue)))

#define GeoSupportsDynamicUV(geoPtr)                                                                \
    ((geoPtr)                                                                                        \
     && (geoPtr)->meshBuffer                                                                        \
     && (geoPtr)->dynamicVerts                                                                      \
     && (geoPtr)->dynamicVertCount > 0                                                              \
     && (geoPtr)->dynamicPrimStart                                                                  \
     && (geoPtr)->dynamicPrimVertCount                                                              \
     && (geoPtr)->dynamicPrimCount > 0)

#define CaptureGeoBaseUVWords(geoPtr, outBaseWordsPtr)                                              \
    ([&](const OriginalGeo* __geo, u16* __outBaseWords) {                                           \
        if (!GeoSupportsDynamicUV(__geo) || !__outBaseWords) {                                      \
            return;                                                                                 \
        }                                                                                           \
        for (u32 _primIndex = 0; _primIndex < __geo->dynamicPrimCount; _primIndex++) {             \
            const u32 _start = __geo->dynamicPrimStart[_primIndex];                                 \
            const u32 _count = static_cast<u32>(__geo->dynamicPrimVertCount[_primIndex]);           \
            for (u32 _corner = 0; _corner < 4u; _corner++) {                                        \
                u16 _packed = 0;                                                                     \
                if (_corner < _count && (_start + _corner) < __geo->dynamicVertCount) {            \
                    const GeoRenderVertex& _vertex = __geo->dynamicVerts[_start + _corner];         \
                    _packed = static_cast<u16>(static_cast<u8>(_vertex.u)                           \
                                               | (static_cast<u16>(static_cast<u8>(_vertex.v))       \
                                                  << 8));                                            \
                }                                                                                   \
                __outBaseWords[_primIndex * 4u + _corner] = _packed;                                \
            }                                                                                       \
        }                                                                                           \
    }((geoPtr), (outBaseWordsPtr)))

#define ApplyGeoUVWords(geoPtr, baseWordsPtr, addWordsPtr)                                          \
    ([&](OriginalGeo* __geo, const u16* __baseWords, const u16* __addWords) {                       \
        if (!GeoSupportsDynamicUV(__geo) || !__baseWords || !__addWords) {                          \
            return;                                                                                 \
        }                                                                                           \
        for (u32 _primIndex = 0; _primIndex < __geo->dynamicPrimCount; _primIndex++) {             \
            const u32 _start = __geo->dynamicPrimStart[_primIndex];                                 \
            const u32 _count = static_cast<u32>(__geo->dynamicPrimVertCount[_primIndex]);           \
            for (u32 _corner = 0; _corner < _count && _corner < 4u; _corner++) {                    \
                const u16 _baseWord = __baseWords[_primIndex * 4u + _corner];                       \
                const u16 _finalWord = static_cast<u16>(_baseWord + __addWords[_corner]);           \
                if ((_start + _corner) < __geo->dynamicVertCount) {                                 \
                    __geo->dynamicVerts[_start + _corner].u = static_cast<f32>(_finalWord & 0xFFu); \
                    __geo->dynamicVerts[_start + _corner].v = static_cast<f32>((_finalWord >> 8)    \
                                                                             & 0xFFu);              \
                }                                                                                   \
            }                                                                                       \
        }                                                                                           \
        __geo->meshBuffer->SetVertexData(__geo->dynamicVerts, __geo->dynamicVertCount);             \
    }((geoPtr), (baseWordsPtr), (addWordsPtr)))

#define LerpPackedColour24(fromColourValue, toColourValue, blend16Value)                            \
    ([&](u32 __fromColour, u32 __toColour, s32 __blend16) -> u32 {                                  \
        if (__blend16 <= 0) {                                                                       \
            return __fromColour & 0x00FFFFFFu;                                                      \
        }                                                                                           \
        if (__blend16 >= 0x10000) {                                                                 \
            return __toColour & 0x00FFFFFFu;                                                        \
        }                                                                                           \
        const s32 _fromR = static_cast<s32>(__fromColour & 0xFFu);                                  \
        const s32 _fromG = static_cast<s32>((__fromColour >> 8) & 0xFFu);                           \
        const s32 _fromB = static_cast<s32>((__fromColour >> 16) & 0xFFu);                          \
        const s32 _toR = static_cast<s32>(__toColour & 0xFFu);                                      \
        const s32 _toG = static_cast<s32>((__toColour >> 8) & 0xFFu);                               \
        const s32 _toB = static_cast<s32>((__toColour >> 16) & 0xFFu);                              \
        const s32 _outR = _fromR + static_cast<s32>((static_cast<s64>(_toR - _fromR) * __blend16)   \
                                                     >> 16);                                         \
        const s32 _outG = _fromG + static_cast<s32>((static_cast<s64>(_toG - _fromG) * __blend16)   \
                                                     >> 16);                                         \
        const s32 _outB = _fromB + static_cast<s32>((static_cast<s64>(_toB - _fromB) * __blend16)   \
                                                     >> 16);                                         \
        return static_cast<u32>((_outR & 0xFF) | ((_outG & 0xFF) << 8) | ((_outB & 0xFF) << 16));  \
    }((fromColourValue), (toColourValue), (blend16Value)))

#define DecodePackedColour24(packedColourValue, outRPtr, outGPtr, outBPtr)                          \
    ([&](u32 __pc24, f32* __outR, f32* __outG, f32* __outB) {                                       \
        const f32 _r = std::min(1.0f, static_cast<f32>(__pc24 & 0xFFu) / 128.0f);                  \
        const f32 _g = std::min(1.0f, static_cast<f32>((__pc24 >> 8) & 0xFFu) / 128.0f);           \
        const f32 _b = std::min(1.0f, static_cast<f32>((__pc24 >> 16) & 0xFFu) / 128.0f);          \
        if (__outR) {                                                                                \
            *__outR = _r;                                                                            \
        }                                                                                            \
        if (__outG) {                                                                                \
            *__outG = _g;                                                                            \
        }                                                                                            \
        if (__outB) {                                                                                \
            *__outB = _b;                                                                            \
        }                                                                                            \
    }((packedColourValue), (outRPtr), (outGPtr), (outBPtr)))

#define ApplyGeoColourWordByOffset(geoPtr, colourWordOffsetValue, packedColourValue)                \
    ([&](OriginalGeo* __geo, u32 __colourWordOffset, u32 __packedColour) -> bool {                  \
        if (!GeoSupportsDynamicUV(__geo) || (__colourWordOffset & 3u) != 0u) {                      \
            return false;                                                                            \
        }                                                                                           \
        const u32 _vertexIndex = __colourWordOffset >> 2;                                           \
        if (_vertexIndex >= __geo->dynamicVertCount) {                                              \
            return false;                                                                            \
        }                                                                                           \
        f32 _decodedR = 0.0f;                                                                        \
        f32 _decodedG = 0.0f;                                                                        \
        f32 _decodedB = 0.0f;                                                                        \
        DecodePackedColour24(__packedColour, &_decodedR, &_decodedG, &_decodedB);                   \
        __geo->dynamicVerts[_vertexIndex].r = _decodedR;                                            \
        __geo->dynamicVerts[_vertexIndex].g = _decodedG;                                            \
        __geo->dynamicVerts[_vertexIndex].b = _decodedB;                                            \
        return true;                                                                                 \
    }((geoPtr), (colourWordOffsetValue), (packedColourValue)))

#define GetMiscAnimFrameCount(nodePtr)                                                               \
    ([&](const MiscAnimNode* __maNode) -> s32 {                                                     \
        if (!__maNode) {                                                                             \
            return 0;                                                                                \
        }                                                                                           \
        if (__maNode->anim) {                                                                        \
            return __maNode->anim->numFrames;                                                       \
        }                                                                                           \
        if (__maNode->paramAnim) {                                                                   \
            return __maNode->paramAnim->numFrames;                                                  \
        }                                                                                           \
        if (__maNode->sequenceAnim) {                                                                \
            return __maNode->sequenceAnim->numFrames;                                               \
        }                                                                                           \
        if (__maNode->compositeAnim) {                                                               \
            return __maNode->compositeAnim->field12;                                                \
        }                                                                                           \
        if (__maNode->frameList) {                                                                   \
            return __maNode->frameList->numFrames;                                                  \
        }                                                                                           \
        if (__maNode->vizAnim) {                                                                     \
            return __maNode->vizAnim->numFrames;                                                    \
        }                                                                                           \
        if (__maNode->cbvParamAnim) {                                                                \
            if (g_animMgr && __maNode->cbvParamAnim->blendAnimUID != 0) {                           \
                MiscAnimNode* _blendNode = g_animMgr->GetMiscAnim(__maNode->cbvParamAnim->blendAnimUID); \
                if (_blendNode && _blendNode->paramAnim) {                                          \
                    return _blendNode->paramAnim->numFrames;                                        \
                }                                                                                   \
            }                                                                                       \
            return 1;                                                                                \
        }                                                                                           \
        if (__maNode->clutAnim) {                                                                    \
            return __maNode->clutAnim->numFrames;                                                   \
        }                                                                                           \
        return 0;                                                                                    \
    }((nodePtr)))

#define ResolveCompositePartNode(partRef)                                                            \
    ([&](const CompositeAnimPartData& __caPart) -> MiscAnimNode* {                                  \
        if (__caPart.animNameUID == 0 || !g_animMgr) {                                              \
            return nullptr;                                                                          \
        }                                                                                           \
        return g_animMgr->GetMiscAnim(__caPart.animNameUID);                                        \
    }((partRef)))

static const s32 kPathAttribNotFound = (s32)0xABCDABCD;

static const s32 kLensFlareFrameScaleTable[16] = {
    0x1945, 0x2721, 0x342E, 0x4139,
    0x3911, 0x34FD, 0x3BEC, 0x4000,
    0x3A4B, 0x328C, 0x3771, 0x4685,
    0x53F9, 0x6D40, 0x88F7, 0xA033,
};

static const s32 kLensFlareFrameScaleTable2[16] = {
    0x68, 0x68, 0x68, 0x68,
    0x68, 0xD1, 0x20A, 0x271,
    0x685, 0xC3B, 0x1390, 0x2033,
    0x2FB2, 0x46EE, 0x78A9, 0xC413,
};

static const s32 kLensFlareClampValues[7] = {
    0x3FCA, 0x56D7, 0x8486, 0xEC3F, 0xA365, 0x6122, 0x3333,
};

class PathInfo {
public:
    PathInfo();
    ~PathInfo();

    s32 Init(const DBPath* pathSource, const DBPoint* pointSource);
    s32 Reset();
    s32 Update();

    s32 OnNewPathNode(s32 nodeChanged);

    const LVector* GetPosition() const;
    const LVector* GetRotation() const;

private:
    Path* path = nullptr;
    LVector fallbackPosition = {};
    LVector rotation = {};
    s32 moveSpeed = 5;
    s32 pathMode = 0;
    s32 state = 1;
    s32 allowMove = 1;
    s32 lerpX = 0;
    s32 lerpY = 0;
    s32 lerpZ = 0;
    bool splinePath = false;

    friend class WEffect;
};

PathInfo::PathInfo() {
    MARKFUNCTION(0x800BE73C);
}

PathInfo::~PathInfo() {
    MARKFUNCTION(0x800BE78C);
    if (path) {
        delete path;
        path = nullptr;
    }
}

s32 PathInfo::Init(const DBPath* pathSource, const DBPoint* pointSource) {
    MARKFUNCTION(0x800BE54C);

    if (!pathSource || !pointSource) {
        return 0;
    }

    u32 value = 0;
    if (!pointSource->FindAttribValue(6, &value)) {
        return 0;
    }

    if (path) {
        delete path;
        path = nullptr;
    }

    if (value == 1) {
        SplinePath* spline = new SplinePath();
        spline->Init(pathSource);
        path = spline;
        splinePath = true;
    }
    else {
        LinearPath* linear = new LinearPath();
        linear->Init(pathSource);
        path = linear;
        splinePath = false;
    }

    if (!path) {
        return 0;
    }

    if (pointSource->FindAttribValue(7, &value) && value) {
        if (value == 2 || value == 3) {
            pathMode = static_cast<s32>(value);
        }
        else {
            pathMode = 1;
        }
    }
    else {
        pathMode = 0;
    }

    if (pointSource->FindAttribValue(8, &value)) {
        lerpZ = static_cast<s32>(value);
    }

    if (pointSource->FindAttribValue(9, &value)) {
        lerpX = static_cast<s32>(value);
    }

    if (pointSource->FindAttribValue(10, &value)) {
        lerpY = static_cast<s32>(value);
    }

    return 1;
}

s32 PathInfo::Reset() {
    MARKFUNCTION(0x800BE7F8);

    if (!path) {
        return 0;
    }

    path->Reset();
    return OnNewPathNode(0);
}

s32 PathInfo::Update() {
    MARKFUNCTION(0x800BE844);

    if (!path) {
        return 0;
    }

    if (state != 0 && state != 2) {
        if (path->EndOfPath()) {
            if (pathMode == 0) {
                return 1;
            }

            if (pathMode == 1) {
                path->Flip();
            }
            else if (pathMode == 3) {
                return 0;
            }

            path->Reset();
            OnNewPathNode(0);
        }

        if (allowMove) {
            s32 crossedNode = 0;
            if (splinePath) {
                crossedNode = static_cast<SplinePath*>(path)->Move(moveSpeed);
            }
            else {
                crossedNode = static_cast<LinearPath*>(path)->Move(moveSpeed);
            }

            if (crossedNode) {
                OnNewPathNode(1);
            }
        }
        else {
            OnNewPathNode(0);
        }
    }

    if (state == 2) {
        path->Reset();
    }

    return 0;
}

s32 PathInfo::OnNewPathNode(s32 /*nodeChanged*/) {
    MARKFUNCTION(0x800BEA44);

    if (!path || !path->nodeAttribs || path->numPoints <= 0) {
        return 0;
    }

    const s32 segment = path->currentSegment;
    if (segment < 0 || segment >= path->numPoints) {
        return 0;
    }

    const NodeAttribs& attribs = path->nodeAttribs[segment];

    const s32 speedAttrib = attribs.GetAttrib(7);
    moveSpeed = (speedAttrib == kPathAttribNotFound) ? 5 : speedAttrib;

    const s32 rotZ = attribs.GetAttrib(8);
    if (rotZ != kPathAttribNotFound) {
        rotation.z = rotZ;
    }

    const s32 rotX = attribs.GetAttrib(9);
    if (rotX != kPathAttribNotFound) {
        rotation.x = rotX;
    }

    const s32 rotY = attribs.GetAttrib(10);
    if (rotY != kPathAttribNotFound) {
        rotation.y = rotY;
    }

    const s32 activeZoneHash = attribs.GetAttrib(11);
    if (activeZoneHash == kPathAttribNotFound) {
        allowMove = 1;
    }
    else {
        allowMove = 0;

        if (g_ai && Player::s_player) {
            for (ccMinNode* node = g_ai->activeZoneList.head; node; node = node->next) {
                ActiveZone* zone = static_cast<ActiveZone*>(static_cast<ccNode*>(node));
                if (zone->nameCRC == static_cast<u32>(activeZoneHash)
                    && zone->IsInActiveZone(Player::s_player)) {
                    allowMove = 1;
                    break;
                }
            }
        }
    }

    return 1;
}

const LVector* PathInfo::GetPosition() const {
    MARKFUNCTION(0x800BED44);

    if (path) {
        return &path->current;
    }

    return &fallbackPosition;
}

const LVector* PathInfo::GetRotation() const {
    MARKFUNCTION(0x800BED50);
    return &rotation;
}

struct ComEffectScaleBinding {
    STreeJoint* joint = nullptr;
    ScaleData* scaleData = nullptr;
    const u8* channelData = nullptr;

    void* previousCallbackData = nullptr;
    STreeJointCallback previousPreCallback = nullptr;
    u32 previousFlags = 0;
    bool previousUseOverrideMatrix = false;
    Mat4 previousOverrideMatrix;
};

static bool ResolveSequenceAnimFrame(SequenceAnim* sequence,
                                     s32 sequenceFrame,
                                     MiscAnimNode** outNode,
                                     TransformAnim** outAnim,
                                     s32* outFrame)
{
    if (!sequence || !outFrame) {
        return false;
    }

    if (outNode) {
        *outNode = nullptr;
    }
    if (outAnim) {
        *outAnim = nullptr;
    }

    while (sequence) {
        if (!sequence->parts || sequence->numParts == 0) {
            return false;
        }

        const s32 partIndex = sequenceFrame >> 8;
        if (partIndex < 0 || static_cast<u32>(partIndex) >= sequence->numParts) {
            return false;
        }

        const s32 partFrame = sequenceFrame & 0xFF;
        SequenceAnimPart& part = sequence->parts[partIndex];

        if (part.animHash != 0) {
            if (!g_animMgr) {
                part.node = nullptr;
                part.anim = nullptr;
                return false;
            }

            MiscAnimNode* liveNode = g_animMgr->GetMiscAnim(part.animHash);
            if (liveNode != part.node) {
                part.node = liveNode;
                part.anim = liveNode ? liveNode->anim : nullptr;
            }
        }

        MiscAnimNode* node = part.node;
        if (node) {
            if (node->anim) {
                part.anim = node->anim;
                if (outNode) {
                    *outNode = node;
                }
                if (outAnim) {
                    *outAnim = part.anim;
                }
                *outFrame = partFrame;
                return true;
            }

            if (node->sequenceAnim) {
                sequence = node->sequenceAnim;
                sequenceFrame = partFrame;
                continue;
            }

            if (outNode) {
                *outNode = node;
            }
            if (outAnim) {
                *outAnim = nullptr;
            }
            *outFrame = partFrame;
            return true;
        }

        if (part.anim) {
            if (outNode) {
                *outNode = nullptr;
            }
            if (outAnim) {
                *outAnim = part.anim;
            }
            *outFrame = partFrame;
            return true;
        }

        return false;
    }

    return false;
}

// PSX: checkForAndFreeSequenceAnims__FP10tAnimation (0x8004DB9C)
// Host uses recursive misc-node emulation rather than allocated sequence puppets,
// so teardown clears resolved sequence part caches for composite-root paths.
static void checkForAndFreeSequenceAnims__FP10tAnimation(MiscAnimNode* node) {
    MARKFUNCTION(0x8004DB9C);

    auto recurse = [&](auto&& self, MiscAnimNode* currentNode, s32 depth) -> void {
        if (depth > 16) {
            return;
        }

        if (!currentNode || !currentNode->compositeAnim || !currentNode->compositeAnim->parts) {
            return;
        }

        for (u32 i = 0; i < currentNode->compositeAnim->numParts; i++) {
            CompositeAnimPartData& part = currentNode->compositeAnim->parts[i];
            MiscAnimNode* partNode = ResolveCompositePartNode(part);
            if (!partNode) {
                continue;
            }

            if (partNode->sequenceAnim && partNode->sequenceAnim->parts) {
                SequenceAnim* sequence = partNode->sequenceAnim;
                for (u32 partIndex = 0; partIndex < sequence->numParts; partIndex++) {
                    sequence->parts[partIndex].node = nullptr;
                    sequence->parts[partIndex].anim = nullptr;
                }
            }

            if (partNode->compositeAnim && partNode != currentNode) {
                self(self, partNode, depth + 1);
            }
        }
    };

    recurse(recurse, node, 0);
}

// PSX: SetupPaletteData__7WEffectUlUlUl (WEFFECT.CPP:218, 0x8008A954)
PaletteData* WEffect::SetupPaletteData(u32 paletteHash, u32 clutMode, u32 flags) {
    MARKFUNCTION(0x8008A954);

    if (!comEffect) {
        return nullptr;
    }

    PaletteData* info = FindPaletteInfo(paletteHash);
    if (!info) {
        return nullptr;
    }

    PaletteData* clone = info->Clone();
    paletteData = clone;
    if (clone) {
        clone->SetupClut(comEffect, static_cast<s32>(clutMode), flags);
    }

    return clone;
}

#define ComEffect_ResetModel(selfPtr)                                                               \
    do {                                                                                            \
        ComEffect* _self = (selfPtr);                                                               \
        if (_self->miscAnimNode && g_animMgr) {                                                     \
            MiscAnimNode* _liveNode = g_animMgr->GetMiscAnim(_self->miscAnimHash);                 \
            if (_liveNode == _self->miscAnimNode && _liveNode->compositeAnim) {                    \
                checkForAndFreeSequenceAnims__FP10tAnimation(_liveNode);                            \
            }                                                                                       \
        }                                                                                           \
                                                                                                    \
        if (_self->scaleBindings) {                                                                  \
            for (u16 _i = 0; _i < _self->scaleBindingCount; _i++) {                                 \
                ComEffectScaleBinding& _binding = _self->scaleBindings[_i];                         \
                STreeJoint* _joint = _binding.joint;                                                \
                if (!_joint) {                                                                      \
                    continue;                                                                       \
                }                                                                                   \
                                                                                                    \
                if (_joint->callbackData == &_binding) {                                            \
                    _joint->callbackData = _binding.previousCallbackData;                           \
                    _joint->preCallback = _binding.previousPreCallback;                             \
                    _joint->flags = _binding.previousFlags;                                         \
                    _joint->useOverrideMatrix = _binding.previousUseOverrideMatrix;                 \
                    _joint->overrideMatrix = _binding.previousOverrideMatrix;                       \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            delete[] _self->scaleBindings;                                                           \
            _self->scaleBindings = nullptr;                                                          \
        }                                                                                           \
                                                                                                    \
        _self->scaleBindingCount = 0;                                                                \
        _self->scaleBindingCapacity = 0;                                                             \
                                                                                                    \
        ComEffect_ClearUVCache(_self);                                                               \
        ComEffect_ClearVertexCache(_self);                                                           \
                                                                                                    \
        if (_self->model) {                                                                          \
            delete _self->model;                                                                     \
            _self->model = nullptr;                                                                  \
        }                                                                                           \
                                                                                                    \
        _self->eModel = nullptr;                                                                     \
        _self->sModel = nullptr;                                                                     \
        _self->miscAnimNode = nullptr;                                                               \
        _self->miscAnim = nullptr;                                                                   \
        _self->boundTransformAnim = nullptr;                                                         \
        _self->frameListAnim = nullptr;                                                              \
        _self->compositeAnim = nullptr;                                                              \
        _self->sequenceAnim = nullptr;                                                               \
        _self->vizAnim = nullptr;                                                                    \
        _self->scaleData = nullptr;                                                                  \
        _self->frameCount = 0;                                                                       \
        _self->currentFrame = 0;                                                                     \
        _self->fastDrawCount = 0;                                                                    \
        _self->geoSwapWordSlot = ComEffect::kGeoSwapWordInactive;                                   \
    } while (0)

#define ComEffect_ClearUVCache(selfPtr)                                                             \
    ([&](ComEffect* __ceSelf) {                                                                      \
        delete[] __ceSelf->uvBaseWords;                                                              \
        __ceSelf->uvBaseWords = nullptr;                                                             \
        __ceSelf->uvPrimCount = 0;                                                                   \
                                                                                                     \
        if (__ceSelf->uvGeoBaseWords) {                                                              \
            for (u16 _geoIndex = 0; _geoIndex < __ceSelf->uvGeoCount; _geoIndex++) {               \
                delete[] __ceSelf->uvGeoBaseWords[_geoIndex];                                        \
            }                                                                                        \
        }                                                                                            \
                                                                                                     \
        delete[] __ceSelf->uvGeoBaseWords;                                                           \
        __ceSelf->uvGeoBaseWords = nullptr;                                                          \
                                                                                                     \
        delete[] __ceSelf->uvGeoList;                                                                \
        __ceSelf->uvGeoList = nullptr;                                                               \
        __ceSelf->uvGeoCount = 0;                                                                    \
    }((selfPtr)))

#define ComEffect_ClearVertexCache(selfPtr)                                                         \
    ([&](ComEffect* __ceSelf) {                                                                      \
        delete[] __ceSelf->vertexAnimInfos;                                                          \
        __ceSelf->vertexAnimInfos = nullptr;                                                         \
        __ceSelf->vertexAnimInfoCount = 0;                                                           \
    }((selfPtr)))

#define ComEffect_BindCommonState(selfPtr, inResourceHashArg, inMiscAnimHashArg)                  \
    ([&]() -> bool {                                                                                \
        ComEffect* _self = (selfPtr);                                                               \
        const s32 _inResourceHash = (inResourceHashArg);                                            \
        const s32 _inMiscAnimHash = (inMiscAnimHashArg);                                            \
        _self->resourceHash = static_cast<u32>(_inResourceHash);                                    \
        _self->miscAnimHash = static_cast<u32>(_inMiscAnimHash);                                    \
        _self->geoSwapWordSlot = ComEffect::kGeoSwapWordInactive;                                   \
                                                                                                     \
        _self->miscAnimNode = nullptr;                                                               \
        _self->miscAnim = nullptr;                                                                   \
        _self->boundTransformAnim = nullptr;                                                         \
        _self->frameListAnim = nullptr;                                                              \
        _self->compositeAnim = nullptr;                                                              \
        _self->sequenceAnim = nullptr;                                                               \
        _self->vizAnim = nullptr;                                                                    \
        _self->scaleData = FindScaleInfo(_self->resourceHash);                                      \
        _self->frameCount = 0;                                                                       \
                                                                                                     \
        if (g_animMgr) {                                                                             \
            _self->miscAnimNode = g_animMgr->GetMiscAnim(static_cast<u32>(_inMiscAnimHash));       \
            if (_self->miscAnimNode) {                                                               \
                _self->miscAnim = _self->miscAnimNode->anim;                                        \
                _self->frameListAnim = _self->miscAnimNode->frameList;                              \
                _self->compositeAnim = _self->miscAnimNode->compositeAnim;                          \
                _self->sequenceAnim = _self->miscAnimNode->sequenceAnim;                            \
                _self->vizAnim = _self->miscAnimNode->vizAnim;                                      \
                                                                                                     \
                const s32 _resolvedFrameCount = GetMiscAnimFrameCount(_self->miscAnimNode);         \
                if (_resolvedFrameCount > 0) {                                                       \
                    _self->frameCount = static_cast<s16>(_resolvedFrameCount);                      \
                }                                                                                    \
            }                                                                                        \
        }                                                                                            \
                                                                                                     \
        return true;                                                                                 \
    }())

#define ComEffect_ResolveLiveMiscAnimNode(selfPtr)                                                  \
    ([&]() -> MiscAnimNode* {                                                                        \
        ComEffect* _self = (selfPtr);                                                                \
        if (!g_animMgr || _self->miscAnimHash == 0) {                                                \
            _self->miscAnimNode = nullptr;                                                           \
            _self->miscAnim = nullptr;                                                               \
            _self->frameListAnim = nullptr;                                                          \
            _self->compositeAnim = nullptr;                                                          \
            _self->sequenceAnim = nullptr;                                                           \
            _self->vizAnim = nullptr;                                                                \
            _self->frameCount = 0;                                                                   \
            return nullptr;                                                                          \
        }                                                                                            \
                                                                                                     \
        MiscAnimNode* _liveNode = g_animMgr->GetMiscAnim(_self->miscAnimHash);                      \
        if (_liveNode != _self->miscAnimNode) {                                                      \
            _self->miscAnimNode = _liveNode;                                                         \
            _self->miscAnim = _liveNode ? _liveNode->anim : nullptr;                                \
            _self->boundTransformAnim = nullptr;                                                     \
            _self->frameListAnim = _liveNode ? _liveNode->frameList : nullptr;                      \
            _self->compositeAnim = _liveNode ? _liveNode->compositeAnim : nullptr;                  \
            _self->sequenceAnim = _liveNode ? _liveNode->sequenceAnim : nullptr;                    \
            _self->vizAnim = _liveNode ? _liveNode->vizAnim : nullptr;                              \
                                                                                                     \
            const s32 _resolvedFrameCount = GetMiscAnimFrameCount(_liveNode);                       \
            _self->frameCount = (_resolvedFrameCount > 0) ? static_cast<s16>(_resolvedFrameCount) : 0; \
        }                                                                                            \
                                                                                                     \
        return _liveNode;                                                                            \
    }())

#define ComEffect_ResolveInitialTransform(nodeArg, frameArg, outAnimPtrArg, outFramePtrArg)       \
    ([&]() -> bool {                                                                                 \
        TransformAnim** _outAnim = (outAnimPtrArg);                                                  \
        s32* _outFrame = (outFramePtrArg);                                                           \
        if (!_outAnim || !_outFrame) {                                                               \
            return false;                                                                            \
        }                                                                                            \
                                                                                                     \
        *_outAnim = nullptr;                                                                         \
        *_outFrame = 0;                                                                              \
                                                                                                     \
        auto _resolve = [&](auto&& self, MiscAnimNode* _node, s32 _frame) -> bool {                \
            if (!_node) {                                                                            \
                return false;                                                                        \
            }                                                                                        \
                                                                                                     \
            if (_node->anim) {                                                                       \
                *_outAnim = _node->anim;                                                             \
                *_outFrame = _frame;                                                                 \
                return true;                                                                         \
            }                                                                                        \
                                                                                                     \
            if (_node->sequenceAnim) {                                                               \
                MiscAnimNode* _partNode = nullptr;                                                   \
                TransformAnim* _partAnim = nullptr;                                                  \
                s32 _partFrame = 0;                                                                  \
                if (!ResolveSequenceAnimFrame(_node->sequenceAnim, _frame, &_partNode, &_partAnim, &_partFrame)) { \
                    return false;                                                                    \
                }                                                                                    \
                                                                                                     \
                if (_partAnim) {                                                                     \
                    *_outAnim = _partAnim;                                                           \
                    *_outFrame = _partFrame;                                                         \
                    return true;                                                                     \
                }                                                                                    \
                                                                                                     \
                return self(self, _partNode, _partFrame);                                            \
            }                                                                                        \
                                                                                                     \
            if (_node->compositeAnim && _node->compositeAnim->parts) {                              \
                for (u32 _i = 0; _i < _node->compositeAnim->numParts; _i++) {                       \
                    CompositeAnimPartData& _part = _node->compositeAnim->parts[_i];                 \
                    MiscAnimNode* _partNode = ResolveCompositePartNode(_part);                       \
                    if (!_partNode) {                                                                \
                        continue;                                                                    \
                    }                                                                                \
                                                                                                     \
                    const s32 _partFrameCount = GetMiscAnimFrameCount(_partNode);                   \
                    if (_partFrameCount <= 0) {                                                      \
                        continue;                                                                    \
                    }                                                                                \
                                                                                                     \
                    const u32 _shift = static_cast<u32>(_part.field1) & 31u;                        \
                    s32 _partFrame = _frame >> _shift;                                               \
                    if (_partFrame >= _partFrameCount) {                                             \
                        if (_part.field0 != 0) {                                                     \
                            _partFrame %= _partFrameCount;                                           \
                        }                                                                            \
                        else {                                                                       \
                            _partFrame = _partFrameCount - 1;                                       \
                        }                                                                            \
                    }                                                                                \
                                                                                                     \
                    if (self(self, _partNode, _partFrame)) {                                         \
                        return true;                                                                 \
                    }                                                                                \
                }                                                                                    \
            }                                                                                        \
                                                                                                     \
            return false;                                                                            \
        };                                                                                           \
                                                                                                     \
        return _resolve(_resolve, (nodeArg), (frameArg));                                            \
    }())

#define ComEffect_BindScaleDataToModel(selfPtr)                                                     \
    do {                                                                                            \
        ComEffect* _self = (selfPtr);                                                               \
        if (_self->scaleBindings) {                                                                  \
            for (u16 _i = 0; _i < _self->scaleBindingCount; _i++) {                                 \
                ComEffectScaleBinding& _binding = _self->scaleBindings[_i];                         \
                STreeJoint* _joint = _binding.joint;                                                \
                if (!_joint) {                                                                      \
                    continue;                                                                       \
                }                                                                                   \
                                                                                                    \
                if (_joint->callbackData == &_binding) {                                            \
                    _joint->callbackData = _binding.previousCallbackData;                           \
                    _joint->preCallback = _binding.previousPreCallback;                             \
                    _joint->flags = _binding.previousFlags;                                         \
                    _joint->useOverrideMatrix = _binding.previousUseOverrideMatrix;                 \
                    _joint->overrideMatrix = _binding.previousOverrideMatrix;                       \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            delete[] _self->scaleBindings;                                                           \
            _self->scaleBindings = nullptr;                                                          \
        }                                                                                           \
                                                                                                    \
        _self->scaleBindingCount = 0;                                                                \
        _self->scaleBindingCapacity = 0;                                                             \
                                                                                                    \
        if (!_self->scaleData || !_self->model || !_self->model->drawable || _self->model->drawableType != 2) { \
            break;                                                                                  \
        }                                                                                           \
                                                                                                    \
        OriginalSTree* _active = GetActiveSTree(_self->model->drawable);                           \
        STreeData* _skeleton = _active ? _active->skeleton : nullptr;                               \
        if (!_skeleton || !_skeleton->joints || _skeleton->numJoints == 0) {                       \
            break;                                                                                  \
        }                                                                                           \
                                                                                                    \
        const u32 _channelCount = ScaleData_GetChannelCount(_self->scaleData);                      \
        if (_channelCount == 0) {                                                                   \
            break;                                                                                  \
        }                                                                                           \
                                                                                                    \
        _self->scaleBindings = new ComEffectScaleBinding[_channelCount];                            \
        if (!_self->scaleBindings) {                                                                 \
            break;                                                                                  \
        }                                                                                           \
                                                                                                    \
        static STreeJointCallback _kScalePreCallback = +[](STreeJoint* joint, u32 jointIndex, const Mat4& currentMatrix) -> s32 { \
            if (!joint || !joint->callbackData) {                                                   \
                return 1;                                                                           \
            }                                                                                       \
                                                                                                    \
            ComEffectScaleBinding* binding = static_cast<ComEffectScaleBinding*>(joint->callbackData); \
            if (!binding->scaleData || !binding->channelData) {                                     \
                return 1;                                                                           \
            }                                                                                       \
                                                                                                    \
            if (binding->previousPreCallback) {                                                     \
                joint->callbackData = binding->previousCallbackData;                                \
                binding->previousPreCallback(joint, jointIndex, currentMatrix);                     \
                joint->callbackData = binding;                                                      \
            }                                                                                       \
                                                                                                    \
            LVector scale = { 0x10000, 0x10000, 0x10000 };                                          \
            ScaleData_GetScale(binding->scaleData, &scale, binding->channelData);                   \
                                                                                                    \
            Mat4 localMatrix;                                                                       \
            p3dBuildRotMatrixYZX(joint->rotationX, joint->rotationY, joint->rotationZ, localMatrix); \
                                                                                                    \
            const f32 sx = FIX16_TO_FLOAT(scale.x);                                                 \
            const f32 sy = FIX16_TO_FLOAT(scale.y);                                                 \
            const f32 sz = FIX16_TO_FLOAT(scale.z);                                                 \
                                                                                                    \
            localMatrix.m[0] *= sx;                                                                 \
            localMatrix.m[1] *= sx;                                                                 \
            localMatrix.m[2] *= sx;                                                                 \
            localMatrix.m[4] *= sy;                                                                 \
            localMatrix.m[5] *= sy;                                                                 \
            localMatrix.m[6] *= sy;                                                                 \
            localMatrix.m[8] *= sz;                                                                 \
            localMatrix.m[9] *= sz;                                                                 \
            localMatrix.m[10] *= sz;                                                                \
                                                                                                    \
            localMatrix.SetTranslation(static_cast<f32>(joint->translationX),                       \
                                       static_cast<f32>(joint->translationY),                       \
                                       static_cast<f32>(joint->translationZ));                      \
                                                                                                    \
            joint->flags = (joint->flags & ~(STF_TRANSFORM | STF_MULT_MATRIX)) | STF_MULT_MATRIX; \
            joint->overrideMatrix = localMatrix;                                                    \
            joint->useOverrideMatrix = true;                                                        \
            return 1;                                                                               \
        };                                                                                          \
                                                                                                    \
        _self->scaleBindingCapacity = static_cast<u16>(_channelCount);                              \
        _self->scaleBindingCount = 0;                                                               \
                                                                                                    \
        for (u32 _channelIndex = 0; _channelIndex < _channelCount; _channelIndex++) {              \
            const u8* _channelData = ScaleData_GetChannelByIndex(_self->scaleData, _channelIndex); \
            if (!_channelData) {                                                                    \
                continue;                                                                           \
            }                                                                                       \
                                                                                                    \
            const u32 _jointHash = ScaleData_GetChannelJointHash(_channelData);                    \
            STreeJoint* _targetJoint = nullptr;                                                     \
            for (u32 _jointIndex = 0; _jointIndex < _skeleton->numJoints; _jointIndex++) {         \
                if (_skeleton->joints[_jointIndex].nameUID == _jointHash) {                        \
                    _targetJoint = &_skeleton->joints[_jointIndex];                                 \
                    break;                                                                          \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            if (!_targetJoint) {                                                                    \
                continue;                                                                           \
            }                                                                                       \
                                                                                                    \
            ComEffectScaleBinding& _binding = _self->scaleBindings[_self->scaleBindingCount++];    \
            _binding.joint = _targetJoint;                                                          \
            _binding.scaleData = _self->scaleData;                                                  \
            _binding.channelData = _channelData;                                                    \
            _binding.previousCallbackData = _targetJoint->callbackData;                             \
            _binding.previousPreCallback = _targetJoint->preCallback;                               \
            _binding.previousFlags = _targetJoint->flags;                                           \
            _binding.previousUseOverrideMatrix = _targetJoint->useOverrideMatrix;                   \
            _binding.previousOverrideMatrix = _targetJoint->overrideMatrix;                         \
                                                                                                    \
            _targetJoint->callbackData = &_binding;                                                 \
            _targetJoint->preCallback = _kScalePreCallback;                                         \
            _targetJoint->flags |= STF_PRE_CALLBACK_MASK;                                           \
        }                                                                                           \
                                                                                                    \
        if (_self->scaleBindingCount == 0) {                                                         \
            delete[] _self->scaleBindings;                                                           \
            _self->scaleBindings = nullptr;                                                          \
            _self->scaleBindingCapacity = 0;                                                         \
        }                                                                                           \
    } while (0)

#define ComEffect_BindTransformAnimToModel(selfPtr, animArg)                                       \
    ([&](ComEffect* __ceSelf, TransformAnim* __ceAnim) -> bool {                                    \
        if (!__ceAnim || !__ceSelf->model) {                                                         \
            return false;                                                                            \
        }                                                                                            \
                                                                                                     \
        if (__ceSelf->sModel) {                                                                      \
            if (!__ceSelf->model->animStructure || __ceSelf->boundTransformAnim != __ceAnim) {      \
                __ceSelf->sModel->ApplyAnimToModelBasic(__ceAnim);                                   \
                __ceSelf->boundTransformAnim = __ceAnim;                                             \
            }                                                                                        \
            return __ceSelf->model->animStructure != nullptr;                                        \
        }                                                                                            \
                                                                                                     \
        if (!__ceSelf->model->animStructure) {                                                       \
            return false;                                                                            \
        }                                                                                            \
                                                                                                     \
        if (__ceSelf->boundTransformAnim != __ceAnim) {                                              \
            AnimStructure* _animStructure = static_cast<AnimStructure*>(__ceSelf->model->animStructure); \
            _animStructure->animation = __ceAnim;                                                    \
            if (_animStructure->flip) {                                                              \
                _animStructure->flip->anim = __ceAnim;                                               \
                _animStructure->flip->dirty = 1;                                                     \
            }                                                                                        \
            _animStructure->ResetCountsToAnim();                                                     \
            __ceSelf->boundTransformAnim = __ceAnim;                                                 \
        }                                                                                            \
                                                                                                     \
        return true;                                                                                 \
    }((selfPtr), (animArg)))

#define ComEffect_ApplyTransformAnimFrame(selfPtr, animArg, frameArg, updateJointsArg)            \
    ([&](ComEffect* __ceSelf, TransformAnim* __ceAnim, s32 __ceFrame, bool __ceUpdateJoints) -> bool { \
        if (!ComEffect_BindTransformAnimToModel(__ceSelf, __ceAnim)) {                               \
            return false;                                                                            \
        }                                                                                            \
                                                                                                     \
        AnimStructure* _animStructure = static_cast<AnimStructure*>(__ceSelf->model ? __ceSelf->model->animStructure : nullptr); \
        if (!_animStructure || !_animStructure->flip) {                                               \
            return false;                                                                            \
        }                                                                                            \
                                                                                                     \
        _animStructure->flip->SetFrame(__ceFrame);                                                   \
        if (__ceUpdateJoints) {                                                                      \
            _animStructure->flip->UpdateJoints();                                                    \
        }                                                                                            \
                                                                                                     \
        return true;                                                                                 \
    }((selfPtr), (animArg), (frameArg), (updateJointsArg)))

ComEffect::ComEffect() {
    MARKFUNCTION(0x8004DB60);
}

ComEffect::~ComEffect() {
    MARKFUNCTION(0x8004DC90);

    if (miscAnimNode && g_animMgr) {
        MiscAnimNode* liveNode = g_animMgr->GetMiscAnim(miscAnimHash);
        if (liveNode == miscAnimNode && liveNode->compositeAnim) {
            checkForAndFreeSequenceAnims__FP10tAnimation(liveNode);
        }
    }

    ComEffect_ResetModel(this);
}

bool ComEffect::ApplyVizAnimFrame(VizAnim* vizAnimData, s32 frame) {
    if (!vizAnimData || !model || model->drawableType != 3 || !model->drawable) {
        return false;
    }

    DrawableETree* drawableETree = static_cast<DrawableETree*>(model->drawable);
    if (!drawableETree || !drawableETree->original) {
        return false;
    }

    OriginalETree* original = drawableETree->original;
    if (!original->geoPartHashes || original->geoPartCount == 0) {
        return false;
    }

    bool applied = false;
    for (u32 nodeIndex = 0; nodeIndex < vizAnimData->numNodes; nodeIndex++) {
        const VizAnimNodeEntry& vizNode = vizAnimData->nodes[nodeIndex];
        if (vizNode.targetHash == 0) {
            continue;
        }

        const bool visible = vizAnimData->IsNodeVisible(nodeIndex, frame);
        drawableETree->SetGeoPartVisibleByHash(vizNode.targetHash, visible);

        for (u16 partIndex = 0; partIndex < original->geoPartCount; partIndex++) {
            if (original->geoPartHashes[partIndex] == vizNode.targetHash) {
                applied = true;
                break;
            }
        }
    }

    return applied;
}

bool ComEffect::ApplyCBVParamAnimFrame(CBVParamAnimData* cbvParamAnimData, s32 frame) {
    if (!cbvParamAnimData || !cbvParamAnimData->values || !cbvParamAnimData->primOffsets
        || cbvParamAnimData->numEntries == 0 || cbvParamAnimData->valueCols == 0
        || cbvParamAnimData->valueRows == 0) {
        return false;
    }

    // PSX tCBVParamFlip::Update supports modes 0/1/2.
    // Mode 1 writes both +64 and +68 lanes; host currently mirrors only
    // the +68-equivalent dynamic-vertex lane used by mode 0.
    const s32 cbvMode = cbvParamAnimData->mode;
    if (cbvMode != 0 && cbvMode != 1 && cbvMode != 2) {
        return false;
    }

    if (!g_animMgr || cbvParamAnimData->blendAnimUID == 0) {
        return false;
    }

    MiscAnimNode* blendNode = g_animMgr->GetMiscAnim(cbvParamAnimData->blendAnimUID);
    if (!blendNode || !blendNode->paramAnim) {
        return false;
    }

    s32 clampedFrame = frame;
    if (blendNode->paramAnim->numFrames > 0) {
        if (clampedFrame < 0) {
            clampedFrame = 0;
        }
        else if (clampedFrame >= blendNode->paramAnim->numFrames) {
            clampedFrame = blendNode->paramAnim->numFrames - 1;
        }
    }

    s32 blend16 = blendNode->paramAnim->EvaluateFrameReal(clampedFrame << 16);
    if (blend16 < 0) {
        blend16 = 0;
    }
    else if (blend16 > 0x10000) {
        blend16 = 0x10000;
    }

    const u32 rowStride = cbvParamAnimData->valueCols;
    const u32 valueCount = cbvParamAnimData->valueRows * cbvParamAnimData->valueCols;
    const u32 entryCount = cbvParamAnimData->numEntries;
    const u32 row1 = (cbvParamAnimData->valueRows > 1) ? 1u : 0u;
    static bool s_loggedCBVC1Coverage = false;
    static bool s_loggedCBVTableBounds = false;
    static bool s_loggedCBVMode2MissingPacketData = false;

    auto getDynGeoPrimPacketSize = [](u8 primCmd) -> u32 {
        switch (primCmd & 0xFCu) {
            case 0x3C:
            case 0x2C:
                return 52;
            case 0x38:
            case 0x28:
                return 36;
            case 0x34:
            case 0x24:
                return 40;
            case 0x30:
            case 0x20:
                return 28;
            default:
                return 0;
        }
    };

    auto applyPacketColourWordByOffset = [&](OriginalGeo* geo, u32 packetWordOffset, u32 packedColour) -> bool {
        if (!GeoSupportsDynamicUV(geo)
            || !geo->dynamicPrimStart
            || !geo->dynamicPrimVertCount
            || !geo->dynamicPrimCmd
            || !geo->dynamicPrimPacketOffset
            || geo->dynamicPrimCount == 0) {
            if (cbvMode == 2 && !s_loggedCBVMode2MissingPacketData) {
                LOG("[WEffect] WARN: CBV mode2 packet lane unavailable for geo 0x%08X (target 0x%08X)",
                    geo ? geo->nameCRC : 0,
                    cbvParamAnimData->targetUID);
                s_loggedCBVMode2MissingPacketData = true;
            }
            return false;
        }

        f32 decodedR = 0.0f;
        f32 decodedG = 0.0f;
        f32 decodedB = 0.0f;
        DecodePackedColour24(packedColour, &decodedR, &decodedG, &decodedB);

        auto writeCorner = [&](u32 primIndex, u32 corner) -> bool {
            const u32 count = static_cast<u32>(geo->dynamicPrimVertCount[primIndex]);
            if (corner >= count) {
                return false;
            }

            const u32 vertexIndex = geo->dynamicPrimStart[primIndex] + corner;
            if (vertexIndex >= geo->dynamicVertCount) {
                return false;
            }

            geo->dynamicVerts[vertexIndex].r = decodedR;
            geo->dynamicVerts[vertexIndex].g = decodedG;
            geo->dynamicVerts[vertexIndex].b = decodedB;
            return true;
        };

        auto writeAllCorners = [&](u32 primIndex) -> bool {
            bool wroteAny = false;
            const u32 count = static_cast<u32>(geo->dynamicPrimVertCount[primIndex]);
            for (u32 corner = 0; corner < count && corner < 4u; corner++) {
                if (writeCorner(primIndex, corner)) {
                    wroteAny = true;
                }
            }

            return wroteAny;
        };

        for (u32 primIndex = 0; primIndex < geo->dynamicPrimCount; primIndex++) {
            const u32 packetBase = geo->dynamicPrimPacketOffset[primIndex];
            const u32 packetSize = getDynGeoPrimPacketSize(geo->dynamicPrimCmd[primIndex]);
            if (packetSize == 0) {
                continue;
            }

            if (packetWordOffset < packetBase || (packetWordOffset + 4u) > (packetBase + packetSize)) {
                continue;
            }

            const u32 localOffset = packetWordOffset - packetBase;
            switch (geo->dynamicPrimCmd[primIndex] & 0xFCu) {
                case 0x3C:
                    if (localOffset == 4u) return writeCorner(primIndex, 3u);
                    if (localOffset == 16u) return writeCorner(primIndex, 2u);
                    if (localOffset == 28u) return writeCorner(primIndex, 1u);
                    if (localOffset == 40u) return writeCorner(primIndex, 0u);
                    return false;

                case 0x2C:
                    return (localOffset == 4u) ? writeAllCorners(primIndex) : false;

                case 0x38:
                    if (localOffset == 4u) return writeCorner(primIndex, 3u);
                    if (localOffset == 12u) return writeCorner(primIndex, 2u);
                    if (localOffset == 20u) return writeCorner(primIndex, 1u);
                    if (localOffset == 28u) return writeCorner(primIndex, 0u);
                    return false;

                case 0x28:
                    return (localOffset == 4u) ? writeAllCorners(primIndex) : false;

                case 0x34:
                    if (localOffset == 4u) return writeCorner(primIndex, 2u);
                    if (localOffset == 16u) return writeCorner(primIndex, 1u);
                    if (localOffset == 28u) return writeCorner(primIndex, 0u);
                    return false;

                case 0x24:
                    return (localOffset == 4u) ? writeAllCorners(primIndex) : false;

                case 0x30:
                    if (localOffset == 4u) return writeCorner(primIndex, 2u);
                    if (localOffset == 12u) return writeCorner(primIndex, 1u);
                    if (localOffset == 20u) return writeCorner(primIndex, 0u);
                    return false;

                case 0x20:
                    return (localOffset == 4u) ? writeAllCorners(primIndex) : false;

                default:
                    return false;
            }
        }

        return false;
    };

    auto applyToGeo = [&](OriginalGeo* geo) -> bool {
        if (!GeoSupportsDynamicUV(geo)) {
            return false;
        }

        bool wrote = false;
        u32 wroteCount = 0;
        u32 zeroColourCount = 0;
        for (u32 i = 0; i < entryCount; i++) {
            const u32 tableIndex0 = i;
            const u32 tableIndex1 = row1 * rowStride + i;
            if (tableIndex0 >= valueCount || tableIndex1 >= valueCount) {
                if (!s_loggedCBVTableBounds) {
                    LOG("[WEffect] WARN: CBV table bounds stop: target=0x%08X mode=%d entries=%u rows=%u cols=%u valueCount=%u at i=%u",
                        cbvParamAnimData->targetUID,
                        cbvMode,
                        entryCount,
                        cbvParamAnimData->valueRows,
                        cbvParamAnimData->valueCols,
                        valueCount,
                        i);
                    s_loggedCBVTableBounds = true;
                }
                break;
            }

            const u32 fromColour = cbvParamAnimData->values[tableIndex0];
            const u32 toColour = cbvParamAnimData->values[tableIndex1];
            const u32 mixedColour = LerpPackedColour24(fromColour, toColour, blend16);
            if ((mixedColour & 0x00FFFFFFu) == 0) {
                zeroColourCount++;
            }

            const u32 colourWordOffset = cbvParamAnimData->primOffsets[i];
            const bool wroteEntry = (cbvMode == 2)
                ? applyPacketColourWordByOffset(geo, colourWordOffset, mixedColour)
                : ApplyGeoColourWordByOffset(geo, colourWordOffset, mixedColour);
            if (wroteEntry) {
                wrote = true;
                wroteCount++;
            }
        }

        if (!s_loggedCBVC1Coverage && geo->nameCRC == 0x074A76B4u) {
            LOG("[WEffect] CBV C1A coverage: mode=%d target=0x%08X geo=0x%08X entries=%u wrote=%u dynVerts=%u zeroRGB=%u rows=%u cols=%u",
                cbvMode,
                cbvParamAnimData->targetUID,
                geo->nameCRC,
                entryCount,
                wroteCount,
                geo->dynamicVertCount,
                zeroColourCount,
                cbvParamAnimData->valueRows,
                cbvParamAnimData->valueCols);
            s_loggedCBVC1Coverage = true;
        }

        if (wrote) {
            geo->meshBuffer->SetVertexData(geo->dynamicVerts, geo->dynamicVertCount);
        }

        return wrote;
    };

    if (!model || !model->drawable) {
        return false;
    }

    bool applied = false;
    if (model->drawableType == 3) {
        DrawableETree* drawableETree = static_cast<DrawableETree*>(model->drawable);
        OriginalETree* original = drawableETree ? drawableETree->original : nullptr;
        if (!original || !original->geoParts || original->geoPartCount == 0) {
            return false;
        }

        for (u16 i = 0; i < original->geoPartCount; i++) {
            if (cbvParamAnimData->targetUID != 0 && original->geoPartHashes
                && original->geoPartHashes[i] != cbvParamAnimData->targetUID) {
                continue;
            }

            OriginalGeo* geo = original->geoParts[i];
            if (applyToGeo(geo)) {
                applied = true;
            }
        }
    }
    else if (model->drawableType == 1) {
        DrawableGeo* drawableGeo = static_cast<DrawableGeo*>(model->drawable);
        OriginalGeo* geo = drawableGeo ? drawableGeo->original : nullptr;
        if (geo && (cbvParamAnimData->targetUID == 0 || geo->nameCRC == cbvParamAnimData->targetUID)) {
            applied = applyToGeo(geo);
        }
    }

    return applied;
}

bool ComEffect::ApplyClutAnimFrame(ClutAnimData* clutAnimData, s32 frame) {
    if (!clutAnimData || !clutAnimData->frames || clutAnimData->numFrames <= 0) {
        return false;
    }

    // PSX mode==0 writes to material cba; mode!=0 patches prim packets at
    // explicit offsets, which requires source packet storage not yet mirrored.
    if (clutAnimData->mode != 0) {
        return false;
    }

    const u16 clut = clutAnimData->GetFrameValue(frame);

    auto applyMaterialToGeo = [&](OriginalGeo* geo) -> bool {
        if (!GeoSupportsDynamicUV(geo)
            || !geo->dynamicPrimMaterialUID
            || geo->dynamicPrimCount == 0) {
            return false;
        }

        bool wrote = false;
        const f32 cbaWord = static_cast<f32>(clut);

        for (u32 primIndex = 0; primIndex < geo->dynamicPrimCount; primIndex++) {
            if (clutAnimData->materialUID != 0
                && geo->dynamicPrimMaterialUID[primIndex] != clutAnimData->materialUID) {
                continue;
            }

            const u32 start = geo->dynamicPrimStart[primIndex];
            const u32 count = static_cast<u32>(geo->dynamicPrimVertCount[primIndex]);
            if (count == 0 || start >= geo->dynamicVertCount) {
                continue;
            }

            for (u32 corner = 0; corner < count; corner++) {
                const u32 vertexIndex = start + corner;
                if (vertexIndex >= geo->dynamicVertCount) {
                    break;
                }

                geo->dynamicVerts[vertexIndex].cba = cbaWord;
            }

            wrote = true;
        }

        if (wrote) {
            geo->meshBuffer->SetVertexData(geo->dynamicVerts, geo->dynamicVertCount);
        }

        return wrote;
    };

    // Mirror PSX mode==0 material write when primitive material ownership is
    // available in the retained dynamic geo data.
    if (clutAnimData->materialUID != 0 && model && model->drawable) {
        bool appliedMaterial = false;

        if (model->drawableType == 1) {
            DrawableGeo* drawableGeo = static_cast<DrawableGeo*>(model->drawable);
            OriginalGeo* geo = drawableGeo ? drawableGeo->original : nullptr;
            if (applyMaterialToGeo(geo)) {
                appliedMaterial = true;
            }
        }
        else if (model->drawableType == 3) {
            DrawableETree* drawableETree = static_cast<DrawableETree*>(model->drawable);
            OriginalETree* original = drawableETree ? drawableETree->original : nullptr;
            if (original && original->geoParts && original->geoPartCount > 0) {
                for (u16 i = 0; i < original->geoPartCount; i++) {
                    if (applyMaterialToGeo(original->geoParts[i])) {
                        appliedMaterial = true;
                    }
                }
            }
        }

        if (appliedMaterial) {
            geoSwapWordSlot = kGeoSwapWordInactive;
            return true;
        }
    }

    auto resolveFirstTexInfo = [&](u16* outTpage, u16* outCba) -> bool {
        if (outTpage) {
            *outTpage = 0;
        }
        if (outCba) {
            *outCba = 0;
        }

        auto floatToTexInfoWord = [](f32 value, u16* outWord) -> bool {
            if (value < 0.0f || value > 65535.0f) {
                return false;
            }

            if (outWord) {
                *outWord = static_cast<u16>(value);
            }

            return true;
        };

        auto resolveFromVertex = [&](const GeoRenderVertex& vertex) -> bool {
            u16 tpage = 0;
            u16 cba = 0;
            if (!floatToTexInfoWord(vertex.tpage, &tpage) || !floatToTexInfoWord(vertex.cba, &cba)) {
                return false;
            }

            if (outTpage) {
                *outTpage = tpage;
            }
            if (outCba) {
                *outCba = cba;
            }
            return true;
        };

        auto resolveFromSkinVertex = [&](const SkinVertex& vertex) -> bool {
            u16 tpage = 0;
            u16 cba = 0;
            if (!floatToTexInfoWord(vertex.tpage, &tpage) || !floatToTexInfoWord(vertex.cba, &cba)) {
                return false;
            }

            if (outTpage) {
                *outTpage = tpage;
            }
            if (outCba) {
                *outCba = cba;
            }
            return true;
        };

        if (!model || !model->drawable) {
            return false;
        }

        if (model->drawableType == 2) {
            OriginalSTree* active = GetActiveSTree(model->drawable);
            if (active && active->skinData && active->skinData->verts && active->skinData->numVerts > 0) {
                return resolveFromSkinVertex(active->skinData->verts[0]);
            }

            return false;
        }

        if (model->drawableType == 1) {
            DrawableGeo* drawableGeo = static_cast<DrawableGeo*>(model->drawable);
            OriginalGeo* geo = drawableGeo ? drawableGeo->original : nullptr;
            if (geo && geo->dynamicVerts && geo->dynamicVertCount > 0) {
                return resolveFromVertex(geo->dynamicVerts[0]);
            }

            return false;
        }

        if (model->drawableType == 3) {
            DrawableETree* drawableETree = static_cast<DrawableETree*>(model->drawable);
            OriginalETree* original = drawableETree ? drawableETree->original : nullptr;
            if (!original || !original->geoParts || original->geoPartCount == 0) {
                return false;
            }

            for (u16 i = 0; i < original->geoPartCount; i++) {
                OriginalGeo* geo = original->geoParts[i];
                if (!geo || !geo->dynamicVerts || geo->dynamicVertCount == 0) {
                    continue;
                }

                if (resolveFromVertex(geo->dynamicVerts[0])) {
                    return true;
                }
            }
        }

        return false;
    };

    u16 tpage = 0;
    if (geoSwapWordSlot != kGeoSwapWordInactive) {
        tpage = static_cast<u16>((geoSwapWordSlot >> 16) & 0xFFFFu);
    }
    else {
        u16 unusedCba = 0;
        if (!resolveFirstTexInfo(&tpage, &unusedCba)) {
            return false;
        }
    }

    geoSwapWordSlot = (static_cast<u32>(tpage) << 16) | static_cast<u32>(clut);
    return true;
}

bool ComEffect::ApplyMiscAnimFrame(MiscAnimNode* node, s32 frame, bool updateJoints) {
    if (!node) {
        return false;
    }

    if (node->anim) {
        return ComEffect_ApplyTransformAnimFrame(this, node->anim, frame, updateJoints);
    }

    if (node->vizAnim) {
        return ApplyVizAnimFrame(node->vizAnim, frame);
    }

    if (node->cbvParamAnim) {
        return ApplyCBVParamAnimFrame(node->cbvParamAnim, frame);
    }

    if (node->clutAnim) {
        return ApplyClutAnimFrame(node->clutAnim, frame);
    }

    if (node->paramAnim) {
        return true;
    }

    if (node->sequenceAnim) {
        MiscAnimNode* partNode = nullptr;
        TransformAnim* partAnim = nullptr;
        s32 partFrame = 0;
        if (!ResolveSequenceAnimFrame(node->sequenceAnim, frame, &partNode, &partAnim, &partFrame)) {
            return false;
        }

        if (partNode) {
            return ApplyMiscAnimFrame(partNode, partFrame, updateJoints);
        }

        if (partAnim) {
            return ComEffect_ApplyTransformAnimFrame(this, partAnim, partFrame, updateJoints);
        }

        return false;
    }

    if (node->compositeAnim && node->compositeAnim->parts) {
        bool applied = false;
        bool resolvedAnyPart = false;
        for (u32 i = 0; i < node->compositeAnim->numParts; i++) {
            CompositeAnimPartData& part = node->compositeAnim->parts[i];
            MiscAnimNode* partNode = ResolveCompositePartNode(part);
            if (!partNode) {
                continue;
            }

            resolvedAnyPart = true;

            const s32 partFrameCount = GetMiscAnimFrameCount(partNode);
            if (partFrameCount <= 0) {
                continue;
            }

            const u32 shift = static_cast<u32>(part.field1) & 31u;
            s32 partFrame = frame >> shift;
            if (partFrame >= partFrameCount) {
                if (part.field0 != 0) {
                    partFrame %= partFrameCount;
                }
                else {
                    partFrame = partFrameCount - 1;
                }
            }

            if (ApplyMiscAnimFrame(partNode, partFrame, updateJoints)) {
                applied = true;
            }
        }

        // PSX tCompositeFlip::Update skips null part anim pointers and still
        // treats the composite update as valid no-op. Mirror that behavior so
        // unresolved optional leaves do not become unsupported warnings.
        if (!resolvedAnyPart) {
            return true;
        }

        return applied;
    }

    if (node->frameList) {
        // Frame-list animation is consumed by SetVertexInfo paths; no joint
        // transform update is required here.
        return true;
    }

    return false;
}

bool ComEffect::LoadETree(s32 inResourceHash, s32 inMiscAnimHash) {
    MARKFUNCTION(0x8004E580);

    s32 geoCursor = -1;
    auto FindFirstGeo = [&]() -> s32 {
        geoCursor = -1;
        if (!model || !model->drawable) {
            return 0;
        }

        if (model->drawableType == 2) {
            OriginalSTree* active = GetActiveSTree(model->drawable);
            if (!active || !active->skinData || active->skinData->numPrims == 0) {
                return 0;
            }
        }

        geoCursor = 0;
        return 1;
    };

    auto FindNextGeo = [&]() -> s32 {
        if (geoCursor < 0) {
            return 0;
        }

        geoCursor = -1;
        return 0;
    };

    auto GetMiscAnim = [&](u32 hash) -> MiscAnimNode* {
        return g_animMgr ? g_animMgr->GetMiscAnim(hash) : nullptr;
    };

    if (!g_levelManager) {
        return false;
    }

    OriginalBasic* found = g_levelManager->FindETree(inResourceHash);
    if (!found || found->GetType() != 2) {
        return false;
    }

    OriginalETree* original = static_cast<OriginalETree*>(found);
    if (!original) {
        return false;
    }

    ComEffect_ResetModel(this);
    ComEffect_BindCommonState(this, inResourceHash, inMiscAnimHash);
    (void)FindScaleInfo(resourceHash);
    (void)GetMiscAnim(static_cast<u32>(inMiscAnimHash));

    TransformAnim* initialAnim = miscAnim;
    if (!initialAnim) {
        s32 resolvedFrame = 0;
        ComEffect_ResolveInitialTransform(miscAnimNode, 0, &initialAnim, &resolvedFrame);
    }
    if (initialAnim) {
        miscAnim = initialAnim;
        boundTransformAnim = initialAnim;
    }

    eModel = new EModel();
    eModel->SetOriginalETree(original, initialAnim);
    model = eModel;

    if (miscAnimNode) {
        ApplyMiscAnimFrame(miscAnimNode, 0, true);
    }

    if (FindFirstGeo()) {
        while (FindNextGeo()) {
        }
    }

    ComEffect_BindScaleDataToModel(this);

    return model && model->drawable;
}

bool ComEffect::LoadSTree(s32 inResourceHash, s32 inMiscAnimHash) {
    MARKFUNCTION(0x8004E6D4);

    auto GetMiscAnim = [&](u32 hash) -> MiscAnimNode* {
        return g_animMgr ? g_animMgr->GetMiscAnim(hash) : nullptr;
    };

    if (!g_levelManager) {
        return false;
    }

    OriginalBasic* found = g_levelManager->FindSTree(inResourceHash);
    if (!found || found->GetType() != 1) {
        return false;
    }

    OriginalSTree* original = static_cast<OriginalSTree*>(found);
    if (!original) {
        return false;
    }

    ComEffect_ResetModel(this);
    ComEffect_BindCommonState(this, inResourceHash, inMiscAnimHash);
    (void)FindScaleInfo(resourceHash);
    (void)GetMiscAnim(static_cast<u32>(inMiscAnimHash));

    TransformAnim* initialAnim = miscAnim;
    if (!initialAnim) {
        s32 resolvedFrame = 0;
        ComEffect_ResolveInitialTransform(miscAnimNode, 0, &initialAnim, &resolvedFrame);
    }
    if (initialAnim) {
        miscAnim = initialAnim;
        boundTransformAnim = initialAnim;
    }

    sModel = new SModel();
    sModel->SetOriginalSTree(original);
    if (initialAnim) {
        sModel->ApplyAnimToModelBasic(initialAnim);
    }
    model = sModel;

    if (miscAnimNode) {
        ApplyMiscAnimFrame(miscAnimNode, 0, true);
    }

    ComEffect_BindScaleDataToModel(this);

    return model && model->drawable;
}

void ComEffect::SetFrame(s32 frame) {
    MARKFUNCTION(0x8004E7F8);

    currentFrame = static_cast<s16>(frame < 0 ? -frame : frame);

    if (scaleData) {
        ScaleData_SetFrame(scaleData, currentFrame);
    }

    MiscAnimNode* liveNode = ComEffect_ResolveLiveMiscAnimNode(this);

    const bool canApplyFrame = (frameCount > 0) && (currentFrame < frameCount);

    if (canApplyFrame && liveNode && !ApplyMiscAnimFrame(liveNode, currentFrame, false) && !warnedSequenceUnsupported) {
        LOG("[WEffect] WARN: unresolved or unsupported misc anim frame set for effect hash 0x%08X (anim hash 0x%08X)",
            resourceHash,
            miscAnimHash);
        warnedSequenceUnsupported = true;
    }
}

bool ComEffect::EndOfFrame(s32 frame) const {
    MARKFUNCTION(0x8004E89C);

    if (frameCount <= 0) {
        return false;
    }

    return frame >= frameCount;
}

bool ComEffect::PointInView(const LVector& pos, s32 radius) const {
    MARKFUNCTION(0x8004E8D4);

    auto unk_80000000 = []() {
    };

    auto TransMatrix = [&](const LVector& inPos) {
        Mat4 matrix = p3d::context->GetViewMatrix();
        p3dFillTransMatrix(inPos, matrix);
    };

    auto P3DClipCodeSphere = [&](const LVector& center, s32 sphereRadius) -> s32 {
        const Mat4& view = p3d::context->GetViewMatrix();

        f32 tx = 0.0f;
        f32 ty = 0.0f;
        f32 tz = 0.0f;
        Mat4TransformPoint(view,
                           static_cast<f32>(center.x),
                           static_cast<f32>(center.y),
                           static_cast<f32>(center.z),
                           tx,
                           ty,
                           tz);

        const s32 vx = static_cast<s32>(tx);
        const s32 vy = static_cast<s32>(-ty);
        const s32 vz = static_cast<s32>(-tz);

        const ChanProjectionState port = g_display->GetChanProjectionState();

        if (vz + sphereRadius < static_cast<s32>(port.nearClip)) {
            return 1;
        }

        if (vz - sphereRadius > static_cast<s32>(port.farClip)) {
            return 1;
        }

        if (vz <= 0) {
            return 1;
        }

        const s32 sx = port.centerX
            + static_cast<s32>((static_cast<f32>(vx) * port.projectionDistanceX) / static_cast<f32>(vz));
        const s32 sy = port.centerY
            + static_cast<s32>((static_cast<f32>(vy) * port.projectionDistanceY) / static_cast<f32>(vz));

        s32 margin = 0;
        if (sphereRadius > 0) {
            margin = static_cast<s32>((static_cast<f32>(sphereRadius) * port.projectionDistanceX)
                                      / static_cast<f32>(vz));
        }

        if (sx < -margin || sx >= (port.width + margin)) {
            return 1;
        }

        if (sy < -margin || sy >= (port.height + margin)) {
            return 1;
        }

        return 0;
    };

    if (!g_display || !p3d::context) {
        return true;
    }

    unk_80000000();
    TransMatrix(pos);
    const bool inView = (P3DClipCodeSphere(pos, radius) == 0);
    unk_80000000();
    return inView;
}

void ComEffect::Render(const LVector& pos, const LVector* scale, const u16* rotation, u32 flags) {
    MARKFUNCTION(0x8004E950);

    if (!model || !model->drawable || !p3d::context) {
        return;
    }

    MiscAnimNode* liveNode = ComEffect_ResolveLiveMiscAnimNode(this);
    const bool canApplyFrame = (frameCount > 0) && (currentFrame < frameCount);
    if (canApplyFrame && liveNode && (flags & 0x80000u) == 0u) {
        if (!ApplyMiscAnimFrame(liveNode, currentFrame, true) && !warnedSequenceUnsupported) {
            LOG("[WEffect] WARN: unresolved or unsupported misc anim update for effect hash 0x%08X (anim hash 0x%08X)",
                resourceHash,
                miscAnimHash);
            warnedSequenceUnsupported = true;
        }
    }

    Mat4 world;

    const bool billboard = (flags & 0x202u) != 0u;
    if (billboard) {
        if (!g_display || !g_display->GetCamera()) {
            p3dFillTransMatrix(pos, world);
        }
        else {
            const LVector& cameraPos = g_display->GetCamera()->GetPosition();
            const Vec3 heading(
                static_cast<f32>(cameraPos.x - pos.x),
                ((flags & 0x200u) != 0u) ? 0.0f : static_cast<f32>(cameraPos.y - pos.y),
                static_cast<f32>(cameraPos.z - pos.z));

            const Vec3 up(0.0f, 1.0f, 0.0f);
            world = Mat4();
            p3dFillHeadingMatrix(heading, up, world);
            p3dFillTransMatrix(pos, world);
        }

        // PSX Render__9ComEffect billboard lane: when 0x100440 bits are set,
        // offset translation along billboard forward before applying rotations.
        if ((flags & 0x100440u) != 0u) {
            LVector forward = {
                static_cast<s32>(world.m[8] * 65536.0f),
                static_cast<s32>(world.m[9] * 65536.0f),
                static_cast<s32>(world.m[10] * 65536.0f),
            };

            s32 offsetScale = 0;
            if ((flags & 0x40u) != 0u) {
                offsetScale = -300;
            }
            else if ((flags & 0x400u) != 0u) {
                offsetScale = -150;
            }
            else if ((flags & 0x100000u) != 0u) {
                offsetScale = 300;
            }

            if (offsetScale != 0) {
                rmV3Scale(&forward, &forward, offsetScale);
                LVector offsetPos = {
                    pos.x + forward.x,
                    pos.y + forward.y,
                    pos.z + forward.z,
                };
                p3dFillTransMatrix(offsetPos, world);
            }
        }

        // PSX applies optional axis rotations after billboard setup.
        if (rotation && (flags & 0x10u) != 0u) {
            Mat4 rotY;
            p3dBuildRotMatrixY(ANGLE2RAD(rotation[1] & 0xFFFF), rotY);
            world = world * rotY;
        }

        if (rotation && (flags & 8u) != 0u) {
            Mat4 rotZ;
            p3dBuildRotMatrixZ(ANGLE2RAD(rotation[2] & 0xFFFF), rotZ);
            world = world * rotZ;
        }

        if (rotation && (flags & 0x100u) != 0u) {
            Mat4 rotX;
            p3dBuildRotMatrixX(ANGLE2RAD(rotation[0] & 0xFFFF), rotX);
            world = world * rotX;
        }
    }
    else {
        const s32 rx = (rotation && (flags & 0x100u) != 0u) ? rotation[0] : 0;
        const s32 ry = (rotation && (flags & 0x10u) != 0u) ? rotation[1] : 0;
        const s32 rz = (rotation && (flags & 8u) != 0u) ? rotation[2] : 0;

        p3dBuildRotMatrixZYX(rx, ry, rz, world);
        p3dFillTransMatrix(pos, world);
    }

    if ((flags & 4u) != 0u && scale) {
        const f32 sx = FIX16_TO_FLOAT(scale->x);
        const f32 sy = FIX16_TO_FLOAT(scale->y);
        const f32 sz = FIX16_TO_FLOAT(scale->z);

        world.m[0] *= sx;
        world.m[1] *= sx;
        world.m[2] *= sx;

        world.m[4] *= sy;
        world.m[5] *= sy;
        world.m[6] *= sy;

        world.m[8] *= sz;
        world.m[9] *= sz;
        world.m[10] *= sz;
    }

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    const bool useSwapWord = (geoSwapWordSlot != kGeoSwapWordInactive);
    if (useSwapWord) {
        p3d::context->SetTexInfoOverride(true, geoSwapWordSlot);
    }

    p3d::context->SetWorldMatrix(world);
    model->drawable->Display(flags);
    p3d::context->SetWorldMatrix(savedWorld);

    if (useSwapWord) {
        p3d::context->SetTexInfoOverride(false, 0);
    }
}

void ComEffect::Render(const Mat4& worldMatrix, u32 flags) {
    MARKFUNCTION(0x8004EE48);

    if (!model || !model->drawable || !p3d::context) {
        return;
    }

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();

    if ((flags & 0x800000u) != 0u && fastDrawCount < kMaxFastDrawEntries) {
        FastDrawEntry& entry = fastDrawEntries[fastDrawCount++];
        entry.worldMatrix = worldMatrix;
        entry.flags = flags & ~0x800000u;
        entry.frame = currentFrame;
        entry.swapWord = geoSwapWordSlot;
        return;
    }

    MiscAnimNode* liveNode = ComEffect_ResolveLiveMiscAnimNode(this);
    const bool canApplyFrame = (frameCount > 0) && (currentFrame < frameCount);
    if (canApplyFrame && liveNode) {
        if (!ApplyMiscAnimFrame(liveNode, currentFrame, true) && !warnedSequenceUnsupported) {
            LOG("[WEffect] WARN: unresolved or unsupported misc anim update for effect hash 0x%08X (anim hash 0x%08X)",
                resourceHash,
                miscAnimHash);
            warnedSequenceUnsupported = true;
        }
    }

    const bool useSwapWord = (geoSwapWordSlot != kGeoSwapWordInactive);
    if (useSwapWord) {
        p3d::context->SetTexInfoOverride(true, geoSwapWordSlot);
    }

    p3d::context->SetWorldMatrix(worldMatrix);
    model->drawable->Display(flags);
    p3d::context->SetWorldMatrix(savedWorld);

    if (useSwapWord) {
        p3d::context->SetTexInfoOverride(false, 0);
    }
}

u32 ComEffect::GetGeoCount() const {
    if (!model || !model->drawable) {
        return 0;
    }

    if (model->drawableType == 3) {
        const DrawableETree* drawable = static_cast<const DrawableETree*>(model->drawable);
        const OriginalETree* original = drawable ? drawable->original : nullptr;
        return CountRenderableETreeGeos(original);
    }

    if (model->drawableType == 2) {
        OriginalSTree* active = GetActiveSTree(model->drawable);
        return (active && active->meshBuffer) ? 1u : 0u;
    }

    if (model->drawableType == 1) {
        const DrawableGeo* drawable = static_cast<const DrawableGeo*>(model->drawable);
        return (drawable && drawable->original && drawable->original->meshBuffer) ? 1u : 0u;
    }

    return 0;
}

bool ComEffect::ResolveGeoByIndex(u32 geoIndex, OriginalGeo** outGeo, Mat4* outLocalMatrix) {
    if (outGeo) {
        *outGeo = nullptr;
    }

    if (outLocalMatrix) {
        *outLocalMatrix = Mat4();
    }

    if (!model || !model->drawable) {
        return false;
    }

    if (model->drawableType == 3) {
        DrawableETree* drawable = static_cast<DrawableETree*>(model->drawable);
        OriginalETree* original = drawable ? drawable->original : nullptr;
        if (!original) {
            return false;
        }

        u16 partIndex = 0xFFFFu;
        OriginalGeo* geo = GetRenderableETreeGeoByIndex(original, geoIndex, &partIndex);
        if (!geo) {
            return false;
        }

        if (outGeo) {
            *outGeo = geo;
        }

        if (outLocalMatrix
            && original->geoPartJointHashes
            && partIndex < original->geoPartCount
            && model->animStructure)
        {
            AnimStructure* anim = static_cast<AnimStructure*>(model->animStructure);
            TransformFlip* flip = anim ? anim->GetFlip() : nullptr;
            STreeData* skeleton = (flip && flip->tree && flip->tree->joints && flip->tree->numJoints > 0)
                ? flip->tree
                : nullptr;

            if (skeleton) {
                Mat4* partJointMatrices = new Mat4[skeleton->numJoints];
                if (partJointMatrices) {
                    skeleton->ComputeWorldMatricesWithCallbacks(partJointMatrices);
                    const Mat4* partMatrix = FindETreeJointWorldMatrixByHash(
                        skeleton,
                        partJointMatrices,
                        original->geoPartJointHashes[partIndex]);
                    if (partMatrix) {
                        *outLocalMatrix = *partMatrix;
                    }

                    delete[] partJointMatrices;
                }
            }
        }

        return true;
    }

    if (model->drawableType == 1 && geoIndex == 0u) {
        DrawableGeo* drawable = static_cast<DrawableGeo*>(model->drawable);
        OriginalGeo* geo = drawable ? drawable->original : nullptr;
        if (!geo || !geo->meshBuffer) {
            return false;
        }

        if (outGeo) {
            *outGeo = geo;
        }

        return true;
    }

    return false;
}

bool ComEffect::RenderGeoByIndex(u32 geoIndex, const Mat4& worldMatrix, u32 flags) {
    if (!model || !model->drawable || !p3d::context) {
        return false;
    }

    if (model->drawableType != 3) {
        return false;
    }

    DrawableETree* drawable = static_cast<DrawableETree*>(model->drawable);
    OriginalETree* original = drawable ? drawable->original : nullptr;
    if (!original) {
        return false;
    }

    MiscAnimNode* liveNode = ComEffect_ResolveLiveMiscAnimNode(this);
    const bool canApplyFrame = (frameCount > 0) && (currentFrame < frameCount);
    if (canApplyFrame && liveNode) {
        if (!ApplyMiscAnimFrame(liveNode, currentFrame, true) && !warnedSequenceUnsupported) {
            LOG("[WEffect] WARN: unresolved or unsupported misc anim update for effect hash 0x%08X (anim hash 0x%08X)",
                resourceHash,
                miscAnimHash);
            warnedSequenceUnsupported = true;
        }
    }

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();

    if (g_display) {
        const Mat4& view = p3d::context->GetViewMatrix();

        f32 tx = 0.0f;
        f32 ty = 0.0f;
        f32 tz = 0.0f;
        Mat4TransformPoint(view,
                           worldMatrix.m[12],
                           worldMatrix.m[13],
                           worldMatrix.m[14],
                           tx,
                           ty,
                           tz);

        const s32 vz = static_cast<s32>(-tz);
        const ChanProjectionState port = g_display->GetChanProjectionState();
        if (vz <= 0 || vz < static_cast<s32>(port.nearClip) || vz > static_cast<s32>(port.farClip)) {
            return false;
        }
    }

    bool rendered = false;

    const bool useSwapWord = (geoSwapWordSlot != kGeoSwapWordInactive);
    if (useSwapWord) {
        p3d::context->SetTexInfoOverride(true, geoSwapWordSlot);
    }

    p3d::context->SetWorldMatrix(worldMatrix);

    OriginalGeo* geo = GetRenderableETreeGeoByIndex(original, geoIndex, nullptr);
    if (geo) {
        if (geo->meshBuffer) {
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
        rendered = true;
    }
    else if (geoIndex == 0u && original->meshBuffer) {
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

        rendered = true;
    }

    p3d::context->SetWorldMatrix(savedWorld);

    if (useSwapWord) {
        p3d::context->SetTexInfoOverride(false, 0);
    }

    (void)flags;
    return rendered;
}

u32* ComEffect::GetGeoSwapWordSlot(s32 geoIndex) {
    if (geoIndex < 0) {
        return nullptr;
    }

    if (static_cast<u32>(geoIndex) >= GetGeoCount()) {
        return nullptr;
    }

    return &geoSwapWordSlot;
}

void ComEffect::InitFastRender() {
    MARKFUNCTION(0x8004F288);
    fastDrawCount = 0;
}

void ComEffect::DoFastRender() {
    MARKFUNCTION(0x8004F318);

    if (fastDrawCount == 0) {
        return;
    }

    if (!p3d::context) {
        fastDrawCount = 0;
        return;
    }

    const Mat4& view = p3d::context->GetViewMatrix();

    for (u32 i = 0; i + 1 < fastDrawCount; i++) {
        u32 bestIndex = i;

        f32 bestTx = 0.0f;
        f32 bestTy = 0.0f;
        f32 bestTz = 0.0f;
        Mat4TransformPoint(view,
                           fastDrawEntries[bestIndex].worldMatrix.m[12],
                           fastDrawEntries[bestIndex].worldMatrix.m[13],
                           fastDrawEntries[bestIndex].worldMatrix.m[14],
                           bestTx,
                           bestTy,
                           bestTz);
        f32 bestDepth = -bestTz;

        for (u32 j = i + 1; j < fastDrawCount; j++) {
            f32 tx = 0.0f;
            f32 ty = 0.0f;
            f32 tz = 0.0f;
            Mat4TransformPoint(view,
                               fastDrawEntries[j].worldMatrix.m[12],
                               fastDrawEntries[j].worldMatrix.m[13],
                               fastDrawEntries[j].worldMatrix.m[14],
                               tx,
                               ty,
                               tz);

            const f32 depth = -tz;
            if (depth > bestDepth) {
                bestDepth = depth;
                bestIndex = j;
            }
        }

        if (bestIndex != i) {
            const FastDrawEntry temp = fastDrawEntries[i];
            fastDrawEntries[i] = fastDrawEntries[bestIndex];
            fastDrawEntries[bestIndex] = temp;
        }
    }

    const u32 drawCount = fastDrawCount;
    fastDrawCount = 0;
    const u32 savedSwapWord = geoSwapWordSlot;

    for (u32 i = 0; i < drawCount; i++) {
        geoSwapWordSlot = fastDrawEntries[i].swapWord;
        SetFrame(fastDrawEntries[i].frame);
        Render(fastDrawEntries[i].worldMatrix, fastDrawEntries[i].flags);
    }

    geoSwapWordSlot = savedSwapWord;
}

void ComEffect::SetUpUVlists() {
    MARKFUNCTION(0x8004DEE8);

    if (uvBaseWords || uvGeoBaseWords) {
        return;
    }

    SkinData* skin = nullptr;
    if (model && model->drawable) {
        OriginalSTree* active = GetActiveSTree(model->drawable);
        if (active) {
            skin = active->skinData;
        }
    }
    if (skin && skin->verts && skin->numPrims > 0 && skin->primStart && skin->primVertCount) {
        uvPrimCount = skin->numPrims;
        uvBaseWords = new u16[uvPrimCount * 4u];
        if (!uvBaseWords) {
            uvPrimCount = 0;
            return;
        }

        for (u32 primIndex = 0; primIndex < uvPrimCount; primIndex++) {
            const u32 start = skin->primStart[primIndex];
            const u32 count = static_cast<u32>(skin->primVertCount[primIndex]);

            for (u32 corner = 0; corner < 4u; corner++) {
                u16 packed = 0;
                if (corner < count && (start + corner) < skin->numVerts) {
                    const SkinVertex& vertex = skin->verts[start + corner];
                    packed = static_cast<u16>(static_cast<u8>(vertex.u)
                                              | (static_cast<u16>(static_cast<u8>(vertex.v)) << 8));
                }

                uvBaseWords[primIndex * 4u + corner] = packed;
            }
        }

        return;
    }

    if (!model || !model->drawable) {
        return;
    }

    if (model->drawableType == 3) {
        DrawableETree* drawable = static_cast<DrawableETree*>(model->drawable);
        OriginalETree* original = drawable ? drawable->original : nullptr;
        if (original) {
            const u32 renderableCount = CountRenderableETreeGeos(original);
            u16 dynamicGeoCount = 0;
            for (u32 geoIndex = 0; geoIndex < renderableCount; geoIndex++) {
                OriginalGeo* geo = GetRenderableETreeGeoByIndex(original, geoIndex, nullptr);
                if (GeoSupportsDynamicUV(geo)) {
                    dynamicGeoCount++;
                }
            }

            if (dynamicGeoCount > 0) {
                uvGeoBaseWords = new u16*[dynamicGeoCount]();
                uvGeoList = new OriginalGeo*[dynamicGeoCount]();
                if (!uvGeoBaseWords || !uvGeoList) {
                    ComEffect_ClearUVCache(this);
                    return;
                }

                u16 cacheIndex = 0;
                for (u32 geoIndex = 0; geoIndex < renderableCount; geoIndex++) {
                    OriginalGeo* geo = GetRenderableETreeGeoByIndex(original, geoIndex, nullptr);
                    if (!GeoSupportsDynamicUV(geo)) {
                        continue;
                    }

                    u16* baseWords = new u16[geo->dynamicPrimCount * 4u];
                    if (!baseWords) {
                        continue;
                    }

                    CaptureGeoBaseUVWords(geo, baseWords);
                    uvGeoBaseWords[cacheIndex] = baseWords;
                    uvGeoList[cacheIndex] = geo;
                    cacheIndex++;
                }

                uvGeoCount = cacheIndex;
                if (uvGeoCount > 0) {
                    return;
                }

                ComEffect_ClearUVCache(this);
            }
        }
    }
    else if (model->drawableType == 1) {
        DrawableGeo* drawable = static_cast<DrawableGeo*>(model->drawable);
        OriginalGeo* geo = drawable ? drawable->original : nullptr;
        if (GeoSupportsDynamicUV(geo)) {
            uvGeoBaseWords = new u16*[1]();
            uvGeoList = new OriginalGeo*[1]();
            if (!uvGeoBaseWords || !uvGeoList) {
                ComEffect_ClearUVCache(this);
                return;
            }

            uvGeoBaseWords[0] = new u16[geo->dynamicPrimCount * 4u];
            if (!uvGeoBaseWords[0]) {
                ComEffect_ClearUVCache(this);
                return;
            }

            CaptureGeoBaseUVWords(geo, uvGeoBaseWords[0]);
            uvGeoList[0] = geo;
            uvGeoCount = 1;
            return;
        }
    }

    if (!warnedUVUnsupported) {
        LOG("[WEffect] WARN: ComEffect UV setup unsupported for active geo path");
        warnedUVUnsupported = true;
    }
}

void ComEffect::AddUV(const WEffectUVData* uvData, u16 uOffset, u16 vOffset) {
    MARKFUNCTION(0x8004E028);

    if (!uvData) {
        return;
    }

    const u16 frameOffset = static_cast<u16>(static_cast<u8>(uOffset)
                                             | (static_cast<u16>(static_cast<u8>(vOffset)) << 8));
    u16 addWords[4] = {};
    for (u32 corner = 0; corner < 4u; corner++) {
        const u16 base = static_cast<u16>(static_cast<u8>(uvData->baseUVWords[corner * 2u + 0u])
                                          | (static_cast<u16>(static_cast<u8>(uvData->baseUVWords[corner * 2u + 1u]))
                                             << 8));
        addWords[corner] = static_cast<u16>(base + frameOffset);
    }

    if (uvBaseWords) {
        SkinData* skin = nullptr;
        if (model && model->drawable) {
            OriginalSTree* active = GetActiveSTree(model->drawable);
            if (active) {
                skin = active->skinData;
            }
        }
        if (!skin || !skin->verts || !skin->primStart || !skin->primVertCount || skin->numPrims != uvPrimCount) {
            return;
        }

        for (u32 primIndex = 0; primIndex < uvPrimCount; primIndex++) {
            const u32 start = skin->primStart[primIndex];
            const u32 count = static_cast<u32>(skin->primVertCount[primIndex]);

            for (u32 corner = 0; corner < count && corner < 4u; corner++) {
                const u16 baseWord = uvBaseWords[primIndex * 4u + corner];
                const u16 finalWord = static_cast<u16>(baseWord + addWords[corner]);

                if ((start + corner) < skin->numVerts) {
                    skin->verts[start + corner].u = static_cast<f32>(finalWord & 0xFFu);
                    skin->verts[start + corner].v = static_cast<f32>((finalWord >> 8) & 0xFFu);
                }
            }
        }

        return;
    }

    if (!uvGeoBaseWords || !uvGeoList || uvGeoCount == 0) {
        return;
    }

    for (u16 geoIndex = 0; geoIndex < uvGeoCount; geoIndex++) {
        OriginalGeo* geo = uvGeoList[geoIndex];
        u16* baseWords = uvGeoBaseWords[geoIndex];
        if (!GeoSupportsDynamicUV(geo) || !baseWords) {
            continue;
        }

        ApplyGeoUVWords(geo, baseWords, addWords);
    }
}

void ComEffect::SetUpVertexlists() {
    MARKFUNCTION(0x8004E140);

    auto GetPart = [&](u32 partIndex) -> MiscAnimNode* {
        if (!compositeAnim || !compositeAnim->parts || partIndex >= compositeAnim->numParts) {
            return nullptr;
        }

        return ResolveCompositePartNode(compositeAnim->parts[partIndex]);
    };

    auto GetVertexFrame = [](FrameListAnim* list, s32 frameIndex) -> const s16* {
        return list ? list->GetFrame(frameIndex) : nullptr;
    };

    if (vertexAnimInfos) {
        return;
    }

    SkinData* skin = nullptr;
    if (model && model->drawable) {
        OriginalSTree* active = GetActiveSTree(model->drawable);
        if (active) {
            skin = active->skinData;
        }
    }
    if (!skin || !skin->verts || skin->numVerts == 0) {
        if (!warnedVertexUnsupported) {
            LOG("[WEffect] WARN: ComEffect vertex setup requires active STree skin data");
            warnedVertexUnsupported = true;
        }
        return;
    }

    VertexAnimInfo infoBuffer[32] = {};
    u16 infoCount = 0;

    auto addFromFrameList = [&](FrameListAnim* list) {
        if (!list || list->numFrames <= 0 || infoCount >= 32) {
            return;
        }

        const s32 nextFrame = (list->numFrames > 1) ? 1 : 0;
        const s16* frame0 = GetVertexFrame(list, 0);
        const s16* frame1 = GetVertexFrame(list, nextFrame);
        if (!frame0 || !frame1) {
            return;
        }

        u32 vertCount = list->GetFrameVertexCount(0);
        const u32 frame1Count = list->GetFrameVertexCount(nextFrame);
        if (frame1Count < vertCount) {
            vertCount = frame1Count;
        }

        for (u32 index = 0; index < vertCount && infoCount < 32; index++) {
            const s16* v0 = frame0 + index * 4u;
            const s16* v1 = frame1 + index * 4u;

            const s16 dx = static_cast<s16>(v1[0] - v0[0]);
            const s16 dy = static_cast<s16>(v1[1] - v0[1]);
            const s16 dz = static_cast<s16>(v1[2] - v0[2]);

            if ((dx < 0 ? -dx : dx) < 6
                && (dy < 0 ? -dy : dy) < 6
                && (dz < 0 ? -dz : dz) < 6)
            {
                continue;
            }

            bool exists = false;
            for (u16 existing = 0; existing < infoCount; existing++) {
                if (infoBuffer[existing].sourceIndex == index) {
                    exists = true;
                    break;
                }
            }
            if (exists) {
                continue;
            }

            VertexAnimInfo& info = infoBuffer[infoCount++];
            info.sourceIndex = static_cast<u16>(index);
            info.baseY = v0[1];
            info.deltaY = dy;
        }
    };

    if (compositeAnim) {
        for (u32 partIndex = 0; partIndex < compositeAnim->numParts && infoCount < 32; partIndex++) {
            MiscAnimNode* partNode = GetPart(partIndex);
            if (partNode && partNode->frameList) {
                addFromFrameList(partNode->frameList);
            }
        }
    }
    else if (frameListAnim) {
        addFromFrameList(frameListAnim);
    }

    if (infoCount == 0) {
        if (!warnedVertexUnsupported) {
            LOG("[WEffect] WARN: no vertex-flip frame lists resolved for effect hash 0x%08X", resourceHash);
            warnedVertexUnsupported = true;
        }
        return;
    }

    vertexAnimInfos = new VertexAnimInfo[infoCount];
    for (u16 i = 0; i < infoCount; i++) {
        vertexAnimInfos[i] = infoBuffer[i];
    }
    vertexAnimInfoCount = infoCount;
}

void ComEffect::SetVertexInfo(s16 frame, s16 speed) {
    MARKFUNCTION(0x8004E3C8);

    if (!vertexAnimInfos || vertexAnimInfoCount == 0) {
        return;
    }

    SkinData* skin = nullptr;
    if (model && model->drawable) {
        OriginalSTree* active = GetActiveSTree(model->drawable);
        if (active) {
            skin = active->skinData;
        }
    }
    if (!skin || !skin->verts || skin->numVerts == 0) {
        return;
    }

    const s16 phase = static_cast<s16>(456 * frame);
    const s32 sinPrimary = rmSin16(static_cast<s16>(frame * speed));
    const s32 sinAlternate = rmSin16(static_cast<s16>(phase + 0x4000));

    for (u16 infoIndex = 0; infoIndex < vertexAnimInfoCount; infoIndex++) {
        const VertexAnimInfo& info = vertexAnimInfos[infoIndex];
        const s32 phaseSin = ((infoIndex & 1u) != 0u) ? sinPrimary : sinAlternate;
        const s16 y = static_cast<s16>(((static_cast<s32>(info.deltaY) * phaseSin) >> 16) + info.baseY);

        for (u32 vertIndex = 0; vertIndex < skin->numVerts; vertIndex++) {
            SkinVertex& vertex = skin->verts[vertIndex];
            if (vertex.sourceIndex == info.sourceIndex) {
                vertex.ly = static_cast<f32>(y);
            }
        }
    }
}

u32 ComEffect::GetClut(s32 /*mode*/) {
    MARKFUNCTION(0x8004DE40);

    auto resolveFirstTexInfo = [&](u16* outTpage, u16* outCba) -> bool {
        if (outTpage) {
            *outTpage = 0;
        }
        if (outCba) {
            *outCba = 0;
        }

        auto floatToTexInfoWord = [](f32 value, u16* outWord) -> bool {
            if (value < 0.0f || value > 65535.0f) {
                return false;
            }

            if (outWord) {
                *outWord = static_cast<u16>(value);
            }

            return true;
        };

        auto resolveFromVertex = [&](const GeoRenderVertex& vertex) -> bool {
            u16 tpage = 0;
            u16 cba = 0;
            if (!floatToTexInfoWord(vertex.tpage, &tpage) || !floatToTexInfoWord(vertex.cba, &cba)) {
                return false;
            }

            if (outTpage) {
                *outTpage = tpage;
            }
            if (outCba) {
                *outCba = cba;
            }
            return true;
        };

        auto resolveFromSkinVertex = [&](const SkinVertex& vertex) -> bool {
            u16 tpage = 0;
            u16 cba = 0;
            if (!floatToTexInfoWord(vertex.tpage, &tpage) || !floatToTexInfoWord(vertex.cba, &cba)) {
                return false;
            }

            if (outTpage) {
                *outTpage = tpage;
            }
            if (outCba) {
                *outCba = cba;
            }
            return true;
        };

        if (!model || !model->drawable) {
            return false;
        }

        if (model->drawableType == 2) {
            OriginalSTree* active = GetActiveSTree(model->drawable);
            if (active && active->skinData && active->skinData->verts && active->skinData->numVerts > 0) {
                return resolveFromSkinVertex(active->skinData->verts[0]);
            }

            return false;
        }

        if (model->drawableType == 1) {
            DrawableGeo* drawableGeo = static_cast<DrawableGeo*>(model->drawable);
            OriginalGeo* geo = drawableGeo ? drawableGeo->original : nullptr;
            if (geo && geo->dynamicVerts && geo->dynamicVertCount > 0) {
                return resolveFromVertex(geo->dynamicVerts[0]);
            }

            return false;
        }

        if (model->drawableType == 3) {
            DrawableETree* drawableETree = static_cast<DrawableETree*>(model->drawable);
            OriginalETree* original = drawableETree ? drawableETree->original : nullptr;
            if (!original || !original->geoParts || original->geoPartCount == 0) {
                return false;
            }

            for (u16 i = 0; i < original->geoPartCount; i++) {
                OriginalGeo* geo = original->geoParts[i];
                if (!geo || !geo->dynamicVerts || geo->dynamicVertCount == 0) {
                    continue;
                }

                if (resolveFromVertex(geo->dynamicVerts[0])) {
                    return true;
                }
            }
        }

        return false;
    };

    if (geoSwapWordSlot != kGeoSwapWordInactive) {
        return static_cast<u16>(geoSwapWordSlot & 0xFFFFu);
    }

    u16 cba = 0;
    if (resolveFirstTexInfo(nullptr, &cba)) {
        return cba;
    }

    return 0x4000;
}

void ComEffect::SetZFar() {
    MARKFUNCTION(0x8004E4D4);

    s32 geoCursor = -1;
    auto FindFirstGeo = [&]() -> s32 {
        geoCursor = -1;
        if (!model || !model->drawable) {
            return 0;
        }

        geoCursor = 0;
        return 1;
    };

    auto FindNextGeo = [&]() -> s32 {
        if (geoCursor < 0) {
            return 0;
        }

        geoCursor = -1;
        return 0;
    };

    if (FindFirstGeo()) {
        while (FindNextGeo()) {
        }
    }
}

s32 CBVEffect_CreateForHash(u32 effectHash) {
    MARKFUNCTION(0x8008CD74);

    for (ccMinNode* node = g_wEffectPool.head; node; node = node->next) {
        Effects* effect = static_cast<Effects*>(static_cast<ccNode*>(node));
        if (effect->nameCRC == effectHash && effect->effectType == 6) {
            CBVEffect* cbvEffect = static_cast<CBVEffect*>(effect);
            cbvEffect->Create2();
            g_wEffectPool.RemNode(cbvEffect);
            Effects_AddEffect(cbvEffect, 0);
            return 1;
        }
    }

    return 0;
}

CBVEffect::CBVEffect() {
    MARKFUNCTION(0x8008CE18);
}

CBVEffect::~CBVEffect() {
    MARKFUNCTION(0x8008CE70);
}

s32 CBVEffect::Create() {
    MARKFUNCTION(0x8008CE98);

    if (disabledOnCreate) {
        return 0;
    }

    cbvData = FindCBVPrimInfo(hash);
    if (!cbvData) {
        return 0;
    }

    cbvData->Init(frameCount, frameCountMin, fixedColour, restartDelay, holdDelay, mode);
    return 1;
}

s32 CBVEffect::Create2() {
    MARKFUNCTION(0x8008CF24);

    cbvData = FindCBVPrimInfo(hash);
    if (!cbvData) {
        return 0;
    }

    return cbvData->Init(frameCount, frameCountMin, fixedColour, restartDelay, holdDelay, mode);
}

s32 CBVEffect::Update() {
    MARKFUNCTION(0x8008CF94);

    if (cbvData) {
        return cbvData->Update();
    }

    return 0;
}

s32 CBVEffect::PutBackEffect() {
    MARKFUNCTION(0x8008CFC4);

    g_wEffectPool.AddNode(g_wEffectPool.tail, this);
    if (cbvData) {
        cbvData->Release();
        cbvData = nullptr;
    }

    return 0;
}

void CBVEffect::Display(s32 /*blockNum*/) {
    MARKFUNCTION(0x8008D094);
}

WEffect::WEffect() {
    MARKFUNCTION(0x8008B538);
}

SpotLight::SpotLight() {
    MARKFUNCTION(0x800BE270);
}

SpotLight::~SpotLight() {
    MARKFUNCTION(0x800BE2A4);
}

FWEffect::FWEffect() {
    MARKFUNCTION(0x8008BE44);
}

FWEffect::~FWEffect() {
    MARKFUNCTION(0x8008BE88);

    if (scaleRoll) {
        delete[] scaleRoll;
        scaleRoll = nullptr;
    }
}

LensFlare::LensFlare() {
    MARKFUNCTION(0x800BEE3C);

    trackingFlags = 0x800;
}

LensFlare::~LensFlare() {
    MARKFUNCTION(0x800BEE90);

    if (flareComEffects) {
        delete[] flareComEffects;
        flareComEffects = nullptr;
    }

    clampColourEntry = nullptr;

    if (pathNodes) {
        delete[] pathNodes;
        pathNodes = nullptr;
    }

    pathNodeCount = 0;
}

WEffect::~WEffect() {
    MARKFUNCTION(0x8008B5B0);

    if (uvData) {
        delete uvData;
        uvData = nullptr;
    }

    if (paletteData) {
        delete paletteData;
        paletteData = nullptr;
    }

    if (pathInfo) {
        delete pathInfo;
        pathInfo = nullptr;
    }

    ReleaseSound();
}

s32 WEffect::CreateSound(const LVector* posOverride) {
    MARKFUNCTION(0x8008B67C);

    if (sound) {
        return 0;
    }

    u32 soundId = 0;
    if (comEffect) {
        soundId = comEffect->resourceHash;
    }

    CSound* createdSound = nullptr;
    s32 result = CSoundFactory::CreateObject(10010, &createdSound, soundId);
    if (result >= 0 && createdSound) {
        sound = static_cast<CWorldEffectSound*>(createdSound);

        const LVector* initPos = posOverride;
        if (!initPos) {
            initPos = &pos;
        }

        return sound->Initialize(initPos);
    }

    return result;
}

s32 WEffect::UpdateSound() {
    MARKFUNCTION(0x8008B6F0);

    if (sound) {
        sound->Update(static_cast<u32>(static_cast<u16>(frame)));
    }

    return 0;
}

s32 WEffect::ReleaseSound() {
    MARKFUNCTION(0x8008B728);

    if (sound) {
        sound->Release();
        sound = nullptr;
    }

    return 0;
}

s32 WEffect::Create() {
    MARKFUNCTION(0x8008B850);

    if (!active) {
        return 0;
    }

    frameCounter = 0;
    frame = 0;
    vertexFrame = 0;

    if (paletteData) {
        paletteData->InitPalette();
    }

    if (pathInfo) {
        pathInfo->Reset();

        const LVector* pathPos = pathInfo->GetPosition();
        if (pathPos) {
            pos = *pathPos;
        }

        const LVector* pathRot = pathInfo->GetRotation();
        if (pathRot) {
            rotation[0] = static_cast<u16>(pathRot->x);
            rotation[1] = static_cast<u16>(pathRot->y);
            rotation[2] = static_cast<u16>(pathRot->z);
        }

        if (g_blockManager) {
            blockNum = static_cast<s32>(g_blockManager->GetBlockNumber(pos));
        }
    }

    CreateSound(nullptr);

    return 1;
}

s32 WEffect::Update() {
    MARKFUNCTION(0x8008BA24);

    if (triggerFWHash && g_ai) {
        for (ccMinNode* node = g_ai->humanoidList.head; node; node = node->next) {
            Thing* thing = static_cast<Thing*>(static_cast<ccNode*>(node));
            const u32 thingFlags = thing->flags;
            const bool hasModel = ((thingFlags >> 6) & 1u) != 0u;
            const bool needsActivation = ((thingFlags >> 2) & 1u) != 0u;

            if (!hasModel || !needsActivation) {
                continue;
            }

            const LVector triggerPos = thing->pos;
            if (triggerPos.y >= pos.y || static_cast<s32>(thing->blockNum) != blockNum) {
                continue;
            }

            FWEffect* triggerEffect = FWEffect::Find(triggerFWHash);
            if (!triggerEffect || triggerEffect->blockNum != blockNum) {
                continue;
            }

            FWEffect::Create2(triggerFWHash, &triggerPos, nullptr, nullptr, 4);
            thing->flags &= ~TF_NEEDS_ACTIVATION;
        }
    }

    const s16 previousCounter = frameCounter;
    frameCounter = static_cast<s16>(frameCounter + 1);

    if (previousCounter == frameDelay) {
        frameCounter = 0;
        frame = static_cast<s16>(frame + 1);

        if (comEffect && comEffect->EndOfFrame(frame)) {
            frame = 0;
        }

        if (vertexUseGlobalFrame) {
            vertexFrame = static_cast<s16>(g_wEffectGlobalFrame);
        }
        else {
            vertexFrame = static_cast<s16>(vertexFrame + 1);
        }

        if (paletteData) {
            paletteData->Update();
            if (paletteData->NextFrame()) {
                paletteData->InitPalette();
            }
        }

        if (pathInfo) {
            bool shouldDeactivate = false;
            if (pathInfo->Update()) {
                shouldDeactivate = (pathInfo->state == 0);
            }

            if (shouldDeactivate) {
                active = 0;
                Effects_RemoveEffect(this);
                g_wEffectPool.AddNode(g_wEffectPool.tail, this);
            }

            const LVector* pathPos = pathInfo->GetPosition();
            if (pathPos) {
                pos = *pathPos;
            }

            const LVector* pathRot = pathInfo->GetRotation();
            if (pathRot) {
                rotation[0] = static_cast<u16>(pathRot->x);
                rotation[1] = static_cast<u16>(pathRot->y);
                rotation[2] = static_cast<u16>(pathRot->z);
            }

            if (g_blockManager) {
                blockNum = static_cast<s32>(g_blockManager->GetBlockNumber(pos));
            }
        }

        if (uvData) {
            uvData->accumU = static_cast<u16>((uvData->accumU + uvData->stepU) & uvData->maskU);
            uvData->accumV = static_cast<u16>((uvData->accumV + uvData->stepV) & uvData->maskV);
        }
    }

    return UpdateSound();
}

void WEffect::Display(s32 inBlockNum) {
    MARKFUNCTION(0x8008BD28);

    if (!comEffect) {
        return;
    }

    if (blockNum != inBlockNum) {
        return;
    }

    if (clipDistance >= 0 && !comEffect->PointInView(pos, clipDistance)) {
        return;
    }

    if (uvData) {
        comEffect->AddUV(uvData, uvData->accumU, uvData->accumV);
    }

    if (vertexEnabled) {
        comEffect->SetVertexInfo(vertexFrame, vertexSpeed);
    }

    comEffect->SetFrame(frame);

    const LVector* scalePtr = hasScale ? &scale : nullptr;
    comEffect->Render(pos, scalePtr, rotation, renderFlags);

    if (paletteData) {
        paletteData->TransferVram();
    }
}

s32 WEffect::PutBackEffect() {
    MARKFUNCTION(0x8008B988);
    g_wEffectPool.AddNode(g_wEffectPool.tail, this);
    return ReleaseSound();
}

s32 SpotLight::Create() {
    MARKFUNCTION(0x800BE2CC);

    visible = 1;
    zOffset = 0;
    basePos = pos;

    linkedEffect = WEffect::Find(linkedEffectHash);

    renderFlags &= ~0x10u;
    rotation[1] = 0x8000;
    rotation[2] = 0;

    return linkedEffect ? 1 : 0;
}

s32 SpotLight::Update() {
    MARKFUNCTION(0x800BE344);

    if (!linkedEffect) {
        return 0;
    }

    visible = 1;

    pos = linkedEffect->pos;
    pos.y = basePos.y;

    if (g_blockManager) {
        blockNum = static_cast<s32>(g_blockManager->GetBlockNumber(pos));
    }

    if (uvData) {
        s32 vOffset = static_cast<s32>(uvData->maskV);

        zOffset = 0;
        renderFlags &= ~0x10u;

        const s32 deltaUpper = pos.z - (basePos.z + static_cast<s32>(followRange) - 1024);
        const s32 deltaLower = basePos.z - static_cast<s32>(followRange) + 1024 - pos.z;

        if (deltaUpper < 0 && deltaLower < 0) {
            vOffset = 0;
        }
        else if (deltaUpper <= 0) {
            if (deltaLower > 0) {
                if (deltaLower < 2048) {
                    vOffset = -(deltaLower >> 5);
                    zOffset = deltaLower;
                }
                else {
                    visible = 0;
                }
            }
        }
        else if (deltaUpper < 2048) {
            vOffset = -(deltaUpper >> 5);
            zOffset = -deltaUpper;
            renderFlags |= 0x10u;
        }
        else {
            visible = 0;
        }

        uvData->accumV = static_cast<u16>(vOffset);
    }

    return blockNum;
}

void SpotLight::Display(s32 inBlockNum) {
    MARKFUNCTION(0x800BE450);

    if (!visible || blockNum != inBlockNum || !comEffect) {
        return;
    }

    if (uvData) {
        comEffect->AddUV(uvData, uvData->accumU, uvData->accumV);
    }

    comEffect->SetFrame(frame);

    LVector renderPos = pos;
    renderPos.z += zOffset;

    const LVector* scalePtr = hasScale ? &scale : nullptr;
    comEffect->Render(renderPos, scalePtr, rotation, renderFlags);
}

void SpotLight::SetUp(u32 effectHash, u32 range, u32 linkedHash) {
    MARKFUNCTION(0x800BE510);

    setupValue = effectHash;
    followRange = range;
    linkedEffectHash = linkedHash;
}

s32 LensFlare::InitLensFlare(s32 mode, DBPath* path) {
    MARKFUNCTION(0x800BF000);

    s32 initResult = (mode != 0) ? 1 : 0;

    if (flareComEffects) {
        delete[] flareComEffects;
        flareComEffects = nullptr;
    }

    flareComEffects = new ComEffect[3]();

    if (mode == 0 && flareComEffects) {
        bool loaded = false;
        if ((flareComEffects[0].LoadETree(362594, 0) || flareComEffects[0].LoadSTree(362594, 0))
            && (flareComEffects[1].LoadETree(43500977, 0) || flareComEffects[1].LoadSTree(43500977, 0)))
        {
            loaded = flareComEffects[2].LoadETree(289117, 0) || flareComEffects[2].LoadSTree(289117, 0);
        }

        if (loaded) {
            initResult = 1;

            ComEffect* geo = &flareComEffects[1];
            clampColourEntry = geo ? &colourPrimary : nullptr;

            frameScalePrimary = 0;
            frameScaleSecondary = 0;
            streakPitch = 0;
            streakYaw = 5461;
            hasSecondaryGlow = 1;
        }
        else {
            hasSecondaryGlow = 0;
            clampColourEntry = nullptr;
        }
    }
    else {
        clampColourEntry = nullptr;
    }

    if (pathNodes) {
        delete[] pathNodes;
        pathNodes = nullptr;
    }

    pathNodeCount = 0;
    pathNodeIndex = 0;

    if (!path || path->pointCount == 0) {
        return initResult;
    }

    pathNodeCount = static_cast<s32>(path->pointCount);
    pathNodes = new PathNode[pathNodeCount]();
    if (!pathNodes) {
        pathNodeCount = 0;
        return initResult;
    }

    bool reverse = true;
    DBPoint* firstPoint = static_cast<DBPoint*>(path->points.GetFirst());
    if (firstPoint && firstPoint->FindAttrib(4)) {
        reverse = false;
    }

    DBPoint* current = reverse
        ? static_cast<DBPoint*>(path->points.GetLast())
        : firstPoint;

    for (s32 index = 0; index < pathNodeCount && current; index++) {
        pathNodes[index].pos = current->pos;

        const DBAttrib* scaleAttrib = current->FindAttrib(10);
        if (scaleAttrib) {
            pathNodes[index].scale = static_cast<u16>(scaleAttrib->value);
        }
        else {
            pathNodes[index].scale = 1024;
        }

        const DBAttrib* clampAttrib = current->FindAttrib(11);
        if (clampAttrib) {
            pathNodes[index].clamp = static_cast<u16>(clampAttrib->value);
        }
        else {
            pathNodes[index].clamp = 0;
        }

        current = reverse
            ? static_cast<DBPoint*>(current->prev)
            : static_cast<DBPoint*>(current->next);
    }

    return initResult;
}

u32 LensFlare::BigScreenGlow() {
    MARKFUNCTION(0x800BF244);

    return 0;
}

s32 LensFlare::ComputeTracking(LVector& from, LVector& to) {
    MARKFUNCTION(0x800BF45C);

    if (!pathNodes || pathNodeCount <= 0) {
        return 0;
    }

    const s32 dz = from.z - to.z;
    if (dz == 0) {
        return 0;
    }

    const s32 t = rmDiv16i(pathNodes[0].pos.z - to.z, dz);
    if ((u32)t > 0x10000u) {
        return 0;
    }

    const s32 xOnRay = to.x + static_cast<s32>((static_cast<s64>(t) * static_cast<s64>(from.x - to.x)) >> 16);
    if (xOnRay < pathNodes[0].pos.x) {
        return 0;
    }

    u16 clamp = 0;
    for (s32 i = 0; i < pathNodeCount; i++) {
        if (pathNodes[i].pos.x >= xOnRay) {
            break;
        }

        clamp = pathNodes[i].clamp;
    }

    return clamp ? (1 << 11) : 0;
}

s32 SpotLight::PutBackEffect() {
    MARKFUNCTION(0x800BE51C);

    g_wEffectPool.AddNode(g_wEffectPool.tail, this);
    return 0;
}

s32 WEffect::IsDone(s32& doneMask) {
    MARKFUNCTION(0x8008B9C4);

    doneMask &= isMentorTarget;
    if (mentor) {
        return mentor->IsDone(doneMask);
    }

    return doneMask;
}

void WEffect::NISRemoveEffect() {
    MARKFUNCTION(0x8008BE08);

    active = 0;
    Effects_RemoveEffect(this);
    g_wEffectPool.AddNode(g_wEffectPool.tail, this);
}

bool WEffect::IsDirectorOverlay() const {
    return (renderFlags & 0x800u) != 0;
}

void WEffect_LoadChunk(const u8* body, u32 bodySize) {
    MARKFUNCTION(0x8008A708);

    if (!body || bodySize < 8) {
        return;
    }

    const u8* cursor = body;
    const u32 resourceHash = static_cast<u32>(cursor[0] | (cursor[1] << 8) | (cursor[2] << 16) | (cursor[3] << 24));
    cursor += 4;
    const u32 miscAnimHash = static_cast<u32>(cursor[0] | (cursor[1] << 8) | (cursor[2] << 16) | (cursor[3] << 24));

    if (g_wEffectComEffectCount >= 64) {
        return;
    }

    ComEffect* effect = GEffect_FindEffect(resourceHash);
    if (effect) {
        g_wEffectComEffects[g_wEffectComEffectCount++] = effect;
        return;
    }

    effect = new ComEffect();
    const s32 comIndex = g_wEffectComEffectCount;
    const s32 ownedIndex = g_wEffectOwnedCount;
    g_wEffectComEffects[g_wEffectComEffectCount++] = effect;
    g_wEffectOwnedEffects[g_wEffectOwnedCount++] = effect;

    if (!effect->LoadETree(static_cast<s32>(resourceHash), static_cast<s32>(miscAnimHash))
        && !effect->LoadSTree(static_cast<s32>(resourceHash), static_cast<s32>(miscAnimHash))) {
        delete effect;
        g_wEffectComEffects[comIndex] = nullptr;
        g_wEffectOwnedEffects[ownedIndex] = nullptr;
        g_wEffectComEffectCount = comIndex;
        g_wEffectOwnedCount = ownedIndex;
    }
}

void WEffect_Unload() {
    MARKFUNCTION(0x8008A848);

    for (s32 i = 0; i < g_wEffectOwnedCount; i++) {
        if (g_wEffectOwnedEffects[i]) {
            delete g_wEffectOwnedEffects[i];
        }

        g_wEffectOwnedEffects[i] = nullptr;
        g_wEffectComEffects[i] = nullptr;
    }

    g_wEffectOwnedCount = 0;
    g_wEffectComEffectCount = 0;
    g_wEffectGlobalFrame = 0;

    WEffect_PurgePool();
}

WEffect* WEffect::Find(u32 effectHash) {
    MARKFUNCTION(0x8008A8E0);

    for (ccMinNode* node = g_wEffectPool.head; node; node = node->next) {
        Effects* effect = static_cast<Effects*>(static_cast<ccNode*>(node));
        if (effect->effectType == 1 && effect->nameCRC == effectHash) {
            return static_cast<WEffect*>(effect);
        }
    }

    return static_cast<WEffect*>(Effects_Find(1, effectHash));
}

FWEffect* FWEffect::Find(u32 effectHash) {
    MARKFUNCTION(0x8008BEDC);

    for (ccMinNode* node = g_wEffectPool.head; node; node = node->next) {
        Effects* effect = static_cast<Effects*>(static_cast<ccNode*>(node));
        if (effect->effectType == 4 && effect->nameCRC == effectHash) {
            return static_cast<FWEffect*>(effect);
        }
    }

    return static_cast<FWEffect*>(Effects_Find(4, effectHash));
}

void FWEffect::SetScaleRoll(s32 scaleValue, s32 rollValue) {
    MARKFUNCTION(0x8008C3E0);

    if (scaleRoll) {
        delete[] scaleRoll;
        scaleRoll = nullptr;
    }

    scaleRoll = new s32[5]();
    if (!scaleRoll) {
        return;
    }

    scaleRoll[0] = scaleValue;
    scaleRoll[1] = rollValue;
}

s32 FWEffect::Create(s32 blockNum) {
    MARKFUNCTION(0x8008BF50);

    s32 result = static_cast<s32>(0x800E0000);

    for (ccMinNode* node = g_wEffectPool.head; node; node = node->next) {
        Effects* effect = static_cast<Effects*>(static_cast<ccNode*>(node));
        if ((effect->effectType != 4 && effect->effectType != 5) || effect->blockNum != blockNum) {
            continue;
        }

        result = effect->Create();

        if (result) {
            g_wEffectPool.RemNode(effect);
            Effects_AddEffect(effect, 0);
        }
    }

    return result;
}

s32 FWEffect::Create() {
    MARKFUNCTION(0x8008C024);

    frameCounter = 0;
    if (!SetMentor()) {
        return 0;
    }

    frame = 0;
    startDelayCounter = startDelay;
    CreateSound(nullptr);

    return 1;
}

s32 FWEffect::Create2(u32 effectHash,
                      const LVector* posOverride,
                      const LVector* scaleOverride,
                      const u16* rotationOverride,
                      s32 flags)
{
    MARKFUNCTION(0x8008C078);

    for (ccMinNode* node = g_wEffectPool.head; node; node = node->next) {
        Effects* effect = static_cast<Effects*>(static_cast<ccNode*>(node));
        if ((effect->effectType != 4 && effect->effectType != 5) || effect->nameCRC != effectHash) {
            continue;
        }

        WEffect* wEffect = static_cast<WEffect*>(effect);
        if (wEffect->isMentorTarget != 0) {
            continue;
        }

        static_cast<FWEffect*>(wEffect)->Create2(posOverride, scaleOverride, rotationOverride, flags);
        g_wEffectPool.RemNode(effect);
        Effects_AddEffect(effect, (flags & 4) ? 1 : 0);
        return 1;
    }

    return 0;
}

s32 FWEffect::Create2(const LVector* posOverride,
                      const LVector* scaleOverride,
                      const u16* rotationOverride,
                      s32 flags)
{
    MARKFUNCTION(0x8008C13C);

    frameCounter = 0;
    frame = 0;
    canDisplay = 1;

    mentorLink = nullptr;
    mentorPosRef = nullptr;

    overrideFlags = 0;
    pingPongReverse = 0;
    createFlags = static_cast<u16>(flags);
    startDelayCounter = 0;

    if (posOverride) {
        overridePos = *posOverride;
        overrideFlags |= 1;

        if (g_blockManager) {
            const s32 overrideBlock = static_cast<s32>(g_blockManager->GetBlockNumber(*posOverride));
            if (overrideBlock != -1) {
                blockNum = overrideBlock;
            }
        }
    }
    else if (g_blockManager) {
        const s32 currentBlock = static_cast<s32>(g_blockManager->GetBlockNumber(pos));
        if (currentBlock != -1) {
            blockNum = currentBlock;
        }
    }

    CreateSound(posOverride);

    if (scaleOverride) {
        overrideScale = *scaleOverride;
        overrideFlags |= 2;
    }

    if (rotationOverride) {
        overrideRotation[0] = rotationOverride[0];
        overrideRotation[1] = rotationOverride[1];
        overrideRotation[2] = rotationOverride[2];
        overrideFlags |= 4;
    }
    else {
        overrideRotation[0] = rotation[0];
        overrideRotation[1] = rotation[1];
        overrideRotation[2] = rotation[2];
    }

    if (mentor) {
        return static_cast<FWEffect*>(mentor)->Create2(posOverride, scaleOverride, rotationOverride, flags);
    }

    return 0;
}

s32 FWEffect::SetMentor() {
    MARKFUNCTION(0x8008C2F0);

    if (!followHash) {
        canDisplay = 1;
        mentorLink = nullptr;
        mentorPosRef = nullptr;
        return 0;
    }

    ccNode* mentorNode = g_ai->moveList.FindNodeCRC(followHash, nullptr);
    if (!mentorNode) {
        canDisplay = 1;
        mentorLink = nullptr;
        mentorPosRef = nullptr;
        return 0;
    }

    mentorLink = static_cast<Thing*>(mentorNode);

    if (pathMode != 5) {
        mentorPosRef = &mentorLink->pos;

        LVector* soundPos = mentorLink->GetSoundPosPtr();
        if (!soundPos) {
            soundPos = &mentorLink->pos;
        }

        mentorOffset.x = pos.x - soundPos->x;
        mentorOffset.y = pos.y - soundPos->y;
        mentorOffset.z = pos.z - soundPos->z;
    }

    canDisplay = 0;

    return 1;
}

s32 FWEffect::Continue() {
    MARKFUNCTION(0x8008C434);

    if (mentor) {
        static_cast<FWEffect*>(mentor)->Continue();
    }

    createFlags = static_cast<u16>(createFlags & ~1u);
    return createFlags;
}

s32 FWEffect::Update() {
    MARKFUNCTION(0x8008C47C);

    if (mentor) {
        mentor->Update();
    }

    if (startDelayCounter > 0) {
        startDelayCounter = static_cast<s16>(startDelayCounter - 1);
        return startDelayCounter;
    }

    auto stepFrame = [&]() -> bool {
        const s16 previousCounter = frameCounter;
        frameCounter = static_cast<s16>(frameCounter + 1);
        if (previousCounter != frameDelay) {
            return false;
        }

        frameCounter = 0;
        if ((createFlags & 1u) != 0u) {
            frame = 0;
        }
        else {
            frame = static_cast<s16>(frame + 1);
        }

        return comEffect ? comEffect->EndOfFrame(frame) : false;
    };

    if (!mentorLink) {
        if (stepFrame()) {
            if ((createFlags & 2u) == 0u) {
                if (isMentorTarget) {
                    canDisplay = 0;
                }
                else {
                    s32 doneMask = 1;
                    if (mentor) {
                        mentor->IsDone(doneMask);
                    }

                    if (doneMask) {
                        Effects_RemoveEffect(this);
                        g_wEffectPool.AddNode(g_wEffectPool.tail, this);
                        ReleaseSound();
                    }
                }

                return 1;
            }

            frame = 1;
        }
    }
    else {
        const bool mentorAlive = (*(reinterpret_cast<const u8*>(mentorLink) + 115u) != 0u);
        Platform* platform = nullptr;
        if (mentorLink->thingType == AITypes::TT_PLATFORM) {
            platform = static_cast<Platform*>(mentorLink);
        }

        blockNum = mentorLink->blockNum;

        if (pathMode == 3) {
            if (platform && ((platform->platformFlags >> 1) & 1) != 0) {
                canDisplay = 1;
                pathMode = 99;
            }
        }
        else if (pathMode < 4) {
            if (pathMode == 1) {
                if (!mentorAlive) {
                    canDisplay = 1;
                    pathMode = 99;
                }
            }
            else if (pathMode >= 2) {
                canDisplay = 1;
                if (!mentorAlive) {
                    pathMode = 99;
                }
            }
            else if (pathMode == 0) {
                canDisplay = 1;

                if (stepFrame()) {
                    frame = 0;
                }
            }
        }
        else if (pathMode == 5) {
            canDisplay = 1;

            if (platform) {
                if (platform->drawDistSq < platform->pos.y) {
                    pos = platform->pos;
                    pos.y -= 512;

                    frame = 0;
                    frameCounter = 0;
                    oscillationMode = 0;
                }
                else {
                    const s16 previousCounter = frameCounter;
                    frameCounter = static_cast<s16>(frameCounter + 1);

                    if (previousCounter == frameDelay) {
                        frameCounter = 0;
                        frame = static_cast<s16>(oscillationMode ? (frame - 1) : (frame + 1));

                        if (!oscillationMode) {
                            if (comEffect && comEffect->EndOfFrame(frame)) {
                                oscillationMode = 1;
                                frame = static_cast<s16>(frame - 1);
                            }
                        }

                        if (oscillationMode && frame <= 0) {
                            frame = 0;
                        }
                    }
                }
            }
        }
        else if (pathMode == 99) {
            if (stepFrame()) {
                Effects_RemoveEffect(this);
                g_wEffectPool.AddNode(g_wEffectPool.tail, this);
                ReleaseSound();
            }
        }
        else if (pathMode < 5) {
            if (platform && platform->isActive && modeThreshold >= platform->deathCountdown) {
                canDisplay = 1;
                pathMode = 99;
            }
        }
    }

    if (scaleRoll) {
        LVector sourceScale = { 0x10000, 0x10000, 0x10000 };
        if ((renderFlags & 4u) != 0u) {
            sourceScale = scale;
        }

        scaleRoll[2] = sourceScale.x;
        scaleRoll[3] = sourceScale.y;
        scaleRoll[4] = sourceScale.z;

        if (mentorLink) {
            const s32 mentorScale = rmDiv16i(mentorLink->orientation.x, 0x10000);
            const s32 absMentorScale = (mentorScale < 0) ? -mentorScale : mentorScale;
            const s32 interpScale = static_cast<s32>(
                ((static_cast<s64>(absMentorScale) * static_cast<s64>(scaleRoll[1] - scaleRoll[0])) >> 16)
                + scaleRoll[0]);

            LVector scaled = {};
            rmV3Scale(&scaled, &sourceScale, interpScale);
            scaleRoll[2] = scaled.x;
            scaleRoll[3] = scaled.y;
            scaleRoll[4] = scaled.z;
        }
    }

    if (canDisplay) {
        return UpdateSound();
    }

    return canDisplay;
}

void FWEffect::Display(s32 inBlockNum) {
    MARKFUNCTION(0x8008C9D0);

    if (!canDisplay || blockNum != inBlockNum || startDelayCounter > 0 || !comEffect) {
        return;
    }

    if (mentor) {
        mentor->Display(inBlockNum);
    }

    if (comEffect->EndOfFrame(frame)) {
        return;
    }

    comEffect->SetFrame(frame);

    LVector renderPos = pos;
    if (mentorPosRef) {
        if (mentorLink
            && (mentorLink->orientation.x || mentorLink->orientation.y || mentorLink->orientation.z)
            && createMode == 0)
        {
            Mat4 rotMatrix;
            p3dBuildRotMatrixXYZ(static_cast<u16>(mentorLink->orientation.x),
                                 static_cast<u16>(mentorLink->orientation.y),
                                 static_cast<u16>(mentorLink->orientation.z),
                                 rotMatrix);

            Vec3 rotatedOffset = p3dVecTimesRotMatrix(
                Vec3(static_cast<f32>(mentorOffset.x),
                     static_cast<f32>(mentorOffset.y),
                     static_cast<f32>(mentorOffset.z)),
                rotMatrix);

            renderPos.x = mentorPosRef->x + static_cast<s32>(rotatedOffset.x);
            renderPos.y = mentorPosRef->y + static_cast<s32>(rotatedOffset.y);
            renderPos.z = mentorPosRef->z + static_cast<s32>(rotatedOffset.z);
        }
        else {
            renderPos.x = mentorPosRef->x + mentorOffset.x;
            renderPos.y = mentorPosRef->y + mentorOffset.y;
            renderPos.z = mentorPosRef->z + mentorOffset.z;
        }
    }

    u32 flags = renderFlags;
    const LVector* scalePtr = hasScale ? &scale : nullptr;

    u16 rotationWords[3] = { rotation[0], rotation[1], rotation[2] };
    const u16* rotationPtr = rotationWords;

    if (overrideFlags) {
        if ((overrideFlags & 1u) != 0u) {
            renderPos = overridePos;
        }

        if ((overrideFlags & 2u) != 0u) {
            scalePtr = &overrideScale;
            flags |= 4u;
        }

        if ((overrideFlags & 4u) != 0u) {
            rotationWords[0] = overrideRotation[0];
            rotationWords[1] = overrideRotation[1];
            rotationWords[2] = overrideRotation[2];
            flags |= 0x118u;
        }
    }
    else if (scaleRoll) {
        LVector rollScale = { scaleRoll[2], scaleRoll[3], scaleRoll[4] };
        if (clipDistance < 0 || comEffect->PointInView(renderPos, clipDistance)) {
            comEffect->Render(renderPos, &rollScale, rotationPtr, flags | 4u);
        }
        return;
    }

    if (clipDistance < 0 || comEffect->PointInView(renderPos, clipDistance)) {
        comEffect->Render(renderPos, scalePtr, rotationPtr, flags);
    }
}

s32 LensFlare::Create() {
    MARKFUNCTION(0x800BEF44);

    WEffect* found = FWEffect::Find(followHash);
    if (!found) {
        Effects* activeFW = Effects_Find(4, followHash);
        if (activeFW) {
            found = static_cast<WEffect*>(activeFW);
        }
    }

    if (!found) {
        Effects* baseEffect = Effects_Find(1, followHash);
        if (baseEffect) {
            found = static_cast<WEffect*>(baseEffect);
        }
    }

    if (!found) {
        return 0;
    }

    targetEffect = found;
    targetOffset.x = pos.x - found->pos.x;
    targetOffset.y = pos.y - found->pos.y;
    targetOffset.z = pos.z - found->pos.z;
    return 1;
}

s32 LensFlare::Update() {
    MARKFUNCTION(0x800BF578);

    if (!g_display || !g_display->GetCamera() || !g_display->GetCamera()->GetP3DCamera()) {
        flareVisible = 0;
        return 0;
    }

    const Mat4& cameraMatrix = g_display->GetCamera()->GetP3DCamera()->GetCameraMatrix();

    LVector cameraForward = {};
    cameraForward.x = static_cast<s32>(cameraMatrix.m[2] * 4096.0f);
    cameraForward.y = static_cast<s32>(cameraMatrix.m[6] * 4096.0f);
    cameraForward.z = static_cast<s32>(cameraMatrix.m[10] * 4096.0f);

    LVector cameraPos = {};
    cameraPos.x = static_cast<s32>(cameraMatrix.m[12]);
    cameraPos.y = static_cast<s32>(cameraMatrix.m[13]);
    cameraPos.z = static_cast<s32>(cameraMatrix.m[14]);

    LVector sourceRot = {};
    LVector sourcePos = {};

    if (targetEffect) {
        sourceRot.x = targetEffect->rotation[0];
        sourceRot.y = targetEffect->rotation[1];
        sourceRot.z = targetEffect->rotation[2];
        sourcePos = targetEffect->pos;
        pos = targetEffect->pos;
    }
    else {
        if (!mentorLink) {
            flareVisible = 0;
            return 0;
        }

        if ((mentorLink->flags2 & TF2_KILLED) != 0u) {
            Effects_RemoveEffect(this);
            g_wEffectPool.AddNode(g_wEffectPool.tail, this);
            return 0;
        }

        sourceRot.x = -mentorLink->orientation.x;
        sourceRot.y = mentorLink->orientation.y + 0x8000;
        sourceRot.z = mentorLink->orientation.z;
        sourcePos = mentorLink->pos;
        pos = mentorLink->pos;
    }

    Mat4 rotMatrix;
    p3dBuildRotMatrixXYZ(sourceRot.x, sourceRot.y, sourceRot.z, rotMatrix);

    const Vec3 rotatedOffset = p3dVecTimesRotMatrix(
        Vec3(static_cast<f32>(targetOffset.x),
             static_cast<f32>(targetOffset.y),
             static_cast<f32>(targetOffset.z)),
        rotMatrix);

    flarePos.x = sourcePos.x + static_cast<s32>(rotatedOffset.x);
    flarePos.y = sourcePos.y + static_cast<s32>(rotatedOffset.y);
    flarePos.z = sourcePos.z + static_cast<s32>(rotatedOffset.z);

    if (g_blockManager) {
        blockNum = static_cast<s32>(g_blockManager->GetBlockNumber(flarePos));
    }

    if (pathNodes) {
        trackingFlags = static_cast<u32>(ComputeTracking(flarePos, cameraPos));
    }

    LVector effectForward = {};
    effectForward.x = static_cast<s32>(rotMatrix.m[2] * 4096.0f);
    effectForward.y = static_cast<s32>(rotMatrix.m[6] * 4096.0f);
    effectForward.z = static_cast<s32>(rotMatrix.m[10] * 4096.0f);

    const s32 dot = rmV3Dot(&cameraForward, &effectForward);
    flareVisible = 0;
    if (dot < 0) {
        bigScreenGlow = (dot < -0x6000) ? 1 : 0;

        s32 tableIndex = (-15 * dot) >> 16;
        if (tableIndex < 0) {
            tableIndex = 0;
        }
        if (tableIndex > 15) {
            tableIndex = 15;
        }

        const s32 targetPrimary = kLensFlareFrameScaleTable[tableIndex];
        frameScalePrimary += static_cast<s32>(
            (static_cast<s64>(0x2000) * static_cast<s64>(targetPrimary - frameScalePrimary)) >> 16);

        const s32 targetSecondary = kLensFlareFrameScaleTable2[tableIndex];
        frameScaleSecondary += static_cast<s32>(
            (static_cast<s64>(0x2000) * static_cast<s64>(targetSecondary - frameScaleSecondary)) >> 16);

        const u8 mainR = static_cast<u8>((255 * frameScalePrimary) >> 16);
        const u8 mainG = static_cast<u8>((220 * frameScalePrimary) >> 16);
        const u8 mainB = static_cast<u8>((170 * frameScalePrimary) >> 16);
        colourPrimary = static_cast<u32>(mainR | (mainG << 8) | (mainB << 16));

        const u8 secR = static_cast<u8>((255 * frameScaleSecondary) >> 16);
        const u8 secG = static_cast<u8>((176 * frameScaleSecondary) >> 16);
        const u8 secB = static_cast<u8>((96 * frameScaleSecondary) >> 16);
        colourSecondary = static_cast<u32>(secR | (secG << 8) | (secB << 16));

        streakPitch = static_cast<s32>((-20024LL * static_cast<s64>(effectForward.y)) >> 16);

        const s32 targetYaw = static_cast<s32>((9102LL * static_cast<s64>(effectForward.x)) >> 16);
        streakYaw += static_cast<s32>((19660LL * static_cast<s64>(targetYaw - streakYaw)) >> 16);

        flareVisible = 1;

        rmV3Scale(&flareTrailDir, &cameraForward, 0x10000);
        flareTrailDir.x = flareTrailDir.x + cameraPos.x - flarePos.x;
        flareTrailDir.y = flareTrailDir.y + cameraPos.y - flarePos.y;
        flareTrailDir.z = flareTrailDir.z + cameraPos.z - flarePos.z;
    }

    return flareTrailDir.z;
}

void LensFlare::Display(s32 inBlockNum) {
    MARKFUNCTION(0x800BFB34);

    if ((targetEffect && !targetEffect->active) || !g_blockManager) {
        return;
    }

    if (inBlockNum != static_cast<s32>(g_blockManager->GetBlockNumber(flarePos))) {
        return;
    }

    ComEffect* mainEffect = flareComEffects ? &flareComEffects[0] : comEffect;
    ComEffect* streakEffect = flareComEffects ? &flareComEffects[1] : comEffect;
    ComEffect* glowEffect = flareComEffects ? &flareComEffects[2] : comEffect;
    if (!mainEffect || !streakEffect || !glowEffect) {
        return;
    }

    const bool inView = mainEffect->PointInView(flarePos, 512);

    if (flareVisible) {
        if (!inView) {
            return;
        }

        LVector mainScale = { frameScalePrimary, frameScalePrimary, frameScalePrimary };
        u16 mainRotation[3] = { 0, 0, 0 };
        mainEffect->Render(flarePos, &mainScale, mainRotation, trackingFlags | 6u);

        if (hasSecondaryGlow) {
            LVector glowScale = { 0x8000, 0x8000, 0x8000 };
            u16 glowRotation[3] = {
                static_cast<u16>(streakPitch & 0xFFFF),
                static_cast<u16>(streakYaw & 0xFFFF),
                0,
            };
            glowEffect->Render(flarePos, &glowScale, glowRotation, trackingFlags | 0x10Cu);
        }

        s32 fadeScale = 0x10000 - frameScalePrimary;
        if (fadeScale < 6553) {
            fadeScale = 6553;
        }

        const s32 clampSpan = 3;
        const s32 divStep = rmDiv16i(fadeScale, ((clampSpan * 2) + 1) << 16);

        s32 step = divStep * clampSpan;
        const u32 oldColour = clampColourEntry ? *clampColourEntry : 0;

        for (s32 i = -clampSpan; i < clampSpan; i++) {
            if (clampColourEntry) {
                *clampColourEntry = (i < -1 || i >= 2) ? colourPrimary : colourSecondary;
            }

            const s32 clampValue = kLensFlareClampValues[i + clampSpan];
            const s32 streakScaleValue = static_cast<s32>((static_cast<s64>(frameScalePrimary) * clampValue) >> 16);
            LVector streakScale = { streakScaleValue, streakScaleValue, frameScalePrimary };

            LVector streakPos = {};
            rmV3Scale(&streakPos, &flareTrailDir, step);
            step -= divStep;
            streakPos.x += flarePos.x;
            streakPos.y += flarePos.y;
            streakPos.z += flarePos.z;

            streakEffect->Render(streakPos, &streakScale, mainRotation, trackingFlags | 6u);
        }

        if (clampColourEntry) {
            *clampColourEntry = oldColour;
        }

        if (bigScreenGlow) {
            BigScreenGlow();
        }
    }
    else if (hasSecondaryGlow && inView) {
        LVector glowScale = { 0x8000, 0x8000, 0x8000 };
        u16 glowRotation[3] = {
            static_cast<u16>(streakPitch & 0xFFFF),
            static_cast<u16>(streakYaw & 0xFFFF),
            0,
        };
        glowEffect->Render(flarePos, &glowScale, glowRotation, 268u);
    }
}

void WEffect_InitWorldEffects(DBPoint* firstPoint) {
    MARKFUNCTION(0x8008A9C0);

    WEffect_PurgePool();

    bool hasMentorLinks = false;

    for (DBPoint* point = firstPoint; point; point = static_cast<DBPoint*>(point->next)) {
        u32 type = 0;
        if (!point->FindAttribValue(1, &type) || type != 100) {
            continue;
        }

        u32 subType = 0;
        if (!point->FindAttribValue(2, &subType)) {
            continue;
        }

        if (subType == 20 || subType == 30) {
            continue;
        }

        u32 resourceHash = 0;
        ComEffect* comEffect = nullptr;
        const bool hasResourceHash = point->FindAttribValue(5, &resourceHash);
        if (hasResourceHash) {
            for (s32 i = 0; i < g_wEffectComEffectCount; i++) {
                ComEffect* candidate = g_wEffectComEffects[i];
                if (candidate && candidate->resourceHash == resourceHash) {
                    comEffect = candidate;
                    break;
                }
            }
        }

        if (!comEffect && subType != 50) {
            continue;
        }

        if (subType == 50) {
            CBVEffect* effect = new CBVEffect();
            effect->effectType = 6;
            effect->nameCRC = point->nameCRC;

            u32 value = 0;
            if (point->FindAttribValue(5, &value)) {
                effect->hash = value;
            }

            u32 red = 0;
            u32 green = 0;
            u32 blue = 0;
            point->FindAttribValue(6, &red);
            point->FindAttribValue(7, &green);
            point->FindAttribValue(8, &blue);
            effect->fixedColour = (red & 0xFFu) | ((green & 0xFFu) << 8) | ((blue & 0xFFu) << 16);

            if (point->FindAttribValue(9, &value)) {
                effect->frameCount = static_cast<s32>(value);
            }

            if (point->FindAttribValue(10, &value)) {
                effect->mode = static_cast<s32>(value);
            }

            if (point->FindAttribValue(11, &value)) {
                effect->restartDelay = static_cast<s32>(value);
            }

            if (point->FindAttribValue(12, &value)) {
                effect->holdDelay = static_cast<s32>(value);
            }

            effect->frameCountMin = 0;
            if (point->FindAttribValue(13, &value)) {
                effect->frameCountMin = static_cast<s32>(value);
            }

            effect->disabledOnCreate = 0;
            if (point->FindAttribValue(14, &value)) {
                effect->disabledOnCreate = 1;
            }

            if (point->FindAttribValue(15, &value)) {
                effect->blockNum = static_cast<s32>(value);
            }

            g_wEffectPool.AddNode(g_wEffectPool.tail, effect);
            continue;
        }

        Effects* baseEffect = nullptr;

        if (subType < 51) {
            if (subType == 10) {
                u32 value = 0;
                if (point->FindAttribValue(29, &value)) {
                    SpotLight* spot = new SpotLight();
                    spot->effectType = 7;
                    baseEffect = spot;
                }
                else {
                    WEffect* effect = new WEffect();
                    effect->effectType = 1;
                    baseEffect = effect;
                }
            }
            else if (subType == 40) {
                u32 value = 0;
                if (point->FindAttribValue(29, &value)) {
                    LensFlare* flare = new LensFlare();
                    flare->effectType = 5;
                    baseEffect = flare;
                }
                else {
                    FWEffect* fwEffect = new FWEffect();
                    fwEffect->effectType = 4;
                    baseEffect = fwEffect;
                }

                if (baseEffect) {
                    FWEffect* fwEffect = static_cast<FWEffect*>(baseEffect);

                    if (point->FindAttribValue(50, &value)) {
                        fwEffect->followHash = value;
                    }
                    else {
                        fwEffect->followHash = 0;
                    }

                    if (point->FindAttribValue(27, &value)) {
                        fwEffect->createMode = static_cast<s16>(value);
                    }
                    else {
                        fwEffect->createMode = 0;
                    }

                    u32 scaleValue = 0;
                    if (point->FindAttribValue(51, &scaleValue)) {
                        u32 rollValue = 0;
                        if (point->FindAttribValue(52, &rollValue)) {
                            const s32 scaleRoll = rmDiv16i(static_cast<s32>(scaleValue << 16), 6553600);
                            const s32 rollRoll = rmDiv16i(static_cast<s32>(rollValue << 16), 6553600);
                            fwEffect->SetScaleRoll(scaleRoll, rollRoll);
                        }
                    }

                    fwEffect->pathMode = 0;
                    if (point->FindAttribValue(53, &value)) {
                        fwEffect->pathMode = static_cast<s16>(value);
                    }

                    fwEffect->modeThreshold = 30;
                    if (point->FindAttribValue(55, &value)) {
                        fwEffect->modeThreshold = static_cast<s16>(value);
                    }

                    fwEffect->oscillationMode = 0;
                    if (point->FindAttribValue(56, &value)) {
                        fwEffect->oscillationMode = static_cast<s16>(value);
                    }

                    fwEffect->startDelay = 0;
                    if (point->FindAttribValue(57, &value)) {
                        fwEffect->startDelay = static_cast<s16>(value);
                    }
                }
            }
        }

        if (!baseEffect) {
            continue;
        }

        WEffect* effect = static_cast<WEffect*>(baseEffect);
        u32 value = 0;
        u32 spotLinkedHash = 0;

        effect->nameCRC = point->nameCRC;
        effect->comEffect = comEffect;

        effect->pos = point->pos;
        effect->spawnPos = point->pos;

        if (point->FindAttribValue(30, &value) && value != 0) {
            effect->renderFlags = (value == 1) ? 2u : 0x200u;
        }
        else {
            effect->rotation[0] = static_cast<u16>(point->field40);
            effect->rotation[1] = static_cast<u16>(point->field44);
            effect->rotation[2] = static_cast<u16>(point->field48);
            effect->renderFlags = 0x118u;
        }

        if (point->FindAttribValue(41, &value)) {
            effect->scale.x = rmDiv16i(static_cast<s32>(value << 16), 6553600);
            effect->hasScale = true;
            effect->renderFlags |= 4u;
        }

        if (point->FindAttribValue(42, &value)) {
            effect->scale.y = rmDiv16i(static_cast<s32>(value << 16), 6553600);
            effect->hasScale = true;
            effect->renderFlags |= 4u;
        }

        if (point->FindAttribValue(43, &value)) {
            effect->scale.z = rmDiv16i(static_cast<s32>(value << 16), 6553600);
            effect->hasScale = true;
            effect->renderFlags |= 4u;
        }

        if (point->FindAttribValue(54, &value)) {
            effect->frameDelay = static_cast<s16>(value);
        }

        if (point->FindAttribValue(45, &value)) {
            u32 paletteFlags = 0;
            point->FindAttribValue(47, &paletteFlags);

            u32 clutMode = 0;
            point->FindAttribValue(46, &clutMode);

            effect->SetupPaletteData(value, clutMode, paletteFlags);
        }

        if (point->FindAttribValue(44, &value)) {
            hasMentorLinks = true;
            effect->mentorHash = value;
        }

        if (point->FindAttribValue(15, &value)) {
            effect->blockNum = static_cast<s32>(value);
        }
        else {
            effect->blockNum = -1;
        }

        if (point->FindAttribValue(4, &value)) {
            if (effect->effectType == 7) {
                spotLinkedHash = value;
            }
            else {
                DBPath* path = g_database ? g_database->FindPath(value) : nullptr;
                if (path) {
                    effect->pathInfo = new PathInfo();
                    if (!effect->pathInfo || !effect->pathInfo->Init(path, point)) {
                        delete effect->pathInfo;
                        effect->pathInfo = nullptr;
                    }
                }
            }
        }

        effect->vertexUseGlobalFrame = 0;
        if (point->FindAttribValue(39, &value)) {
            effect->vertexUseGlobalFrame = 1;
        }

        u32 uvBaseU = 0;
        if (point->FindAttribValue(31, &uvBaseU)) {
            effect->uvData = new WEffectUVData();

            u32 uvBaseV = 0;
            point->FindAttribValue(32, &uvBaseV);

            for (s32 i = 0; i < 4; i++) {
                effect->uvData->baseUVWords[i * 2 + 0] = static_cast<u16>(static_cast<u8>(uvBaseU));
                effect->uvData->baseUVWords[i * 2 + 1] = static_cast<u16>(static_cast<u8>(uvBaseV));
            }

            if (point->FindAttribValue(33, &value)) {
                effect->uvData->stepU = static_cast<u16>(value);
            }

            if (point->FindAttribValue(34, &value)) {
                effect->uvData->stepV = static_cast<u16>(value);
            }

            if (point->FindAttribValue(35, &value)) {
                effect->uvData->maskU = static_cast<u16>(value - 1);
            }

            if (point->FindAttribValue(36, &value)) {
                effect->uvData->maskV = static_cast<u16>(value - 1);
            }

            effect->uvData->accumU = 0;
            effect->uvData->accumV = 0;

            comEffect->SetUpUVlists();
            effect->renderFlags |= 0x80000u;
        }

        if (point->FindAttribValue(37, &value)) {
            effect->vertexEnabled = 1;
            effect->vertexSpeed = static_cast<s16>(228 + rmRangedRandom(0x1C8));
            comEffect->SetUpVertexlists();
        }

        if (effect->effectType == 5) {
            DBPath* flarePath = nullptr;
            const DBAttrib* lensAttrib = point->FindAttrib(11);
            if (lensAttrib) {
                const char* lensName = lensAttrib->GetAttribString();
                if (lensName) {
                    flarePath = g_database ? g_database->FindPath(p3dHash(lensName)) : nullptr;
                }
            }

            static_cast<LensFlare*>(effect)->InitLensFlare(0, flarePath);
        }

        if (point->FindAttribValue(28, &value)) {
            if (static_cast<s32>(value) <= 0) {
                effect->renderFlags |= 0x1000000u;
            }
            else if (value == 2) {
                comEffect->SetZFar();
            }
            else {
                effect->renderFlags |= 0x800u;
            }
        }

        effect->clipDistance = 384;
        if (point->FindAttribValue(12, &value)) {
            effect->clipDistance = static_cast<s16>(value);
        }

        effect->triggerFWHash = 0;
        if (const DBAttrib* triggerAttrib = point->FindAttrib(11)) {
            const char* triggerName = triggerAttrib->GetAttribString();
            if (triggerName) {
                effect->triggerFWHash = p3dHash(triggerName);
            }
        }

        if (effect->effectType == 7) {
            u32 spotSetup = 0;
            u32 spotRange = 0;
            point->FindAttribValue(55, &spotSetup);
            point->FindAttribValue(56, &spotRange);
            static_cast<SpotLight*>(effect)->SetUp(spotSetup, spotRange, spotLinkedHash);
        }

        g_wEffectPool.AddNode(g_wEffectPool.tail, effect);
    }

    if (hasMentorLinks) {
        for (ccMinNode* node = g_wEffectPool.head; node; node = node->next) {
            Effects* baseEffect = static_cast<Effects*>(static_cast<ccNode*>(node));
            WEffect* effect = static_cast<WEffect*>(baseEffect);
            if (!effect->mentorHash) {
                continue;
            }

            ccNode* mentorNode = g_wEffectPool.FindNodeCRC(effect->mentorHash, nullptr);
            if (!mentorNode) {
                continue;
            }

            effect->mentor = static_cast<WEffect*>(mentorNode);
            effect->mentor->isMentorTarget = 1;
        }
    }
}

void WEffect_CreateForBlock(s32 inBlockNum) {
    MARKFUNCTION(0x8008B774);

    for (ccMinNode* node = g_wEffectPool.head; node;) {
        ccMinNode* next = node->next;
        Effects* effect = static_cast<Effects*>(static_cast<ccNode*>(node));

        if ((effect->effectType == 1 || effect->effectType == 6 || effect->effectType == 7)
            && effect->blockNum == inBlockNum) {
            if (effect->Create()) {
                g_wEffectPool.RemNode(effect);
                Effects_AddEffect(effect, 0);
            }
        }

        node = next;
    }
}

void WEffect_PurgePool() {
    MARKFUNCTION(0x8008B924);

    while (ccMinNode* node = g_wEffectPool.RemHead()) {
        delete static_cast<Effects*>(static_cast<ccNode*>(node));
    }
}

void WEffect_PopulateWEffects() {
    MARKFUNCTION(0x800463F0);

    if (!g_blockManager) {
        return;
    }

    const u32 numBlocks = g_blockManager->GetNumBlocks();
    for (u32 i = 0; i < numBlocks; i++) {
        Block* block = g_blockManager->GetBlock(i);
        if (!block) {
            continue;
        }

        const s32 blockNum = static_cast<s32>(block->blockNum);
        if (!g_blockManager->InActiveList(static_cast<u32>(blockNum))) {
            continue;
        }

        WEffect_CreateForBlock(blockNum);
        FWEffect::Create(blockNum);
        PWEffect_CreateForBlock(blockNum);
        FPWEffect_CreateForBlock(blockNum);
    }
}

void WEffect_UnPopulateWEffects(s32 blockNum) {
    MARKFUNCTION(0x80046464);

    Effects_Die(blockNum, 1);
    Effects_Die(blockNum, 4);
    Effects_Die(blockNum, 2);
    Effects_Die(blockNum, 3);
    Effects_Die(blockNum, 5);
    Effects_Die(blockNum, 6);
    Effects_Die(blockNum, 7);
}
