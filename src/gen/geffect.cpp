#include "gen/geffect.h"
#include "gen/blockmgr.h"
#include "gen/effects.h"
#include "gen/weffect.h"
#include "p3d/byteread.h"
#include "pc/log.h"
#include "snd/basesnd.h"
#include "snd/esound.h"
#include "snd/sndfact.h"

class GEffect : public Effects {
public:
    GEffect();
    ~GEffect() override;

    s32 PutBackEffect() override;
    s32 Create() override;
    s32 Update() override;
    void Display(s32 inBlockNum) override;

    s32 CreateSound();
    s32 UpdateSound();
    s32 ReleaseSound();

    const LVector* posRef = nullptr;
    const LVector* rotationRef = nullptr;
    const LVector* scaleRef = nullptr;

    LVector pos = {};
    LVector rotation = {};
    LVector scale = { 0x10000, 0x10000, 0x10000 };

    u32 renderFlags = 0;
    u32 createFlags = 0;

    s32 frame = 0;
    s32 frameDelay = 0;
    s32 frameCounter = 0;
    s32 lifeFrames = 0;
    s32 holdAlive = 0;

    ComEffect* comEffect = nullptr;
    void* particleMgr = nullptr;

    CWorldEffectSound* worldSound = nullptr;
    CSound* particleSound = nullptr;
};

static ComEffect** g_loadedComEffects = nullptr;
static u32 g_loadedComEffectCount = 0;

static ccList g_genericEffectPool;
static GEffect** g_genericEffects = nullptr;
static u32 g_genericEffectCount = 0;

GEffect::GEffect() {
    MARKFUNCTION(0x8008E184);
}

GEffect::~GEffect() {
    MARKFUNCTION(0x8008E1CC);
    ReleaseSound();
}

s32 GEffect::PutBackEffect() {
    MARKFUNCTION(0x8008E228);

    g_genericEffectPool.AddNode(g_genericEffectPool.tail, this);
    return ReleaseSound();
}

s32 GEffect::Create() {
    MARKFUNCTION(0x8008E264);
    return 1;
}

s32 GEffect::Update() {
    MARKFUNCTION(0x8008E26C);

    const LVector* blockPos = &pos;
    if ((createFlags & 1u) != 0u && posRef) {
        blockPos = posRef;
    }

    if (g_blockManager) {
        blockNum = static_cast<s32>(g_blockManager->GetBlockNumber(*blockPos));
    }

    const s32 previousCounter = frameCounter;
    frameCounter = previousCounter + 1;
    if (previousCounter != frameDelay) {
        return frameCounter;
    }

    frameCounter = 0;

    if (comEffect) {
        if (frame < lifeFrames || holdAlive) {
            UpdateSound();
            frame += 1;
            return frame;
        }

        Effects_RemoveEffect(this);
        g_genericEffectPool.AddNode(g_genericEffectPool.tail, this);
        ReleaseSound();

        frame += 1;
        return frame;
    }

    if ((lifeFrames - frame) > 0 && !holdAlive) {
        frame += 1;
        return frame;
    }

    Effects_RemoveEffect(this);
    g_genericEffectPool.AddNode(g_genericEffectPool.tail, this);
    ReleaseSound();

    frame += 1;
    return frame;
}

void GEffect::Display(s32 inBlockNum) {
    MARKFUNCTION(0x8008E3DC);

    if (blockNum != inBlockNum) {
        return;
    }

    UpdateSound();

    LVector renderPos = pos;
    if ((createFlags & 1u) != 0u && posRef) {
        renderPos = *posRef;
    }

    LVector renderScale = scale;
    if ((createFlags & 0x10u) != 0u) {
        renderScale = scale;
    }
    else if ((createFlags & 2u) != 0u && scaleRef) {
        renderScale = *scaleRef;
    }

    LVector renderRotation = rotation;
    if ((createFlags & 0x20u) != 0u) {
        renderRotation = rotation;
    }
    else if ((createFlags & 4u) != 0u && rotationRef) {
        renderRotation = *rotationRef;
    }

    if (!comEffect) {
        return;
    }

    comEffect->SetFrame(frame);

    const u16 rotationWords[3] = {
        static_cast<u16>(renderRotation.x),
        static_cast<u16>(renderRotation.y),
        static_cast<u16>(renderRotation.z),
    };

    comEffect->Render(renderPos, &renderScale, rotationWords, renderFlags);
}

