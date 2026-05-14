#include "common.h"

#include "gen/particle.h"

#include "gen/camera.h"
#include "gen/model.h"
#include "gen/psxmath_helpers.h"
#include "gen/weffect.h"

#include "p3d/context.h"
#include "p3d/byteread.h"
#include "p3d/p3dmath.h"

#include <cmath>
#include <cstring>

static const s32 kMaxParticleSystems = 64;
static const s32 kParticleInfoCount = 140;

class ParticleInfo : public ccMinNode {
public:
    ParticleInfo() {
        MARKFUNCTION(0x80097C4C);
        life = 0;
        active = 0;
        meshIndex = 0;
    }

    ~ParticleInfo() override {
        MARKFUNCTION(0x80097C8C);
    }

    s16 posX = 0;
    s16 posY = 0;
    s16 posZ = 0;

    s16 velX = 0;
    s16 velY = 0;
    s16 velZ = 0;

    s16 velNormX = 0;
    s16 velNormY = 0;
    s16 velNormZ = 0;

    s32 scale = 0;
    s32 scaleStep = 0;
    s32 scaleStep2 = 0;
    s32 dragScale = 0;
    s32 rotation = 0;
    s32 rotationStep = 0;

    u16 meshIndex = 0;

    u8 axisMask = 0;
    u8 life = 0;
    u8 age = 0;
    u8 frameAge = 0;
    u8 animFrame = 0;
    u8 animFrameCounter = 0;
    u8 animHoldCounter = 0;
    u8 active = 0;
};

struct ParticleStats {
    ParticleStats() {
        MARKFUNCTION(0x80097CB4);
    }

    ~ParticleStats() {
        MARKFUNCTION(0x80097CC8);
        if (comEffect) {
            delete comEffect;
            comEffect = nullptr;
        }
    }

    u32 flags = 0;

    s32 speedBase = 0;
    s32 speedRange = 0;

    s32 scaleBase = 0x10000;
    s32 scaleRange = 0;

    s32 dragBase = 0;
    s32 dragRange = 0;

    s32 accelBase = 0;
    s32 accelRange = 0;

    s32 dirX = 0;
    s32 dirY = 0;
    s32 dirXDefault = 0;
    s32 dirYDefault = 0;

    s32 spreadX = 0;
    s32 spreadY = 0;

    u16 spawnPerBurst = 1;
    u16 maxParticles = 0;
    u16 maxActive = 0;
    u16 gravity = 0;

    u16 lifeBase = 1;
    u16 lifeRange = 0;

    u16 growFrames = 0;
    u16 shrinkFrames = 0;
    u16 normalizeFrames = 0;

    u16 particleLife = 30;
    u16 burstStart = 0;
    u16 burstEnd = 0;
    u16 burstCounter = 0;

    u16 animEndFrame = 0;
    u16 animDelay = 0;

    ComEffect* comEffect = nullptr;
    void* fastRenderInfo = nullptr;
    u32 fastRenderValue = 0;
};

struct ParticleMeshEntry {
    u16 geoIndex = 0;
    Mat4 localMatrix = Mat4();
};

class ParticleSystem : public ccNode {
public:
    ParticleSystem();
    ~ParticleSystem() override;

    s32 ParseData(const u8* body, u32 bodySize);
    s32 AnalyzeMesh();

    s32 SetParticleDirection(const LVector* direction);
    s32 ResetParticleDirection();

    s32 PurgeParticles();
    s32 ActiveParticles();
    s32 CreateParticles(const LVector& origin, ParticleStats* overrideStats);
    s32 InitParticles(const LVector& origin);
    s32 Update();
    void Display();

    void BindParticleList(ccMinList* list) {
        particleList = list ? list : &ownedParticles;
    }

    void SetDisplayOffset(LVector* offset) {
        displayOffset = offset;
    }

    ParticleStats* stats = nullptr;
    ccMinList ownedParticles;
    ccMinList* particleList = &ownedParticles;

    ParticleMeshEntry* meshEntries = nullptr;
    u16 meshEntryCount = 0;
    u32* meshFrameList = nullptr;
    u16 meshFrameMask = 0;

    LVector averagePos = {};
    LVector basePos = {};
    LVector extraOffset = {};

    LVector* displayOffset = nullptr;

    s32 freeLife = 0x10000;
    s32 activeFlag = 1;
};

static ParticleSystem* g_particleArray[kMaxParticleSystems] = {};
static ParticleSystem* g_commonParticleArray[kMaxParticleSystems] = {};

static s32 g_particleCount = 0;
static s32 g_commonParticleCount = 0;
static s32 g_commonParticlesPending = 0;

static ParticleInfo* g_particleInfoMemory = nullptr;
static ccMinList g_particleAvailList;

static u16 ReadU16Particle(const u8* p) {
    return static_cast<u16>(p3dReadU16LE(p));
}

static s32 ReadS32Particle(const u8* p) {
    return static_cast<s32>(p3dReadU32LE(p));
}

static s32 RandomSigned16() {
    return static_cast<s32>(rmRangedRandom(0x1FFFFu)) - 0xFFFF;
}

static s32 Reciprocal16(s32 value) {
    if (value == 0) {
        return 0;
    }

    if (value < 0) {
        value = -value;
    }

    return rmDiv16i(0x10000, value);
}

