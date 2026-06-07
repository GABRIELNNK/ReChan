#include "ai/blast.h"
#include "ai/humanoid.h"
#include "gen/common.h"
#include "gen/database.h"
#include "gen/colvol.h"
#include "gen/effects.h"
#include "gen/geffect.h"
#include "gen/weffect.h"
#include "ai/obstacle_shared.h"
#include "p3d/hash.h"
#include "snd/esound.h"
#include "snd/sndfact.h"
#include "snd/snddrct.h"

#include <cmath>

static constexpr s32 COLLISION_TAG_HIT_TYPE = static_cast<s32>(0x80000003u);
static constexpr s32 COLLISION_TAG_DAMAGE = static_cast<s32>(0x80000007u);
static constexpr s32 COLLISION_TAG_END = 0;
static constexpr s32 BLAST_HIT_TYPE_FIRE = 18;
static constexpr s32 BLAST_DEFAULT_LENGTH = 512;
static constexpr s32 BLAST_DEFAULT_HALF_WIDTH = 128;

static s32 BlastAttribValue(const DBRoot* root, u32 id, s32 defaultValue = 0) {
    u32 value = 0;
    return (root && root->FindAttribValue(id, &value)) ? static_cast<s32>(value) : defaultValue;
}

static u32 BlastAttribStringHash(const DBRoot* root, u32 id) {
    if (!root) {
        return 0;
    }

    const DBAttrib* attrib = root->FindAttrib(id);
    const char* str = attrib ? attrib->GetAttribString() : nullptr;
    return (str && str[0] != '\0') ? p3dHash(str) : 0;
}

static s32 AbsS32(s32 value) {
    return value < 0 ? -value : value;
}

static bool BlastUsesFramedEffect(const Blast* blast) {
    return blast && blast->framedEffect != 0;
}

static s32 BlastDirX(const Blast* blast) {
    return blast ? blast->endPosX - blast->pos.x : 0;
}

static s32 BlastDirY(const Blast* blast) {
    return blast ? blast->endPosY - blast->pos.y : 0;
}

static s32 BlastDirZ(const Blast* blast) {
    return blast ? blast->endPosZ - blast->pos.z : 0;
}

static void NormalizeBlastDirection(const LVector& direction, LVector& out) {
    const s64 x = static_cast<s64>(direction.x);
    const s64 y = static_cast<s64>(direction.y);
    const s64 z = static_cast<s64>(direction.z);
    const s64 magSq = x * x + y * y + z * z;
    if (magSq <= 0) {
        out = {};
        return;
    }

    const double mag = std::sqrt(static_cast<double>(magSq));
    out.x = static_cast<s32>((static_cast<double>(direction.x) * 65536.0) / mag);
    out.y = static_cast<s32>((static_cast<double>(direction.y) * 65536.0) / mag);
    out.z = static_cast<s32>((static_cast<double>(direction.z) * 65536.0) / mag);
}

static void SetBlastEffectFrame(Blast* blast, s32 state) {
    if (!blast) {
        return;
    }

    (void)state;
    blast->effectFrame = 0;
}

static void StartBlastFire(Blast* blast) {
    if (!blast || blast->blastState != 0) {
        return;
    }

    blast->blastState = 1;
    blast->stateTimer = 0;
    SetBlastEffectFrame(blast, 1);

    const u32 effectHash = static_cast<u32>(blast->effectHash);
    if (effectHash && !BlastUsesFramedEffect(blast)) {
        const s32 lifeFrames = blast->extendFrames + blast->holdFrames + blast->retractFrames;
        GEffect_Create(effectHash, &blast->pos, nullptr, nullptr, 0, lifeFrames, 0);
    }

    blast->CreateSound();
}

static void BuildBlastCollisionBox(Blast* blast, s32 progress) {
    if (!blast) {
        return;
    }

    const s32 halfWidth = blast->halfWidth > 0 ? blast->halfWidth : BLAST_DEFAULT_HALF_WIDTH;
    const s32 length = progress > 0 ? progress : 0;

    tagCollisionBox box = {};
    box.minX = static_cast<s16>(-halfWidth);
    box.maxX = static_cast<s16>(halfWidth);
    box.minY = static_cast<s16>(-halfWidth);
    box.maxY = static_cast<s16>(halfWidth);
    box.minZ = 0;
    box.maxZ = static_cast<s16>(length);

    switch (blast->majorAxis) {
        case 0:
            if (BlastDirX(blast) < 0) {
                box.minX = static_cast<s16>(-length);
                box.maxX = 0;
            }
            else {
                box.minX = 0;
                box.maxX = static_cast<s16>(length);
            }
            box.minZ = static_cast<s16>(-halfWidth);
            box.maxZ = static_cast<s16>(halfWidth);
            break;
        case 1:
            if (BlastDirY(blast) < 0) {
                box.minY = static_cast<s16>(-length);
                box.maxY = 0;
            }
            else {
                box.minY = 0;
                box.maxY = static_cast<s16>(length);
            }
            box.minZ = static_cast<s16>(-halfWidth);
            box.maxZ = static_cast<s16>(halfWidth);
            break;
        default:
            if (BlastDirZ(blast) < 0) {
                box.minZ = static_cast<s16>(-length);
                box.maxZ = 0;
            }
            break;
    }

    blast->SetCollisionBox(box);
}

