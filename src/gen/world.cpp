#include "common.h"
#include "ai/obstacle.h"
#include "ai/player.h"
#include "gen/world.h"
#include "gen/ai.h"
#include "gen/camera.h"
#include "gen/charmgr.h"
#include "gen/database.h"
#include "gen/display.h"
#include "gen/director.h"
#include "gen/envmgr.h"
#include "gen/game.h"
#include "gen/blockmgr.h"
#include "gen/deadpool.h"
#include "gen/geometry.h"
#include "gen/effects.h"
#include "gen/geffect.h"
#include "gen/levelmgr.h"
#include "gen/model.h"
#include "gen/mplayer.h"
#include "gen/paldata.h"
#include "gen/scaledata.h"
#include "gen/switch.h"
#include "gen/skeleton.h"
#include "gen/animmgr.h"
#include "gen/particle.h"
#include "gen/pweffect.h"
#include "gen/scoremgr.h"
#include "gen/weffect.h"
#include "snd/rsevent.h"
#include "snd/snddrct.h"
#include "fe/hdmenu.h"
#include "fe/hud.h"
#include "fe/loadanim.h"
#include "p3d/hash.h"
#include "p3d/byteread.h"
#include "p3d/p3dmath.h"
#include "p3d/context.h"
#include "p3d/stream.h"
#include "p3d/texture.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"

static void UploadRawTextureToWorldVRAM(s16 x, s16 y, s16 w, s16 h, const u8* raw) {
    if (!g_game || !g_game->GetWorld()) {
        return;
    }

    g_game->GetWorld()->UploadToVRAM(x, y, w, h, raw);
}
#include "ai/colfight.h"
#include "pc/log.h"

#include "gen/uvdata.h"
#include "ai/obstacle.h"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <unordered_map>
#include <vector>

// Global block manager pointer (PSX: gp scope, set by World)
BlockManager* g_blockManager = nullptr;

// PSX globals used by destination select return positioning.
LVector g_destSelectReturnPos = { 0, 0, 0 };
bool g_destSelectReturnPosValid = false;

// PSX: _5Arrow_gInside (0x800DD558) - set by Construct when returning to hub
u8 g_arrowInside = 0;

// DynamicThing physics globals (PSX: gp+1740, gp+1744)
s32 g_maxFallSpeed = 0x4000;
s32 g_dampingFactor = 0xCCCC;

// PSX: BLOCK_DRAW_SEAM_OFFSET_CODE (0x800DC9A8)
// Layout in PSX data: { code, differenceFactor, max, reserved }
// Defaults: { 1, 0x400, 0x10, 0 }
static s32 BLOCK_DRAW_SEAM_OFFSET_CODE[4] = { 1, 0x400, 0x10, 0 };

static u32 g_wEffectChunkCount = 0;
static u32 g_particleSystemChunkCount = 0;

// PSX BGR555 to RGBA8
static void PsxToRGBA(u16 c, u8& r, u8& g, u8& b, u8& a) {
    r = static_cast<u8>((c & 0x1F) << 3);
    g = static_cast<u8>(((c >> 5) & 0x1F) << 3);
    b = static_cast<u8>(((c >> 10) & 0x1F) << 3);
    a = (c == 0) ? 0 : 255;
}

void PsxVRAM::DecodePage(u16 tpage, u16 cba, u8* out) const {
    int tx = tpage & 0xF;
    int ty = (tpage >> 4) & 1;
    int depth = (tpage >> 7) & 3;

    int pageX = tx * 64;  // VRAM word column
    int pageY = ty * 256; // VRAM row

    int clutX = (cba & 0x3F) * 16;
    int clutY = (cba >> 6) & 0x1FF;

    if (depth == 0) {
        // 4-bit indexed: 16-color CLUT
        u16 clut[16];
        for (int i = 0; i < 16; i++)
            clut[i] = Get(clutX + i, clutY);

        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int wordX = pageX + x / 4;
                int wordY = pageY + y;
                if (wordX >= 1024 || wordY >= 512) continue;
                u16 word = Get(wordX, wordY);
                int nibble = (word >> ((x % 4) * 4)) & 0xF;
                u16 color = clut[nibble];
                int idx = (y * 256 + x) * 4;
                PsxToRGBA(color, out[idx], out[idx + 1], out[idx + 2], out[idx + 3]);
            }
        }
    }
    else if (depth == 1) {
        // 8-bit indexed: 256-color CLUT
        u16 clut[256];
        for (int i = 0; i < 256; i++)
            clut[i] = Get(clutX + i, clutY);

        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int wordX = pageX + x / 2;
                int wordY = pageY + y;
                if (wordX >= 1024 || wordY >= 512) continue;
                u16 word = Get(wordX, wordY);
                int byteIdx = (x & 1) ? ((word >> 8) & 0xFF) : (word & 0xFF);
                u16 color = clut[byteIdx];
                int idx = (y * 256 + x) * 4;
                PsxToRGBA(color, out[idx], out[idx + 1], out[idx + 2], out[idx + 3]);
            }
        }
    }
    else {
        // 15-bit direct color
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int wordX = pageX + x;
                int wordY = pageY + y;
                if (wordX >= 1024 || wordY >= 512) continue;
                u16 color = Get(wordX, wordY);
                int idx = (y * 256 + x) * 4;
                PsxToRGBA(color, out[idx], out[idx + 1], out[idx + 2], out[idx + 3]);
            }
        }
    }
}

static void LoadWEffectChunk(const u8* body, u32 bodySize) {
    g_wEffectChunkCount++;

    WEffect_LoadChunk(body, bodySize);
}

static void LoadParticleSystemChunk(const u8* body, u32 bodySize) {
    g_particleSystemChunkCount++;

    if (!ParticleSystem_LoadChunk(body, bodySize)) {
        LOG("[World] Failed loading ParticleSystem chunk 0x8A30 (entry=%u, size=%u)",
            g_particleSystemChunkCount,
            bodySize);
    }
}

static bool ReadChunkPStringHash(const u8* body, u32 bodySize, u32& cursor, u32* outHash) {
    if (!body || cursor >= bodySize) {
        return false;
    }

    const u8 nameLen = body[cursor++];
    if (cursor + nameLen > bodySize) {
        return false;
    }

    if (outHash) {
        char nameBuf[256];
        const u32 copyLen = (nameLen < 255) ? nameLen : 255;
        std::memcpy(nameBuf, body + cursor, copyLen);
        nameBuf[copyLen] = '\0';
        *outHash = p3dHash(nameBuf);
    }

    cursor += nameLen;
    return true;
}

static FrameListAnim* ParseFrameListAnimRaw(const u8* rawData, u32 rawSize) {
    if (!rawData || rawSize < 24) {
        return nullptr;
    }

    FrameListAnim* frameList = new FrameListAnim();
    frameList->nameUID = p3dReadU32LE(rawData + 0);
    frameList->numFrames = static_cast<s32>(p3dReadU32LE(rawData + 12));
    frameList->targetUID = p3dReadU32LE(rawData + 20);
    frameList->rawSize = rawSize;

    if (frameList->numFrames <= 0 || frameList->numFrames > 4096) {
        delete frameList;
        return nullptr;
    }

    const u32 frameArrayOff = p3dReadU32LE(rawData + 16) * 4u;
    if (frameArrayOff + static_cast<u32>(frameList->numFrames) * 4u > rawSize) {
        delete frameList;
        return nullptr;
    }

    frameList->rawData = static_cast<u8*>(std::malloc(rawSize));
    if (!frameList->rawData) {
        delete frameList;
        return nullptr;
    }
    std::memcpy(frameList->rawData, rawData, rawSize);

    frameList->frameOffsets = new u32[frameList->numFrames];
    for (s32 i = 0; i < frameList->numFrames; i++) {
        const u32 frameOff = p3dReadU32LE(frameList->rawData + frameArrayOff + i * 4u) * 4u;
        if (frameOff >= rawSize) {
            delete frameList;
            return nullptr;
        }
        frameList->frameOffsets[i] = frameOff;
    }

    return frameList;
}

static SequenceAnim* ParseSequenceAnimRaw(const u8* rawData, u32 rawSize) {
    if (!rawData || rawSize < 20) {
        return nullptr;
    }

    SequenceAnim* sequence = new SequenceAnim();
    sequence->nameUID = p3dReadU32LE(rawData + 0);
    const u32 packedFrames = p3dReadU32LE(rawData + 12);
    sequence->numFrames = static_cast<s32>(packedFrames);

    const u32 numParts = (packedFrames >> 8) + 1u;

    if (16u + numParts * 4u > rawSize) {
        delete sequence;
        return nullptr;
    }

    sequence->numParts = numParts;
    sequence->parts = new SequenceAnimPart[numParts]();
    for (u32 i = 0; i < numParts; i++) {
        sequence->parts[i].animHash = p3dReadU32LE(rawData + 16u + i * 4u);
    }

    return sequence;
}

static CompositeAnimData* ParseCompositeAnimChunkBody(const u8* body, u32 bodySize) {
    if (!body || bodySize < 5) {
        return nullptr;
    }

    u32 cursor = 0;
    u32 nameHash = 0;
    if (!ReadChunkPStringHash(body, bodySize, cursor, &nameHash) || cursor + 4 > bodySize) {
        return nullptr;
    }

    CompositeAnimData* composite = new CompositeAnimData();
    composite->nameUID = nameHash;
    composite->field12 = p3dReadU16LE(body + cursor);
    cursor += 2;
    composite->numParts = p3dReadU16LE(body + cursor);
    cursor += 2;

    if (composite->numParts > 0) {
        composite->parts = new CompositeAnimPartData[composite->numParts]();
    }

    for (u32 partIndex = 0; partIndex < composite->numParts; partIndex++) {
        CompositeAnimPartData& part = composite->parts[partIndex];
        if (!ReadChunkPStringHash(body, bodySize, cursor, &part.animNameUID) || cursor + 4 > bodySize) {
            delete composite;
            return nullptr;
        }

        part.field0 = p3dReadU16LE(body + cursor);
        cursor += 2;
        part.field1 = p3dReadU16LE(body + cursor);
        cursor += 2;
    }

    return composite;
}

static bool ShouldKeepParamChannel(s32 animType, s32 paramID) {
    if (animType == 257) {
        return paramID == 257 || (paramID >= 273 && paramID < 275);
    }

    if (animType >= 258) {
        return animType == 768 || animType < 0;
    }

    if (animType > 0 && animType < 3) {
        switch (paramID) {
            case 1:
            case 2:
            case 17:
            case 18:
            case 33:
            case 34:
                return true;
            default:
                return false;
        }
    }

    return animType < 0;
}

static ParamAnimData* ParseParamAnimChunkBody(const u8* body, u32 bodySize) {
    if (!body || bodySize < 18) {
        return nullptr;
    }

    u32 cursor = 0;
    u32 nameHash = 0;
    if (!ReadChunkPStringHash(body, bodySize, cursor, &nameHash) || cursor + 16 > bodySize) {
        return nullptr;
    }

    cursor += 4; // unknown/load flags field
    const s32 animType = p3dReadS32LE(body + cursor);
    cursor += 4;

    // target name string is stored but not required for CBV multiplier eval.
    if (!ReadChunkPStringHash(body, bodySize, cursor, nullptr) || cursor + 8 > bodySize) {
        return nullptr;
    }

    s32 numFrames = p3dReadS32LE(body + cursor);
    cursor += 4;
    cursor += 4; // expected param count

    std::vector<u8> keyTimes;
    std::vector<s32> keyValues;

    while (cursor + 6 <= bodySize) {
        const u16 subChunkId = p3dReadU16LE(body + cursor);
        const u32 subChunkSize = p3dReadU32LE(body + cursor + 2);
        if (subChunkSize < 6 || cursor + subChunkSize > bodySize) {
            break;
        }

        if (subChunkId == 0x4301) {
            const u8* paramBody = body + cursor + 6;
            const u32 paramBodySize = subChunkSize - 6;
            u32 paramCursor = 0;

            if (!ReadChunkPStringHash(paramBody, paramBodySize, paramCursor, nullptr) || paramCursor + 8 > paramBodySize) {
                cursor += subChunkSize;
                continue;
            }

            paramCursor += 4; // unknown
            const s32 paramID = p3dReadS32LE(paramBody + paramCursor);
            paramCursor += 4;

            if (!ShouldKeepParamChannel(animType, paramID)) {
                cursor += subChunkSize;
                continue;
            }

            std::vector<u8> candidateTimes;
            std::vector<s32> candidateValues;

            while (paramCursor + 6 <= paramBodySize) {
                const u16 valueChunkId = p3dReadU16LE(paramBody + paramCursor);
                const u32 valueChunkSize = p3dReadU32LE(paramBody + paramCursor + 2);
                if (valueChunkSize < 6 || paramCursor + valueChunkSize > paramBodySize) {
                    break;
                }

                const u8* valueBody = paramBody + paramCursor + 6;
                const u32 valueBodySize = valueChunkSize - 6;

                if (valueChunkId == 0x4203 && valueBodySize >= 4) {
                    const u32 count = p3dReadU32LE(valueBody + 0);
                    if (count > 0 && 4u + count * 2u <= valueBodySize) {
                        candidateTimes.resize(count);
                        for (u32 i = 0; i < count; i++) {
                            candidateTimes[i] = static_cast<u8>(p3dReadU16LE(valueBody + 4 + i * 2u));
                        }
                    }
                }
                else if ((valueChunkId == 0x4210 || valueChunkId == 0x4283) && valueBodySize >= 8) {
                    const u32 count = p3dReadU32LE(valueBody + 0);
                    if (count > 0 && 8u + count * 4u <= valueBodySize) {
                        candidateValues.resize(count);
                        for (u32 i = 0; i < count; i++) {
                            candidateValues[i] = p3dReadS32LE(valueBody + 8 + i * 4u);
                        }
                    }
                }
                else if (valueChunkId == 0x4213 && valueBodySize >= 8) {
                    const u32 count = p3dReadU32LE(valueBody + 0);
                    if (count > 0 && 8u + count * 2u <= valueBodySize) {
                        candidateValues.resize(count);
                        for (u32 i = 0; i < count; i++) {
                            candidateValues[i] = static_cast<s32>(p3dReadU16LE(valueBody + 8 + i * 2u));
                        }
                    }
                }

                paramCursor += valueChunkSize;
            }

            const u32 keyCount = static_cast<u32>(std::min(candidateTimes.size(), candidateValues.size()));
            if (keyCount > 0) {
                keyTimes = candidateTimes;
                keyValues = candidateValues;
                keyTimes.resize(keyCount);
                keyValues.resize(keyCount);
                break;
            }
        }

        cursor += subChunkSize;
    }

    if (keyTimes.empty() || keyValues.empty()) {
        return nullptr;
    }

    ParamAnimData* paramAnim = new ParamAnimData();
    paramAnim->nameUID = nameHash;
    paramAnim->animType = animType;
    paramAnim->numFrames = numFrames;
    paramAnim->keyCount = static_cast<u32>(keyTimes.size());
    paramAnim->keyTimes = new u8[paramAnim->keyCount];
    paramAnim->keyValues = new s32[paramAnim->keyCount];
    for (u32 i = 0; i < paramAnim->keyCount; i++) {
        paramAnim->keyTimes[i] = keyTimes[i];
        paramAnim->keyValues[i] = keyValues[i];
    }

    if (paramAnim->numFrames <= 0 && paramAnim->keyCount > 0) {
        paramAnim->numFrames = static_cast<s32>(paramAnim->keyTimes[paramAnim->keyCount - 1]) + 1;
    }

    return paramAnim;
}

