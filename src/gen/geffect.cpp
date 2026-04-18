#include "gen/geffect.h"
#include "gen/animmgr.h"
#include "pc/log.h"

struct LoadedComEffectAnim {
    u32 effectHash;
    u32 animHash;
    MiscAnimNode* animNode;
};

static LoadedComEffectAnim* g_loadedComEffects = nullptr;
static u32 g_loadedComEffectCount = 0;

static u32 ReadU32LE(const u8* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void GEffect_Unload() {
    MARKFUNCTION(0x8008DCE8);
    delete[] g_loadedComEffects;
    g_loadedComEffects = nullptr;
    g_loadedComEffectCount = 0;
}

void GEffect_LoadChunk(const u8* body, u32 bodySize) {
    MARKFUNCTION(0x8008DB2C);

    GEffect_Unload();
    if (!body || bodySize < 4) {
        return;
    }

    u32 count = ReadU32LE(body + 0);
    u32 remaining = bodySize - 4;
    u32 maxCountFromBody = remaining / 8;
    if (count > maxCountFromBody) {
        LOG("[GEffect] Truncated 0x8A10 chunk: count=%u max=%u", count, maxCountFromBody);
        count = maxCountFromBody;
    }

    if (count == 0) {
        return;
    }

    g_loadedComEffects = new LoadedComEffectAnim[count];
    g_loadedComEffectCount = count;

    const u8* p = body + 4;
    for (u32 i = 0; i < count; i++) {
        u32 effectHash = ReadU32LE(p + 0);
        u32 animHash = ReadU32LE(p + 4);
        p += 8;

        MiscAnimNode* misc = nullptr;
        if (g_animMgr) {
            misc = g_animMgr->GetMiscAnim(animHash);
        }

        g_loadedComEffects[i].effectHash = effectHash;
        g_loadedComEffects[i].animHash = animHash;
        g_loadedComEffects[i].animNode = misc;
    }

    LOG("[GEffect] Loaded %u ComEffect entries from chunk 0x8A10", g_loadedComEffectCount);
}

bool GEffect_FindEffectAnim(u32 effectHash, MiscAnimNode** outAnim) {
    MARKFUNCTION(0x8008DDD0);

    for (u32 i = 0; i < g_loadedComEffectCount; i++) {
        if (g_loadedComEffects[i].effectHash == effectHash) {
            // PSX stores both effect hash (+52) and anim hash (+56) on ComEffect.
            // If anim wasn't resolved when 0x8A10 was read, retry here.
            if (!g_loadedComEffects[i].animNode && g_animMgr) {
                g_loadedComEffects[i].animNode = g_animMgr->GetMiscAnim(g_loadedComEffects[i].animHash);
            }
            if (outAnim) {
                *outAnim = g_loadedComEffects[i].animNode;
            }
            return true;
        }
    }

    if (outAnim) {
        *outAnim = nullptr;
    }
    return false;
}