static bool ParticleGeoEligible(const OriginalGeo* geo) {
    return geo
        && geo->meshBuffer
        && geo->dynamicVerts
        && geo->dynamicVertCount > 0
        && geo->dynamicPrimStart
        && geo->dynamicPrimVertCount
        && geo->dynamicPrimCount > 0;
}

static void BuildParticleScaleMatrix(s32 scale, Mat4& out) {
    out = Mat4();
    const f32 s = FIX16_TO_FLOAT(scale);
    out.m[0] = s;
    out.m[5] = s;
    out.m[10] = s;
}

static s32 QuantizeParticleAngle(s32 rotation) {
    const u16 angle = static_cast<u16>(rotation >> 8);
    const u16 quantized = static_cast<u16>(((static_cast<u32>(angle) + 2u) >> 2) << 2);
    return static_cast<s32>(quantized);
}

static void PreMultiplyRotZ(Mat4& matrix, s32 angle16) {
    Mat4 rot;
    p3dBuildRotMatrixZ(ANGLE2RAD(angle16), rot);
    matrix = rot * matrix;
}

static void PreMultiplyRotY(Mat4& matrix, s32 angle16) {
    Mat4 rot;
    p3dBuildRotMatrixY(ANGLE2RAD(angle16), rot);
    matrix = rot * matrix;
}

static void PreMultiplyRotX(Mat4& matrix, s32 angle16) {
    Mat4 rot;
    p3dBuildRotMatrixX(ANGLE2RAD(angle16), rot);
    matrix = rot * matrix;
}

static void BuildBillboardMatrixPSX(const LVector& pos, Mat4& out, bool lockY) {
    if (!g_display || !g_display->GetCamera()) {
        p3dFillTransMatrix(pos, out);
        return;
    }

    const LVector& cameraPos = g_display->GetCamera()->GetPosition();
    Vec3 heading(
        static_cast<f32>(cameraPos.x - pos.x),
        lockY ? 0.0f : static_cast<f32>(cameraPos.y - pos.y),
        static_cast<f32>(cameraPos.z - pos.z));

    const Vec3 up(0.0f, 1.0f, 0.0f);
    out = Mat4();
    p3dFillHeadingMatrix(heading, up, out);
    p3dFillTransMatrix(pos, out);
}

static void SphereToCart(s16 azimuth, s16 polar, LVector& out) {
    MARKFUNCTION(0x80098020);

    const s32 az90 = static_cast<s16>(azimuth + 0x4000);
    const s32 sinAz90 = rmSin16(az90);

    out.x = static_cast<s32>((static_cast<s64>(sinAz90) * rmSin16(polar)) >> 16);
    out.y = rmSin16(azimuth);
    out.z = static_cast<s32>((static_cast<s64>(sinAz90) * rmSin16(static_cast<s16>(polar + 0x4000))) >> 16);
}

ParticleSystem::ParticleSystem() {
    MARKFUNCTION(0x80097B18);

    freeLife = 0x10000;
    activeFlag = 1;
    stats = nullptr;
    meshEntries = nullptr;
    meshEntryCount = 0;
    meshFrameList = nullptr;
    meshFrameMask = 0;
    particleList = &ownedParticles;
}

ParticleSystem::~ParticleSystem() {
    MARKFUNCTION(0x80097B98);

    PurgeParticles();

    delete stats;
    stats = nullptr;

    delete[] meshEntries;
    meshEntries = nullptr;
    meshEntryCount = 0;

    delete[] meshFrameList;
    meshFrameList = nullptr;
    meshFrameMask = 0;
}