Blast::Blast(const LVector* pos, u16 type)
    : Obstacle(pos, type) {
    MARKFUNCTION(0x80015AB0);
}


Blast::~Blast() {
    MARKFUNCTION(0x80015AF8);
}

void Blast::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x80015B54);
    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;
    Obstacle::AnalyzeMesh(root);
    tagCollisionBox localBox = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
    ObstacleFillCollisionBox(localBox, root, 5);
    SetCollisionBox(localBox);

    cooldownFrames = static_cast<s16>(BlastAttribValue(root, 9));
    extendFrames = static_cast<s16>(BlastAttribValue(root, 10));
    holdFrames = static_cast<s16>(BlastAttribValue(root, 11));
    halfWidth = BlastAttribValue(root, 14, BLAST_DEFAULT_HALF_WIDTH);
    damagePreset = BlastAttribValue(root, 21);
    requireTrigger = BlastAttribValue(root, 22);
    initialTimerAdvance = static_cast<s16>(BlastAttribValue(root, 32));

    const s32 blastDirX = BlastAttribValue(root, 6);
    const s32 blastDirY = BlastAttribValue(root, 7);
    const s32 blastDirZ = BlastAttribValue(root, 8);
    endPosX = pos.x + blastDirX;
    endPosY = pos.y + blastDirY;
    endPosZ = pos.z + blastDirZ;

    majorAxis = 2;
    s32 length = AbsS32(blastDirZ);
    if (AbsS32(blastDirX) > length) {
        majorAxis = 0;
        length = AbsS32(blastDirX);
    }
    if (AbsS32(blastDirY) > length) {
        majorAxis = 1;
        length = AbsS32(blastDirY);
    }
    if (length == 0) {
        length = (collBox.extent > 0) ? collBox.extent : BLAST_DEFAULT_LENGTH;
    }

    LVector normalizedDirection = {};
    NormalizeBlastDirection({ blastDirX << 16, blastDirY << 16, blastDirZ << 16 }, normalizedDirection);
    const s32 forceMin = BlastAttribValue(root, 12);
    const s32 forceMax = BlastAttribValue(root, 13);
    minForceX = MulShift16(normalizedDirection.x, forceMin) << 16;
    minForceY = MulShift16(normalizedDirection.y, forceMin) << 16;
    minForceZ = MulShift16(normalizedDirection.z, forceMin) << 16;
    maxForceX = MulShift16(normalizedDirection.x, forceMax) << 16;
    maxForceY = MulShift16(normalizedDirection.y, forceMax) << 16;
    maxForceZ = MulShift16(normalizedDirection.z, forceMax) << 16;

    lengthPerFrame = extendFrames > 0 ? length / extendFrames : length;
    if (lengthPerFrame <= 0) {
        lengthPerFrame = length;
    }

    effectHash = static_cast<s32>(BlastAttribStringHash(root, 20));
    framedEffect = (BlastAttribValue(root, 23) != 0 && effectHash != 0) ? 1 : 0;

    retractFrames = BlastUsesFramedEffect(this) ? extendFrames : 0;
    switch (damagePreset) {
        case 0:
            collisionDamage = 0;
            break;
        case 1:
            collisionDamage = 3;
            break;
        case 2:
            collisionDamage = 6;
            break;
        case 3:
            collisionDamage = 1;
            break;
        case 4:
            collisionDamage = 4;
            break;
        default:
            collisionDamage = 0;
            break;
    }
    hitCooldownFrames = 2;
    hitCooldownTimer = hitCooldownFrames;
    stateTimer = cooldownFrames - initialTimerAdvance;
    blastState = 0;
    SetBlastEffectFrame(this, 0);
}

void Blast::CreateSound() {
    MARKFUNCTION(0x80016284);
    if (sound) {
        return;
    }
    if (!effect) {
        return;
    }
    CSound* soundObj = nullptr;
    const u32 soundId = effect ? effect->resourceHash : 0;
    if (CSoundFactory::CreateObject(10010, &soundObj, soundId) >= 0) {
        sound = static_cast<CWorldEffectSound*>(soundObj);
        if (sound) {
            sound->Initialize(&pos);
        }
    }
}