static bool RegisterParamAnimChunk(const u8* chunkBody, u32 chunkBodySize, u8 animType) {
    if (!chunkBody || chunkBodySize < 18) {
        return true;
    }

    ParamAnimData* paramAnim = ParseParamAnimChunkBody(chunkBody, chunkBodySize);
    if (!paramAnim) {
        return true;
    }

    if (!g_animMgr || paramAnim->nameUID == 0) {
        delete paramAnim;
        return true;
    }

    if (g_animMgr->GetMiscAnim(paramAnim->nameUID)) {
        delete paramAnim;
        return true;
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = paramAnim->nameUID;
    node->type = animType;
    node->paramAnim = paramAnim;
    g_animMgr->AddAnim(node);
    return true;
}

static CBVParamAnimData* ParseCBVParamAnimChunkBody(const u8* body, u32 bodySize) {
    if (!body || bodySize < 20) {
        return nullptr;
    }

    u32 cursor = 0;
    u32 nameHash = 0;
    u32 targetHash = 0;
    u32 blendHash = 0;
    if (!ReadChunkPStringHash(body, bodySize, cursor, &nameHash)
        || !ReadChunkPStringHash(body, bodySize, cursor, &targetHash)
        || !ReadChunkPStringHash(body, bodySize, cursor, &blendHash)
        || cursor + 8 > bodySize) {
        return nullptr;
    }

    // PSX tCBVParamAnimLoader reads two longs after the three pstrings and
    // uses the second one as mode (first is an unused/reserved field).
    cursor += 4; // reserved/unused
    const s32 mode = p3dReadS32LE(body + cursor);
    cursor += 4;

    u32 valueRows = 0;
    u32 valueCols = 0;
    std::vector<u32> values;
    std::vector<u32> offsets;

    while (cursor + 6 <= bodySize) {
        const u16 subChunkId = p3dReadU16LE(body + cursor);
        const u32 subChunkSize = p3dReadU32LE(body + cursor + 2);
        if (subChunkSize < 6 || cursor + subChunkSize > bodySize) {
            break;
        }

        const u8* subBody = body + cursor + 6;
        const u32 subBodySize = subChunkSize - 6;

        if (subChunkId == 0x6022 && subBodySize >= 8) {
            const u32 rows = p3dReadU32LE(subBody + 0);
            const u32 cols = p3dReadU32LE(subBody + 4);
            if (rows > 0 && cols > 0 && rows <= (0xFFFFFFFFu / cols)) {
                const u32 valueCount = rows * cols;
                if (8u + valueCount * 4u > subBodySize) {
                    cursor += subChunkSize;
                    continue;
                }

                valueRows = rows;
                valueCols = cols;
                values.resize(valueCount);
                for (u32 i = 0; i < valueCount; i++) {
                    values[i] = p3dReadU32LE(subBody + 8 + i * 4u);
                }
            }
        }
        else if (subChunkId == 0x6023 && subBodySize >= 4) {
            const u32 count = p3dReadU32LE(subBody + 0);
            if (count > 0 && 4u + count * 4u <= subBodySize) {
                offsets.resize(count);
                for (u32 i = 0; i < count; i++) {
                    offsets[i] = p3dReadU32LE(subBody + 4 + i * 4u);
                }
            }
        }

        cursor += subChunkSize;
    }

    if (nameHash == 0 || values.empty() || offsets.empty() || valueCols == 0) {
        return nullptr;
    }

    CBVParamAnimData* cbvParamAnim = new CBVParamAnimData();
    cbvParamAnim->nameUID = nameHash;
    cbvParamAnim->targetUID = targetHash;
    cbvParamAnim->blendAnimUID = blendHash;
    cbvParamAnim->mode = mode;
    cbvParamAnim->valueRows = valueRows;
    cbvParamAnim->valueCols = valueCols;
    cbvParamAnim->numEntries = static_cast<u32>(offsets.size());

    cbvParamAnim->values = new u32[values.size()];
    for (u32 i = 0; i < values.size(); i++) {
        cbvParamAnim->values[i] = values[i];
    }

    cbvParamAnim->primOffsets = new u32[offsets.size()];
    for (u32 i = 0; i < offsets.size(); i++) {
        cbvParamAnim->primOffsets[i] = offsets[i];
    }

    return cbvParamAnim;
}

static bool RegisterCBVParamAnimChunk(const u8* chunkBody, u32 chunkBodySize, u8 animType) {
    if (!chunkBody || chunkBodySize < 20) {
        return true;
    }

    CBVParamAnimData* cbvParamAnim = ParseCBVParamAnimChunkBody(chunkBody, chunkBodySize);
    if (!cbvParamAnim) {
        return true;
    }

    if (!g_animMgr || cbvParamAnim->nameUID == 0 || cbvParamAnim->numEntries == 0) {
        delete cbvParamAnim;
        return true;
    }

    if (g_animMgr->GetMiscAnim(cbvParamAnim->nameUID)) {
        delete cbvParamAnim;
        return true;
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = cbvParamAnim->nameUID;
    node->type = animType;
    node->cbvParamAnim = cbvParamAnim;
    g_animMgr->AddAnim(node);
    return true;
}

static ClutAnimData* ParseClutAnimChunkBody(const u8* body, u32 bodySize) {
    if (!body || bodySize < 5) {
        return nullptr;
    }

    u32 cursor = 0;
    u32 nameHash = 0;
    u32 materialHash = 0;
    u32 primHash = 0;
    if (!ReadChunkPStringHash(body, bodySize, cursor, &nameHash)
        || !ReadChunkPStringHash(body, bodySize, cursor, &materialHash)
        || !ReadChunkPStringHash(body, bodySize, cursor, &primHash)
        || cursor + 2 > bodySize) {
        return nullptr;
    }

    const u16 mode = p3dReadU16LE(body + cursor);
    cursor += 2;

    std::vector<u16> frames;
    std::vector<u16> offsets;

    while (cursor + 6 <= bodySize) {
        const u16 subChunkId = p3dReadU16LE(body + cursor);
        const u32 subChunkSize = p3dReadU32LE(body + cursor + 2);
        if (subChunkSize < 6 || cursor + subChunkSize > bodySize) {
            break;
        }

        const u8* subBody = body + cursor + 6;
        const u32 subBodySize = subChunkSize - 6;

        if ((subChunkId == 0x600C || subChunkId == 0x600D) && subBodySize >= 2) {
            const u32 count = p3dReadU16LE(subBody + 0);
            if (count > 0 && 2u + count * 2u <= subBodySize) {
                std::vector<u16>& out = (subChunkId == 0x600C) ? frames : offsets;
                out.resize(count);
                for (u32 i = 0; i < count; i++) {
                    out[i] = p3dReadU16LE(subBody + 2u + i * 2u);
                }
            }
        }

        cursor += subChunkSize;
    }

    if (nameHash == 0 || frames.empty()) {
        return nullptr;
    }

    ClutAnimData* clutAnim = new ClutAnimData();
    clutAnim->nameUID = nameHash;
    clutAnim->materialUID = materialHash;
    clutAnim->primUID = primHash;
    clutAnim->mode = mode;
    clutAnim->numFrames = static_cast<s32>(frames.size());
    clutAnim->frames = new u16[frames.size()];
    for (u32 i = 0; i < frames.size(); i++) {
        clutAnim->frames[i] = frames[i];
    }

    if (!offsets.empty()) {
        clutAnim->numOffsets = static_cast<s32>(offsets.size());
        clutAnim->offsets = new u16[offsets.size()];
        for (u32 i = 0; i < offsets.size(); i++) {
            clutAnim->offsets[i] = offsets[i];
        }
    }

    return clutAnim;
}

static bool RegisterClutAnimChunk(const u8* chunkBody, u32 chunkBodySize, u8 animType) {
    if (!chunkBody || chunkBodySize < 5) {
        return true;
    }

    ClutAnimData* clutAnim = ParseClutAnimChunkBody(chunkBody, chunkBodySize);
    if (!clutAnim) {
        return true;
    }

    if (!g_animMgr || clutAnim->nameUID == 0) {
        delete clutAnim;
        return true;
    }

    if (g_animMgr->GetMiscAnim(clutAnim->nameUID)) {
        delete clutAnim;
        return true;
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = clutAnim->nameUID;
    node->type = animType;
    node->clutAnim = clutAnim;
    g_animMgr->AddAnim(node);
    return true;
}

static bool RegisterTransformAnimChunk(const u8* chunkBody,
                                       u32 chunkBodySize,
                                       const u8* permData,
                                       u32& permCursor,
                                       u32 permSize,
                                       u8 animType)
{
    if (!chunkBody || chunkBodySize < 5 || !permData) {
        return true;
    }

    u32 cursor = 0;
    u32 chunkNameHash = 0;
    if (!ReadChunkPStringHash(chunkBody, chunkBodySize, cursor, &chunkNameHash) || cursor + 4 > chunkBodySize) {
        LOG("[World] Malformed transform anim chunk (0x6400)");
        return false;
    }

    const u32 rawSize = p3dReadU32LE(chunkBody + cursor);
    if (permCursor + rawSize > permSize) {
        LOG("[World] Transform anim perm overflow at %u (size=%u total=%u)", permCursor, rawSize, permSize);
        return false;
    }

    TransformAnim* anim = nullptr;
    if (rawSize >= 40) {
        u8* rawCopy = static_cast<u8*>(std::malloc(rawSize));
        if (rawCopy) {
            std::memcpy(rawCopy, permData + permCursor, rawSize);
            anim = TransformAnim::Parse(rawCopy, rawSize);
            if (anim) {
                anim->ownedRawData = rawCopy;
                if (chunkNameHash != 0) {
                    anim->nameUID = chunkNameHash;
                }
            }
            else {
                std::free(rawCopy);
            }
        }
    }

    permCursor += rawSize;

    if (!anim) {
        return true;
    }

    if (!g_animMgr) {
        delete anim;
        return true;
    }

    if (g_animMgr->GetMiscAnim(anim->nameUID)) {
        delete anim;
        return true;
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = anim->nameUID;
    node->type = animType;
    node->anim = anim;
    g_animMgr->AddAnim(node);
    return true;
}

static bool RegisterFrameListAnimChunk(const u8* chunkBody,
                                       u32 chunkBodySize,
                                       const u8* permData,
                                       u32& permCursor,
                                       u32 permSize,
                                       u8 animType)
{
    if (!chunkBody || chunkBodySize < 8 || !permData) {
        return true;
    }

    u32 cursor = 0;
    (void)p3dReadU32LE(chunkBody + cursor);
    cursor += 4;
    const u32 rawSize = p3dReadU32LE(chunkBody + cursor);
    cursor += 4;

    u32 chunkNameHash = 0;
    u32 targetHash = 0;
    ReadChunkPStringHash(chunkBody, chunkBodySize, cursor, &chunkNameHash);
    ReadChunkPStringHash(chunkBody, chunkBodySize, cursor, &targetHash);

    if (permCursor + rawSize > permSize) {
        LOG("[World] FrameList perm overflow at %u (size=%u total=%u)", permCursor, rawSize, permSize);
        return false;
    }

    FrameListAnim* frameList = ParseFrameListAnimRaw(permData + permCursor, rawSize);
    permCursor += rawSize;

    if (!frameList) {
        return true;
    }

    if (!g_animMgr) {
        delete frameList;
        return true;
    }

    if (chunkNameHash != 0) {
        frameList->nameUID = chunkNameHash;
    }
    if (targetHash != 0) {
        frameList->targetUID = targetHash;
    }

    if (g_animMgr->GetMiscAnim(frameList->nameUID)) {
        delete frameList;
        return true;
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = frameList->nameUID;
    node->type = animType;
    node->frameList = frameList;
    g_animMgr->AddAnim(node);
    return true;
}

static bool RegisterSequenceAnimChunk(const u8* chunkBody,
                                      u32 chunkBodySize,
                                      const u8* permData,
                                      u32& permCursor,
                                      u32 permSize,
                                      u8 animType)
{
    if (!chunkBody || chunkBodySize < 4 || !permData) {
        return true;
    }

    const u32 rawSize = p3dReadU32LE(chunkBody + 0);
    if (permCursor + rawSize > permSize) {
        LOG("[World] SequenceAnim perm overflow at %u (size=%u total=%u)", permCursor, rawSize, permSize);
        return false;
    }

    SequenceAnim* sequence = ParseSequenceAnimRaw(permData + permCursor, rawSize);
    permCursor += rawSize;

    if (!sequence) {
        return true;
    }

    if (!g_animMgr) {
        delete sequence;
        return true;
    }

    for (u32 i = 0; i < sequence->numParts; i++) {
        SequenceAnimPart& part = sequence->parts[i];
        if (part.animHash != 0) {
            part.node = g_animMgr->GetMiscAnim(part.animHash);
            if (part.node && part.node->anim) {
                part.anim = part.node->anim;
            }
        }
    }

    if (g_animMgr->GetMiscAnim(sequence->nameUID)) {
        delete sequence;
        return true;
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = sequence->nameUID;
    node->type = animType;
    node->sequenceAnim = sequence;
    g_animMgr->AddAnim(node);
    return true;
}

static void RegisterCompositeAnimChunk(const u8* chunkBody, u32 chunkBodySize, u8 animType) {
    if (!chunkBody || chunkBodySize < 5 || !g_animMgr) {
        return;
    }

    CompositeAnimData* composite = ParseCompositeAnimChunkBody(chunkBody, chunkBodySize);
    if (!composite) {
        return;
    }

    if (g_animMgr->GetMiscAnim(composite->nameUID)) {
        delete composite;
        return;
    }

    for (u32 i = 0; i < composite->numParts; i++) {
        CompositeAnimPartData& part = composite->parts[i];
        if (part.animNameUID != 0) {
            part.animNode = g_animMgr->GetMiscAnim(part.animNameUID);
        }
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = composite->nameUID;
    node->type = animType;
    node->compositeAnim = composite;
    g_animMgr->AddAnim(node);
}

static VizAnim* ParseVizAnimChunkBody(const u8* body, u32 bodySize) {
    if (!body || bodySize < 10) {
        return nullptr;
    }

    u32 cursor = 0;
    u32 nameHash = 0;
    u32 targetHash = 0;
    if (!ReadChunkPStringHash(body, bodySize, cursor, &nameHash)
        || !ReadChunkPStringHash(body, bodySize, cursor, &targetHash)
        || cursor + 4 > bodySize) {
        return nullptr;
    }

    const s16 numFrames = p3dReadS16LE(body + cursor);
    cursor += 2;
    const u16 numNodes = p3dReadU16LE(body + cursor);
    cursor += 2;

    if (cursor + 6 > bodySize) {
        return nullptr;
    }

    const u16 subChunkId = p3dReadU16LE(body + cursor);
    cursor += 2;
    const u32 subChunkSize = p3dReadU32LE(body + cursor);
    cursor += 4;

    if (subChunkId != 0x4021 || subChunkSize < 6) {
        return nullptr;
    }

    const u32 subBodySize = subChunkSize - 6;
    if (cursor + subBodySize > bodySize || numNodes == 0 || subBodySize < static_cast<u32>(numNodes) * 8u) {
        return nullptr;
    }

    VizAnim* vizAnim = new VizAnim();
    vizAnim->nameUID = nameHash;
    vizAnim->targetUID = targetHash;
    vizAnim->numFrames = (numFrames > 0) ? numFrames : 0;
    vizAnim->numNodes = numNodes;

    vizAnim->rawData = static_cast<u8*>(std::malloc(subBodySize));
    if (!vizAnim->rawData) {
        delete vizAnim;
        return nullptr;
    }

    vizAnim->rawSize = subBodySize;
    std::memcpy(vizAnim->rawData, body + cursor, subBodySize);

    vizAnim->nodes = new VizAnimNodeEntry[numNodes]();
    for (u16 nodeIndex = 0; nodeIndex < numNodes; nodeIndex++) {
        const u32 entryOffset = static_cast<u32>(nodeIndex) * 8u;
        const u32 geoHash = p3dReadU32LE(vizAnim->rawData + entryOffset + 0);
        const u32 bitsOffsetWords = p3dReadU32LE(vizAnim->rawData + entryOffset + 4);

        VizAnimNodeEntry& outNode = vizAnim->nodes[nodeIndex];
        outNode.targetHash = geoHash;
        // tVizNode stores bit offsets in 32-bit words in packed data streams.
        const u32 bitsOffsetBytes = bitsOffsetWords * 4u;
        if (bitsOffsetBytes < vizAnim->rawSize) {
            outNode.bitWords = reinterpret_cast<const u32*>(vizAnim->rawData + bitsOffsetBytes);
        }
    }

    return vizAnim;
}

static void RegisterVizAnimChunk(const u8* chunkBody, u32 chunkBodySize, u8 animType) {
    if (!chunkBody || chunkBodySize < 10 || !g_animMgr) {
        return;
    }

    VizAnim* vizAnim = ParseVizAnimChunkBody(chunkBody, chunkBodySize);
    if (!vizAnim) {
        return;
    }

    if (g_animMgr->GetMiscAnim(vizAnim->nameUID)) {
        delete vizAnim;
        return;
    }

    MiscAnimNode* node = new MiscAnimNode();
    node->hash = vizAnim->nameUID;
    node->type = animType;
    node->vizAnim = vizAnim;
    g_animMgr->AddAnim(node);
}

static bool SwitchStringEqualsNoCase(const char* lhs, const char* rhs) {
    if (!lhs || !rhs) {
        return false;
    }

    while (*lhs && *rhs) {
        unsigned char left = (unsigned char)*lhs++;
        unsigned char right = (unsigned char)*rhs++;

        if (left >= 'A' && left <= 'Z') {
            left = (unsigned char)(left + ('a' - 'A'));
        }
        if (right >= 'A' && right <= 'Z') {
            right = (unsigned char)(right + ('a' - 'A'));
        }

        if (left != right) {
            return false;
        }
    }

    return *lhs == '\0' && *rhs == '\0';
}

static s32 SwitchBehaviorTrigger(Thing* /*thing*/, u32 /*argc*/, const char** /*argv*/) {
    return 1;
}

static s32 SwitchSoundAmbiantSpace(Thing* thing, u32 argc, const char** argv) {
    MARKFUNCTION(0x80093E68);

    if (!thing || thing->thingType != AITypes::TT_PLAYER) {
        return 1;
    }

    if (argc == 0 || !argv || !argv[0]) {
        return 1;
    }

    const s32 ambienceSpace = atol(argv[0]);
    const s32 crossFade = (argc == 2 && argv[1]) ? atol(argv[1]) : 0;
    rsEvent(20, ambienceSpace, crossFade, 0);
    return 1;
}

static s32 SwitchEnemyObstacleDeathVol(Thing* thing, u32 /*argc*/, const char** /*argv*/) {
    if (!thing) {
        return 0;
    }

    if (g_scoreManager && thing->thingType >= AITypes::TT_HUMANOID_FIRST
        && thing->thingType <= AITypes::TT_HUMANOID_LAST) {
        g_scoreManager->AddStylePoints(100);
    }

    thing->Kill();
    return 0;
}

static Humanoid* ResolveSwitchDialogTarget(Thing* thing, s32 targetPlayer) {
    if (targetPlayer != 0) {
        return Player::s_player;
    }

    return thing ? static_cast<Humanoid*>(thing) : nullptr;
}

static s32 SwitchLoadDialog(Thing* thing, u32 argc, const char** argv) {
    if (argc != 3 || !argv) {
        return 0;
    }

    s32 targetPlayer = argv[0] ? atol(argv[0]) : 0;
    u32 dialogID = argv[1] ? (u32)atol(argv[1]) : 0;
    s32 priority = argv[2] ? atol(argv[2]) : 0;

    Humanoid* target = ResolveSwitchDialogTarget(thing, targetPlayer);
    if (!target) {
        return 0;
    }

    target->LoadDialog(dialogID, priority);
    return 1;
}

static s32 SwitchPlayDialog(Thing* thing, u32 argc, const char** argv) {
    if (argc != 3 || !argv) {
        return 0;
    }

    s32 targetPlayer = argv[0] ? atol(argv[0]) : 0;
    u32 dialogID = argv[1] ? (u32)atol(argv[1]) : 0;
    s32 priority = argv[2] ? atol(argv[2]) : 0;

    Humanoid* target = ResolveSwitchDialogTarget(thing, targetPlayer);
    if (!target) {
        return 0;
    }

    target->PlayDialog(dialogID, priority);
    return 1;
}

static s32 SwitchCheckpoint(Thing* /*thing*/, u32 argc, const char** argv) {
    if (!Player::s_player) {
        return 0;
    }

    Player::s_player->OnCheckpoint();

    if (argc < 2 || !argv) {
        Player::s_player->checkpoint.field44 = 0;
        Player::s_player->checkpoint.field48 = 0;
    }
    else {
        Player::s_player->checkpoint.field44 = (s32)p3dHash(argv[1]);
        Player::s_player->checkpoint.field48 = (argc < 3) ? 0 : (s32)p3dHash(argv[2]);
    }

    return 1;
}

struct PendingCharModelLoad {
    s32 oldType;
    s32 newType;
};

static std::vector<PendingCharModelLoad> g_pendingCharModelLoads;

static bool IsSwitchCharModelType(s32 thingType) {
    return (u32)(thingType - 1) < 0x1C;
}

static void ClearPendingCharModelLoads() {
    g_pendingCharModelLoads.clear();
}

static void SwitchUnloadLoadCharModel(s32 oldType, s32 newType) {
    if (!g_characterManager) {
        return;
    }

    if (IsSwitchCharModelType(oldType)) {
        g_characterManager->UnloadAnimation((u32)oldType, 0, 0x188);
        g_characterManager->UnloadCharacter((u32)oldType);
    }

    if (IsSwitchCharModelType(newType)) {
        g_characterManager->LoadCharacter((u32)newType, nullptr);
        g_characterManager->LoadAnimation((u32)newType, 0, 0x7C, nullptr);
    }
}

static void QueuePendingCharModelLoad(s32 oldType, s32 newType) {
    g_pendingCharModelLoads.push_back({ oldType, newType });
}

static u32 g_switchLoadGroupHash[2] = {};
static s32 g_switchLoadGroupIndex = 0;
// PSX: directorGOTO (0x800DD6E8)
static s32 g_directorGOTO = 0;

static void PurgeSwitchLoadGroups() {
    if (g_characterManager) {
        for (s32 index = 0; index < 2; index++) {
            if (g_switchLoadGroupHash[index] != 0) {
                g_characterManager->UnloadAnimation(AITypes::TT_PLAYER, g_switchLoadGroupHash[index]);
            }
        }
    }

    for (s32 index = 0; index < 2; index++) {
        g_switchLoadGroupHash[index] = 0;
        g_switchLoadGroupIndex = index;
    }
}

static void LoadSwitchLoadGroup(const char* name, u32 hash) {
    if (!g_characterManager || !name) {
        return;
    }

    const s32 slot = g_switchLoadGroupIndex;
    if (g_switchLoadGroupHash[slot] != 0) {
        g_characterManager->UnloadAnimation(AITypes::TT_PLAYER, g_switchLoadGroupHash[slot]);
    }

    g_switchLoadGroupHash[slot] = 0;

    if (hash != 0) {
        g_switchLoadGroupHash[slot] = hash;
        g_characterManager->LoadAnimation(AITypes::TT_PLAYER, hash, nullptr);
    }

    g_switchLoadGroupIndex = (slot == 0) ? 1 : 0;
}

static PlayerModel* ResolveSwitchPlayerModel(Thing* thing) {
    if (!thing || !thing->model) {
        return nullptr;
    }

    Model* model = static_cast<Model*>(thing->model);
    return dynamic_cast<PlayerModel*>(model);
}

static s32 SwitchAsyncLoadGroup(Thing* /*thing*/, u32 argc, const char** argv) {
    if (argc == 0 || !argv || !SwitchStringEqualsNoCase(argv[0], "keep")) {
        PurgeSwitchLoadGroups();
    }

    u32 argIndex = 0;
    if (argc > 0 && argv && SwitchStringEqualsNoCase(argv[0], "keep")) {
        argIndex = 1;

        if (argIndex < argc && argv[argIndex]) {
            const u32 keepHash = p3dHash(argv[argIndex]);
            for (s32 index = 0; index < 2; index++) {
                if (g_switchLoadGroupHash[index] == keepHash) {
                    g_switchLoadGroupIndex = (index == 0) ? 1 : 0;
                    break;
                }
            }
            argIndex++;
        }
    }

    while (argIndex < argc) {
        const char* name = argv[argIndex++];
        if (!name) {
            continue;
        }

        LoadSwitchLoadGroup(name, p3dHash(name));
    }

    return 1;
}

// PSX: gfAsyncLoadNIS__FP5ThingUlPPCc (SWITCH.CPP:884, 0x80094684)
static s32 SwitchAsyncLoadNIS(Thing* thing, u32 argc, const char** argv) {
    MARKFUNCTION(0x80094684);

    u32 loadArgc = argc;
    if (argc == 0 || !argv || !argv[argc - 1]
        || !SwitchStringEqualsNoCase(argv[argc - 1], "nopurge")) {
        PurgeSwitchLoadGroups();
    }
    else {
        loadArgc--;
    }

    PlayerModel* playerModel = ResolveSwitchPlayerModel(thing);
    if (playerModel && argv && loadArgc > 0) {
        playerModel->LoadNIS(loadArgc, argv, 1, 0);
    }

    g_directorGOTO = 0;
    return 1;
}

// PSX: gfAsyncLoadNISGOTO__FP5ThingUlPPCc (SWITCH.CPP:912, 0x80094714)
static s32 SwitchAsyncLoadNISGOTO(Thing* thing, u32 argc, const char** argv) {
    MARKFUNCTION(0x80094714);

    if (g_directorGOTO != 0) {
        g_directorGOTO = 0;

        u32 loadArgc = argc;
        if (argc == 0 || !argv || !argv[argc - 1]
            || !SwitchStringEqualsNoCase(argv[argc - 1], "nopurge")) {
            PurgeSwitchLoadGroups();
        }
        else {
            loadArgc--;
        }

        PlayerModel* playerModel = ResolveSwitchPlayerModel(thing);
        if (playerModel && argv && loadArgc > 0) {
            playerModel->LoadNIS(loadArgc, argv, 1, 0);
        }
    }

    return 1;
}

static s32 SwitchCharModelLoad(Thing* /*thing*/, u32 argc, const char** argv) {
    if (argc < 2 || !argv) {
        return 0;
    }

    const s32 oldType = argv[0] ? atol(argv[0]) : 0;
    const s32 newType = argv[1] ? atol(argv[1]) : 0;

    s32 killedAny = 0;
    if (IsSwitchCharModelType(oldType) && g_ai) {
        for (ccMinNode* node = g_ai->humanoidList.head; node; node = node->next) {
            Humanoid* humanoid = static_cast<Humanoid*>(node);
            if (humanoid->thingType == (u16)oldType) {
                killedAny = 1;
                humanoid->Kill();
            }
        }
    }

    if (killedAny != 0) {
        QueuePendingCharModelLoad(oldType, newType);
    }
    else {
        SwitchUnloadLoadCharModel(oldType, newType);
    }

    return 1;
}

static void PlayThingDeathVolSound(s32 deathVolType) {
    if (deathVolType == 0 && g_game && g_game->GetWorld()) {
        const s32 levelID = g_game->GetWorld()->GetCurLevelID();
        deathVolType = ((levelID >= 2) && (levelID < 4)) ? 2 : 1;
    }

    u16 soundID = 0;
    if (deathVolType == 1) {
        soundID = 8;
    }
    else if (deathVolType == 2) {
        soundID = 25;
    }
    else {
        return;
    }

    CSoundDirect::PlayTransient(soundID, nullptr, 0, 0);
}

static s32 SwitchPlayerDeathVol(Thing* thing, u32 argc, const char** argv) {
    if (!thing) {
        return 0;
    }

    thing->flags |= TF_NEEDS_ACTIVATION;

    s32 deathType = -1;
    if (argc > 0 && argv && argv[0]) {
        deathType = atol(argv[0]);
    }

    if (thing->thingType != AITypes::TT_PLAYER) {
        if (deathType >= 0) {
            PlayThingDeathVolSound(deathType);
        }
        return SwitchEnemyObstacleDeathVol(thing, argc, argv);
    }

    if (g_blockManager) {
        g_blockManager->SetDeathVolumeFlag(0);
    }

    if (g_director && g_director->TriggerDeathVolume(deathType)) {
        if (deathType == 4) {
            thing->health = 1;
        }
    }

    return 1;
}

// PSX: gfDirectorVol__FP5ThingUlPPCc (SWITCH.CPP:500, 0x800940D4)
static s32 SwitchDirectorVol(Thing* thing, u32 argc, const char** argv) {
    MARKFUNCTION(0x800940D4);

    if (!g_director || argc == 0 || !argv || !argv[0]) {
        return 0;
    }

    const s32 scriptIndex = atol(argv[0]);
    s32* script = Director::GetGlobalScriptByIndex(scriptIndex);
    if (!script) {
        return 0;
    }

    g_director->SetCodeSnip(script, thing);
    return 1;
}

static s32 SwitchGoToVol(Thing* thing, u32 argc, const char** argv) {
    if (argc < 3 || !argv || !g_director) {
        return 0;
    }

    const s32 x = argv[0] ? atol(argv[0]) : 0;
    const s32 y = argv[1] ? atol(argv[1]) : 0;
    const s32 z = argv[2] ? atol(argv[2]) : 0;

    g_director->TriggerGotoPoint(x, y, z, thing);
    return 1;
}

static bool IsLevelCompleteHumanoidType(u16 thingType) {
    // PSX switch table at 0x800D382C accepts only these thing types.
    return thingType == 10 || thingType == 12 || thingType == 13 ||
        thingType == 15 || thingType == 17;
}

// PSX: gfLevelComplete__FP5ThingUlPPCc (SWITCH.CPP:1096, 0x80094964)
static s32 SwitchLevelComplete(Thing* thing, u32 argc, const char** argv) {
    MARKFUNCTION(0x80094964);

    if (Player::s_player) {
        Player::s_player->encounterState = 3;
    }

    if (!g_ai || !g_director) {
        return 0;
    }

    for (u32 index = 0; index < argc; index++) {
        if (!argv || !argv[index]) {
            return 0;
        }

        const u32 crc = p3dHash(argv[index]);
        if (index == 0) {
            g_director->victoryBossCRC = crc;
        }

        ccNode* humNode = g_ai->humanoidList.FindNodeCRC(crc);
        if (humNode) {
            Humanoid* hum = static_cast<Humanoid*>(static_cast<Thing*>(humNode));
            if (!IsLevelCompleteHumanoidType(hum->thingType)) {
                return 0;
            }
            if (hum->actionState != AS_DEAD) {
                return 0;
            }
            continue;
        }

        ccNode* moveNode = g_ai->moveList.FindNodeCRC(crc);
        if (moveNode) {
            const u8* base = reinterpret_cast<const u8*>(moveNode);

            s32 lhs = 0;
            s32 rhs = 0;
            std::memcpy(&lhs, base + 192, sizeof(lhs));
            std::memcpy(&rhs, base + 200, sizeof(rhs));
            if (lhs < rhs) {
                return 0;
            }

            const void* ptr284 = nullptr;
            std::memcpy(&ptr284, base + 284, sizeof(ptr284));
            if (ptr284) {
                u8 loopCount = 0;
                std::memcpy(&loopCount, reinterpret_cast<const u8*>(ptr284) + 84, sizeof(loopCount));
                if (loopCount != 0) {
                    return 0;
                }
            }
        }
    }

    s32* levelEnd = Director::GetLevelEndScript();
    if (g_director->codeSnipPtr != levelEnd) {
        g_director->SetCodeSnip(levelEnd, thing);
    }

    return 1;
}

static s32 SwitchResetPlayer(Thing* /*thing*/, u32 /*argc*/, const char** /*argv*/) {
    MARKFUNCTION(0x800941DC);

    Player::s_player->Reset();
    g_display->GetCamera()->Reset();
    return 1;
}

static s32 SwitchBossVol(Thing* /*thing*/, u32 /*argc*/, const char** /*argv*/) {
    MARKFUNCTION(0x80094BBC);
    return 1;
}

static s32 SwitchGateCleanupVol(Thing* /*thing*/, u32 /*argc*/, const char** /*argv*/) {
    MARKFUNCTION(0x800942B8);

    if (!g_ai || !g_blockManager || g_blockManager->GetNumBlocks() == 0) {
        return 1;
    }

    const Block* loadedBlocks = g_blockManager->GetBlocks();
    u16 blockThreshold = BLOCK_UNASSIGNED;

    for (u32 index = 0; index < g_blockManager->GetNumBlocks(); index++) {
        const u16 blockNum = loadedBlocks[index].blockNum;
        if (blockNum < blockThreshold) {
            blockThreshold = blockNum;
        }
    }

    if (blockThreshold == BLOCK_UNASSIGNED) {
        return 1;
    }

    s32 cleanedCount = 0;

    for (ccMinNode* node = g_ai->humanoidList.head; node;) {
        Thing* current = static_cast<Thing*>(node);
        node = node->next;

        if (current->blockNum < blockThreshold) {
            cleanedCount++;
            current->Reset();
        }
    }

    for (ccMinNode* node = g_ai->moveList.head; node;) {
        Thing* current = static_cast<Thing*>(node);
        node = node->next;

        if (current->blockNum < blockThreshold) {
            cleanedCount++;
            current->Reset();
        }
    }

    LOG("[Switch] GateCleanupVol cleaned %d", cleanedCount);
    return 1;
}

static s32 SwitchExitTest(Thing* /*thing*/, u32 /*argc*/, const char** /*argv*/) {
    return 1;
}

struct SwitchGameFuncEntry {
    const char* name;
    SwitchGameFunc func;
    u32 bucket;
};

// PSX _9WDBSwitch_gameFuncs at 0x800D9778 (SWITCH.CPP)
// SoundAmbiantSpace, SwitchEntryTest, PlayerDeathVol, EnemyObstDeathVol,
// DirectorVol, GoToVol, SwitchExitTest, ResetPlayer, DeathState,
// BehaviorTrigger, ProximityEvent, GateCleanupVol, AsyncLoadNIS,
// AsyncLoadNISGOTO, AsyncLoadGroup, LevelComplete, Checkpoint,
// CharacterModelLoad, BossVol, PlayerLoadDialog, PlayerPlayDialog,
// EnemyLoadDialog, EnemyPlayDialog.
// Missing host handlers: DeathState.
static const SwitchGameFuncEntry kSwitchGameFuncs[] = {
    { "SoundAmbiantSpace", SwitchSoundAmbiantSpace, 1 },
    { "SwitchEntryTest", SwitchPlayerDeathVol, 1 },
    { "PlayerDeathVol", SwitchPlayerDeathVol, 0 },
    { "EnemyObstDeathVol", SwitchEnemyObstacleDeathVol, 2 },
    { "DirectorVol", SwitchDirectorVol, 1 },
    { "GoToVol", SwitchGoToVol, 1 },
    { "SwitchExitTest", SwitchExitTest, 1 },
    { "ResetPlayer", SwitchResetPlayer, 1 },
    { "BehaviorTrigger", SwitchBehaviorTrigger, 1 },
    { "ProximityEvent", SwitchBehaviorTrigger, 1 },
    { "GateCleanupVol", SwitchGateCleanupVol, 1 },
    { "AsyncLoadNIS", SwitchAsyncLoadNIS, 1 },
    { "AsyncLoadNISGOTO", SwitchAsyncLoadNISGOTO, 1 },
    { "AsyncLoadGroup", SwitchAsyncLoadGroup, 1 },
    { "LevelComplete", SwitchLevelComplete, 1 },
    { "Checkpoint", SwitchCheckpoint, 1 },
    { "CharacterModelLoad", SwitchCharModelLoad, 1 },
    { "BossVol", SwitchBossVol, 1 },
    { "PlayerLoadDialog", SwitchLoadDialog, 1 },
    { "PlayerPlayDialog", SwitchPlayDialog, 1 },
    { "EnemyLoadDialog", SwitchLoadDialog, 2 },
    { "EnemyPlayDialog", SwitchPlayDialog, 2 },
    { nullptr, nullptr, 0 },
};

static bool ResolveSwitchGameFuncByName(const char* funcName, SwitchGameFunc& outFunc, u32& outBucket) {
    if (!funcName) {
        return false;
    }

    for (const SwitchGameFuncEntry* entry = kSwitchGameFuncs; entry->name; entry++) {
        if (!SwitchStringEqualsNoCase(funcName, entry->name)) {
            continue;
        }

        outFunc = entry->func;
        outBucket = entry->bucket;
        return true;
    }

    return false;
}

struct GeoMaterialInfo {
    u8 primCmd = 0;
    u16 cba = 0;
    u16 tpage = 0;
};

static u32 GetDynGeoPrimPacketSize(u8 primCmd) {
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
}

static void DecodePackedUV(u16 packed, GeoRenderVertex& vertex, const GeoMaterialInfo& material) {
    vertex.u = static_cast<f32>(packed & 0xFF);
    vertex.v = static_cast<f32>((packed >> 8) & 0xFF);
    vertex.tpage = static_cast<f32>(material.tpage);
    vertex.cba = static_cast<f32>(material.cba);
}

static pddiPrimBuffer* ParseDynGeoPrims(
    const u8* geoData,
    u32 geoSize,
    const std::unordered_map<u32, GeoMaterialInfo>& materials,
    bool* outUsesSemiTrans,
    u8* outSemiTransMode,
    std::vector<GeoRenderVertex>* outVerts,
    std::vector<u32>* outPrimStart,
    std::vector<u8>* outPrimVertCount,
    std::vector<u32>* outPrimMaterialUID,
    std::vector<u8>* outPrimCmd,
    std::vector<u32>* outPrimPacketOffset)
{
    if (outUsesSemiTrans) {
        *outUsesSemiTrans = false;
    }
    if (outSemiTransMode) {
        *outSemiTransMode = 0;
    }

    if (!geoData || geoSize < 0x58) {
        return nullptr;
    }

    u32 vertListOff = p3dReadU32LE(geoData + 0x10) << 2;
    u16 numVerts = p3dReadU16LE(geoData + 0x14);
    u16 numPolys = p3dReadU16LE(geoData + 0x16);
    u32 polyListOff = p3dReadU32LE(geoData + 0x40) << 2;
    u32 colourListOff = p3dReadU32LE(geoData + 0x44) << 2;

    if (numVerts == 0 || numPolys == 0) {
        return nullptr;
    }
    if (vertListOff + numVerts * 8 > geoSize) {
        return nullptr;
    }
    if (polyListOff + numPolys * 24 > geoSize) {
        return nullptr;
    }

    const u8* verts = geoData + vertListOff;
    const u8* polys = geoData + polyListOff;
    const u8* colours = nullptr;
    if (colourListOff && colourListOff + numVerts * 4 <= geoSize) {
        colours = geoData + colourListOff;
    }

    std::vector<GeoRenderVertex> vertBuf;
    std::vector<u16> idxBuf;
    std::vector<u32> primStart;
    std::vector<u8> primVertCount;
    std::vector<u32> primMaterialUID;
    std::vector<u8> primCmdList;
    std::vector<u32> primPacketOffset;
    bool usesSemiTrans = false;
    bool hasSemiTransMode = false;
    u8 semiTransMode = 0;
    u32 primListPacketOffset = 0;

    auto makeVertex = [&](u16 index) -> GeoRenderVertex {
        GeoRenderVertex vertex = {};
        if (index >= numVerts) {
            return vertex;
        }

        const u8* src = verts + index * 8;
        vertex.x = static_cast<f32>(p3dReadS16LE(src + 0));
        vertex.y = static_cast<f32>(p3dReadS16LE(src + 2));
        vertex.z = static_cast<f32>(p3dReadS16LE(src + 4));

        // Default neutral colour for flat primitive paths.
        vertex.r = 0.85f;
        vertex.g = 0.85f;
        vertex.b = 0.85f;

        vertex.u = 0.0f;
        vertex.v = 0.0f;
        vertex.tpage = -1.0f; // default for untextured prims
        vertex.cba = 0.0f;
        return vertex;
    };

    auto applyVertexColour = [&](u16 index, GeoRenderVertex& vertex) {
        if (!colours || index >= numVerts) {
            return;
        }

        const u32 packedColour = p3dReadU32LE(colours + index * 4);
        vertex.r = std::min(1.0f, static_cast<f32>(packedColour & 0xFFu) / 128.0f);
        vertex.g = std::min(1.0f, static_cast<f32>((packedColour >> 8) & 0xFFu) / 128.0f);
        vertex.b = std::min(1.0f, static_cast<f32>((packedColour >> 16) & 0xFFu) / 128.0f);
    };

    for (u16 polyIndex = 0; polyIndex < numPolys; polyIndex++) {
        const u8* poly = polys + polyIndex * 24;
        u32 materialHash = p3dReadU32LE(poly + 0);
        auto materialIt = materials.find(materialHash);
        if (materialIt == materials.end()) {
            continue;
        }

        const GeoMaterialInfo& material = materialIt->second;
        if ((material.primCmd & 0x02u) != 0u) {
            usesSemiTrans = true;
            if (!hasSemiTransMode) {
                semiTransMode = static_cast<u8>((material.tpage >> 5) & 3u);
                hasSemiTransMode = true;
            }
        }

        u8 primCmd = static_cast<u8>(material.primCmd & 0xFD);
        const u32 packetSize = GetDynGeoPrimPacketSize(primCmd);
        if (packetSize == 0) {
            continue;
        }
        const u32 packetOffset = primListPacketOffset;
        primListPacketOffset += packetSize;

        GeoRenderVertex v0 = makeVertex(p3dReadU16LE(poly + 8));
        GeoRenderVertex v1 = makeVertex(p3dReadU16LE(poly + 10));
        GeoRenderVertex v2 = makeVertex(p3dReadU16LE(poly + 12));
        GeoRenderVertex v3 = makeVertex(p3dReadU16LE(poly + 14));

        // PSX colour-table modulation is for gouraud primitive commands only.
        // Flat commands carry colour through a separate packet lane.
        if ((primCmd & 0x10u) != 0u) {
            applyVertexColour(p3dReadU16LE(poly + 8), v0);
            applyVertexColour(p3dReadU16LE(poly + 10), v1);
            applyVertexColour(p3dReadU16LE(poly + 12), v2);
            applyVertexColour(p3dReadU16LE(poly + 14), v3);
        }

        if (primCmd == 0x34 || primCmd == 0x24 || primCmd == 0x3C || primCmd == 0x2C) {
            DecodePackedUV(p3dReadU16LE(poly + 16), v0, material);
            DecodePackedUV(p3dReadU16LE(poly + 18), v1, material);
            DecodePackedUV(p3dReadU16LE(poly + 20), v2, material);
            if (primCmd == 0x3C || primCmd == 0x2C) {
                DecodePackedUV(p3dReadU16LE(poly + 22), v3, material);
            }
        }

        u16 base = static_cast<u16>(vertBuf.size());
        switch (primCmd) {
            case 0x30:
            case 0x20:
            case 0x34:
            case 0x24:
                vertBuf.push_back(v2);
                vertBuf.push_back(v1);
                vertBuf.push_back(v0);

                // Keep rendered winding identical to (v0,v1,v2) while dynamic
                // vertex lanes follow PSX packet order (v2,v1,v0).
                idxBuf.push_back(base + 1);
                idxBuf.push_back(base + 2);
                idxBuf.push_back(base + 0);
                primStart.push_back(base);
                primVertCount.push_back(3);
                primMaterialUID.push_back(materialHash);
                primCmdList.push_back(primCmd);
                primPacketOffset.push_back(packetOffset);
                break;

            case 0x38:
            case 0x28:
            case 0x3C:
            case 0x2C:
                vertBuf.push_back(v3);
                vertBuf.push_back(v2);
                vertBuf.push_back(v1);
                vertBuf.push_back(v0);

                // Render as original (v0,v1,v2) + (v1,v3,v2) while packing
                // dynamic lanes in PSX packet order (v3,v2,v1,v0).
                idxBuf.push_back(base + 3);
                idxBuf.push_back(base + 2);
                idxBuf.push_back(base + 1);
                idxBuf.push_back(base + 2);
                idxBuf.push_back(base + 0);
                idxBuf.push_back(base + 1);
                primStart.push_back(base);
                primVertCount.push_back(4);
                primMaterialUID.push_back(materialHash);
                primCmdList.push_back(primCmd);
                primPacketOffset.push_back(packetOffset);
                break;

            default:
                break;
        }
    }

    if (idxBuf.empty()) {
        return nullptr;
    }

    // Temp diagnostic: dump vertex stats
    if (!vertBuf.empty()) {
        f32 minX = vertBuf[0].x, maxX = vertBuf[0].x;
        f32 minY = vertBuf[0].y, maxY = vertBuf[0].y;
        f32 minZ = vertBuf[0].z, maxZ = vertBuf[0].z;
        for (auto& v : vertBuf) {
            if (v.x < minX) minX = v.x; if (v.x > maxX) maxX = v.x;
            if (v.y < minY) minY = v.y; if (v.y > maxY) maxY = v.y;
            if (v.z < minZ) minZ = v.z; if (v.z > maxZ) maxZ = v.z;
        }
        LOG("[ParseGeo] verts=%u idx=%u bbox=(%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f) v0=(%.0f,%.0f,%.0f) tpage=%.0f cba=%.0f",
            (u32)vertBuf.size(), (u32)idxBuf.size(),
            minX, minY, minZ, maxX, maxY, maxZ,
            vertBuf[0].x, vertBuf[0].y, vertBuf[0].z, vertBuf[0].tpage, vertBuf[0].cba);
    }

    u32 format = PDDI_V_POSITION | PDDI_V_COLOUR | PDDI_V_UV | PDDI_V_TEXINFO;
    pddiPrimBufferDesc desc(
        PDDI_PRIM_TRIANGLES,
        format,
        static_cast<u32>(vertBuf.size()),
        static_cast<u32>(idxBuf.size()));

    pddiPrimBuffer* buffer = p3d::device->NewPrimBuffer(desc);
    buffer->SetVertexData(vertBuf.data(), static_cast<u32>(vertBuf.size()));
    buffer->SetIndices(idxBuf.data(), static_cast<u32>(idxBuf.size()));

    if (outUsesSemiTrans) {
        *outUsesSemiTrans = usesSemiTrans;
    }
    if (outSemiTransMode) {
        *outSemiTransMode = semiTransMode;
    }
    if (outVerts) {
        *outVerts = vertBuf;
    }
    if (outPrimStart) {
        *outPrimStart = primStart;
    }
    if (outPrimVertCount) {
        *outPrimVertCount = primVertCount;
    }
    if (outPrimMaterialUID) {
        *outPrimMaterialUID = primMaterialUID;
    }
    if (outPrimCmd) {
        *outPrimCmd = primCmdList;
    }
    if (outPrimPacketOffset) {
        *outPrimPacketOffset = primPacketOffset;
    }

    return buffer;
}

static void LoadGeoPair(
    World* world,
    const u8* permData,
    u32 permSize,
    const u8* p3dData,
    u32 p3dSize,
    s32 storeId)
{
    if (!g_levelManager || !permData || !p3dData || p3dSize < 6) {
        return;
    }

    if (p3dReadU16LE(p3dData) != 0xFF04) {
        return;
    }

        std::unordered_map<u32, GeoMaterialInfo> materials;

        // Track PRM (tPrimGeom) perm locations from 0x6009 chunks for STree lookup
        struct PrmInfo { u32 permOffset; u32 permSize; };
        std::unordered_map<u32, PrmInfo> prmMap; // nameHash → perm location

        u32 rootSize = p3dReadU32LE(p3dData + 2);
        u32 chunkEnd = (rootSize < p3dSize) ? rootSize : p3dSize;
        u32 chunkPos = 6;
        u32 permCursor = 0;

        while (chunkPos + 6 <= chunkEnd) {
            u16 chunkId = p3dReadU16LE(p3dData + chunkPos);
            u32 chunkSize = p3dReadU32LE(p3dData + chunkPos + 2);
            if (chunkSize < 6 || chunkPos + chunkSize > chunkEnd) {
                break;
            }

            const u8* chunkBody = p3dData + chunkPos + 6;
            const u32 permBefore = permCursor;

            if (chunkId == 0x6001 || chunkId == 0x6002) {
                u32 nameCount = p3dReadU32LE(chunkBody + 0);
                u32 chunkPermSize = p3dReadU32LE(chunkBody + 4);
                u32 namesPos = 8;
                std::vector<std::string> names;
                names.reserve(nameCount);

                for (u32 i = 0; i < nameCount; i++) {
                    if (namesPos >= chunkSize - 6) {
                        break;
                    }
                    u8 nameLen = chunkBody[namesPos++];
                    if (namesPos + nameLen > chunkSize - 6) {
                        break;
                    }
                    names.emplace_back(reinterpret_cast<const char*>(chunkBody + namesPos), nameLen);
                    namesPos += nameLen;
                }

                if (permCursor + chunkPermSize > permSize) {
                    LOG("[World] Geo perm overflow for chunk 0x%04X (need 0x%X, have 0x%X)",
                        chunkId, permCursor + chunkPermSize, permSize);
                    break;
                }

                if (chunkId == 0x6001) {
                    if (nameCount != 0) {
                        u32 recordSize = chunkPermSize / nameCount;
                        if (recordSize >= 24) {
                            for (u32 i = 0; i < nameCount; i++) {
                                u32 recordOff = permCursor + i * recordSize;
                                const u8* record = permData + recordOff;

                                GeoMaterialInfo info = {};
                                // PSX primitive command byte lives in the high byte of this word.
                                info.primCmd = static_cast<u8>((p3dReadU32LE(record + 16) >> 24) & 0xFF);
                                u32 texInfo = p3dReadU32LE(record + 20);
                                info.cba = static_cast<u16>(texInfo & 0xFFFF);
                                info.tpage = static_cast<u16>(texInfo >> 16);

                                u32 materialHash = p3dReadU32LE(record + 0);
                                if (materialHash == 0 && i < names.size()) {
                                    materialHash = p3dHash(names[i].c_str());
                                }
                                materials[materialHash] = info;
                                LOG("[GeoMat] hash=0x%08X primCmd=0x%02X tpage=%u cba=%u (tx=%u ty=%u depth=%u clutX=%u clutY=%u)",
                                    materialHash, info.primCmd, info.tpage, info.cba,
                                    info.tpage & 0xF, (info.tpage >> 4) & 1, (info.tpage >> 7) & 3,
                                    (info.cba & 0x3F) * 16, (info.cba >> 6) & 0x1FF);
                            }
                        }
                    }
                }
                else if (chunkId == 0x6002) {
                    if (nameCount != 1 || names.empty()) {
                        LOG("[World] Unsupported multi-geo chunk with %u entries", nameCount);
                    }
                    else {
                        u32 modelHash = p3dReadU32LE(permData + permCursor + 0);
                        if (!g_levelManager->FindModel(static_cast<s32>(modelHash))) {
                            bool usesSemiTrans = false;
                            u8 semiTransMode = 0;
                            std::vector<GeoRenderVertex> dynamicVerts;
                            std::vector<u32> dynamicPrimStart;
                            std::vector<u8> dynamicPrimVertCount;
                            std::vector<u32> dynamicPrimMaterialUID;
                            std::vector<u8> dynamicPrimCmd;
                            std::vector<u32> dynamicPrimPacketOffset;
                            pddiPrimBuffer* buffer = ParseDynGeoPrims(
                                permData + permCursor,
                                chunkPermSize,
                                materials,
                                &usesSemiTrans,
                                &semiTransMode,
                                &dynamicVerts,
                                &dynamicPrimStart,
                                &dynamicPrimVertCount,
                                &dynamicPrimMaterialUID,
                                &dynamicPrimCmd,
                                &dynamicPrimPacketOffset);
                            if (buffer) {
                                OriginalGeo* original = new OriginalGeo();
                                original->nameCRC = modelHash ? modelHash : p3dHash(names[0].c_str());
                                original->SetStoreID(static_cast<s8>(storeId));
                                original->meshBuffer = buffer;
                                original->usesSemiTrans = usesSemiTrans;
                                original->semiTransMode = semiTransMode;
                                original->bboxMin[0] = p3dReadS32LE(permData + permCursor + 0x18);
                                original->bboxMin[1] = p3dReadS32LE(permData + permCursor + 0x1C);
                                original->bboxMin[2] = p3dReadS32LE(permData + permCursor + 0x20);
                                original->bboxMax[0] = p3dReadS32LE(permData + permCursor + 0x24);
                                original->bboxMax[1] = p3dReadS32LE(permData + permCursor + 0x28);
                                original->bboxMax[2] = p3dReadS32LE(permData + permCursor + 0x2C);

                                if (!dynamicVerts.empty()) {
                                    original->dynamicVertCount = static_cast<u32>(dynamicVerts.size());
                                    original->dynamicVerts = new GeoRenderVertex[original->dynamicVertCount];
                                    if (original->dynamicVerts) {
                                        memcpy(original->dynamicVerts,
                                               dynamicVerts.data(),
                                               sizeof(GeoRenderVertex) * original->dynamicVertCount);
                                    }
                                }

                                if (!dynamicPrimStart.empty()
                                    && dynamicPrimStart.size() == dynamicPrimVertCount.size()
                                     && dynamicPrimStart.size() == dynamicPrimMaterialUID.size()
                                     && dynamicPrimStart.size() == dynamicPrimCmd.size()
                                     && dynamicPrimStart.size() == dynamicPrimPacketOffset.size()) {
                                    original->dynamicPrimCount = static_cast<u32>(dynamicPrimStart.size());
                                    original->dynamicPrimStart = new u32[original->dynamicPrimCount];
                                    original->dynamicPrimVertCount = new u8[original->dynamicPrimCount];
                                    original->dynamicPrimMaterialUID = new u32[original->dynamicPrimCount];
                                     original->dynamicPrimCmd = new u8[original->dynamicPrimCount];
                                     original->dynamicPrimPacketOffset = new u32[original->dynamicPrimCount];
                                    if (original->dynamicPrimStart
                                        && original->dynamicPrimVertCount
                                         && original->dynamicPrimMaterialUID
                                         && original->dynamicPrimCmd
                                         && original->dynamicPrimPacketOffset) {
                                        memcpy(original->dynamicPrimStart,
                                               dynamicPrimStart.data(),
                                               sizeof(u32) * original->dynamicPrimCount);
                                        memcpy(original->dynamicPrimVertCount,
                                               dynamicPrimVertCount.data(),
                                               sizeof(u8) * original->dynamicPrimCount);
                                        memcpy(original->dynamicPrimMaterialUID,
                                               dynamicPrimMaterialUID.data(),
                                               sizeof(u32) * original->dynamicPrimCount);
                                         memcpy(original->dynamicPrimCmd,
                                             dynamicPrimCmd.data(),
                                             sizeof(u8) * original->dynamicPrimCount);
                                         memcpy(original->dynamicPrimPacketOffset,
                                             dynamicPrimPacketOffset.data(),
                                             sizeof(u32) * original->dynamicPrimCount);
                                    }
                                }

                                g_levelManager->AddOriginal(original, 0);
                                LOG("[World] Loaded Geo model '%s' (hash 0x%08X, store %d)",
                                    names[0].c_str(), original->nameCRC, storeId);
                            }
                        }
                    }
                }

                permCursor += chunkPermSize;
            }

            // PSX: tVertAnimLoader::Load (TVRTLOAD.CPP:32)
            // Chunk 0x6004 consumes perm payload and registers tFrameList in INVANI.
            else if (chunkId == 0x6004) {
                if (!RegisterFrameListAnimChunk(chunkBody,
                                                chunkSize - 6,
                                                permData,
                                                permCursor,
                                                permSize,
                                                static_cast<u8>(storeId))) {
                    break;
                }
            }

            // PSX: tClutAnimLoader::Load (TCLTLOAD.CPP:29)
            // Chunk 0x6006 registers tClutList in INVANI.
            else if (chunkId == 0x6006) {
                if (!RegisterClutAnimChunk(chunkBody, chunkSize - 6, static_cast<u8>(storeId))) {
                    break;
                }
            }

            // PSX: tTranAnimLoader2::Load (TRANLOAD.CPP:98)
            // Chunk 0x6400 consumes perm payload and registers tTransformAnim in INVANI.
            else if (chunkId == 0x6400) {
                if (!RegisterTransformAnimChunk(chunkBody,
                                                chunkSize - 6,
                                                permData,
                                                permCursor,
                                                permSize,
                                                static_cast<u8>(storeId))) {
                    break;
                }
            }

            // PSX: tSequenceAnimLoader::Load (SEQUENCE.CPP:26)
            // Chunk 0x6040 consumes perm payload and registers a tSequenceAnim in INVANI.
            else if (chunkId == 0x6040) {
                if (!RegisterSequenceAnimChunk(chunkBody,
                                               chunkSize - 6,
                                               permData,
                                               permCursor,
                                               permSize,
                                               static_cast<u8>(storeId))) {
                    break;
                }
            }

            // PSX: tCompAnimLoader::Load (COMPANIM.CPP:78)
            // Chunk 0x4007 defines tCompositeAnim parts by animation name.
            else if (chunkId == 0x4007) {
                RegisterCompositeAnimChunk(chunkBody, chunkSize - 6, static_cast<u8>(storeId));
            }

            // PSX: tParamAnimLoader::Load (PARAMLOAD.CPP:279)
            // Chunk 0x4300 registers tParamAnim in INVANI.
            else if (chunkId == 0x4300) {
                if (!RegisterParamAnimChunk(chunkBody, chunkSize - 6, static_cast<u8>(storeId))) {
                    break;
                }
            }

            // PSX: tVizAnimLoader::Load (VIZANIM.CPP:116)
            // Chunk 0x4020 registers tVizAnim in INVANI (used by composite VIZ_* parts).
            else if (chunkId == 0x4020) {
                RegisterVizAnimChunk(chunkBody, chunkSize - 6, static_cast<u8>(storeId));
            }

            // PSX: tCBVParamAnimLoader::Load (CBVPARAM.CPP:243)
            // Chunk 0x6021 registers cbv_* misc anim tables in INVANI.
            else if (chunkId == 0x6021) {
                if (!RegisterCBVParamAnimChunk(chunkBody, chunkSize - 6, static_cast<u8>(storeId))) {
                    break;
                }
            }

            // PSX PALDATA.CPP: 0x8C00/0x8C01 use shared palette metadata + perm cursor.
            else if (chunkId == 0x8C00 || chunkId == 0x8C01) {
                if (!LoadPaletteData(chunkId, chunkBody, chunkSize - 6, permData, permCursor, permSize)) {
                    LOG("[World] Failed loading palette chunk 0x%04X", chunkId);
                    break;
                }
            }

            // PSX SCALEDAT.CPP: 0x8B01 consumes variable-size perm payload.
            else if (chunkId == 0x8B01) {
                if (!LoadScaleData(chunkId, chunkBody, chunkSize - 6, permData, permCursor, permSize)) {
                    LOG("[World] Failed loading ScaleData chunk 0x%04X", chunkId);
                    break;
                }
            }

            else if (chunkId == 0x8C20 || chunkId == 0x8C21) {
                LoadUVPrimData(chunkId, chunkBody, chunkSize - 6,
                               permData, permCursor, permSize);
            }

            else if (chunkId == 0x8C30 || chunkId == 0x8C31) {
                LoadCBVPrimData(chunkId, chunkBody, chunkSize - 6,
                                permData, permCursor, permSize);
            }

            else if (chunkId == 0x8A00) {
                LoadWEffectChunk(chunkBody, chunkSize - 6);
            }

            else if (chunkId == 0x8A10) {
                GEffect_LoadChunk(chunkBody, chunkSize - 6);
            }

            else if (chunkId == 0x8A20) {
                Obstacle_LoadAnimChunk(chunkBody, chunkSize - 6);
            }

            else if (chunkId == 0x8A30) {
                LoadParticleSystemChunk(chunkBody, chunkSize - 6);
            }

            // PSX: tETreeLoader::Load / myETreeLoaderCallback (STREAM.CPP:678)
            // Chunk 0x6140 = tETree. Body: pstring name, u16 jointCount, sub-chunks 0x6141.
            // Creates OriginalETree and registers in LevelManager for FindModel lookups.
            else if (chunkId == 0x6140) {
                u32 bodyLen = chunkSize - 6;
                if (bodyLen >= 3) {
                    u8 nameLen = chunkBody[0];
                    if ((u32)(1 + nameLen + 2) <= bodyLen) {
                        char nameBuf[256];
                        u32 copyLen = (nameLen < 255) ? nameLen : 255;
                        memcpy(nameBuf, chunkBody + 1, copyLen);
                        nameBuf[copyLen] = '\0';

                        u32 nameHash = p3dHash(nameBuf);
                        struct ETreePartEntry {
                            u32 jointHash;
                            u32 modelHash;
                        };
                        std::vector<ETreePartEntry> geoPartEntries;

                        u32 subCursor = 1u + static_cast<u32>(nameLen) + 2u;
                        while (subCursor + 6 <= bodyLen) {
                            const u16 subChunkId = p3dReadU16LE(chunkBody + subCursor);
                            const u32 subChunkSize = p3dReadU32LE(chunkBody + subCursor + 2);
                            if (subChunkSize < 6 || subCursor + subChunkSize > bodyLen) {
                                break;
                            }

                            if (subChunkId == 0x6141) {
                                const u8* subBody = chunkBody + subCursor + 6;
                                const u32 subBodyLen = subChunkSize - 6;
                                u32 subBodyCursor = 0;
                                u32 jointNameHash = 0;
                                u32 modelNameHash = 0;

                                if (ReadChunkPStringHash(subBody, subBodyLen, subBodyCursor, &jointNameHash)
                                    && ReadChunkPStringHash(subBody, subBodyLen, subBodyCursor, &modelNameHash)
                                    && modelNameHash != 0
                                    && modelNameHash != nameHash) {
                                    ETreePartEntry entry = {};
                                    entry.jointHash = jointNameHash;
                                    entry.modelHash = modelNameHash;
                                    geoPartEntries.push_back(entry);
                                }
                            }

                            subCursor += subChunkSize;
                        }

                        // PSX: always creates ETree (no duplicate check against Geo list).
                        // Both Geo and ETree can coexist with the same nameCRC in different lists.
                        OriginalETree* et = new OriginalETree();
                        et->nameCRC = nameHash;
                        et->SetStoreID(static_cast<s8>(storeId));

                        if (!geoPartEntries.empty()) {
                            const u16 partCount = static_cast<u16>(geoPartEntries.size());
                            et->geoPartCount = partCount;
                            et->geoPartHashes = new u32[partCount]();
                            et->geoPartJointHashes = new u32[partCount]();
                            et->geoParts = new OriginalGeo*[partCount]();

                            u16 resolvedParts = 0;
                            for (u16 partIndex = 0; partIndex < partCount; partIndex++) {
                                const ETreePartEntry& entry = geoPartEntries[partIndex];
                                const u32 modelHash = entry.modelHash;
                                et->geoPartJointHashes[partIndex] = entry.jointHash;
                                et->geoPartHashes[partIndex] = modelHash;

                                OriginalBasic* geoBasic = g_levelManager->FindGeo(static_cast<s32>(modelHash));
                                if (geoBasic && geoBasic->GetType() == 0) {
                                    et->geoParts[partIndex] = static_cast<OriginalGeo*>(geoBasic);
                                    if (et->geoParts[partIndex] && et->geoParts[partIndex]->meshBuffer) {
                                        resolvedParts++;
                                    }
                                }

                                LOG("[World] ETree '%s' part[%u] jointHash=0x%08X modelHash=0x%08X resolved=%d",
                                    nameBuf,
                                    partIndex,
                                    et->geoPartJointHashes ? et->geoPartJointHashes[partIndex] : 0,
                                    modelHash,
                                    (et->geoParts[partIndex] && et->geoParts[partIndex]->meshBuffer) ? 1 : 0);
                            }

                            LOG("[World] ETree '%s' parts=%u resolved=%u", nameBuf, partCount, resolvedParts);
                        }

                        g_levelManager->AddOriginal(et, 0);
                        LOG("[World] Loaded ETree '%s' (hash 0x%08X, store %d)", nameBuf, nameHash, storeId);
                    }
                }
            }

            else if (chunkId == 0x6008 && world) {
                u32 p = 0;
                u32 bodyLen = chunkSize - 6;
                if (bodyLen < 1) { chunkPos += chunkSize; continue; }
                u8 nameLen = chunkBody[p++];
                p += nameLen;
                if (p + 12 > bodyLen) { chunkPos += chunkSize; continue; }
                s16 rx = p3dReadS16LE(chunkBody + p); p += 2;
                s16 ry = p3dReadS16LE(chunkBody + p); p += 2;
                s16 rw = p3dReadS16LE(chunkBody + p); p += 2;
                s16 rh = p3dReadS16LE(chunkBody + p); p += 2;
                p += 4; // skip type
                if (rw > 0 && rh > 0 && rw <= 1024 && rh <= 512 &&
                    p + (u32)(rw * rh * 2) <= bodyLen) {
                    world->UploadToVRAM(rx, ry, rw, rh, chunkBody + p);
                    LOG("[GeoTex] VRAM upload: x=%d y=%d w=%d h=%d", rx, ry, rw, rh);
                }
            }

            // PSX: tPrimLoader::Load (TPRMLOAD.CPP:61, 0x80088A80)
            // Chunk 0x6009 = tPrimGeom. Body: u32 permSize, p-string name.
            // Creates tPrimGeom from perm data at current cursor, stores in P3D inventory.
            // PC: record perm offset/size for later STree lookup, advance permCursor.
            else if (chunkId == 0x6009) {
                u32 bodyLen = chunkSize - 6;
                if (bodyLen >= 5) {
                    u32 prmPermSize = p3dReadU32LE(chunkBody + 0);
                    u8 prmNameLen = chunkBody[4];
                    if ((u32)(5 + prmNameLen) <= bodyLen) {
                        char prmNameBuf[256];
                        u32 copyLen = (prmNameLen < 255) ? prmNameLen : 255;
                        memcpy(prmNameBuf, chunkBody + 5, copyLen);
                        prmNameBuf[copyLen] = '\0';
                        u32 prmHash = p3dHash(prmNameBuf);

                        if (permCursor + prmPermSize <= permSize) {
                            prmMap[prmHash] = { permCursor, prmPermSize };
                            LOG("[World] PRM '%s' (hash 0x%08X) at permOff=%u size=%u",
                                prmNameBuf, prmHash, permCursor, prmPermSize);
                        }
                        permCursor += prmPermSize;
                    }
                }
            }

            // PSX: tSTreeLoader::Load / mySTreeLoaderCallback (STREAM.CPP:704, 0x8009976C)
            // Chunk 0x6120 = tSTree. Body: p-string name, u16 jointCount, p-string prmName,
            // u32 permSize, sub-chunks 0x6121 (tSJoint), optional 0x6122 (joint map).
            // Creates OriginalSTree with nameCRC, references PRM for geometry data.
            // PC: create OriginalSTree, build mesh from PRM perm data, register via AddOriginal.
            else if (chunkId == 0x6120) {
                u32 bodyLen = chunkSize - 6;
                u32 p = 0;
                if (bodyLen >= 3) {
                    u8 nameLen = chunkBody[p++];
                    if (p + nameLen + 2 <= bodyLen) {
                        char nameBuf[256];
                        u32 copyLen = (nameLen < 255) ? nameLen : 255;
                        memcpy(nameBuf, chunkBody + p, copyLen);
                        nameBuf[copyLen] = '\0';
                        p += nameLen;

                        u16 jointCount = p3dReadU16LE(chunkBody + p); p += 2;

                        // Read PRM name
                        char prmNameBuf[256] = {};
                        if (p < bodyLen) {
                            u8 prmNameLen = chunkBody[p++];
                            u32 prmCopy = (prmNameLen < 255) ? prmNameLen : 255;
                            if (p + prmCopy <= bodyLen) {
                                memcpy(prmNameBuf, chunkBody + p, prmCopy);
                                prmNameBuf[prmCopy] = '\0';
                                p += prmNameLen;
                            }
                        }

                        // Read permSize (perm consumed by STree joint data)
                        u32 streePermSize = 0;
                        if (p + 4 <= bodyLen) {
                            streePermSize = p3dReadU32LE(chunkBody + p);
                            p += 4;
                        }

                        u32 nameHash = p3dHash(nameBuf);

                        OriginalSTree* original = new OriginalSTree();
                        original->nameCRC = nameHash;
                        original->SetStoreID(static_cast<s8>(storeId));
                        original->xformVertsCallback = RP_XformVertsLitCBF_CL;
                        original->fixUpPolysCallback = RP_FixUpPolysCBF_CL;
                        original->skeleton = ParseSTreeChunk(chunkBody, bodyLen, false);

                        // Look up PRM geometry data from perm
                        u32 prmHash = p3dHash(prmNameBuf);
                        auto prmIt = prmMap.find(prmHash);
                        if (prmIt != prmMap.end()) {
                            const PrmInfo& prm = prmIt->second;
                            if (prm.permOffset + prm.permSize <= permSize) {
                                if (original->skeleton) {
                                    BuildPerJointMeshes(original,
                                        permData + prm.permOffset, prm.permSize);
                                }

                                if (original->skeleton && original->skinData &&
                                    original->skeleton->joints &&
                                    original->skeleton->numJoints > 0 &&
                                    original->skeleton->joints[0].meshBuffer) {
                                    LOG("[World] STree '%s' skinned mesh built from PRM '%s' (%u verts, %u joints)",
                                        nameBuf, prmNameBuf,
                                        original->skeleton->joints[0].meshBuffer->GetVertexCount(),
                                        original->skeleton->numJoints);
                                }
                                else {
                                    pddiPrimBuffer* meshBuf = BuildPrimBufferFromRawPrimGeom(
                                        permData + prm.permOffset, prm.permSize);
                                    if (meshBuf) {
                                        original->meshBuffer = meshBuf;
                                        LOG("[World] STree '%s' mesh built from PRM '%s' (%u verts)",
                                            nameBuf, prmNameBuf, meshBuf->GetVertexCount());
                                    }
                                    else {
                                        LOG("[World] STree '%s' PRM '%s' mesh parse failed",
                                            nameBuf, prmNameBuf);
                                    }
                                }
                            }
                        } else {
                            LOG("[World] STree '%s' PRM '%s' not found in prmMap",
                                nameBuf, prmNameBuf);
                        }

                        g_levelManager->AddOriginal(original, 0);
                        permCursor += streePermSize;

                        LOG("[World] Loaded STree '%s' (hash 0x%08X, %u joints, store %d)",
                            nameBuf, nameHash, jointCount, storeId);
                    }
                }
            }

            if (storeId == 2 && ((permBefore >= 32000 && permBefore <= 34000)
                || chunkId == 0x8C20 || chunkId == 0x8C21)) {
                LOG("[World] GeoPair chunk: store=%d id=0x%04X chunkPos=%u bodyLen=%u permBefore=%u permAfter=%u",
                    storeId,
                    chunkId,
                    chunkPos,
                    chunkSize - 6,
                    permBefore,
                    permCursor);
            }

            chunkPos += chunkSize;
        }
    }

static void LoadGeoPairsInRange(
    World* world,
    const std::vector<tStreamEntry>& entries,
    const u8* fileData,
    u32 fileSize,
    u32 rangeStart,
    u32 rangeEnd,
    const char* permMagic,
    const char* p3dMagic,
    s32 storeId)
{
    (void)p3dMagic;

    for (u32 i = rangeStart; i < rangeEnd; i++) {
        if (strncmp(entries[i].magic, permMagic, 4) != 0) {
            continue;
        }

        // PSX STREAM.CPP LoadPermChunk__6Stream consumes the current entry as
        // perm payload and then advances to the immediate next stream entry for
        // the paired P3D payload.
        const u32 pairIndex = i + 1;
        if (pairIndex >= rangeEnd) {
            continue;
        }

        const tStreamEntry& permEntry = entries[i];
        const tStreamEntry& p3dEntry = entries[pairIndex];
        if (permEntry.offset + permEntry.size > fileSize ||
            p3dEntry.offset + p3dEntry.size > fileSize) {
            continue;
        }

        LoadGeoPair(
            world,
            fileData + permEntry.offset,
            permEntry.size,
            fileData + p3dEntry.offset,
            p3dEntry.size,
            storeId);
    }
}

static bool LoadBlocksForPetalFromStream(
    BlockManager& blockMgr,
    const std::vector<u8>& streamData,
    u32 targetPetal,
    u32 startBlockNum,
    const char* logTag)
{
    if (streamData.empty()) {
        return false;
    }

    const u32 dataSize = static_cast<u32>(streamData.size());
    const u8* data = streamData.data();
    const auto entries = ParseStreamHeader(data, dataSize);
    if (entries.empty()) {
        return false;
    }

    std::vector<u32> wdbIndices;
    for (u32 i = 0; i < (u32)entries.size(); i++) {
        if (strncmp(entries[i].magic, ".WDB", 4) == 0) {
            wdbIndices.push_back(i);
        }
    }
    if (wdbIndices.empty()) {
        return false;
    }

    u32 petalIndex = targetPetal;
    if (petalIndex >= (u32)wdbIndices.size()) {
        petalIndex = 0;
    }

    const u32 petalStart = wdbIndices[petalIndex];
    const u32 petalEnd = (petalIndex + 1 < (u32)wdbIndices.size())
                             ? wdbIndices[petalIndex + 1]
                             : (u32)entries.size();

    std::vector<const u8*> blkPtrs;
    std::vector<u32> blkSizes;
    for (u32 i = petalStart; i < petalEnd; i++) {
        if (strncmp(entries[i].magic, ".BLK", 4) != 0) {
            continue;
        }

        if (entries[i].offset + entries[i].size > dataSize) {
            blkPtrs.push_back(nullptr);
            blkSizes.push_back(0);
        }
        else {
            blkPtrs.push_back(data + entries[i].offset);
            blkSizes.push_back(entries[i].size);
        }
    }

    const u32 blkCount = static_cast<u32>(blkPtrs.size());
    blockMgr.LoadBlocks(startBlockNum, blkPtrs.data(), blkSizes.data(), blkCount);

    LOG("[%s] Parsed %u BLK entries for petal %u startBlock=%u", logTag, blkCount, petalIndex, startBlockNum);
    return true;
}
// Database::Scan handles WDB parsing now (see database.cpp).

void World::LoadTPGTextures(const u8* lcfData, u32 lcfSize) {
    vram.Clear();

    // Re-parse stream header to find TPG entries
    if (lcfSize < 4) return;
    u32 count = (lcfData[0] << 24) | (lcfData[1] << 16) | (lcfData[2] << 8) | lcfData[3];
    u32 pos = 4;
    for (u32 i = 0; i < count; i++) {
        if (pos + 16 > lcfSize) break;
        char magic[5] = {};
        memcpy(magic, lcfData + pos, 4);
        u32 size = (lcfData[pos + 4] << 24) | (lcfData[pos + 5] << 16) | (lcfData[pos + 6] << 8) | lcfData[pos + 7];
        u32 offset = (lcfData[pos + 8] << 24) | (lcfData[pos + 9] << 16) | (lcfData[pos + 10] << 8) | lcfData[pos + 11];
        u32 extraLen = (lcfData[pos + 12] << 24) | (lcfData[pos + 13] << 16) | (lcfData[pos + 14] << 8) | lcfData[pos + 15];
        pos += 16;
        if (extraLen > 0)
            pos += (extraLen + 3) & ~3;

        if (strncmp(magic, ".TPG", 4) != 0) continue;
        if (offset + size > lcfSize || size < 6) continue;

        const u8* d = lcfData + offset;
        u16 rootId = p3dReadU16LE(d);
        u32 rootSize = p3dReadU32LE(d + 2);
        if (rootId != 0xFF04) continue;

        u32 cpos = 6;
        u32 cend = (rootSize < size) ? rootSize : size;
        while (cpos + 6 <= cend) {
            u16 chunkId = p3dReadU16LE(d + cpos);
            u32 chunkSize = p3dReadU32LE(d + cpos + 2);
            if (chunkSize < 6 || cpos + chunkSize > cend) break;

            if (chunkId == 0x6008) {
                u32 doff = cpos + 6;
                u32 dlen = chunkSize - 6;
                u32 p = doff;

                // PString: u8 len + chars
                if (p >= cpos + chunkSize) { cpos += chunkSize; continue; }
                u8 nameLen = d[p++];
                p += nameLen; // skip name

                // RECT16: s16 x, y, w, h + u32 type
                if (p + 12 > doff + dlen) { cpos += chunkSize; continue; }
                s16 rx = p3dReadS16LE(d + p); p += 2;
                s16 ry = p3dReadS16LE(d + p); p += 2;
                s16 rw = p3dReadS16LE(d + p); p += 2;
                s16 rh = p3dReadS16LE(d + p); p += 2;
                p += 4; // skip type

                // Upload raw pixel data to VRAM
                if (rw > 0 && rh > 0 && rw <= 1024 && rh <= 512 &&
                    p + rw * rh * 2 <= offset + size) {
                    vram.Upload(rx, ry, rw, rh, d + p);
                    LOG("[World] VRAM upload: x=%d y=%d w=%d h=%d", rx, ry, rw, rh);
                }
            }
            cpos += chunkSize;
        }
    }

    // Upload raw VRAM as R16UI texture for shader-side palette lookup
    if (vramHandle) {
        p3d::context->DestroyVRAMTexture(vramHandle);
        vramHandle = 0;
    }
    vramHandle = p3d::context->CreateVRAMTexture(1024, 512, vram.data);
    LOG("[World] Uploaded raw VRAM as R16UI (1024x512, handle=%u)", vramHandle);
}

World::World() {
    p3d::rawTextureUploader = UploadRawTextureToWorldVRAM;
}

World::~World() {
    if (p3d::rawTextureUploader == UploadRawTextureToWorldVRAM) {
        p3d::rawTextureUploader = nullptr;
    }

    Unload();
    // Free level table data
    if (levelList) { delete[] levelList; levelList = nullptr; }
    if (highestPetal) { delete[] highestPetal; highestPetal = nullptr; }
    if (levelNames) {
        for (s32 i = 0; i < levelCount; i++)
            delete[] levelNames[i];
        delete[] levelNames;
        levelNames = nullptr;
    }
    if (petalNames) {
        for (s32 i = 0; i < levelCount; i++) {
            if (petalNames[i]) {
                s32 pc = levelList ? levelList[i * 2 + 1] : 0;
                for (s32 j = 0; j <= pc; j++)
                    delete[] petalNames[i][j];
                delete[] petalNames[i];
            }
        }
        delete[] petalNames;
        petalNames = nullptr;
    }
    if (petalSoundIDs) {
        for (s32 i = 0; i < levelCount; i++)
            delete[] petalSoundIDs[i];
        delete[] petalSoundIDs;
        petalSoundIDs = nullptr;
    }
}

void World::PurgeSwitches() {
    ClearPendingCharModelLoads();

    for (ccList& list : switchLists) {
        while (ccMinNode* node = list.RemHead()) {
            delete node;
        }
    }
}

void World::ProcessPendingSwitchActions() {
    if (g_pendingCharModelLoads.empty()) {
        return;
    }

    for (const PendingCharModelLoad& request : g_pendingCharModelLoads) {
        SwitchUnloadLoadCharModel(request.oldType, request.newType);
    }

    g_pendingCharModelLoads.clear();
}

void World::ProcessSwitches() {
    PurgeSwitches();

    if (!g_database) {
        return;
    }

    for (DBRoot* root = static_cast<DBRoot*>(g_database->GetFirstVolume()); root;
         root = static_cast<DBRoot*>(root->next)) {
        if (root->type != 9 || root->subType != 0x9B) {
            continue;
        }

        WDBVolumeSwitch* sw = new WDBVolumeSwitch();
        if (!sw->Setup(root) || !sw->Bind(ResolveSwitchGameFuncByName)) {
            delete sw;
            continue;
        }

        sw->SetVolume(static_cast<DBVolume*>(root));
        if (sw->listBucket < 4) {
            switchLists[sw->listBucket].AddNode(switchLists[sw->listBucket].tail, sw);
        }
        else {
            delete sw;
        }
    }
}

void World::CheckSwitchList(ccList& list, Thing* thing) {
    if (!thing) {
        return;
    }

    for (ccMinNode* node = list.head; node;) {
        WDBSwitch* sw = static_cast<WDBSwitch*>(node);
        node = node->next;

        if (sw->IsInside(thing->pos)) {
            sw->Execute(thing);
            if (sw->persistent != 0) {
                list.RemNode(sw);
                delete sw;
            }
        }
        else {
            sw->Reject(thing);
        }
    }
}

void World::CheckThingSwitches(Thing* thing) {
    if (!thing) {
        return;
    }

    CheckSwitchList(switchLists[0], thing);
    CheckSwitchList(switchLists[3], thing);

    u32 bucket = (thing->thingType != 0) ? 2u : 1u;
    CheckSwitchList(switchLists[bucket], thing);
}

WDBSwitch* World::FindPrimarySwitchByCRC(u32 crc) {
    // PSX Platform path-node attrib 11 uses FindNodeCRC(theWorldMgr + 0x6C, crc, 0).
    ccNode* node = switchLists[0].FindNodeCRC(crc, nullptr);
    return node ? static_cast<WDBSwitch*>(node) : nullptr;
}

// Simple tokenizer matching PSX GetNextToken__4Game
static bool GetNextToken(char* out, char** cursor, const char* delims) {
    char* p = *cursor;
    while (*p && strchr(delims, *p))
        p++;
    if (!*p) {
        *out = '\0';
        return false;
    }
    char* dst = out;
    while (*p && !strchr(delims, *p))
        *dst++ = *p++;
    *dst = '\0';
    *cursor = p;
    return true;
}

// PSX: LoadLevelNames__5World (WORLD.CPP:990, 0x80045700)
void World::LoadLevelNames() {
    // Local arrays matching PSX stack layout (max 16 levels, 16 petals each)
    char* tmpLevNames[16] = {};
    s32 tmpLevIDs[16] = {};
    s32 tmpPetalIdx[16][16] = {};
    s32 tmpSoundBytes[16][16] = {};
    char* tmpPetalNames[16][16] = {};
    s32 tmpPetalCounts[16] = {};

    // Read RTARGET/GAME_LN.TXT
    char filename[128];
    std::snprintf(filename, sizeof(filename), "RTARGET/GAME_LN.TXT");

    std::ifstream file(filename);
    if (!file)
        return;

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    levelCount = 0;
    char* cursor = content.data();
    char token[128];
    s32 levIdx = -1;
    s32 petalSeq = -1;

    // PSX: tokenize with " \r\n\t", matching the comma-operator ++v4 pattern
    while (GetNextToken(token, &cursor, " \r\n\t")) {
        if (token[0] == 'L' || token[0] == 'l') {
            // Level line: "levNN"
            ++levIdx;
            tmpLevIDs[levIdx] = atoi(token + 3);
            GetNextToken(token, &cursor, "\r\n");
            s32 len = (s32)strlen(token);
            tmpLevNames[levIdx] = new char[len + 1];
            memcpy(tmpLevNames[levIdx], token, len + 1);
            petalSeq = -1;
            ++levelCount;
        }
        else {
            // Petal line: <index> <soundByte> <name>
            ++petalSeq;
            tmpPetalIdx[levIdx][petalSeq] = atoi(token);
            GetNextToken(token, &cursor, " \t");
            tmpSoundBytes[levIdx][petalSeq] = atoi(token);
            GetNextToken(token, &cursor, "\r\n");
            s32 len = (s32)strlen(token);
            tmpPetalNames[levIdx][petalSeq] = new char[len + 1];
            memcpy(tmpPetalNames[levIdx][petalSeq], token, len + 1);
            ++tmpPetalCounts[levIdx];
        }
    }

    // Build levelList: pairs of {levelID, petalCount}
    levelList = new s32[levelCount * 2];
    highestPetal = new s32[levelCount];
    for (s32 i = 0; i < levelCount; i++) {
        levelList[i * 2] = tmpLevIDs[i];
        levelList[i * 2 + 1] = tmpPetalCounts[i];
    }

    // Build levelNames
    levelNames = new char* [levelCount + 1];
    levelNames[levelCount] = nullptr;

    // Build petalNames sub-arrays (null-initialized)
    petalNames = new char** [levelCount + 1];
    petalNames[levelCount] = nullptr;
    for (s32 i = 0; i <= levelCount; i++) {
        if (i == levelCount) break;
        s32 pc = tmpPetalCounts[i];
        petalNames[i] = new char* [pc + 1];
        for (s32 j = 0; j <= pc; j++)
            petalNames[i][j] = nullptr;
    }

    // Copy level names, petal names, and compute highestPetal
    for (s32 i = 0; i < levelCount; i++) {
        s32 len = (s32)strlen(tmpLevNames[i]);
        levelNames[i] = new char[len + 1];
        memcpy(levelNames[i], tmpLevNames[i], len + 1);
        delete[] tmpLevNames[i];

        s32 pc = tmpPetalCounts[i];
        for (s32 j = 0; j < pc; j++) {
            s32 pidx = tmpPetalIdx[i][j];
            s32 nlen = (s32)strlen(tmpPetalNames[i][j]);
            petalNames[i][pidx] = new char[nlen + 1];
            memcpy(petalNames[i][pidx], tmpPetalNames[i][j], nlen + 1);
            delete[] tmpPetalNames[i][j];
        }

        highestPetal[i] = tmpPetalIdx[i][pc - 1];
    }

    // Build petalSoundIDs (u8 arrays, sequential order)
    petalSoundIDs = new u8 * [levelCount];
    for (s32 i = 0; i < levelCount; i++) {
        s32 pc = tmpPetalCounts[i];
        petalSoundIDs[i] = new u8[pc];
        for (s32 j = 0; j < pc; j++)
            petalSoundIDs[i][j] = (u8)tmpSoundBytes[i][j];
    }
}

// PSX: LoadPermanent__5World (WORLD.CPP:1062, 0x80045D6C)
void World::LoadPermanent() {
    LoadLevelNames();

    // PSX: LoadLevel__12LevelManager() - no-op on PSX
    // PSX: PurgeLevelP3DInventory__12LevelManager() - also no-op

    // PSX: OpenCharacter(type=0), EnableCache(type=0, 1)
    if (g_characterManager) {
        g_characterManager->OpenCharacter(0);
        g_characterManager->EnableCache(0, 1);

        // PSX: allocate CharMgrCallback, LoadCharacter(type=0, callback), spin until done
        CharMgrCallback* callback = new CharMgrCallback();
        g_characterManager->LoadCharacter(0, callback);
        // PSX spins: while (!callback->done) rDoTaskList(rMainTaskList, 0);
        // PC: LoadCharacter is synchronous, callback already fired

        // PSX: LoadAnimation(type=0, animEnum=0, count=124, callback), spin until done
        callback->done = 0;
        g_characterManager->LoadAnimation(0, 0, 124, callback);
        // PSX spins again - PC is synchronous

        // PSX queues the same load again asynchronously here. In the host sync port,
        // reissuing it would repeat the full 124-animation traversal on the main thread.

        // PSX: EnableCache(type=0, 0), delete callback
        g_characterManager->EnableCache(0, 0);
        delete callback;
    }

    // PSX: AddThingNoTagList("Jackie", 0, {0,0,0}, {0,0,0}, "JACKIELOHIER", nullptr)
    if (g_ai) {
        LVector zeroPos = { 0, 0, 0 };
        SVector zeroOrient = { 0, 0, 0 };
        g_ai->AddThingNoTagList("Jackie", 0, &zeroPos, &zeroOrient, "JACKIELOHIER", nullptr);
    }
}

// PSX: LoadLevel__5WorldUl (WORLD.CPP:1389, 0x8004624C)
bool World::LoadLevelIndex(u32 levelIndex) {
    MARKFUNCTION(0x8004624C);

    // PSX: clamp levelIndex to valid range
    if (levelCount > 0 && levelIndex >= (u32)levelCount)
        levelIndex = (u32)(levelCount - 1);

    u32 prevLevel = currentLevelIndex;
    currentLevelIndex = levelIndex;
    previousLevelIndex = prevLevel;

    targetLevelIndex = levelIndex;

    // PSX callers keep target petal in-range for the destination level.
    // Clamp here so subsequent sound/block indexing stays consistent.
    u32 levelPetalCount = 1;
    if (levelList && levelIndex < (u32)levelCount) {
        s32 count = levelList[levelIndex * 2 + 1];
        if (count > 0) {
            levelPetalCount = (u32)count;
        }
    }
    if (targetPetalIndex >= levelPetalCount) {
        targetPetalIndex = 0;
    }

    // PSX: EstimateLoadTime, StartLogo, FillMeter(100)
    StartLogo("RUNFIRST.TIM");
    FillMeter(100);

    // PSX: rSPrintf(v8, "%slev%02d.lcf", "RTARGET\\", levelList[levelIndex * 2])
    char levelPath[64];
    s32 levNum = (levelList && levelCount > 0) ? levelList[levelIndex * 2] : (s32)(levelIndex + 1);
    std::snprintf(levelPath, sizeof(levelPath), "RTARGET/LEV%02d.LCF", levNum);
    if (!Load(levelPath)) {
        StopLogo();
        return false;
    }

    currentPetalIndex = targetPetalIndex;

    // Level-begin stats must reset before Populate registers collectibles.
    if (g_scoreManager) {
        g_scoreManager->HandleLevelBegin();
    }

    if (g_hud) {
        g_hud->OnLoadLevel();
    }

    // PSX: rsEvent(4, petalSoundIDs[levelIndex][targetPetalIndex] - 1, 0, 0)
    if (petalSoundIDs && levelIndex < (u32)levelCount) {
        s32 soundLocation = (s32)petalSoundIDs[levelIndex][targetPetalIndex] - 1;
        rsEvent(RS_SET_LOCATION, soundLocation, 0, 0);
    }

    // PSX: ExecuteLoadCallbacks -> cameraLoadFunc -> SetupPaths
    // (handled by gsQueueLevelLoad on PC)

    // PSX: Construct__5World (WORLD.CPP:1399, 0x80046E80)
    // On PSX this is a separate function called after LoadLevel.
    // It initializes fighting collision, effects, populates AI entities,
    // loads backgrounds, resets Director, and sets up the level script.
    // We inline the steps we can handle here.

    blockMgr.SetDeathVolumeFlag(1);

    // PSX: Init__17FightingCollision, InsertHumanoid (player)
    FightingCollision::Init();
    if (Player::s_player) {
        FightingCollision::InsertHumanoid(static_cast<Humanoid*>(Player::s_player));
    }

    // PSX: CheckpointInfo
    u32 startBlockNum = 0;
    bool hasCheckpoint = false;
    if (Player::s_player && Player::s_player->checkpoint.IsValid()) {
        startBlockNum = (u32)Player::s_player->checkpoint.field24;
        hasCheckpoint = true;
        if (g_scoreManager) {
            g_scoreManager->HandleCheckpointBegin();
        }
    }

    // PSX: InitWorldEffects__7WEffectP7DBPoint
    if (g_database) {
        WEffect_InitWorldEffects(g_database->GetFirstPoint());
        PWEffect_InitWorldEffects(g_database->GetFirstPoint());
    }

    ParticleSystem_InitParticleInfoMemory();

    // PSX: Populate__2AI(0) - spawn entities from WDB database
    if (g_ai) {
        g_ai->Populate();
    }

    // PSX: v5 = player->blockNum (after Populate sets it from attrib 15)
    u16 playerBlockNum = 0x1000;
    if (Player::s_player) {
        playerBlockNum = Player::s_player->blockNum;
    }

    // PSX: if no checkpoint, start block = player's block
    if (!hasCheckpoint) {
        startBlockNum = playerBlockNum;
    }

    // PSX: LoadBG, InitBG - background rendering
    // TODO: BackG not yet reversed

    // PSX: ScoreManager::SetPar
    if (g_scoreManager) {
        g_scoreManager->SetPar();
    }

    // PSX: Director->Reset() then Director->SetScript()
    if (g_director) {
        g_director->Reset();
        g_director->SetScript();
    }

    if (g_environmentManager && g_levelManager) {
        g_environmentManager->SetupModelAmbientLighting(&g_levelManager->modelLists[0]);
    }

    // PSX: SetupModelAmbientLighting, ProcessSwitches
    ProcessSwitches();

    // PSX: Close__8Database(0)
    // PC: deferred until Game::gsQueueLevelLoad after CameraManager::SetupPaths,
    // because we still run the camera load callback after Construct returns.

    // PSX: AllocBlockPool__12BlockManager(0) - allocate block node pool
    // PC: blocks already allocated by LoadBlocksFunc

    // PSX: LoadBlocks__12BlockManagerUl(0, startBlockNum)
    // On PC we deferred Parse() in Load() and do it here to match PSX timing.
    LoadBlocksForPetalFromStream(blockMgr, streamData, currentPetalIndex, startBlockNum, "World");

    // PopulateBlock is called by LoadBlocks on PSX. On PC we call it explicitly.
    if (g_ai) {
        g_ai->PopulateBlock();
    }

    WEffect_PopulateWEffects();

    // PSX: if (IsValidBlockNumber(playerBlockNum) == 4096) -> reposition player
    // PSX returns 0x1000 (4096) when block is NOT valid.
    if (Player::s_player && g_blockManager) {
        if (!g_blockManager->IsValidBlockNumber(playerBlockNum)) {
            // PSX: get first loaded block position, add 2048 to Y
            Block* firstBlock = g_blockManager->GetBlock(0);
            if (firstBlock) {
                Player::s_player->pos.x = firstBlock->posX;
                Player::s_player->pos.y = firstBlock->posY + 2048;
                Player::s_player->pos.z = firstBlock->posZ;
                Player::s_player->homePos = Player::s_player->pos;
                LOG("[World] Player blockNum %u invalid, repositioned to block 0 (%d,%d,%d)",
                    playerBlockNum, firstBlock->posX, firstBlock->posY + 2048, firstBlock->posZ);
            }
        }
    }

    // PSX hub return flow: when current level ID == 7, apply saved return position
    if (levelList && currentLevelIndex < (u32)levelCount) {
        if (levelList[currentLevelIndex * 2] == 7 && Player::s_player) {
            Player* player = Player::s_player;

            bool mappedPrevLevel = false;
            if (previousLevelIndex < (u32)levelCount) {
                const s32 prevLevelID = levelList[previousLevelIndex * 2];
                switch (prevLevelID) {
                case 6:
                    mappedPrevLevel = true;
                    break;
                case 8:
                    previousLevelIndex = (u32)LevelIDToIndex(5);
                    mappedPrevLevel = true;
                    break;
                case 11:
                    previousLevelIndex = (u32)LevelIDToIndex(1);
                    mappedPrevLevel = true;
                    break;
                case 12:
                    previousLevelIndex = (u32)LevelIDToIndex(2);
                    mappedPrevLevel = true;
                    break;
                case 13:
                    previousLevelIndex = (u32)LevelIDToIndex(3);
                    mappedPrevLevel = true;
                    break;
                case 14:
                    previousLevelIndex = (u32)LevelIDToIndex(4);
                    mappedPrevLevel = true;
                    break;
                default:
                    break;
                }
            }

            if (g_hud) {
                g_hud->ShowDestLevel();
            }

            // PSX: if previousLevelIndex >= levelCount, save player pos as original return pos
            static LVector sOrigDestSelectReturnPos = {};
            if (previousLevelIndex >= (u32)levelCount) {
                sOrigDestSelectReturnPos = player->homePos;
            }

            // PSX: determine if we should show level selection
            bool doShowLevel = false;
            if (previousLevelIndex < (u32)levelCount
                && previousLevelIndex != currentLevelIndex
                && (!mappedPrevLevel || !g_game || g_game->GetState() != GameState::EndLevelExit)) {
                doShowLevel = true;
            }

            LVector returnPos;
            if (doShowLevel) {
                returnPos = g_destSelectReturnPos;
                if (g_hud) {
                    g_hud->destSelect.ShowLevel(0);
                    if (previousLevelIndex < (u32)levelCount) {
                        s32 prevLevelID = levelList[previousLevelIndex * 2];
                        g_hud->destSelect.ShowLevel(prevLevelID);
                    }
                }
                g_arrowInside = 1;
            } else {
                returnPos = sOrigDestSelectReturnPos;
                if (g_hud) {
                    g_hud->DisplayTake(player->livesLeft, 1);
                }
                g_arrowInside = 0;
            }

            player->homePos = returnPos;
            player->pos = returnPos;

            if (g_display) {
                Camera* cam = g_display->GetCamera();
                if (cam) {
                    // PSX: SetLookAtTarget(theCamera, thePlayer, 1)
                    cam->SetLookAtTarget(player, 1);
                }
            }
        }
    }

    // PSX: rsEvent(5, 0, 0, 0) - start music for current location
    rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);

    // PSX: StopLogo after load completes
    StopLogo();
    return true;
}

bool World::Load(const std::string& lcfPath) {
    Unload();

    g_wEffectChunkCount = 0;
    g_particleSystemChunkCount = 0;

    // Read LCF file from disc (PC equivalent of Stream::Open + disc read)
    std::ifstream file(lcfPath, std::ios::binary | std::ios::ate);
    if (!file) {
        LOG("[World] Failed to open: %s", lcfPath.c_str());
        return false;
    }
    auto fileSize = file.tellg();
    file.seekg(0);
    streamData.resize(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(streamData.data()), fileSize);
    file.close();

    u32 dataSize = static_cast<u32>(streamData.size());
    const u8* data = streamData.data();

    // Parse stream header (PSX Stream::Open reads this from disc)
    auto entries = ParseStreamHeader(data, dataSize);
    if (entries.empty()) {
        LOG("[World] No stream entries in: %s", lcfPath.c_str());
        streamData.clear();
        return false;
    }

    // Load TPG textures into VRAM (PSX HandleTPGChunk)
    LoadTPGTextures(data, dataSize);

    // RCI/RCP resources are level-wide and survive petal reloads until PurgeLevel.
    LoadGeoPairsInRange(this, entries, data, dataSize, 0, (u32)entries.size(), ".RCI", ".RCP", 1);

    // PSX petal-based loading: the LCF contains multiple WDB+BLK groups,
    // one per petal. Each petal starts with a .WDB entry followed by .BLK entries.
    // PSX LoadPetal__6Stream finds the N-th WDB and reads only that petal's data.
    // We replicate that: find petal boundaries, load only the target petal.

    // Find WDB entry indices to identify petal boundaries
    std::vector<u32> wdbIndices;
    for (u32 i = 0; i < (u32)entries.size(); i++) {
        if (strncmp(entries[i].magic, ".WDB", 4) == 0) {
            wdbIndices.push_back(i);
        }
    }

    if (wdbIndices.empty()) {
        LOG("[World] No WDB entries in %s", lcfPath.c_str());
        streamData.clear();
        return false;
    }

    // Clamp target petal to valid range
    u32 petalIdx = targetPetalIndex;
    if (petalIdx >= (u32)wdbIndices.size()) {
        petalIdx = 0;
    }
    targetPetalIndex = petalIdx;

    // Determine entry range for this petal: [wdbIndex, nextWdbIndex)
    u32 petalStart = wdbIndices[petalIdx];
    u32 petalEnd = (petalIdx + 1 < (u32)wdbIndices.size())
                       ? wdbIndices[petalIdx + 1]
                       : (u32)entries.size();

    LOG("[World] Loading petal %u/%u (entries %u-%u) from %s",
        petalIdx, (u32)wdbIndices.size(), petalStart, petalEnd - 1, lcfPath.c_str());

    // Count BLK entries for this petal
    u32 blkCount = 0;
    for (u32 i = petalStart; i < petalEnd; i++) {
        if (strncmp(entries[i].magic, ".BLK", 4) == 0) blkCount++;
    }

    // Scan only this petal's WDB into the database
    g_database->Close();
    for (u32 i = petalStart; i < petalEnd; i++) {
        if (strncmp(entries[i].magic, ".WDB", 4) != 0) continue;
        if (entries[i].offset + entries[i].size > dataSize) continue;
        g_database->Scan(data + entries[i].offset, entries[i].size);
    }

    LoadGeoPairsInRange(this, entries, data, dataSize, petalStart, petalEnd, ".PCI", ".PCP", 2);

    RefreshVRAMTexture();

    // Build block volume list from this petal's WDB
    std::vector<DBVolume*> blockVolumes;
    for (DBRoot* v = g_database->GetFirstBlock(); v; v = static_cast<DBRoot*>(v->next)) {
        blockVolumes.push_back(static_cast<DBVolume*>(v));
    }
    LOG("[World] Parsed %u block volumes from petal %u WDB", (u32)blockVolumes.size(), petalIdx);

    // Initialize blocks from volumes (PSX _LoadBlocksFunc - Block::Init)
    blockMgr.LoadBlocksFunc(blockVolumes);

    // PSX timing: BLK parse/load is performed later during Construct, after AI::Populate.
    // Keep only block metadata (LoadBlocksFunc) here and defer Parse() to preserve
    // spawn-time activation semantics in AddThingNoTagList.
    LOG("[World] Deferring parse of %u BLK entries until post-populate construct step", blkCount);

    // Debug: log ALL block positions and compute level AABB
    s32 minX = 0x7FFFFFFF, minY = 0x7FFFFFFF, minZ = 0x7FFFFFFF;
    s32 maxX = -0x7FFFFFFF, maxY = -0x7FFFFFFF, maxZ = -0x7FFFFFFF;
    for (u32 i = 0; i < blockMgr.GetNumBlocks(); i++) {
        Block* b = blockMgr.GetBlock(i);
        if (!b) continue;
        LOG("[World] Block %u: blockNum=%u pos=(%d,%d,%d) dim=(%d,%d,%d) parsed=%d",
            i, b->blockNum, b->posX, b->posY, b->posZ, b->dimX, b->dimY, b->dimZ, b->parsed);
        s32 bMinX = b->posX + b->halfExtNegX, bMaxX = b->posX + b->halfExtPosX;
        s32 bMinY = b->posY + b->halfExtNegY, bMaxY = b->posY + b->halfExtPosY;
        s32 bMinZ = b->posZ + b->halfExtNegZ, bMaxZ = b->posZ + b->halfExtPosZ;
        if (bMinX < minX) minX = bMinX; if (bMaxX > maxX) maxX = bMaxX;
        if (bMinY < minY) minY = bMinY; if (bMaxY > maxY) maxY = bMaxY;
        if (bMinZ < minZ) minZ = bMinZ; if (bMaxZ > maxZ) maxZ = bMaxZ;
    }
    levelMin = { minX, minY, minZ };
    levelMax = { maxX, maxY, maxZ };
    LOG("[World] Level AABB: min=(%d,%d,%d) max=(%d,%d,%d)",
        minX, minY, minZ, maxX, maxY, maxZ);
    LOG("[World] Level size: (%d, %d, %d)",
        maxX - minX, maxY - minY, maxZ - minZ);

    return blockMgr.GetNumBlocks() > 0;
}

void World::UploadToVRAM(s16 x, s16 y, s16 w, s16 h, const u8* raw) {
    LOG("[VRAM] ext upload: x=%d y=%d w=%d h=%d", x, y, w, h);
    vram.Upload(x, y, w, h, raw);
}

void World::RefreshVRAMTexture() {
    if (vramHandle) {
        p3d::context->DestroyVRAMTexture(vramHandle);
        vramHandle = 0;
    }
    vramHandle = p3d::context->CreateVRAMTexture(1024, 512, vram.data);
}

void World::Render(const LVector* playerPos) {
    p3d::context->SetVRAMHandle(vramHandle);
    DrawEverythingHandler(playerPos);
    p3d::context->SetVRAMHandle(0);
}

// TransformVector - PC equivalent of PSX tPort::TransformVector
// Multiplies world-space point by the current view matrix, then converts the
// result back into tPort space (+Y down, +Z forward).
static void TransformVector(const Mat4& vm, s32 inX, s32 inY, s32 inZ,
                            s32* outX, s32* outY, s32* outZ) {
    f32 ox, oy, oz;
    Mat4TransformPoint(vm, (f32)inX, (f32)inY, (f32)inZ, ox, oy, oz);
    *outX = static_cast<s32>(ox);
    *outY = static_cast<s32>(-oy);
    *outZ = static_cast<s32>(-oz);
}

// chanp3dClipCode - PC equivalent of PSX chanp3dClipCode
// Projects a tPort-space point into screen space and computes PSX-style clip bits.
// bit 0: left, bit 1: right, bit 2: top, bit 3: bottom, bit 4: behind camera
static u32 chanp3dClipCode(const ChanProjectionState& portState, s32 vx, s32 vy, s32 vz) {
    u32 code = 0;

    // PSX chanp3dClipCode only checks z-sign (behind camera), not near/far clip planes.
    if (vz < 0) code |= 0x10;

    const s32 denom = (vz == 0) ? 1 : vz;

    s32 sx = portState.centerX + static_cast<s32>((static_cast<f32>(vx) * portState.projectionDistanceX) / static_cast<f32>(denom));
    s32 sy = portState.centerY + static_cast<s32>((static_cast<f32>(vy) * portState.projectionDistanceY) / static_cast<f32>(denom));

    if (sx < 0) code |= 0x01;
    else if (sx >= portState.width) code |= 0x02;

    if (sy < 0) code |= 0x04;
    else if (sy >= portState.height) code |= 0x08;

    return code;
}

// vecLengthSquared - PC equivalent of PSX vecLengthSquared
// PSX formula: ((x*x)>>2) + ((y*y)>>2) + ((z*z)>>2)
static s32 vecLengthSquared(s32 x, s32 y, s32 z) {
    const s64 xx = static_cast<s64>(x) * static_cast<s64>(x);
    const s64 yy = static_cast<s64>(y) * static_cast<s64>(y);
    const s64 zz = static_cast<s64>(z) * static_cast<s64>(z);
    return static_cast<s32>((xx >> 2) + (yy >> 2) + (zz >> 2));
}

// PSX: DrawLoop__FP6ccListUl (GAME.CPP:2543, 0x8002B224)
// Iterates a ccList and calls Draw() on entities in the given block.
static void DrawEntityList(ccList& list, u16 blockNum) {
    MARKFUNCTION(0x8002B224);
    for (ccMinNode* n = list.head; n; n = n->next) {
        Thing* thing = static_cast<Thing*>(n);
        if (thing->blockNum == blockNum) {
            thing->Draw();
        }
    }
}

// DrawEverythingHandler__FP7Handler (GAME.CPP:2211, 0x8002A98C)
// Reversed from PSX: builds draw list from loaded blocks, selection-sorts by distSq
// DESCENDING (farthest first for back-to-front rendering), applies OffsetToPreventSeams,
// checks InDrawList, renders entities + block geometry.
void World::DrawEverythingHandler(const LVector* playerPos) {
    MARKFUNCTION(0x8002A98C);

    u32 numBlocks = blockMgr.GetNumBlocks();
    if (numBlocks == 0) return;

    // PSX: tick UV accumulators each frame
    TickAllUVPrimData();

    // PSX: DemandLoading when game state == 8
    if (g_game && g_game->GetState() == GameState::Play) {
        blockMgr.DemandLoading();
    }

    // Build draw entry array: {Block*, distSq, zDepth}
    // PSX: iterates loaded block linked list (offset +144)
    // PC: iterates all blocks (all are loaded)
    struct DrawEntry {
        Block* block;
        s32 distSq;
        s32 zDepth;
    };
    DrawEntry drawArray[128];
    u32 count = 0;

    for (u32 i = 0; i < numBlocks && count < 128; i++) {
        Block* block = blockMgr.GetBlock(i);
        if (!block || !block->primGeom) continue;

        s32 distSq, zDepth;
        computeBlockToPointDistances(block, playerPos, &distSq, &zDepth);
        drawArray[count].block = block;
        drawArray[count].distSq = distSq;
        drawArray[count].zDepth = zDepth;
        count++;
    }

    if (count == 0) return;

    // PSX selection sort: DESCENDING by distSq (farthest first = back-to-front)
    // PSX inner loop finds the entry with the SMALLEST distSq, swaps to front.
    // After sorting: index 0 = farthest, last = nearest.
    for (u32 i = 0; i < count - 1; i++) {
        u32 minIdx = i;
        for (u32 j = i + 1; j < count; j++) {
            if (drawArray[minIdx].distSq < drawArray[j].distSq) {
                minIdx = j;
            }
        }
        if (minIdx != i) {
            DrawEntry tmp = drawArray[i];
            drawArray[i] = drawArray[minIdx];
            drawArray[minIdx] = tmp;
        }
    }

    // PSX: count visible blocks (positive distSq) = v27
    u32 visibleCount = 0;
    for (u32 i = 0; i < count; i++) {
        if (drawArray[i].distSq > 0) {
            visibleCount = i + 1;
        }
    }

    // PSX: find maxZDepth among far blocks (index >= 5), add 64, clamp to 0xFFFF
    // Used for OT layer setup on PSX - not functionally needed with z-buffer on PC.

    // Render visible blocks
    for (u32 i = 0; i < visibleCount; i++) {
        DrawEntry& entry = drawArray[i];

        // Copy block->pos to local and apply OffsetToPreventSeams
        LVector localPos;
        localPos.x = entry.block->posX;
        localPos.y = entry.block->posY;
        localPos.z = entry.block->posZ;
        OffsetToPreventSeams(localPos, *playerPos);

        u16 bn = entry.block->blockNum;

        // PSX: only draw entities + geometry if block is in draw list
        if (blockMgr.InDrawList(bn)) {
            // PSX: DrawLoop for each entity list
            if (g_ai) {
                DrawEntityList(g_ai->humanoidList, bn);
                DrawEntityList(g_ai->inactivePickupList, bn);
                DrawEntityList(g_ai->pickupList, bn);
                DrawEntityList(g_ai->moveList, bn);
            }

            // Host immediate path: draw opaque block first, then effects.
            // PSX submits both into OT; final composition is depth-sorted at flush.
            entry.block->Draw(&localPos);

            Effects_DrawEffects(static_cast<s32>(bn));
        }
    }

    // PSX: DebugDrawSector, ExitLayer(2), profile end(7)
}

// computeBlockToPointDistances (GAME.CPP:1976)
// Reversed from PSX: builds 8 bounding box corners + center (9 points),
// transforms each through view matrix, computes clip codes + view-space distance,
// tests 13 clip code pairs for frustum culling.
// a0=block, a1=playerPos (unused - view matrix already set), a2=outDistSq, a3=outZDepth
void World::computeBlockToPointDistances(const Block* block, const LVector* playerPos,
                                         s32* outDistSq, s32* outZDepth) {
    MARKFUNCTION(0x8002A238);

    // PSX: reads bounding box from tPrimGeom virtual call: *(*(block->primGeom+8)+20)()
    // PC: uses block half-extent fields (same bounding box data, different access path)
    // s3 equivalent: bbox[0]=negX, [1]=negY, [2]=negZ, [3]=posX, [4]=posY, [5]=posZ
    s32 bbox[6] = {
        block->halfExtNegX, block->halfExtNegY, block->halfExtNegZ,
        block->halfExtPosX, block->halfExtPosY, block->halfExtPosZ
    };

    // s5 = &block->posX (block position at offset +4)
    const s32* pos = &block->posX; // pos[0]=X, pos[1]=Y, pos[2]=Z

    // Get view matrix and current tPort projection state.
    const Mat4& vm = p3d::context->GetViewMatrix();
    ChanProjectionState portState;
    if (g_display) {
        portState = g_display->GetChanProjectionState();
    }

    s32 minDistSq = 0; // s7
    s32 maxZDepth = 0; // s6
    u32 clipCodes[8];
    s32 tvx, tvy, tvz; // transformed view-space coords

    // Build and process 8 bounding box corners
    // PSX corner pattern: (bbox[negX/posX], bbox[negY/posY], bbox[negZ/posZ]) + blockPos
    // Corner 0: pos + (negX, negY, negZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[1], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz; // save pre-project coords
        clipCodes[0] = chanp3dClipCode(portState, tvx, tvy, tvz);
        minDistSq = vecLengthSquared(svx, svy, svz);
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 1: pos + (posX, negY, negZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[1], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[1] = chanp3dClipCode(portState, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 2: pos + (negX, posY, negZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[4], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[2] = chanp3dClipCode(portState, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 3: pos + (posX, posY, negZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[4], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[3] = chanp3dClipCode(portState, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 4: pos + (negX, negY, posZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[1], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[4] = chanp3dClipCode(portState, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 5: pos + (posX, negY, posZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[1], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[5] = chanp3dClipCode(portState, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 6: pos + (negX, posY, posZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[4], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[6] = chanp3dClipCode(portState, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 7: pos + (posX, posY, posZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[4], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[7] = chanp3dClipCode(portState, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Center (9th point): just blockPos, no bbox offset
    // PSX: TransformVector + vecLengthSquared only (no ProjectVector/chanp3dClipCode)
    {
        TransformVector(vm, pos[0], pos[1], pos[2], &tvx, &tvy, &tvz);
        s32 d = vecLengthSquared(tvx, tvy, tvz);
        if (d < minDistSq) minDistSq = d;
        s32 z = tvz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Frustum cull test: 13 specific clip code pairs ANDed
    // If ANY pair ANDs to 0 - visible (at least one edge straddles a frustum plane)
    // If ALL pairs are non-zero - fully culled
    // PSX pairs: (0,1)(0,2)(0,4)(1,3)(1,5)(2,3)(2,6)(3,7)(4,5)(4,6)(5,7)(6,7)(0,7)
    if ((clipCodes[0] & clipCodes[1]) != 0 &&
        (clipCodes[0] & clipCodes[2]) != 0 &&
        (clipCodes[0] & clipCodes[4]) != 0 &&
        (clipCodes[1] & clipCodes[3]) != 0 &&
        (clipCodes[1] & clipCodes[5]) != 0 &&
        (clipCodes[2] & clipCodes[3]) != 0 &&
        (clipCodes[2] & clipCodes[6]) != 0 &&
        (clipCodes[3] & clipCodes[7]) != 0 &&
        (clipCodes[4] & clipCodes[5]) != 0 &&
        (clipCodes[4] & clipCodes[6]) != 0 &&
        (clipCodes[5] & clipCodes[7]) != 0 &&
        (clipCodes[6] & clipCodes[7]) != 0 &&
        (clipCodes[0] & clipCodes[7]) != 0) {
        // All 13 pairs non-zero - block is fully outside the frustum
        *outDistSq = -1;
        return;
    }

    // Visible - output minimum distance and maximum z-depth
    *outDistSq = minDistSq;
    *outZDepth = maxZDepth;
}

// OffsetToPreventSeams__FR10tagLVectorRC10tagLVector (GAME.CPP:2482)
// PSX: computes per-axis sign of (pos - playerPos),
// then offset = -sign * (sign * delta / divisor + 1), clamped to +/-limit.
void World::OffsetToPreventSeams(LVector& pos, const LVector& playerPos) {
    MARKFUNCTION(0x8002AF88);

    s32 dx = pos.x - playerPos.x;
    s32 dy = pos.y - playerPos.y;
    s32 dz = pos.z - playerPos.z;

    // Compute sign per axis: -1, 0, or +1 - sp[16], sp[20], sp[24]
    s32 signX = (dx < 0) ? -1 : (dx > 0) ? 1 : 0;
    s32 signY = (dy < 0) ? -1 : (dy > 0) ? 1 : 0;
    s32 signZ = (dz < 0) ? -1 : (dz > 0) ? 1 : 0;

    // PSX small-data values loaded from gp+0x60 / gp+0x64.
    // These correspond to BLOCK_DRAW_SEAM_OFFSET_CODE[1] and [2].
    s32 seamDivisor = BLOCK_DRAW_SEAM_OFFSET_CODE[1];
    s32 seamLimit = BLOCK_DRAW_SEAM_OFFSET_CODE[2];

    // PSX: a3 = (signX * dx) / seamDivisor
    s32 rawX = (signX * dx) / seamDivisor;
    s32 rawY = (signY * dy) / seamDivisor;
    s32 rawZ = (signZ * dz) / seamDivisor;

    // PSX: offset = (-sign) * (raw + 1) - pushes block position toward camera
    s32 offX = (-signX) * (rawX + 1);
    s32 offY = (-signY) * (rawY + 1);
    s32 offZ = (-signZ) * (rawZ + 1);

    // PSX: clamp each to +/-seamLimit (gp[100])
    if (offX < -seamLimit) offX = -seamLimit;
    else if (offX > seamLimit) offX = seamLimit;
    if (offY < -seamLimit) offY = -seamLimit;
    else if (offY > seamLimit) offY = seamLimit;
    if (offZ < -seamLimit) offZ = -seamLimit;
    else if (offZ > seamLimit) offZ = seamLimit;

    // PSX: add offsets to position
    pos.x += offX;
    pos.y += offY;
    pos.z += offZ;
}

void World::Unload() {
    if (g_hud) {
        g_hud->OnUnloadLevel();
    }

    const bool hadLoadedLevel = !streamData.empty() || (vramHandle != 0);
    if (hadLoadedLevel) {
        blockMgr.InternalClose();
        if (g_ai) {
            g_ai->UnPopulate(0);
        }

        WEffect_UnPopulateWEffects(-1);

        Obstacle_ClearPetalAnimList();
        Effects_UnloadAll();
        WEffect_Unload();
        PWEffect_Unload();
        GEffect_Unload();
        UnloadScaleData();
        UnloadPaletteData();
        ParticleSystem_Unload();
        UnloadUVPrimData();
        UnloadCBVPrimData();
        PurgeSwitches();

        if (g_director) {
            g_director->PurgeAnims();
        }
        if (g_display && g_display->GetCamera()) {
            g_display->GetCamera()->PurgeAnims();
        }
        if (g_levelManager) {
            g_levelManager->PurgeLevel();
        }
    }
    else {
        WEffect_UnPopulateWEffects(-1);
        Effects_UnloadAll();
        WEffect_Unload();
        PWEffect_Unload();
        GEffect_Unload();
        UnloadScaleData();
        UnloadPaletteData();
        ParticleSystem_Unload();
        UnloadUVPrimData();
        UnloadCBVPrimData();
        PurgeSwitches();
    }

    g_wEffectChunkCount = 0;
    g_particleSystemChunkCount = 0;

    streamData.clear();
    if (vramHandle && p3d::context) {
        p3d::context->DestroyVRAMTexture(vramHandle);
        vramHandle = 0;
    }

    rsEvent(RS_UNLOAD_LEVEL, 0, 0, 0);
}

// PSX: UnloadLevelPart2__5World (WORLD.CPP:1355, 0x80046208)
void World::UnloadLevelPart2() {
    MARKFUNCTION(0x80046208);

    streamData.clear();
    DeletePlayerBlendAndAnimData();
    WorldPoints_Reset();
}

// PSX: UnloadPermanent__5World (WORLD.CPP:1886, 0x80046CB0)
void World::UnloadPermanent() {
    MARKFUNCTION(0x80046CB0);
}

// PSX: UnloadPetal__5World (WORLD.CPP:1176, 0x80045F34)
void World::UnloadPetal() {
    MARKFUNCTION(0x80045F34);
    WEffect_UnPopulateWEffects(-1);
    WEffect_Unload();
    PWEffect_Unload();
    ParticleSystem_UnloadLevel();
    UnloadLevelScaleData();
    UnloadPaletteData();
    UnloadUVPrimData();
    UnloadCBVPrimData();
    PurgeSwitches();

    // Unload current blocks (collision sectors, geometry)
    blockMgr.InternalClose();

    // Clear all AI entities from previous petal
    if (g_ai) {
        g_ai->UnPopulate(0);
    }

    if (g_director) {
        g_director->PurgeAnims();
    }

    if (g_levelManager) {
        g_levelManager->PurgePetal();
    }

    rsEvent(RS_UNLOAD_LEVEL, 0, 0, 0);
}

// PSX: LoadPetal__5WorldUl (WORLD.CPP:1222, 0x8004604C)
void World::LoadPetal(u32 petalIndex) {
    MARKFUNCTION(0x8004604C);

    // PSX: if current level is DestSelect (lev07), save as previous
    if (levelList && currentLevelIndex < (u32)levelCount) {
        if (levelList[currentLevelIndex * 2] == 7)
            previousLevelIndex = currentLevelIndex;
    }

    // PSX: EstimateLoadTime, StartLogo, FillMeter(100)
    StartLogo("RUNFIRST.TIM");
    FillMeter(100);

    // Keep petal selection valid for this level before indexing sound tables.
    s32 petalCount = GetCurLevelPetals();
    if (petalCount <= 0 || petalIndex >= (u32)petalCount) {
        petalIndex = 0;
    }

    // PSX: rsEvent(4, petalSoundIDs[currentLevelIndex][petalIndex] - 1, 0, 0)
    if (petalSoundIDs && currentLevelIndex < (u32)levelCount) {
        s32 soundLocation = (s32)petalSoundIDs[currentLevelIndex][petalIndex] - 1;
        rsEvent(RS_SET_LOCATION, soundLocation, 0, 0);
    }

    targetPetalIndex = petalIndex;
    currentPetalIndex = petalIndex;

    blockMgr.SetDeathVolumeFlag(1);
    g_wEffectChunkCount = 0;
    g_particleSystemChunkCount = 0;

    // Petal load is a new level segment; clear per-level score/collect state first.
    if (g_scoreManager) {
        g_scoreManager->HandleLevelBegin();
    }

    if (g_hud) {
        g_hud->OnLoadLevel();
    }

    // PSX: LevelManager::LoadPetal re-reads from Stream at petal position.
    // PC: re-parse the already-loaded LCF data for the new petal.
    if (!streamData.empty()) {
        u32 dataSize = static_cast<u32>(streamData.size());
        const u8* data = streamData.data();

        auto entries = ParseStreamHeader(data, dataSize);

        // Find WDB entry indices (petal boundaries)
        std::vector<u32> wdbIndices;
        for (u32 i = 0; i < (u32)entries.size(); i++) {
            if (strncmp(entries[i].magic, ".WDB", 4) == 0) {
                wdbIndices.push_back(i);
            }
        }

        u32 pi = petalIndex;
        if (pi >= (u32)wdbIndices.size()) pi = 0;
        if (pi != petalIndex) {
            petalIndex = pi;
            targetPetalIndex = pi;
            currentPetalIndex = pi;
        }

        u32 petalStart = wdbIndices[pi];
        u32 petalEnd = (pi + 1 < (u32)wdbIndices.size())
                           ? wdbIndices[pi + 1]
                           : (u32)entries.size();

        LOG("[World] LoadPetal %u: entries %u-%u", pi, petalStart, petalEnd - 1);

        // Scan this petal's WDB
        PurgeSwitches();
        g_database->Close();
        for (u32 i = petalStart; i < petalEnd; i++) {
            if (strncmp(entries[i].magic, ".WDB", 4) != 0) continue;
            if (entries[i].offset + entries[i].size > dataSize) continue;
            g_database->Scan(data + entries[i].offset, entries[i].size);
        }

        LoadGeoPairsInRange(this, entries, data, dataSize, petalStart, petalEnd, ".PCI", ".PCP", 2);

        WEffect_InitWorldEffects(g_database->GetFirstPoint());
        PWEffect_InitWorldEffects(g_database->GetFirstPoint());
        ParticleSystem_InitParticleInfoMemory();

        // Refresh VRAM GL texture after Geo texture uploads
        RefreshVRAMTexture();

        // Build block volumes
        std::vector<DBVolume*> blockVolumes;
        for (DBRoot* v = g_database->GetFirstBlock(); v; v = static_cast<DBRoot*>(v->next)) {
            blockVolumes.push_back(static_cast<DBVolume*>(v));
        }
        blockMgr.LoadBlocksFunc(blockVolumes);

        // PSX timing: BLK parse/load happens after AI::Populate.
        LOG("[World] LoadPetal: deferring BLK parse until post-populate");
    }

    // PSX: AI::Populate for new petal entities
    if (g_ai) {
        g_ai->Populate();
    }

    u32 startBlockNum = 0;
    if (Player::s_player) {
        startBlockNum = Player::s_player->blockNum;
    }

    LoadBlocksForPetalFromStream(blockMgr, streamData, currentPetalIndex, startBlockNum, "World::LoadPetal");

    if (g_ai) {
        g_ai->PopulateBlock();
    }

    WEffect_PopulateWEffects();

    if (g_director) {
        g_director->Reset();
        g_director->SetScript();
    }

    rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);

    // PSX: StopLogo after load completes
    StopLogo();
}

// PSX: DeletePlayerBlendAndAnimData__Fv (WORLD.CPP:2059, 0x80047014)
s32 DeletePlayerBlendAndAnimData() {
    MARKFUNCTION(0x80047014);
    return 0;
}

// PSX: ResetLevel__5World (WORLD.CPP:1918, 0x80046DE0)
void World::ResetLevel() {
    MARKFUNCTION(0x80046DE0);

    pendingPlayerReset = true;

    if (Player::s_player) {
        Player::s_player->checkpoint.SetValidState(0);
    }

    if (g_director) {
        g_director->LevelReset();
    }

    levelDeadPool.InternalReset();
}

// PSX: LevelMenuExecute__5WorldP10hdMenuItem (WORLD.CPP:868, 0x80045634)
// Callback invoked when a level is selected in the level menu.
s32 World::LevelMenuExecute(hdMenuItem* item) {
    MARKFUNCTION(0x80045634);

    u32 levelIndex = 0;
    u32 petalIndex = 0;

    // PSX: item->data[5] holds the packed level name (set by InitLevelMenu)
    // hdMenuItem: +20 = itemFlags, +24 = itemID. But PSX uses offset +20 as value.
    // Actually PSX reads item[5] = *(item + 20) = itemFlags field repurposed as value.
    UnpackLevelName(item->itemFlags, levelIndex, petalIndex);

    World* world = g_game ? g_game->GetWorld() : nullptr;
    if (!world) {
        return 4;
    }

    u32 curLevel = world->currentLevelIndex;
    u32 curPetal = world->currentPetalIndex;

    world->targetLevelIndex = levelIndex;
    world->targetPetalIndex = petalIndex;

    // PSX: if same level + same petal -> QueuePetalLoad (21)
    // PSX: else -> stop music, QueueLevelLoad (20)
    bool sameLevel = (curLevel == levelIndex) && (curPetal == petalIndex);
    GameState nextState = GameState::QueuePetalLoad;

    if (!sameLevel) {
        rsEvent(RS_STOP_MUSIC, 0, 0, 0);
        nextState = GameState::QueueLevelLoad;
    }

    g_game->SetState(nextState);
    world->ResetLevel();

    return 4;
}