s32 ParticleSystem::ParseData(const u8* body, u32 bodySize) {
    MARKFUNCTION(0x80095860);

    delete stats;
    stats = new ParticleStats();

    if (!stats || !body || bodySize < 4) {
        return 0;
    }

    u32 flags = 0;
    u32 cursor = 0;

    while (cursor + 4 <= bodySize) {
        const u16 tag = ReadU16Particle(body + cursor);
        const u16 length = ReadU16Particle(body + cursor + 2);
        cursor += 4;

        u32 payloadSize = length;
        if (cursor + payloadSize > bodySize) {
            payloadSize = bodySize - cursor;
        }

        const u8* payload = body + cursor;

        auto readLongAt = [&](u32 offset) -> s32 {
            if (offset + 4 > payloadSize) {
                return 0;
            }
            return ReadS32Particle(payload + offset);
        };

        if (tag == 0x0100) {
            stats->speedBase = readLongAt(0);
            stats->speedRange = readLongAt(4);
        }
        else if (tag == 0x0200) {
            const s32 v0 = readLongAt(0) << 16;
            const s32 v1 = readLongAt(4) << 16;
            const s32 v2 = readLongAt(8) << 16;
            const s32 v3 = readLongAt(12) << 16;

            if (v2 <= 0) {
                stats->scaleBase = 0x10000;
                stats->scaleRange = 0;
            }
            else {
                stats->scaleBase = rmDiv16i(v0, v2);
                stats->scaleRange = rmDiv16i(v1, v3);
            }
        }
        else if (tag == 0x0300) {
            stats->lifeBase = static_cast<u16>(readLongAt(0));
            stats->lifeRange = static_cast<u16>(readLongAt(4));
        }
        else if (tag == 0x0400) {
            stats->dragBase = readLongAt(0);
            stats->dragRange = readLongAt(4);
        }
        else if (tag == 0x0500) {
            const s32 v0 = readLongAt(0) << 16;
            const s32 v1 = readLongAt(4) << 16;
            const s32 v2 = readLongAt(8) << 16;
            const s32 v3 = readLongAt(12) << 16;

            if (v2 > 0) {
                stats->accelBase = rmDiv16i(v0, v2);
                stats->accelRange = rmDiv16i(v1, v3);
                flags |= 4u;
            }
        }
        else if (tag == 0x0510) {
            stats->spawnPerBurst = static_cast<u16>(readLongAt(0));
            stats->maxParticles = static_cast<u16>(readLongAt(8));
        }
        else if (tag == 0x0600) {
            stats->dirX = readLongAt(0);
            stats->dirY = readLongAt(4);
            stats->dirXDefault = stats->dirX;
            stats->dirYDefault = stats->dirY;
        }
        else if (tag == 0x0610) {
            stats->spreadX = readLongAt(0);
            stats->spreadY = readLongAt(4);
        }
        else if (tag == 0x0700) {
            stats->growFrames = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0710) {
            stats->shrinkFrames = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0715) {
            stats->normalizeFrames = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0720) {
            stats->gravity = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0730) {
            stats->particleLife = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0740) {
            stats->burstStart = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0750) {
            stats->burstEnd = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0760) {
            stats->animEndFrame = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0770) {
            stats->animDelay = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0780) {
            stats->maxActive = static_cast<u16>(readLongAt(0));
        }
        else if (tag == 0x0800) {
            if (readLongAt(0) != 0) {
                flags |= 0x10u;
            }
        }
        else if (tag == 0x0810) {
            if (readLongAt(0) != 0) {
                flags |= 0x20u;
            }
        }
        else if (tag == 0x0820) {
            if (readLongAt(0) != 0) {
                flags |= 8u;
            }
        }
        else if (tag == 0x0830) {
            if (readLongAt(0) != 0) {
                flags |= 0x8000u;
            }
        }
        else if (tag == 0x0840) {
            if (readLongAt(0) != 0) {
                flags |= 2u;
            }
        }
        else if (tag == 0x0850) {
            if (readLongAt(0) != 0) {
                flags |= 0x1000u;
            }
        }
        else if (tag == 0x0860) {
            if (readLongAt(0) != 0) {
                flags |= 0x4000u;
            }
        }
        else if (tag == 0x0870) {
            if (readLongAt(0) != 0) {
                flags |= 0x2000u;
            }
        }
        else if (tag == 0x0880) {
            if (readLongAt(0) != 0) {
                flags |= 0x10000u;
            }
        }
        else if (tag == 0x0900) {
            stats->fastRenderValue = static_cast<u32>(readLongAt(0));
        }
        else if (tag == 0x1000) {
            const s32 resourceHash = readLongAt(0);
            const s32 animHash = readLongAt(4);

            if (!stats->comEffect) {
                stats->comEffect = new ComEffect();
            }

            if (stats->comEffect) {
                stats->comEffect->LoadETree(resourceHash, animHash);
            }
        }
        else if (tag == 0x1100) {
            const u32 hash = static_cast<u32>(readLongAt(0));

            char nameBuffer[32] = {};
            const u32 copyLen = (payloadSize > 4) ? ((payloadSize - 4 > 31) ? 31 : payloadSize - 4) : 0;
            if (copyLen > 0) {
                std::memcpy(nameBuffer, payload + 4, copyLen);
                nameBuffer[31] = '\0';
                SetName(nameBuffer, 1);
            }

            nameCRC = hash;
        }

        cursor += payloadSize;
    }

    stats->flags = flags;
    if ((flags & 0x10000u) != 0u && stats->comEffect) {
        (void)stats->comEffect->SetUpFirstGeo();
    }

    return 1;
}

s32 ParticleSystem::AnalyzeMesh() {
    MARKFUNCTION(0x80095F78);

    if (!stats || !stats->comEffect) {
        return 0;
    }

    delete[] meshEntries;
    meshEntries = nullptr;
    meshEntryCount = 0;

    delete[] meshFrameList;
    meshFrameList = nullptr;
    meshFrameMask = 0;

    const s32 geoCount = static_cast<s32>(stats->comEffect->GetGeoCount());

    // PSX AnalyzeMesh takes the non-0x8000 path for matrix/geo entries and
    // the 0x8000 path for packed geo tables.
    if ((stats->flags & 0x8000u) == 0u) {
        if (geoCount <= 0) {
            return 0;
        }

        meshEntries = new ParticleMeshEntry[geoCount];
        if (!meshEntries) {
            return 0;
        }

        u16 entryCount = 0;

        for (s32 i = 0; i < geoCount; i++) {
            OriginalGeo* geo = nullptr;
            Mat4 localMatrix = Mat4();
            if (!stats->comEffect->ResolveGeoByIndex(static_cast<u32>(i), &geo, &localMatrix) || !geo) {
                continue;
            }

            if (!ParticleGeoEligible(geo)) {
                continue;
            }

            meshEntries[entryCount].geoIndex = static_cast<u16>(i);
            meshEntries[entryCount].localMatrix = localMatrix;
            entryCount++;
        }

        meshEntryCount = entryCount;
        return meshEntryCount;
    }

    if (geoCount <= 0 || geoCount >= 33) {
        return 0;
    }

    if (geoCount >= 17) {
        meshFrameMask = 31;
    }
    else if (geoCount >= 9) {
        meshFrameMask = 15;
    }
    else if (geoCount >= 5) {
        meshFrameMask = 7;
    }
    else {
        meshFrameMask = 3;
    }

    meshFrameList = new u32[meshFrameMask + 1];
    if (!meshFrameList) {
        return 0;
    }

    u32 writeIndex = 0;
    for (s32 i = 0; i < geoCount && writeIndex <= meshFrameMask; i++) {
        meshFrameList[writeIndex++] = static_cast<u32>(i);
    }

    while (writeIndex <= meshFrameMask) {
        meshFrameList[writeIndex] = meshFrameList[writeIndex - static_cast<u32>(geoCount)];
        writeIndex++;
    }

    return geoCount;
}