s32 GEffect::CreateSound() {
    MARKFUNCTION(0x8008E608);

    if (particleMgr) {
        if (particleSound) {
            return 0;
        }

        CSound* createdParticleSound = nullptr;
        s32 result = CSoundFactory::CreateObject(10000, &createdParticleSound);
        if (result >= 0 && createdParticleSound) {
            particleSound = createdParticleSound;
            return particleSound->Initialize(const_cast<LVector*>(posRef));
        }

        return result;
    }

    if (!comEffect) {
        return 0;
    }

    if (worldSound) {
        return 0;
    }

    CSound* createdSound = nullptr;
    const s32 result = CSoundFactory::CreateObject(10010, &createdSound, comEffect->resourceHash);
    if (result >= 0 && createdSound) {
        worldSound = static_cast<CWorldEffectSound*>(createdSound);
        return worldSound->Initialize(posRef);
    }

    return result;
}

s32 GEffect::UpdateSound() {
    MARKFUNCTION(0x8008E6A0);

    if (comEffect && worldSound) {
        worldSound->Update(static_cast<u32>(frame));
    }

    if (particleMgr && particleSound) {
        return 0;
    }

    return 0;
}

s32 GEffect::ReleaseSound() {
    MARKFUNCTION(0x8008E714);

    if (worldSound) {
        worldSound->Release();
        worldSound = nullptr;
    }

    if (particleSound) {
        particleSound->Release();
        particleSound = nullptr;
    }

    return 0;
}

void GEffect_Unload() {
    MARKFUNCTION(0x8008DCE8);

    for (u32 i = 0; i < g_loadedComEffectCount; i++) {
        delete g_loadedComEffects[i];
        g_loadedComEffects[i] = nullptr;
    }

    delete[] g_loadedComEffects;
    g_loadedComEffects = nullptr;
    g_loadedComEffectCount = 0;

    for (u32 i = 0; i < g_genericEffectCount; i++) {
        GEffect* effect = g_genericEffects[i];
        if (!effect) {
            continue;
        }

        if (effect->inEffectsList) {
            Effects_RemoveEffect(effect);
        }

        delete effect;
        g_genericEffects[i] = nullptr;
    }

    delete[] g_genericEffects;
    g_genericEffects = nullptr;
    g_genericEffectCount = 0;

    g_genericEffectPool.head = nullptr;
    g_genericEffectPool.tail = nullptr;
}

void GEffect_LoadChunk(const u8* body, u32 bodySize) {
    MARKFUNCTION(0x8008DB2C);

    GEffect_Unload();
    if (!body || bodySize < 4) {
        return;
    }

    const u8* p = body;
    const u8* bodyEnd = body + bodySize;

    u32 count = p3dReadU32LE(p);
    p += 4;

    u32 remaining = static_cast<u32>(bodyEnd - p);
    u32 maxCountFromBody = remaining / 8;
    if (count > maxCountFromBody) {
        LOG("[GEffect] Truncated 0x8A10 chunk: count=%u max=%u", count, maxCountFromBody);
        count = maxCountFromBody;
    }

    g_loadedComEffects = (count > 0) ? new ComEffect*[count]() : nullptr;
    g_loadedComEffectCount = count;

    for (u32 i = 0; i < count; i++) {
        const u32 effectHash = p3dReadU32LE(p + 0);
        const u32 animHash = p3dReadU32LE(p + 4);
        p += 8;

        ComEffect* effect = new ComEffect();
        if (!effect->LoadETree(static_cast<s32>(effectHash), static_cast<s32>(animHash))
            && !effect->LoadSTree(static_cast<s32>(effectHash), static_cast<s32>(animHash))) {
            delete effect;
            continue;
        }

        g_loadedComEffects[i] = effect;
    }

    if (p + 4 <= bodyEnd) {
        const u32 genericCount = p3dReadU32LE(p);
        p += 4;

        if (genericCount > 0) {
            g_genericEffects = new GEffect*[genericCount]();
            g_genericEffectCount = genericCount;

            for (u32 i = 0; i < genericCount; i++) {
                GEffect* effect = new GEffect();
                g_genericEffects[i] = effect;
                g_genericEffectPool.AddNode(g_genericEffectPool.tail, effect);
            }
        }
    }

    if (p + 4 <= bodyEnd) {
        const u32 commonParticleCount = p3dReadU32LE(p);
        (void)commonParticleCount;
    }

    LOG("[GEffect] Loaded %u ComEffects and %u generic slots from chunk 0x8A10", g_loadedComEffectCount,
        g_genericEffectCount);
}

