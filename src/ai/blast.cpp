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

    if (blast->field192Enabled) {
        const s32 index = (state >= 0 && state < 4) ? state * 2 : 0;
        blast->field200 = blast->frameTable[index];
    }
    else {
        blast->field200 = 0;
    }
}

static void StartBlastFire(Blast* blast) {
    if (!blast || blast->field132 != 0) {
        return;
    }

    blast->field132 = 1;
    blast->field128 = 0;
    SetBlastEffectFrame(blast, 1);

    const u32 effectHash = static_cast<u32>(blast->field188);
    if (effectHash && !blast->field192Enabled) {
        const s32 lifeFrames = blast->field120 + blast->field126 + blast->field122;
        GEffect_Create(effectHash, &blast->pos, nullptr, nullptr, 0, lifeFrames, 0);
    }

    blast->CreateSound();
}

static void BuildBlastCollisionBox(Blast* blast, s32 progress) {
    if (!blast) {
        return;
    }

    const s32 halfWidth = blast->field160 > 0 ? blast->field160 : BLAST_DEFAULT_HALF_WIDTH;
    const s32 length = progress > 0 ? progress : 0;

    tagCollisionBox box = {};
    box.minX = static_cast<s16>(-halfWidth);
    box.maxX = static_cast<s16>(halfWidth);
    box.minY = static_cast<s16>(-halfWidth);
    box.maxY = static_cast<s16>(halfWidth);
    box.minZ = 0;
    box.maxZ = static_cast<s16>(length);

    switch (blast->field184) {
        case 0:
            if (blast->blastDirX < 0) {
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
            if (blast->blastDirY < 0) {
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
            if (blast->blastDirZ < 0) {
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

    field124 = BlastAttribValue(root, 9);
    field120 = BlastAttribValue(root, 10);
    field126 = BlastAttribValue(root, 11);
    field160 = BlastAttribValue(root, 14, BLAST_DEFAULT_HALF_WIDTH);
    field168 = BlastAttribValue(root, 21);
    field116 = BlastAttribValue(root, 22);
    field130 = BlastAttribValue(root, 32);

    blastDirX = BlastAttribValue(root, 6);
    blastDirY = BlastAttribValue(root, 7);
    blastDirZ = BlastAttribValue(root, 8);
    field172 = pos.x + blastDirX;
    field176 = pos.y + blastDirY;
    field180 = pos.z + blastDirZ;

    field184 = 2;
    s32 length = AbsS32(blastDirZ);
    if (AbsS32(blastDirX) > length) {
        field184 = 0;
        length = AbsS32(blastDirX);
    }
    if (AbsS32(blastDirY) > length) {
        field184 = 1;
        length = AbsS32(blastDirY);
    }
    if (length == 0) {
        length = (collBox.extent > 0) ? collBox.extent : BLAST_DEFAULT_LENGTH;
    }

    LVector normalizedDirection = {};
    NormalizeBlastDirection({ blastDirX << 16, blastDirY << 16, blastDirZ << 16 }, normalizedDirection);
    const s32 forceMin = BlastAttribValue(root, 12);
    const s32 forceMax = BlastAttribValue(root, 13);
    field136 = MulShift16(normalizedDirection.x, forceMin) << 16;
    field140 = MulShift16(normalizedDirection.y, forceMin) << 16;
    field144 = MulShift16(normalizedDirection.z, forceMin) << 16;
    field148 = MulShift16(normalizedDirection.x, forceMax) << 16;
    field152 = MulShift16(normalizedDirection.y, forceMax) << 16;
    field156 = MulShift16(normalizedDirection.z, forceMax) << 16;

    field164 = field120 > 0 ? length / field120 : length;
    if (field164 <= 0) {
        field164 = length;
    }

    field188 = static_cast<s32>(BlastAttribStringHash(root, 20));
    field192Enabled = BlastAttribValue(root, 23) != 0 && field188 != 0;
    for (s32 i = 0; i < 8; i++) {
        frameTable[i] = static_cast<s16>(BlastAttribValue(root, static_cast<u32>(24 + i)));
    }

    field122 = field192Enabled ? field120 : 0;
    switch (field168) {
        case 0:
            field216 = 0;
            break;
        case 1:
            field216 = 3;
            break;
        case 2:
            field216 = 6;
            break;
        case 3:
            field216 = 1;
            break;
        case 4:
            field216 = 4;
            break;
        default:
            field216 = 0;
            break;
    }
    field220 = 2;
    field224 = field220;
    field128 = field124 - field130;
    field132 = 0;
    SetBlastEffectFrame(this, 0);
}

void Blast::CreateSound() {
    MARKFUNCTION(0x80016284);
    if (field228) {
        return;
    }
    if (!field196) {
        return;
    }
    CSound* soundObj = nullptr;
    const u32 soundId = field196 ? field196->resourceHash : 0;
    if (CSoundFactory::CreateObject(10010, &soundObj, soundId) >= 0) {
        field228 = static_cast<CWorldEffectSound*>(soundObj);
        if (field228) {
            field228->Initialize(&pos);
        }
    }
}

void Blast::UpdateSound() {
    MARKFUNCTION(0x800162E4);
    if (field228) {
        field228->Update((u32)field200);
    }
}

void Blast::ReleaseSound() {
    MARKFUNCTION(0x8001631C);
    if (field228) {
        field228->Release();
        field228 = nullptr;
    }
}

void Blast::CreateModel(const char* name) {
    MARKFUNCTION(0x80016368);
    flags |= TF_MODEL_CREATED;
    (void)name;
    if (field192Enabled) {
        CreateSound();
    }
}

void Blast::DeleteModel() {
    MARKFUNCTION(0x8001639C);
    field192 = 0;
    field196 = nullptr;
    flags &= ~TF_MODEL_CREATED;
    ReleaseSound();
}

void Blast::Reset() {
    MARKFUNCTION(0x800163C8);
    field132 = 0;
    field128 = field124 - field130;
    field196 = nullptr;
    if (field192Enabled && field188) {
        FWEffect* effect = FWEffect::Find(static_cast<u32>(field188));
        if (effect) {
            field196 = effect->comEffect;
            field208 = static_cast<s32>(effect->renderFlags);
            orientation = effect->rotation;
        }
        else {
            field208 = 0;
        }
    }
    SetBlastEffectFrame(this, 0);
    field204 = field200;
    field224 = field220;
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

    if (field132 == 0) {
        if (field128 >= field124 && field116 == 0) {
            StartBlastFire(this);
        }
    }
    else if (field132 == 1) {
        progress = field164 * field128;
        if (field120 > 0 && field128 >= field120) {
            field132 = 2;
            field128 = 0;
            SetBlastEffectFrame(this, 2);
        }
    }
    else if (field132 == 2) {
        progress = field164 * (field120 > 0 ? field120 : 1);
        if (field126 > 0 && field128 >= field126) {
            if (field122 > 0) {
                field132 = 3;
                field128 = 0;
                SetBlastEffectFrame(this, 3);
            }
            else {
                field132 = 0;
                field128 = 0;
                SetBlastEffectFrame(this, 0);
            }
        }
    }
    else if (field132 == 3) {
        const s32 remaining = field122 - field128;
        progress = field164 * (remaining > 0 ? remaining : 0);
        if (field128 >= field122) {
            field132 = 0;
            field128 = 0;
            SetBlastEffectFrame(this, 0);
        }
    }

    BuildBlastCollisionBox(this, progress);
    field128++;

    if (field196) {
        field200++;
        if (field192Enabled) {
            const s32 frameIndex = (field132 >= 0 && field132 < 4) ? field132 * 2 : 0;
            const s32 startFrame = frameTable[frameIndex];
            const s32 endFrame = frameTable[frameIndex + 1];
            if (endFrame > startFrame && field200 > endFrame) {
                field200 = startFrame;
            }
        }
    }

    if (field224 <= 0) {
        field224 = field220;
    }
    field224--;
}

void Blast::Trigger() {
    MARKFUNCTION(0x80016A10);

    if (field132 || field128 < field124) {
        return;
    }

    StartBlastFire(this);
}

void Blast::Draw() {
    MARKFUNCTION(0x80016ACC);
    if (!field196) {
        return;
    }

    UpdateSound();
    field196->SetFrame(field200);
    field196->Render(pos, &orientation, &orientation, static_cast<u32>(field208));
    field204 = field200;
}

void Blast::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x80016B3C);
    (void)pickup;
}

void Blast::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x80016B44);

    if (!field132) {
        return;
    }

    if (field224 != 0) {
        return;
    }

    if (field216 <= 0) {
        return;
    }

    hum->HandleCollision(
        this,
        1,
        COLLISION_TAG_HIT_TYPE,
        BLAST_HIT_TYPE_FIRE,
        COLLISION_TAG_DAMAGE,
        field216,
        COLLISION_TAG_END);
}