s32 ParticleSystem::SetParticleDirection(const LVector* direction) {
    MARKFUNCTION(0x800963A4);

    if (!stats || !direction) {
        return 0;
    }

    stats->dirX = direction->x;
    stats->dirY = direction->y;
    return stats->dirY;
}

s32 ParticleSystem::ResetParticleDirection() {
    MARKFUNCTION(0x800963C4);

    if (!stats) {
        return 0;
    }

    stats->dirX = stats->dirXDefault;
    stats->dirY = stats->dirYDefault;
    return stats->dirY;
}

s32 ParticleSystem::PurgeParticles() {
    MARKFUNCTION(0x800963EC);

    if (!particleList) {
        return 0;
    }

    for (ccMinNode* node = particleList->head; node;) {
        ParticleInfo* info = static_cast<ParticleInfo*>(node);
        node = node->next;

        info->life = 0;
        info->active = 0;

        particleList->RemNode(info);
        g_particleAvailList.AddNodeTail(info);
    }

    if (stats) {
        stats->burstCounter = 0;
    }

    return 0;
}

s32 ParticleSystem::ActiveParticles() {
    MARKFUNCTION(0x8009648C);

    if (!particleList) {
        return 0;
    }

    s32 count = 0;
    for (ccMinNode* node = particleList->head; node; node = node->next) {
        count++;
    }

    return count;
}

s32 ParticleSystem::CreateParticles(const LVector& origin, ParticleStats* overrideStats) {
    MARKFUNCTION(0x800964C0);

    if (overrideStats) {
        stats = overrideStats;
    }

    if (!stats || !particleList) {
        return 0;
    }

    stats->burstCounter = static_cast<u16>(stats->burstCounter + 1);

    const s32 active = ActiveParticles();
    if (stats->burstStart != 0 && stats->burstStart < stats->burstCounter) {
        if (active == 0) {
            if (stats->burstEnd == 0 || stats->burstCounter >= stats->burstEnd) {
                stats->burstCounter = 0;
            }
        }

        return 0;
    }

    if (static_cast<s32>(stats->maxActive) < active) {
        return 0;
    }

    s32 pendingCount = 0;

    while (pendingCount < static_cast<s32>(stats->spawnPerBurst)) {
        if (!g_particleAvailList.head) {
            break;
        }

        ParticleInfo* info = static_cast<ParticleInfo*>(g_particleAvailList.RemHead());
        if (!info || info->life != 0) {
            break;
        }

        particleList->AddNodeTail(info);
        pendingCount++;
    }

    if (pendingCount < static_cast<s32>(stats->spawnPerBurst) && particleList->head) {
        for (ccMinNode* node = particleList->head; node; node = node->next) {
            ParticleInfo* info = static_cast<ParticleInfo*>(node);
            if (info->life == 0) {
                pendingCount++;
                if (pendingCount >= static_cast<s32>(stats->spawnPerBurst)) {
                    break;
                }
            }
        }
    }

    if (pendingCount > 0) {
        InitParticles(origin);
    }

    return pendingCount;
}