ComEffect* GEffect_FindEffect(u32 effectHash) {
    MARKFUNCTION(0x8008DDD0);

    for (u32 i = 0; i < g_loadedComEffectCount; i++) {
        ComEffect* effect = g_loadedComEffects[i];
        if (effect && effect->resourceHash == effectHash) {
            return effect;
        }
    }

    return nullptr;
}

bool GEffect_FindEffectAnim(u32 effectHash, MiscAnimNode** outAnim) {
    ComEffect* effect = GEffect_FindEffect(effectHash);
    if (effect) {
        if (outAnim) {
            *outAnim = effect->GetMiscAnimNode();
        }
        return true;
    }

    if (outAnim) {
        *outAnim = nullptr;
    }
    return false;
}

Effects* GEffect_Create(u32 effectHash,
                        const LVector* pos,
                        const LVector* scale,
                        const LVector* rotation,
                        s32 frameDelay,
                        s32 lifeFrames,
                        u32 createFlags)
{
    MARKFUNCTION(0x8008DE18);

    s32 createdByOtherEffect = 0;

    s32 fwFlags = ((createFlags & 0x40u) != 0u) ? 1 : 0;
    if ((createFlags & 0x80u) != 0u) {
        fwFlags |= 2;
    }

    u16 rotationWords[3] = {};
    const u16* fwRotation = nullptr;
    if (rotation) {
        rotationWords[0] = static_cast<u16>(rotation->x);
        rotationWords[1] = static_cast<u16>(rotation->y);
        rotationWords[2] = static_cast<u16>(rotation->z);
        fwRotation = rotationWords;
    }

    if (FWEffect::Create2(effectHash, pos, scale, fwRotation, fwFlags)) {
        createdByOtherEffect = 1;
    }

    // PSX also dispatches through FPWEffect here; FPWEffect runtime is not reversed on PC yet.

    if (CBVEffect_CreateForHash(effectHash)) {
        createdByOtherEffect += 1;
    }

    if (createdByOtherEffect != 0) {
        return nullptr;
    }

    if (!g_genericEffectPool.head) {
        return nullptr;
    }

    ComEffect* comEffect = GEffect_FindEffect(effectHash);
    if (!comEffect) {
        // Particle-system-backed GEffect creation is still pending full reversal.
        return nullptr;
    }

    if (!pos) {
        LOG("[GEffect_Create] null-pos create miss hash=0x%08X flags=0x%08X (FW/FPW/CBV dispatch miss)",
            effectHash,
            createFlags);
        return nullptr;
    }

    GEffect* effect = static_cast<GEffect*>(g_genericEffectPool.head);
    effect->comEffect = comEffect;
    effect->particleMgr = nullptr;

    effect->frameDelay = frameDelay;
    effect->frame = 0;
    effect->frameCounter = 0;
    effect->lifeFrames = (lifeFrames != 0) ? lifeFrames : static_cast<s32>(comEffect->GetFrameCount());

    effect->createFlags = createFlags | 8u;
    effect->renderFlags = 0;
    effect->holdAlive = 0;

    effect->pos = *pos;
    effect->posRef = pos;

    effect->scaleRef = nullptr;
    if (scale) {
        if ((createFlags & 2u) != 0u) {
            effect->scaleRef = scale;
        }
        else {
            effect->createFlags |= 0x10u;
            effect->scale = *scale;
        }
        effect->renderFlags = 4;
    }

    effect->rotationRef = nullptr;
    if (rotation) {
        if ((createFlags & 4u) != 0u) {
            effect->rotationRef = rotation;
        }
        else {
            effect->createFlags |= 0x20u;
            effect->rotation = *rotation;
        }
        effect->renderFlags |= 0x118u;
    }

    if (static_cast<s32>(createFlags) < 0) {
        effect->renderFlags |= 2u;
    }

    effect->nameCRC = effectHash;
    effect->effectType = 4;

    g_genericEffectPool.RemNode(effect);
    Effects_AddEffect(effect, 0);

    effect->CreateSound();
    return effect;
}