void Blast::UpdateSound() {
    MARKFUNCTION(0x800162E4);
    if (sound) {
        sound->Update((u32)effectFrame);
    }
}

void Blast::ReleaseSound() {
    MARKFUNCTION(0x8001631C);
    if (sound) {
        sound->Release();
        sound = nullptr;
    }
}

void Blast::CreateModel(const char* name) {
    MARKFUNCTION(0x80016368);
    flags |= TF_MODEL_CREATED;
    (void)name;
    if (BlastUsesFramedEffect(this)) {
        CreateSound();
    }
}

void Blast::DeleteModel() {
    MARKFUNCTION(0x8001639C);
    framedEffect = 0;
    effect = nullptr;
    flags &= ~TF_MODEL_CREATED;
    ReleaseSound();
}

void Blast::Reset() {
    MARKFUNCTION(0x800163C8);
    blastState = 0;
    stateTimer = cooldownFrames - initialTimerAdvance;
    effect = nullptr;
    if (BlastUsesFramedEffect(this) && effectHash) {
        FWEffect* fwEffect = FWEffect::Find(static_cast<u32>(effectHash));
        if (fwEffect) {
            effect = fwEffect->comEffect;
            effectRenderFlags = static_cast<s32>(fwEffect->renderFlags);
            orientation = fwEffect->rotation;
        }
        else {
            effectRenderFlags = 0;
        }
    }
    SetBlastEffectFrame(this, 0);
    lastEffectFrame = effectFrame;
    hitCooldownTimer = hitCooldownFrames;
    ReleaseSound();
}

void Blast::Activate() {
    MARKFUNCTION(0x800164A4);
    Thing::Activate();
}

void Blast::Deactivate() {
    MARKFUNCTION(0x800164C4);
    Thing::Deactivate();
}

void Blast::Think() {
    MARKFUNCTION(0x80016528);

    s32 progress = 0;

    if (blastState == 0) {
        if (stateTimer >= cooldownFrames && requireTrigger == 0) {
            StartBlastFire(this);
        }
    }
    else if (blastState == 1) {
        progress = lengthPerFrame * stateTimer;
        if (extendFrames > 0 && stateTimer >= extendFrames) {
            blastState = 2;
            stateTimer = 0;
            SetBlastEffectFrame(this, 2);
        }
    }
    else if (blastState == 2) {
        progress = lengthPerFrame * (extendFrames > 0 ? extendFrames : 1);
        if (holdFrames > 0 && stateTimer >= holdFrames) {
            if (retractFrames > 0) {
                blastState = 3;
                stateTimer = 0;
                SetBlastEffectFrame(this, 3);
            }
            else {
                blastState = 0;
                stateTimer = 0;
                SetBlastEffectFrame(this, 0);
            }
        }
    }
    else if (blastState == 3) {
        const s32 remaining = retractFrames - stateTimer;
        progress = lengthPerFrame * (remaining > 0 ? remaining : 0);
        if (stateTimer >= retractFrames) {
            blastState = 0;
            stateTimer = 0;
            SetBlastEffectFrame(this, 0);
        }
    }

    BuildBlastCollisionBox(this, progress);
    stateTimer++;

    if (effect) {
        effectFrame++;
    }

    if (hitCooldownTimer <= 0) {
        hitCooldownTimer = hitCooldownFrames;
    }
    hitCooldownTimer--;
}

void Blast::Trigger() {
    MARKFUNCTION(0x80016A10);

    if (blastState || stateTimer < cooldownFrames) {
        return;
    }

    StartBlastFire(this);
}

void Blast::Draw() {
    MARKFUNCTION(0x80016ACC);
    if (!effect) {
        return;
    }

    UpdateSound();
    effect->SetFrame(effectFrame);
    effect->Render(pos, &orientation, &orientation, static_cast<u32>(effectRenderFlags));
    lastEffectFrame = effectFrame;
}

void Blast::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80016B3C);
    (void)pickup;
}

void Blast::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80016B44);

    if (!blastState) {
        return;
    }

    if (hitCooldownTimer != 0) {
        return;
    }

    if (collisionDamage <= 0) {
        return;
    }

    hum->HandleCollision(
        this,
        1,
        COLLISION_TAG_HIT_TYPE,
        BLAST_HIT_TYPE_FIRE,
        COLLISION_TAG_DAMAGE,
        collisionDamage,
        COLLISION_TAG_END);
}