s32 ParticleSystem::InitParticles(const LVector& origin) {
    MARKFUNCTION(0x80096698);

    if (!stats || !particleList) {
        return 0;
    }

    basePos = origin;
    averagePos = origin;

    static const u8 axisPattern[4] = { 1, 2, 4, 2 };
    s32 meshCounter = 0;

    for (ccMinNode* node = particleList->head; node; node = node->next) {
        ParticleInfo* info = static_cast<ParticleInfo*>(node);
        if (info->life != 0) {
            continue;
        }

        info->age = 0;
        info->frameAge = 0;
        info->animFrame = 0;
        info->animFrameCounter = 0;
        info->animHoldCounter = 0;
        info->active = 0;

        const s32 lifeRand = static_cast<s32>((static_cast<s64>(RandomSigned16()) * stats->lifeRange) >> 16);
        info->life = static_cast<u8>(static_cast<s32>(stats->lifeBase) + lifeRand);

        info->posX = static_cast<s16>(origin.x);
        info->posY = static_cast<s16>(origin.y);
        info->posZ = static_cast<s16>(origin.z);

        if ((stats->flags & 0x10u) != 0u && meshCounter < static_cast<s32>(stats->spawnPerBurst)) {
            info->meshIndex = static_cast<u16>(meshCounter++);
        }
        else {
            info->meshIndex = 0;
        }

        const s16 azimuth = static_cast<s16>(stats->dirX + ((static_cast<s64>(RandomSigned16()) * stats->spreadX) >> 16));
        const s16 polar = static_cast<s16>(stats->dirY + ((static_cast<s64>(RandomSigned16()) * stats->spreadY) >> 16));

        LVector dir = {};
        SphereToCart(azimuth, polar, dir);

        const s32 speedRandom = static_cast<s32>((static_cast<s64>(stats->speedRange) * RandomSigned16()) >> 16);
        const s32 speed = (speedRandom + stats->speedBase) >> 1;

        dir.x = static_cast<s32>((static_cast<s64>(dir.x) * speed) >> 16);
        dir.y = static_cast<s32>((static_cast<s64>(dir.y) * speed) >> 16);
        dir.z = static_cast<s32>((static_cast<s64>(dir.z) * speed) >> 16);

        if (stats->normalizeFrames != 0) {
            const s32 inv = Reciprocal16(stats->normalizeFrames);
            info->velNormX = static_cast<s16>((static_cast<s64>(dir.x) * inv) >> 16);
            info->velNormY = static_cast<s16>((static_cast<s64>(dir.y) * inv) >> 16);
            info->velNormZ = static_cast<s16>((static_cast<s64>(dir.z) * inv) >> 16);

            if (static_cast<s16>(stats->normalizeFrames) > 0) {
                info->velX = info->velNormX;
                info->velY = info->velNormY;
                info->velZ = info->velNormZ;
            }
            else {
                info->velX = static_cast<s16>(dir.x);
                info->velY = static_cast<s16>(dir.y);
                info->velZ = static_cast<s16>(dir.z);
            }

            stats->flags |= 0x80000u;
        }
        else {
            info->velX = static_cast<s16>(dir.x);
            info->velY = static_cast<s16>(dir.y);
            info->velZ = static_cast<s16>(dir.z);
        }

        const s32 rotationRandom = static_cast<s32>((static_cast<s64>(stats->dragRange) * RandomSigned16()) >> 16);
        s32 rotationStep = (rotationRandom + stats->dragBase) << 8;

        if ((stats->flags & 0x2000u) != 0u && info->life != 0) {
            const s32 invLife = Reciprocal16(static_cast<s32>(info->life));
            rotationStep = static_cast<s32>((static_cast<s64>(rotationStep) * invLife) >> 16);
        }

        if ((rmRangedRandom(2) & 1u) == 0u) {
            rotationStep = -rotationStep;
        }

        info->rotationStep = rotationStep;

        if ((stats->flags & 0x1000u) != 0u) {
            info->rotation = static_cast<s32>((static_cast<s64>(RandomSigned16()) * 0xFFFF) >> 8);
        }
        else {
            info->rotation = 0;
        }

        info->axisMask = axisPattern[(rmRangedRandom(4) + 1u) & 3u];

        const s32 scaleRandom = static_cast<s32>((static_cast<s64>(stats->scaleRange) * RandomSigned16()) >> 16);
        s32 scale = scaleRandom + stats->scaleBase;

        if ((stats->flags & 0x2000u) != 0u && info->life != 0) {
            const s32 invLife = Reciprocal16(static_cast<s32>(info->life));
            scale = static_cast<s32>((static_cast<s64>(scale) * invLife) >> 16);
        }

        info->scale = scale;

        if (stats->growFrames != 0) {
            stats->flags |= 0x40000u;
            info->scaleStep = static_cast<s32>((static_cast<s64>(scale) * Reciprocal16(stats->growFrames)) >> 16);
            info->scale = info->scaleStep;
        }

        if (stats->shrinkFrames != 0) {
            stats->flags |= 0x100000u;
            info->scaleStep2 = static_cast<s32>((static_cast<s64>(scale) * Reciprocal16(stats->shrinkFrames)) >> 16);
        }

        if ((stats->flags & 4u) != 0u) {
            const s32 dragRand = static_cast<s32>((static_cast<s64>(stats->accelRange) * RandomSigned16()) >> 16);
            info->dragScale = dragRand + stats->accelBase;
        }
        else {
            info->dragScale = 0x10000;
        }

        if ((stats->flags & 0x8000u) != 0u && meshFrameMask != 0) {
            info->meshIndex = static_cast<u16>(RandomSigned16() & static_cast<s32>(meshFrameMask));
        }
    }

    return 1;
}

