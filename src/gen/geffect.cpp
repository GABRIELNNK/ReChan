#include "common.h"

#include "gen/geffect.h"
#include "gen/colsect.h"
#include "gen/effects.h"
#include "gen/weffect.h"
#include "gen/pweffect.h"
#include "gen/particle.h"
#include "p3d/byteread.h"
#include "p3d/context.h"
#include "p3d/p3dmath.h"
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
    ParticleSystemMgr* particleMgr = nullptr;

    CWorldEffectSound* worldSound = nullptr;
    CParticleEffectSound* particleSound = nullptr;
};

static ComEffect** g_loadedComEffects = nullptr;
static u32 g_loadedComEffectCount = 0;

static ccList g_genericEffectPool;
static GEffect** g_genericEffects = nullptr;
static u32 g_genericEffectCount = 0;

static Mat4 BuildEffectWorldMatrix(const LVector& pos, const LVector* scale) {
    Mat4 world = Mat4();
    if (scale) {
        world.m[0] = FIX16_TO_FLOAT(scale->x);
        world.m[5] = FIX16_TO_FLOAT(scale->y);
        world.m[10] = FIX16_TO_FLOAT(scale->z);
    }

    world.m[12] = static_cast<f32>(pos.x);
    world.m[13] = static_cast<f32>(pos.y);
    world.m[14] = static_cast<f32>(pos.z);
    return world;
}

GEffect::GEffect() {
    MARKFUNCTION(0x8008E184);
}

GEffect::~GEffect() {
    MARKFUNCTION(0x8008E1CC);

    if (particleMgr) {
        particleMgr->PurgeParticles();
        delete particleMgr;
        particleMgr = nullptr;
    }

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

    blockNum = CollisionSector::GetBlockNumber(*blockPos);

    const s32 previousCounter = frameCounter;
    frameCounter = previousCounter + 1;
    if (previousCounter != frameDelay) {
        return frameCounter;
    }

    frameCounter = 0;

    const bool hasParticleSystem = (particleMgr && particleMgr->GetSystem());
    if (hasParticleSystem) {
        const LVector localOrigin = { 0, 0, 0 };

        if (frame < lifeFrames || holdAlive) {
            particleMgr->CreateParticles(localOrigin, nullptr);
            particleMgr->Update();
            UpdateSound();

            frame += 1;
            return frame;
        }

        if (particleMgr->ActiveParticles()) {
            particleMgr->Update();

            frame += 1;
            return frame;
        }

        particleMgr->PurgeParticles();
        particleMgr->ResetParticleDirection();
    }

    else if ((lifeFrames - frame) > 0 && !holdAlive) {
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

    if (comEffect) {
        comEffect->SetFrame(frame);

        comEffect->Render(renderPos, &renderScale, &renderRotation, renderFlags);
        return;
    }

    const bool hasParticleSystem = (particleMgr && particleMgr->GetSystem());
    if (!hasParticleSystem || !p3d::context) {
        return;
    }

    particleMgr->SetDisplayOffset(nullptr);

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    const Mat4 effectWorld = ((renderFlags & 4u) != 0u)
        ? BuildEffectWorldMatrix(renderPos, &renderScale)
        : BuildEffectWorldMatrix(renderPos, nullptr);
    p3d::context->SetWorldMatrix(savedWorld * effectWorld);
    particleMgr->Display();
    p3d::context->SetWorldMatrix(savedWorld);
}

s32 GEffect::CreateSound() {
    MARKFUNCTION(0x8008E608);

    const bool hasParticleSystem = (particleMgr && particleMgr->GetSystem());
    if (hasParticleSystem) {
        if (particleSound) {
            return 0;
        }

        CSound* createdParticleSound = nullptr;
        s32 result = CSoundFactory::CreateObject(10000, &createdParticleSound, nameCRC);
        if (result >= 0 && createdParticleSound) {
            particleSound = static_cast<CParticleEffectSound*>(createdParticleSound);
            return particleSound->Initialize(posRef);
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
    const s32 result = CSoundFactory::CreateObject(10010, &createdSound, nameCRC);
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

    const bool hasParticleSystem = (particleMgr && particleMgr->GetSystem());
    if (hasParticleSystem) {
        if (particleSound) {
            return particleSound->StartAnimating();
        }

        return 1;
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
                effect->particleMgr = new ParticleSystemMgr();
                g_genericEffects[i] = effect;
                g_genericEffectPool.AddNode(g_genericEffectPool.tail, effect);
            }
        }
    }

    if (p + 4 <= bodyEnd) {
        const u32 commonParticleCount = p3dReadU32LE(p);
        ParticleSystem_CommonParticles(static_cast<s32>(commonParticleCount));
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

    s32 fpwFollowPos = ((createFlags & 1u) != 0u) ? 1 : 0;

    const LVector* fwRotation = rotation;

    if (FWEffect::Create2(effectHash, pos, scale, fwRotation, fwFlags)) {
        createdByOtherEffect = 1;
    }

    if (FPWEffect_Create2(effectHash, pos, rotation, lifeFrames, fpwFollowPos)) {
        createdByOtherEffect += 1;
    }

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
    ParticleSystem* particleSystem = nullptr;
    if (!comEffect) {
        particleSystem = ParticleSystem_Find(effectHash);
        if (!particleSystem) {
            return nullptr;
        }
    }

    const s32 effectType = comEffect ? 4 : 3;
    if (Effects_Find(effectType, effectHash)) {
        return nullptr;
    }

    GEffect* effect = static_cast<GEffect*>(g_genericEffectPool.head);
    effect->particleMgr->SetSystem(nullptr);

    effect->comEffect = comEffect;
    if (particleSystem) {
        effect->particleMgr->InitMgr(particleSystem);
    }

    effect->frameDelay = frameDelay;
    effect->frame = 0;
    effect->frameCounter = 0;
    effect->lifeFrames = comEffect
        ? ((lifeFrames != 0) ? lifeFrames : static_cast<s32>(comEffect->GetFrameCount()))
        : lifeFrames;

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
    if (comEffect) {
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
    }
    else if (rotation && effect->particleMgr) {
        effect->particleMgr->SetParticleDirection(rotation);
    }

    if (comEffect && static_cast<s32>(createFlags) < 0) {
        effect->renderFlags |= 2u;
    }

    effect->nameCRC = effectHash;
    effect->effectType = comEffect ? 4 : 3;

    g_genericEffectPool.RemNode(effect);
    Effects_AddEffect(effect, 0);

    effect->CreateSound();
    return effect;
}

