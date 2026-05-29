#include "common.h"

#include "gen/particle.h"

#include "gen/camera.h"
#include "gen/model.h"
#include "gen/psxmath_helpers.h"
#include "gen/time.h"
#include "gen/weffect.h"

#include "p3d/context.h"
#include "p3d/byteread.h"
#include "p3d/p3dmath.h"

#include <cmath>
#include <cstring>

static const s32 kMaxParticleSystems = 64;
static const s32 kParticleInfoCount = 140;
static constexpr bool kDebugFreezeParticleRenderAtSpawn = false;
static constexpr bool kDebugTraceParticleTransforms = false;

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
    void* fastRenderWord1Slot = nullptr;
    u32 fastRenderWord1Value = 0;
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

static u32 RandomBits17() {
    u32& seed = rmRandomSeedRef();
    const u32 bits = seed & 0x1FFFFu;
    seed = rmAdvanceRandomSeed(seed);
    return bits;
}

static const s32 kOneOverDividePsxRaw[] = {
    65536, 65536, 32768, 21845, 16384, 13107, 10922, 9362, 8192, 7281,
    6553, 5957, 5461, 5041, 4681, 4369, 4096, 3855, 3640, 3449,
    3276, 3120, 2978, 2849, 2730, 2621, 2520, 2427, 2340, 2259,
    2184, 2114, 2048, 1985, 1927, 1872, 1820, 1771, 1724, 1680,
    1638, 1598, 1560, 1524, 1489, 1456, 1424, 1394, 1365, 1337,
    1310, 1285, 1260, 1236, 1213, 1191, 1170, 1149, 1129, 1110,
    1092,

    // Immediate data following gOneODivide in PSX image:
    // NormalTable[1] = {0}; dword_800D9990[2] = {61440, 0}; thePolyDC[12] = {0...}
    0,
    61440, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static s32 RandomSigned16() {
    return static_cast<s32>(RandomBits17()) - 0xFFFF;
}

static s32 MulLo32(s32 lhs, s32 rhs) {
    return static_cast<s32>(static_cast<s64>(lhs) * static_cast<s64>(rhs));
}

static s32 MulHi16FromLo32(s32 lhs, s32 rhs) {
    return MulLo32(lhs, rhs) >> 16;
}

static u8 MulByte2FromLo32(s32 lhs, s32 rhs) {
    return static_cast<u8>(static_cast<u32>(MulLo32(lhs, rhs)) >> 16);
}

static bool RandomSignFromSeedByte() {
    u32& seed = rmRandomSeedRef();
    const u8 lowByte = static_cast<u8>(seed);
    seed = rmAdvanceRandomSeed(seed);
    return (((static_cast<u32>(lowByte) + 1u) & 1u) != 0u);
}

static s32 Reciprocal16(s32 value) {
    if (value == 0) {
        return 0;
    }

    if (value < 0) {
        value = -value;
    }

    const u32 index = static_cast<u32>(value);
    const u32 rawCount = static_cast<u32>(sizeof(kOneOverDividePsxRaw) / sizeof(kOneOverDividePsxRaw[0]));
    if (index < rawCount) {
        return kOneOverDividePsxRaw[index];
    }

    // PSX reads past gOneODivide into subsequent globals with no bounds check.
    // Returning zero keeps deterministic behavior for farther-out indices.
    return 0;
}

static f32 QuantizePsxScale16(s32 scale16) {
    const s16 matrixEntry = static_cast<s16>(scale16 >> 4);
    return static_cast<f32>(matrixEntry) / 4096.0f;
}

static s32 AbsS32(s32 value) {
    return (value < 0) ? -value : value;
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
    const f32 s = QuantizePsxScale16(scale);
    out.m[0] = s;
    out.m[5] = s;
    out.m[10] = s;
}

static s32 QuantizeParticleAngle(s32 rotation) {
    const u16 angle = static_cast<u16>(rotation >> 8);
    const u16 quantized = static_cast<u16>((static_cast<u32>(angle) + 2u) >> 2);
    return static_cast<s32>(quantized);
}

static s16 ExpandPsxParticleSinCosAngle(s32 quantizedAngle) {
    // PSX Display path calls P3D_SinCos_GTE(((u16)(rotation >> 8) + 2) >> 2).
    // P3D_SinCos_GTE consumes quarter-angle units; rmSin16 uses full 0x10000 turn.
    return static_cast<s16>(quantizedAngle << 2);
}

static void PreMultiplyRotZ(Mat4& matrix, s32 angle16) {
    const s16 sinCosAngle = ExpandPsxParticleSinCosAngle(angle16);
    const f32 sinV = FIX16_TO_FLOAT(rmSin16(sinCosAngle));
    const f32 cosV = FIX16_TO_FLOAT(rmSin16(static_cast<s16>(sinCosAngle + 0x4000)));

    Mat4 rot = Mat4();
    rot.m[0] = cosV;
    rot.m[1] = sinV;
    rot.m[4] = -sinV;
    rot.m[5] = cosV;

    matrix = rot * matrix;
}

static void PreMultiplyRotY(Mat4& matrix, s32 angle16) {
    const s16 sinCosAngle = ExpandPsxParticleSinCosAngle(angle16);
    const f32 sinV = FIX16_TO_FLOAT(rmSin16(sinCosAngle));
    const f32 cosV = FIX16_TO_FLOAT(rmSin16(static_cast<s16>(sinCosAngle + 0x4000)));

    Mat4 rot = Mat4();
    rot.m[0] = cosV;
    rot.m[2] = -sinV;
    rot.m[8] = sinV;
    rot.m[10] = cosV;

    matrix = rot * matrix;
}

static void PreMultiplyRotX(Mat4& matrix, s32 angle16) {
    const s16 sinCosAngle = ExpandPsxParticleSinCosAngle(angle16);
    const f32 sinV = FIX16_TO_FLOAT(rmSin16(sinCosAngle));
    const f32 cosV = FIX16_TO_FLOAT(rmSin16(static_cast<s16>(sinCosAngle + 0x4000)));

    Mat4 rot = Mat4();
    rot.m[5] = cosV;
    rot.m[6] = sinV;
    rot.m[9] = -sinV;
    rot.m[10] = cosV;

    matrix = rot * matrix;
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

    auto readLongStream = [&]() -> s32 {
        if (cursor + 4 > bodySize) {
            cursor = bodySize;
            return 0;
        }

        const s32 value = ReadS32Particle(body + cursor);
        cursor += 4;
        return value;
    };

    auto readCharStream = [&]() -> char {
        if (cursor >= bodySize) {
            return '\0';
        }

        const char c = static_cast<char>(body[cursor]);
        cursor++;
        return c;
    };

    while (cursor + 4 <= bodySize) {
        const u16 tag = ReadU16Particle(body + cursor);
        const u16 length = ReadU16Particle(body + cursor + 2);
        cursor += 4;

        if (tag == 0x0100) {
            stats->speedBase = readLongStream();
            stats->speedRange = readLongStream();
            (void)readLongStream();
            (void)readLongStream();
        }
        else if (tag == 0x0200) {
            const s32 v0 = readLongStream() << 16;
            const s32 v1 = readLongStream() << 16;
            const s32 v2 = readLongStream() << 16;
            const s32 v3 = readLongStream() << 16;

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
            stats->lifeBase = static_cast<u16>(readLongStream());
            stats->lifeRange = static_cast<u16>(readLongStream());
            (void)readLongStream();
            (void)readLongStream();
        }
        else if (tag == 0x0400) {
            stats->dragBase = readLongStream();
            stats->dragRange = readLongStream();
            (void)readLongStream();
            (void)readLongStream();
        }
        else if (tag == 0x0500) {
            const s32 v0 = readLongStream() << 16;
            const s32 v1 = readLongStream() << 16;
            const s32 v2 = readLongStream() << 16;
            const s32 v3 = readLongStream() << 16;

            if (v2 > 0) {
                stats->accelBase = rmDiv16i(v0, v2);
                stats->accelRange = rmDiv16i(v1, v3);
                flags |= 4u;
            }
        }
        else if (tag == 0x0510) {
            stats->spawnPerBurst = static_cast<u16>(readLongStream());
            (void)readLongStream();
            stats->maxParticles = static_cast<u16>(readLongStream());
            (void)readLongStream();
        }
        else if (tag == 0x0600) {
            stats->dirX = readLongStream();
            stats->dirY = readLongStream();
            stats->dirXDefault = stats->dirX;
            stats->dirYDefault = stats->dirY;
        }
        else if (tag == 0x0610) {
            stats->spreadX = readLongStream();
            stats->spreadY = readLongStream();
        }
        else if (tag == 0x0700) {
            stats->growFrames = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0710) {
            stats->shrinkFrames = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0715) {
            stats->normalizeFrames = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0720) {
            stats->gravity = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0730) {
            stats->particleLife = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0740) {
            stats->burstStart = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0750) {
            stats->burstEnd = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0760) {
            stats->animEndFrame = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0770) {
            stats->animDelay = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0780) {
            stats->maxActive = static_cast<u16>(readLongStream());
        }
        else if (tag == 0x0800) {
            if (readLongStream() != 0) {
                flags |= 0x10u;
            }
        }
        else if (tag == 0x0810) {
            if (readLongStream() != 0) {
                flags |= 0x20u;
            }
        }
        else if (tag == 0x0820) {
            if (readLongStream() != 0) {
                flags |= 8u;
            }
        }
        else if (tag == 0x0830) {
            if (readLongStream() != 0) {
                flags |= 0x8000u;
            }
        }
        else if (tag == 0x0840) {
            if (readLongStream() != 0) {
                flags |= 2u;
            }
        }
        else if (tag == 0x0850) {
            if (readLongStream() != 0) {
                flags |= 0x1000u;
            }
        }
        else if (tag == 0x0860) {
            if (readLongStream() != 0) {
                flags |= 0x4000u;
            }
        }
        else if (tag == 0x0870) {
            if (readLongStream() != 0) {
                flags |= 0x2000u;
            }
        }
        else if (tag == 0x0880) {
            if (readLongStream() != 0) {
                flags |= 0x10000u;
            }
        }
        else if (tag == 0x0900) {
            stats->fastRenderWord1Value = static_cast<u32>(readLongStream());
        }
        else if (tag == 0x1000) {
            const s32 resourceHash = readLongStream();
            const s32 animHash = readLongStream();

            if (!stats->comEffect) {
                stats->comEffect = new ComEffect();
            }

            if (stats->comEffect) {
                stats->comEffect->LoadETree(resourceHash, animHash);
            }

            if (cursor + 32 > bodySize) {
                cursor = bodySize;
            }
            else {
                cursor += 32;
            }
        }
        else if (tag == 0x1100) {
            const u32 hash = static_cast<u32>(readLongStream());

            char nameBuffer[32] = {};
            for (s32 i = 0; i < 32; i++) {
                nameBuffer[i] = readCharStream();
            }
            nameBuffer[31] = '\0';
            SetName(nameBuffer, 0);

            nameCRC = hash;
        }

        else {
            u32 skip = static_cast<u32>(length);
            if (cursor + skip > bodySize) {
                skip = bodySize - cursor;
            }
            cursor += skip;
        }
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

        const s16 lifeRangeSigned = static_cast<s16>(stats->lifeRange);
        const u8 lifeBaseByte = static_cast<u8>(stats->lifeBase);
        const u8 lifeRand = MulByte2FromLo32(RandomSigned16(), static_cast<s32>(lifeRangeSigned));
        info->life = static_cast<u8>(lifeBaseByte + lifeRand);

        info->posX = static_cast<s16>(origin.x);
        info->posY = static_cast<s16>(origin.y);
        info->posZ = static_cast<s16>(origin.z);

        if ((stats->flags & 0x10u) != 0u && meshCounter < static_cast<s32>(stats->spawnPerBurst)) {
            info->meshIndex = static_cast<u16>(meshCounter++);
        }
        else {
            info->meshIndex = 0;
        }

        const s16 azimuth = static_cast<s16>(stats->dirX + MulHi16FromLo32(RandomSigned16(), stats->spreadX));
        const s16 polar = static_cast<s16>(stats->dirY + MulHi16FromLo32(RandomSigned16(), stats->spreadY));

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

        const s32 rotationRand = RandomSigned16();
        const s32 rotationRandom = static_cast<s32>((static_cast<s64>(stats->dragRange) * rotationRand) >> 16);
        s32 rotationStep = (rotationRandom + stats->dragBase) << 8;

        if ((stats->flags & 0x2000u) != 0u && info->life != 0) {
            const s32 invLife = Reciprocal16(static_cast<s32>(info->life));
            rotationStep = static_cast<s32>((static_cast<s64>(rotationStep) * invLife) >> 16);
        }

        if (!RandomSignFromSeedByte()) {
            rotationStep = -rotationStep;
        }

        info->rotationStep = rotationStep;

        if ((stats->flags & 0x1000u) != 0u) {
            const s64 rotationMul = static_cast<s64>(RandomSigned16()) * 0xFFFF;
            const s32 rotationHigh = static_cast<s32>(rotationMul >> 16);
            info->rotation = static_cast<s32>(rotationHigh << 8);
        }
        else {
            info->rotation = 0;
        }

        const u32 axisRandRaw = static_cast<u32>(RandomSigned16() + 0xFFFF);
        info->axisMask = axisPattern[(axisRandRaw + 1u) & 3u];

        const s32 scaleRandom = MulHi16FromLo32(stats->scaleRange, RandomSigned16());
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
            const s32 dragRand = MulHi16FromLo32(stats->accelRange, RandomSigned16());
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

    const s16 particleLife = static_cast<s16>(stats->particleLife);
    const s16 animEndFrameSigned = static_cast<s16>(stats->animEndFrame);
    const s16 animDelaySigned = static_cast<s16>(stats->animDelay);
    const s16 normalizeFramesSigned = static_cast<s16>(stats->normalizeFrames);
    const s16 normalizeFramesAbs = (normalizeFramesSigned < 0) ? static_cast<s16>(-normalizeFramesSigned) : normalizeFramesSigned;
    const s16 growFramesSigned = static_cast<s16>(stats->growFrames);
    const s16 shrinkFramesSigned = static_cast<s16>(stats->shrinkFrames);
    const s16 gravitySigned = static_cast<s16>(stats->gravity);
    const u16 frameRate = 30;
    const bool lowFrameRate = frameRate < 0x1Au;

    for (ccMinNode* node = particleList->head; node;) {
        ParticleInfo* info = static_cast<ParticleInfo*>(node);
        node = node->next;
        hadAnyNode = true;

        if (particleLife < static_cast<s16>(info->frameAge)) {
            const u8 prevHold = info->animHoldCounter;
            info->animHoldCounter = static_cast<u8>(info->animHoldCounter + 1);
            if (static_cast<s16>(prevHold) >= animDelaySigned) {
                info->animHoldCounter = 0;
                info->animFrame = static_cast<u8>(info->animFrame + 1);
                if (stats->comEffect && stats->comEffect->EndOfFrame(info->animFrame)) {
                    info->animFrame = 0;
                }
            }
        }

        if (lowFrameRate && info->life < 9u) {
            info->life = 0;
        }

        if (info->life == 0) {
            info->active = 0;
            particleList->RemNode(info);
            g_particleAvailList.AddNodeTail(info);
            continue;
        }

        info->frameAge = static_cast<u8>(info->frameAge + 1);

        if (particleLife < static_cast<s16>(info->frameAge)) {
            const u8 prevCounter = info->animFrameCounter;
            info->animFrameCounter = static_cast<u8>(info->animFrameCounter + 2);

            if (static_cast<s16>(static_cast<u8>(prevCounter + 1)) >= animEndFrameSigned) {
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

                if ((stats->flags & 0x80000u) != 0u && static_cast<s16>(info->age) < normalizeFramesAbs) {
                    info->velX = static_cast<s16>(info->velX + info->velNormX);
                    info->velY = static_cast<s16>(info->velY + info->velNormY);
                    info->velZ = static_cast<s16>(info->velZ + info->velNormZ);
                }

                if ((stats->flags & 4u) != 0u) {
                    info->velX = static_cast<s16>((static_cast<s64>(info->velX) * info->dragScale) >> 16);
                    info->velY = static_cast<s16>((static_cast<s64>(info->velY) * info->dragScale) >> 16);
                    info->velZ = static_cast<s16>((static_cast<s64>(info->velZ) * info->dragScale) >> 16);
                }

                if ((stats->flags & 0x40000u) != 0u && static_cast<s16>(info->age) < growFramesSigned) {
                    info->scale += info->scaleStep;
                }

                if ((stats->flags & 0x100000u) != 0u && static_cast<s16>(info->life) < shrinkFramesSigned) {
                    info->scale -= info->scaleStep2;
                }

                info->rotation += info->rotationStep;

                if ((stats->flags & 2u) != 0u) {
                    info->velY = static_cast<s16>(info->velY + ((-gravitySigned) >> 1));
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

    const LVector& cullPos = kDebugFreezeParticleRenderAtSpawn ? basePos : averagePos;
    if (!stats->comEffect->PointInView(cullPos, 512)) {
        return;
    }

    u32 renderFlags = ((stats->flags & 0x800u) != 0u) ? 0x800u : 0u;
    if ((stats->flags & 2u) != 0u) {
        renderFlags |= 0x1000000u;
    }
    if ((stats->flags & 0x10000u) != 0u) {
        renderFlags |= 0x800000u;
        stats->comEffect->InitFastRender(stats->comEffect->GetGeo());
    }

    const bool billboard = (stats->flags & 8u) != 0u;
    Mat4 billboardMatrix = Mat4();
    if (billboard && particleList->head) {
        const Mat4& parentWorld = p3d::context->GetWorldMatrix();
        LVector billboardPos = kDebugFreezeParticleRenderAtSpawn ? basePos : averagePos;
        billboardPos.x += static_cast<s32>(parentWorld.m[12]);
        billboardPos.y += static_cast<s32>(parentWorld.m[13]);
        billboardPos.z += static_cast<s32>(parentWorld.m[14]);
        MakeBillboardMatrix(billboardPos, billboardMatrix, 0);
    }

    if (kDebugTraceParticleTransforms) {
        static s32 sTraceBudget = 200;
        static s32 sTraceDecimator = 0;

        if (sTraceBudget > 0 && ((sTraceDecimator++ & 0x1F) == 0)) {
            ParticleInfo* sample = nullptr;
            for (ccMinNode* node = particleList->head; node; node = node->next) {
                ParticleInfo* info = static_cast<ParticleInfo*>(node);
                if (info->active) {
                    sample = info;
                    break;
                }
            }

            if (sample) {
                const Mat4& parentWorld = p3d::context->GetWorldMatrix();
                const s32 quantized = QuantizeParticleAngle(sample->rotation);
                LOG("[ParticleDbg] hash=0x%08X flags=0x%08X freeze=%d base=(%d,%d,%d) avg=(%d,%d,%d) parentT=(%d,%d,%d) p=(%d,%d,%d) v=(%d,%d,%d) scale=%d scaleStep=%d rotStep=%d rot=%d qrot=%d axis=0x%02X frame=%u",
                    nameCRC,
                    stats->flags,
                    kDebugFreezeParticleRenderAtSpawn ? 1 : 0,
                    basePos.x,
                    basePos.y,
                    basePos.z,
                    averagePos.x,
                    averagePos.y,
                    averagePos.z,
                    static_cast<s32>(parentWorld.m[12]),
                    static_cast<s32>(parentWorld.m[13]),
                    static_cast<s32>(parentWorld.m[14]),
                    static_cast<s32>(sample->posX),
                    static_cast<s32>(sample->posY),
                    static_cast<s32>(sample->posZ),
                    static_cast<s32>(sample->velX),
                    static_cast<s32>(sample->velY),
                    static_cast<s32>(sample->velZ),
                    sample->scale,
                    sample->scaleStep,
                    sample->rotationStep,
                    sample->rotation,
                    quantized,
                    static_cast<u32>(sample->axisMask),
                    static_cast<u32>(sample->animFrame));

                const s32 maxAbsPos = PsxMax(AbsS32(static_cast<s32>(sample->posX)),
                    PsxMax(AbsS32(static_cast<s32>(sample->posY)), AbsS32(static_cast<s32>(sample->posZ))));
                if (maxAbsPos > 30000 || AbsS32(sample->scale) > 0x80000 || AbsS32(sample->rotationStep) > 0x200000) {
                    LOG("[ParticleDbg] anomaly hash=0x%08X maxAbsPos=%d scale=%d rotStep=%d",
                        nameCRC,
                        maxAbsPos,
                        sample->scale,
                        sample->rotationStep);
                }

                sTraceBudget--;
            }
        }
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
        if (kDebugFreezeParticleRenderAtSpawn) {
            renderPos = basePos;
        }

        if (displayOffset) {
            renderPos.x += displayOffset->x;
            renderPos.y += displayOffset->y;
            renderPos.z += displayOffset->z;
        }

        Mat4 world;
        BuildParticleScaleMatrix(kDebugFreezeParticleRenderAtSpawn ? 0x10000 : info->scale, world);

        const s32 angle16 = kDebugFreezeParticleRenderAtSpawn ? 0 : QuantizeParticleAngle(info->rotation);
        const u8 animFrame = kDebugFreezeParticleRenderAtSpawn ? 0 : info->animFrame;
        if (billboard) {
            if (angle16 != 0) {
                PreMultiplyRotZ(world, angle16);
            }

            // PSX composes particle local scale/rotation matrix with the shared
            // billboard basis before translation is filled per particle.
            world = world * billboardMatrix;
        }
        else if (angle16 != 0) {
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
            if (stats->fastRenderWord1Slot) {
                u32* word1 = static_cast<u32*>(stats->fastRenderWord1Slot);
                stats->comEffect->SetFrame(animFrame);
                const u32 previousWord = *word1;
                *word1 = stats->fastRenderWord1Value;

                stats->comEffect->Render(particleWorld, renderFlags);

                *word1 = previousWord;
                continue;
            }
        }

        if ((stats->flags & 0x8000u) != 0u && meshFrameList) {
            const u32 geoIndex = meshFrameList[info->meshIndex & meshFrameMask];
            stats->comEffect->RenderGeoByIndex(geoIndex, particleWorld, renderFlags | 0x80000u);
            continue;
        }

        stats->comEffect->SetFrame(animFrame);
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

    if (system->stats && (system->stats->flags & 0x4000u) != 0u && system->stats->comEffect) {
        system->stats->fastRenderWord1Slot = static_cast<void*>(system->stats->comEffect->GetGeoFastWord1Slot(0));
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