s32 ParticleSystem::Update() {
    MARKFUNCTION(0x80096EF0);

    if (!stats || !particleList) {
        return 0;
    }

    averagePos.x = 0;
    averagePos.y = 0;
    averagePos.z = 0;

    s32 activeCount = 0;
    bool hadAnyNode = false;

    for (ccMinNode* node = particleList->head; node;) {
        ParticleInfo* info = static_cast<ParticleInfo*>(node);
        node = node->next;
        hadAnyNode = true;

        if (info->life == 0) {
            info->active = 0;
            particleList->RemNode(info);
            g_particleAvailList.AddNodeTail(info);
            continue;
        }

        info->age = static_cast<u8>(info->age + 1);

        if (stats->particleLife < info->age) {
            const u8 prevHold = info->animHoldCounter;
            info->animHoldCounter = static_cast<u8>(info->animHoldCounter + 1);
            if (prevHold >= static_cast<u8>(stats->animDelay)) {
                info->animHoldCounter = 0;
                info->animFrame = static_cast<u8>(info->animFrame + 1);
                if (stats->comEffect && stats->comEffect->EndOfFrame(info->animFrame)) {
                    info->animFrame = 0;
                }
            }
        }

        info->frameAge = static_cast<u8>(info->frameAge + 1);

        if (stats->particleLife < info->frameAge) {
            const u8 prevCounter = info->animFrameCounter;
            info->animFrameCounter = static_cast<u8>(info->animFrameCounter + 2);

            if (static_cast<u8>(prevCounter + 1) >= static_cast<u8>(stats->animEndFrame)) {
                info->active = 1;
                info->animFrameCounter = 0;
                info->life = static_cast<u8>(info->life - 1);
                info->age = static_cast<u8>(info->age + 1);

                const s16 extraX = static_cast<s16>(extraOffset.x);
                const s16 extraY = static_cast<s16>(extraOffset.y);
                const s16 extraZ = static_cast<s16>(extraOffset.z);

                info->posX = static_cast<s16>(info->posX + info->velX + extraX);
                info->posY = static_cast<s16>(info->posY + info->velY + extraY);
                info->posZ = static_cast<s16>(info->posZ + info->velZ + extraZ);

                averagePos.x += info->posX;
                averagePos.y += info->posY;
                averagePos.z += info->posZ;
                activeCount++;

                if ((stats->flags & 0x80000u) != 0u && info->age < stats->normalizeFrames) {
                    info->velX = static_cast<s16>(info->velX + info->velNormX);
                    info->velY = static_cast<s16>(info->velY + info->velNormY);
                    info->velZ = static_cast<s16>(info->velZ + info->velNormZ);
                }

                if ((stats->flags & 4u) != 0u) {
                    info->velX = static_cast<s16>((static_cast<s64>(info->velX) * info->dragScale) >> 16);
                    info->velY = static_cast<s16>((static_cast<s64>(info->velY) * info->dragScale) >> 16);
                    info->velZ = static_cast<s16>((static_cast<s64>(info->velZ) * info->dragScale) >> 16);
                }

                if ((stats->flags & 0x40000u) != 0u && info->age < stats->growFrames) {
                    info->scale += info->scaleStep;
                }

                if ((stats->flags & 0x100000u) != 0u && info->life < stats->shrinkFrames) {
                    info->scale -= info->scaleStep2;
                }

                info->rotation += info->rotationStep;

                if ((stats->flags & 2u) != 0u) {
                    info->velY = static_cast<s16>(info->velY + ((-static_cast<s16>(stats->gravity)) >> 1));
                }
            }
        }
    }

    if (activeCount > 0) {
        const s32 inv = Reciprocal16(activeCount);
        averagePos.x = static_cast<s32>((static_cast<s64>(averagePos.x) * inv) >> 16);
        averagePos.y = static_cast<s32>((static_cast<s64>(averagePos.y) * inv) >> 16);
        averagePos.z = static_cast<s32>((static_cast<s64>(averagePos.z) * inv) >> 16);
    }
    else {
        averagePos = basePos;
    }

    if (!hadAnyNode) {
        if (freeLife == 0) {
            activeFlag = 0;
        }
    }

    return activeCount;
}

void ParticleSystem::Display() {
    MARKFUNCTION(0x80097540);

    if (!stats || !stats->comEffect || !particleList || !p3d::context) {
        return;
    }

    if (!stats->comEffect->PointInView(averagePos, 512)) {
        return;
    }

    u32 renderFlags = ((stats->flags & 0x800u) != 0u) ? 0x800u : 0u;
    if ((stats->flags & 0x10000u) != 0u) {
        renderFlags |= 0x800000u;
        stats->comEffect->InitFastRender(stats->comEffect->GetGeo());
    }

    const bool billboard = (stats->flags & 8u) != 0u;
    const bool billboardLockY = (stats->flags & 0x20u) != 0u;
    const Mat4& parentWorld = p3d::context->GetWorldMatrix();
    Mat4 parentInverse = Mat4();
    if (billboard) {
        PsxInverseOrthMatrix(parentWorld, parentInverse);
    }

    for (ccMinNode* node = particleList->head; node; node = node->next) {
        ParticleInfo* info = static_cast<ParticleInfo*>(node);
        if (!info->active) {
            continue;
        }

        LVector renderPos = {
            static_cast<s32>(info->posX),
            static_cast<s32>(info->posY),
            static_cast<s32>(info->posZ),
        };

        if (displayOffset) {
            renderPos.x += displayOffset->x;
            renderPos.y += displayOffset->y;
            renderPos.z += displayOffset->z;
        }

        Mat4 world;
        const s32 angle16 = QuantizeParticleAngle(info->rotation);

        if (billboard) {
            f32 worldX = 0.0f;
            f32 worldY = 0.0f;
            f32 worldZ = 0.0f;
            Mat4TransformPoint(parentWorld,
                               static_cast<f32>(renderPos.x),
                               static_cast<f32>(renderPos.y),
                               static_cast<f32>(renderPos.z),
                               worldX,
                               worldY,
                               worldZ);

            const LVector billboardWorldPos = {
                static_cast<s32>(worldX),
                static_cast<s32>(worldY),
                static_cast<s32>(worldZ),
            };

            Mat4 billboardWorld = Mat4();
            BuildBillboardMatrixPSX(billboardWorldPos, billboardWorld, billboardLockY);
            world = parentInverse * billboardWorld;

            if (angle16 != 0) {
                Mat4 rotZ;
                p3dBuildRotMatrixZ(ANGLE2RAD(angle16), rotZ);
                world = world * rotZ;
            }

            const f32 s = FIX16_TO_FLOAT(info->scale);
            world.m[0] *= s;
            world.m[1] *= s;
            world.m[2] *= s;
            world.m[4] *= s;
            world.m[5] *= s;
            world.m[6] *= s;
            world.m[8] *= s;
            world.m[9] *= s;
            world.m[10] *= s;
        }
        else {
            BuildParticleScaleMatrix(info->scale, world);

            if (angle16 != 0) {
                if ((info->axisMask & 1u) != 0u) {
                    PreMultiplyRotZ(world, angle16);
                }

                if ((info->axisMask & 2u) != 0u) {
                    PreMultiplyRotY(world, angle16);
                }

                if ((info->axisMask & 4u) != 0u) {
                    PreMultiplyRotX(world, angle16);
                }
            }
        }

        p3dFillTransMatrix(renderPos, world);

        Mat4 particleWorld = world;

        if ((stats->flags & 0x10u) != 0u && meshEntries && meshEntryCount > 0) {
            const u16 meshIndex = static_cast<u16>(info->meshIndex % meshEntryCount);
            const ParticleMeshEntry& entry = meshEntries[meshIndex];

            Mat4 entryWorld = particleWorld * entry.localMatrix;
            stats->comEffect->RenderGeoByIndex(entry.geoIndex, entryWorld, renderFlags | 0x80000u);
            continue;
        }

        if ((stats->flags & 0x4000u) != 0u && stats->comEffect) {

            if (!stats->fastRenderInfo) {
                stats->fastRenderInfo = static_cast<void*>(stats->comEffect->GetGeoSwapWordSlot(0));
            }

            if (stats->fastRenderInfo) {
                u32* swapWord = static_cast<u32*>(stats->fastRenderInfo);
                const bool queuedFast = ((renderFlags & 0x800000u) != 0u) && stats->comEffect->FastRenderReady();
                if (!queuedFast) {
                    stats->comEffect->SetFrame(info->animFrame);
                }
                const u32 previousWord = *swapWord;
                *swapWord = stats->fastRenderValue;

                stats->comEffect->Render(particleWorld, renderFlags);

                *swapWord = previousWord;
                continue;
            }
        }

        if ((stats->flags & 0x8000u) != 0u && meshFrameList) {
            const u32 geoIndex = meshFrameList[info->meshIndex & meshFrameMask];
            stats->comEffect->RenderGeoByIndex(geoIndex, particleWorld, renderFlags | 0x80000u);
            continue;
        }

        stats->comEffect->SetFrame(info->animFrame);
        stats->comEffect->Render(particleWorld, renderFlags);
    }

    if ((stats->flags & 0x10000u) != 0u) {
        stats->comEffect->DoFastRender();
    }
}

s32 ParticleSystem_LoadChunk(const u8* body, u32 bodySize) {
    MARKFUNCTION(0x80095494);

    if ((g_particleCount + g_commonParticleCount) >= kMaxParticleSystems) {
        return 1;
    }

    ParticleSystem* system = new ParticleSystem();
    if (!system) {
        return 0;
    }

    if (g_commonParticlesPending > 0) {
        g_commonParticleArray[g_commonParticleCount++] = system;
        g_commonParticlesPending--;
    }
    else {
        g_particleArray[g_particleCount++] = system;
    }

    system->activeFlag = 1;
    system->ParseData(body, bodySize);

    if (system->stats) {
        const char* particleName = system->GetName();
        const u32 effectHash = system->stats->comEffect ? system->stats->comEffect->resourceHash : 0;
        const u32 animHash = system->stats->comEffect ? system->stats->comEffect->miscAnimHash : 0;
        LOG("[Particle] load hash=0x%08X name=%s flags=0x%08X effect=0x%08X anim=0x%08X life=(%u,%u) spawn=%u maxActive=%u maxParticles=%u scale=(%d,%d) dir=(%d,%d) fast=0x%08X",
            system->nameCRC,
            particleName ? particleName : "",
            system->stats->flags,
            effectHash,
            animHash,
            system->stats->lifeBase,
            system->stats->lifeRange,
            system->stats->spawnPerBurst,
            system->stats->maxActive,
            system->stats->maxParticles,
            system->stats->scaleBase,
            system->stats->scaleRange,
            system->stats->dirX,
            system->stats->dirY,
            system->stats->fastRenderValue);
    }

    if (system->stats && (system->stats->flags & 0x4000u) != 0u && system->stats->comEffect) {
        system->stats->fastRenderInfo = static_cast<void*>(system->stats->comEffect->GetGeoSwapWordSlot(0));
    }

    if (system->stats && (system->stats->flags & 0x10u) != 0u) {
        const s32 meshCount = system->AnalyzeMesh();
        system->stats->spawnPerBurst = static_cast<u16>(meshCount);
        system->stats->maxParticles = static_cast<u16>(meshCount);
        system->stats->maxActive = 0;
    }
    else if (system->stats && (system->stats->flags & 0x8000u) != 0u) {
        system->AnalyzeMesh();
    }

    return 1;
}

s32 ParticleSystem_InitParticleInfoMemory() {
    MARKFUNCTION(0x80095610);

    if (g_particleInfoMemory) {
        return 1;
    }

    g_particleAvailList.head = nullptr;
    g_particleAvailList.tail = nullptr;

    g_particleInfoMemory = new ParticleInfo[kParticleInfoCount];
    if (!g_particleInfoMemory) {
        return 0;
    }

    for (s32 i = 0; i < kParticleInfoCount; i++) {
        g_particleAvailList.AddNodeTail(&g_particleInfoMemory[i]);
    }

    return 1;
}

void ParticleSystem_UnloadLevel() {
    MARKFUNCTION(0x80095740);

    for (s32 i = 0; i < g_particleCount; i++) {
        delete g_particleArray[i];
        g_particleArray[i] = nullptr;
    }

    g_particleCount = 0;

    for (s32 i = 0; i < g_commonParticleCount; i++) {
        if (g_commonParticleArray[i]) {
            g_commonParticleArray[i]->PurgeParticles();
        }
    }

    g_particleAvailList.head = nullptr;
    g_particleAvailList.tail = nullptr;

    delete[] g_particleInfoMemory;
    g_particleInfoMemory = nullptr;
}

void ParticleSystem_Unload() {
    MARKFUNCTION(0x800956D0);

    ParticleSystem_UnloadLevel();

    for (s32 i = 0; i < g_commonParticleCount; i++) {
        delete g_commonParticleArray[i];
        g_commonParticleArray[i] = nullptr;
    }

    g_commonParticleCount = 0;
    g_commonParticlesPending = 0;
}

void ParticleSystem_CommonParticles(s32 count) {
    MARKFUNCTION(0x80095854);
    g_commonParticlesPending = count;
}

ParticleSystem* ParticleSystem_Find(u32 hash) {
    MARKFUNCTION(0x80096318);

    for (s32 i = 0; i < g_commonParticleCount; i++) {
        ParticleSystem* system = g_commonParticleArray[i];
        if (system && system->nameCRC == hash) {
            return system;
        }
    }

    for (s32 i = 0; i < g_particleCount; i++) {
        ParticleSystem* system = g_particleArray[i];
        if (system && system->nameCRC == hash) {
            return system;
        }
    }

    return nullptr;
}

const char* ParticleSystem_GetNameByHash(u32 hash) {
    ParticleSystem* system = ParticleSystem_Find(hash);
    if (!system) {
        return nullptr;
    }

    const char* name = system->GetName();
    if (!name || name[0] == '\0') {
        return nullptr;
    }

    return name;
}

ParticleSystemMgr::ParticleSystemMgr() {
    MARKFUNCTION(0x80097D60);
    InitMgr(nullptr);
}

ParticleSystemMgr::ParticleSystemMgr(ParticleSystem* inSystem) {
    MARKFUNCTION(0x80097D1C);
    InitMgr(inSystem);
}

ParticleSystemMgr::~ParticleSystemMgr() {
    MARKFUNCTION(0x80097DA4);

    // Manager-local particle storage dies with this object; make sure
    // the shared system no longer points at this list before teardown.
    PurgeParticles();
    if (system) {
        system->SetDisplayOffset(nullptr);
        system->BindParticleList(nullptr);
    }
}

void ParticleSystemMgr::BindSystemList() {
    if (system) {
        system->BindParticleList(&particles);
    }
}

void ParticleSystemMgr::InitMgr(ParticleSystem* inSystem) {
    MARKFUNCTION(0x80097E00);

    system = inSystem;

    direction.x = 0;
    direction.y = 0;
    direction.z = 0;

    particles.head = nullptr;
    particles.tail = nullptr;

    BindSystemList();
}

s32 ParticleSystemMgr::CreateParticles(const LVector& origin, ParticleStats* statsOverride) {
    MARKFUNCTION(0x80097E1C);

    if (!system) {
        return 0;
    }

    BindSystemList();

    if (direction.x != 0 || direction.y != 0 || direction.z != 0) {
        system->SetParticleDirection(&direction);
    }
    else {
        system->ResetParticleDirection();
    }

    const s32 created = system->CreateParticles(origin, statsOverride);
    system->ResetParticleDirection();
    return created;
}

s32 ParticleSystemMgr::SetParticleDirection(const LVector* inDirection) {
    MARKFUNCTION(0x80097ECC);

    if (!system || !inDirection) {
        return 0;
    }

    BindSystemList();

    direction = *inDirection;
    system->SetParticleDirection(inDirection);
    return direction.z;
}

s32 ParticleSystemMgr::ResetParticleDirection() {
    MARKFUNCTION(0x80097F30);

    if (!system) {
        return 0;
    }

    BindSystemList();
    return system->ResetParticleDirection();
}

void ParticleSystemMgr::SetDisplayOffset(const LVector* offset) {
    if (!system) {
        return;
    }

    BindSystemList();
    system->SetDisplayOffset(const_cast<LVector*>(offset));
}

s32 ParticleSystemMgr::Update() {
    MARKFUNCTION(0x80097F60);

    if (!system) {
        return 0;
    }

    BindSystemList();
    return system->Update();
}

void ParticleSystemMgr::Display() {
    MARKFUNCTION(0x80097F90);

    if (!system) {
        return;
    }

    BindSystemList();
    system->Display();
}

s32 ParticleSystemMgr::ActiveParticles() {
    MARKFUNCTION(0x80097FC0);

    if (!system) {
        return 0;
    }

    BindSystemList();
    return system->ActiveParticles();
}

s32 ParticleSystemMgr::PurgeParticles() {
    MARKFUNCTION(0x80097FF0);

    if (!system) {
        return 0;
    }

    BindSystemList();
    return system->PurgeParticles();
}

u32 ParticleSystemMgr::GetSystemHash() const {
    if (!system) {
        return 0;
    }

    return system->nameCRC;
}
