#include "gen/geometry.h"
#include "gen/display.h"
#include "gen/lights.h"
#include "gen/skeleton.h"
#include "p3d/byteread.h"
#include "p3d/context.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "pc/log.h"
#include <algorithm>
#include <cstring>
#include <tuple>
#include <vector>

// BLK layout:
//   [0-23]   Block header (translation at +8: s16 x,y,z)
//   [24+]    tPrimGeom struct, also the base for word offsets
//
// tPrimGeom fields (all word offsets relative to pg = BLK+24):
//   +0x10  vertList    (u32 word offset -> SVECTOR array)
//   +0x14  numVerts    (u16)
//   +0x16  numPolys    (u16)
//   +0x40  primList    (u32 word offset -> GPU packets)
//   +0x54  polyData    (u32 word offset -> u8[4] vertex indices per face)
//   +0x60  loopVerts   (u32 word offset -> per-loop u32, lo16 = vertex count)
//   +0x66  numLoops    (s16)
//   +0x68  loopPrims   (u32 word offset -> per-loop 4x u16 primitive counts)
//
// PSX evidence:
// - RP_ZCullGClip reads +0x60 as 4-byte loop entries and only consumes lo16
//   as the loop's vertex count.
// - RP_FixUpPolys reads +0x68 as 8-byte loop entries containing four u16
//   primitive-class counts and sums those to walk polyData/primList.
//
// Standard PSX GPU primitive packet layouts (byte offsets from packet start):
//
// POLY_GT4 (cmd 0x3C, wordCount=12, 52 bytes):
//   +0  OT tag          +4  [R0,G0,B0,CMD]     +8  [X0,Y0]
//   +12 [U0,V0,CBA]     +16 [R1,G1,B1,pad]     +20 [X1,Y1]
//   +24 [U1,V1,TPAGE]   +28 [R2,G2,B2,pad]     +32 [X2,Y2]
//   +36 [U2,V2,pad]     +40 [R3,G3,B3,pad]     +44 [X3,Y3]
//   +48 [U3,V3,pad]
//
// POLY_GT3 (cmd 0x34, wordCount=9, 40 bytes):
//   +0  OT tag          +4  [R0,G0,B0,CMD]     +8  [X0,Y0]
//   +12 [U0,V0,CBA]     +16 [R1,G1,B1,pad]     +20 [X1,Y1]
//   +24 [U1,V1,TPAGE]   +28 [R2,G2,B2,pad]     +32 [X2,Y2]
//   +36 [U2,V2,pad]
//
// POLY_G4 (cmd 0x38, wordCount=8, 36 bytes):
//   +0  OT tag     +4  [R0,G0,B0,CMD]  +8  [X0,Y0]
//   +12 [R1,G1,B1] +16 [X1,Y1]         +20 [R2,G2,B2]
//   +24 [X2,Y2]    +28 [R3,G3,B3]      +32 [X3,Y3]
//
// POLY_G3 (cmd 0x30, wordCount=6, 28 bytes):
//   +0  OT tag     +4  [R0,G0,B0,CMD]  +8  [X0,Y0]
//   +12 [R1,G1,B1] +16 [X1,Y1]         +20 [R2,G2,B2]
//   +24 [X2,Y2]

struct RPStreeNormal {
    s16 x;
    s16 y;
    s16 z;
};

static const RPStreeNormal s_rpStreeNormals[256] = {
    {      0,      0,  -4096 },
    {      0,      0,   4096 },
    {      0,  -4096,      0 },
    {      0,   4096,      0 },
    {  -4096,      0,      0 },
    {   4096,      0,      0 },
    {   3664,      0,   1832 },
    {   1132,   3484,   1832 },
    {  -2964,   2153,   1832 },
    {  -2964,  -2153,   1832 },
    {   1132,  -3484,   1832 },
    {   2964,   2153,  -1832 },
    {  -1132,   3484,  -1832 },
    {  -3664,      0,  -1832 },
    {  -1132,  -3484,  -1832 },
    {   2964,  -2153,  -1832 },
    {    900,      0,   3996 },
    {   1755,      0,   3701 },
    {   2525,      0,   3225 },
    {   3172,      0,   2592 },
    {    278,    856,   3996 },
    {    542,   1669,   3701 },
    {    780,   2402,   3225 },
    {    980,   3017,   2592 },
    {   -728,    529,   3996 },
    {  -1420,   1032,   3701 },
    {  -2043,   1484,   3225 },
    {  -2566,   1864,   2592 },
    {   -728,   -529,   3996 },
    {  -1420,  -1032,   3701 },
    {  -2043,  -1484,   3225 },
    {  -2566,  -1864,   2592 },
    {    278,   -856,   3996 },
    {    542,  -1669,   3701 },
    {    780,  -2402,   3225 },
    {    980,  -3017,   2592 },
    {   3450,    856,   2036 },
    {   3068,   1669,   2140 },
    {   2536,   2402,   2140 },
    {   1880,   3017,   2036 },
    {    252,   3545,   2036 },
    {   -640,   3433,   2140 },
    {  -1501,   3154,   2140 },
    {  -2288,   2720,   2036 },
    {  -3294,   1336,   2036 },
    {  -3463,    453,   2140 },
    {  -3463,   -453,   2140 },
    {  -3294,  -1336,   2036 },
    {  -2288,  -2720,   2036 },
    {  -1501,  -3154,   2140 },
    {   -640,  -3433,   2140 },
    {    252,  -3545,   2036 },
    {   1880,  -3017,   2036 },
    {   2536,  -2402,   2140 },
    {   3068,  -1669,   2140 },
    {   3450,   -856,   2036 },
    {   3900,    529,   1136 },
    {   3945,   1032,    385 },
    {   3798,   1484,   -385 },
    {   3466,   1864,  -1136 },
    {    702,   3872,   1136 },
    {    238,   4071,    385 },
    {   -238,   4071,   -385 },
    {   -702,   3872,  -1136 },
    {  -3466,   1864,   1136 },
    {  -3798,   1484,    385 },
    {  -3945,   1032,   -385 },
    {  -3900,    529,  -1136 },
    {  -2844,  -2720,   1136 },
    {  -2585,  -3154,    385 },
    {  -2200,  -3433,   -385 },
    {  -1708,  -3545,  -1136 },
    {   1708,  -3545,   1136 },
    {   2200,  -3433,    385 },
    {   2585,  -3154,   -385 },
    {   2844,  -2720,  -1136 },
    {   3900,   -529,   1136 },
    {   3945,  -1032,    385 },
    {   3798,  -1484,   -385 },
    {   3466,  -1864,  -1136 },
    {   1708,   3545,   1136 },
    {   2200,   3433,    385 },
    {   2585,   3154,   -385 },
    {   2844,   2720,  -1136 },
    {  -2844,   2720,   1136 },
    {  -2585,   3154,    385 },
    {  -2200,   3433,   -385 },
    {  -1708,   3545,  -1136 },
    {  -3466,  -1864,   1136 },
    {  -3798,  -1484,    385 },
    {  -3945,  -1032,   -385 },
    {  -3900,   -529,  -1136 },
    {    702,  -3872,   1136 },
    {    238,  -4071,    385 },
    {   -238,  -4071,   -385 },
    {   -702,  -3872,  -1136 },
    {   2288,   2720,  -2036 },
    {   1501,   3154,  -2140 },
    {    640,   3433,  -2140 },
    {   -252,   3545,  -2036 },
    {  -1880,   3017,  -2036 },
    {  -2536,   2402,  -2140 },
    {  -3068,   1669,  -2140 },
    {  -3450,    856,  -2036 },
    {  -3450,   -856,  -2036 },
    {  -3068,  -1669,  -2140 },
    {  -2536,  -2402,  -2140 },
    {  -1880,  -3017,  -2036 },
    {   -252,  -3545,  -2036 },
    {    640,  -3433,  -2140 },
    {   1501,  -3154,  -2140 },
    {   2288,  -2720,  -2036 },
    {   3294,  -1336,  -2036 },
    {   3463,   -453,  -2140 },
    {   3463,    453,  -2140 },
    {   3294,   1336,  -2036 },
    {    728,    529,  -3996 },
    {   1420,   1032,  -3701 },
    {   2043,   1484,  -3225 },
    {   2566,   1864,  -2592 },
    {   -278,    856,  -3996 },
    {   -542,   1669,  -3701 },
    {   -780,   2402,  -3225 },
    {   -980,   3017,  -2592 },
    {   -900,      0,  -3996 },
    {  -1755,      0,  -3701 },
    {  -2525,      0,  -3225 },
    {  -3172,      0,  -2592 },
    {   -278,   -856,  -3996 },
    {   -542,  -1669,  -3701 },
    {   -780,  -2402,  -3225 },
    {   -980,  -3017,  -2592 },
    {    728,   -529,  -3996 },
    {   1420,  -1032,  -3701 },
    {   2043,  -1484,  -3225 },
    {   2566,  -1864,  -2592 },
    {   1187,    862,   3824 },
    {   2057,    870,   3434 },
    {   1463,   1687,   3434 },
    {   2830,    871,   2830 },
    {   2331,   1694,   2911 },
    {   1703,   2423,   2830 },
    {   -453,   1396,   3824 },
    {   -192,   2225,   3434 },
    {  -1152,   1913,   3434 },
    {     46,   2961,   2830 },
    {   -891,   2741,   2911 },
    {  -1778,   2368,   2830 },
    {  -1467,      0,   3824 },
    {  -2175,    505,   3434 },
    {  -2175,   -505,   3434 },
    {  -2802,    959,   2830 },
    {  -2882,      0,   2911 },
    {  -2802,   -959,   2830 },
    {   -453,  -1396,   3824 },
    {  -1152,  -1913,   3434 },
    {   -192,  -2225,   3434 },
    {  -1778,  -2368,   2830 },
    {   -891,  -2741,   2911 },
    {     46,  -2961,   2830 },
    {   1187,   -862,   3824 },
    {   1463,  -1687,   3434 },
    {   2057,   -870,   3434 },
    {   1703,  -2423,   2830 },
    {   2331,  -1694,   2911 },
    {   2830,   -871,   2830 },
    {   4077,      0,    398 },
    {   4044,   -505,   -410 },
    {   4044,    505,   -410 },
    {   3784,   -959,  -1241 },
    {   3892,      0,  -1276 },
    {   3784,    959,  -1241 },
    {   1260,   3877,    398 },
    {   1730,   3690,   -410 },
    {    769,   4002,   -410 },
    {   2081,   3303,  -1241 },
    {   1203,   3702,  -1276 },
    {    257,   3895,  -1241 },
    {  -3298,   2396,    398 },
    {  -2975,   2786,   -410 },
    {  -3569,   1968,   -410 },
    {  -2498,   3000,  -1241 },
    {  -3149,   2288,  -1276 },
    {  -3625,   1449,  -1241 },
    {  -3298,  -2396,    398 },
    {  -3569,  -1968,   -410 },
    {  -2975,  -2786,   -410 },
    {  -3625,  -1449,  -1241 },
    {  -3149,  -2288,  -1276 },
    {  -2498,  -3000,  -1241 },
    {   1260,  -3877,    398 },
    {    769,  -4002,   -410 },
    {   1730,  -3690,   -410 },
    {    257,  -3895,  -1241 },
    {   1203,  -3702,  -1276 },
    {   2081,  -3303,  -1241 },
    {   3298,   2396,   -398 },
    {   2975,   2786,    410 },
    {   3569,   1968,    410 },
    {   2498,   3000,   1241 },
    {   3149,   2288,   1276 },
    {   3625,   1449,   1241 },
    {  -1260,   3877,   -398 },
    {  -1730,   3690,    410 },
    {   -769,   4002,    410 },
    {  -2081,   3303,   1241 },
    {  -1203,   3702,   1276 },
    {   -257,   3895,   1241 },
    {  -4077,      0,   -398 },
    {  -4044,   -505,    410 },
    {  -4044,    505,    410 },
    {  -3784,   -959,   1241 },
    {  -3892,      0,   1276 },
    {  -3784,    959,   1241 },
    {  -1260,  -3877,   -398 },
    {   -769,  -4002,    410 },
    {  -1730,  -3690,    410 },
    {   -257,  -3895,   1241 },
    {  -1203,  -3702,   1276 },
    {  -2081,  -3303,   1241 },
    {   3298,  -2396,   -398 },
    {   3569,  -1968,    410 },
    {   2975,  -2786,    410 },
    {   3625,  -1449,   1241 },
    {   3149,  -2288,   1276 },
    {   2498,  -3000,   1241 },
    {    453,   1396,  -3824 },
    {    192,   2225,  -3434 },
    {   1152,   1913,  -3434 },
    {    -46,   2961,  -2830 },
    {    891,   2741,  -2911 },
    {   1778,   2368,  -2830 },
    {  -1187,    862,  -3824 },
    {  -2057,    870,  -3434 },
    {  -1463,   1687,  -3434 },
    {  -2830,    871,  -2830 },
    {  -2331,   1694,  -2911 },
    {  -1703,   2423,  -2830 },
    {  -1187,   -862,  -3824 },
    {  -1463,  -1687,  -3434 },
    {  -2057,   -870,  -3434 },
    {  -1703,  -2423,  -2830 },
    {  -2331,  -1694,  -2911 },
    {  -2830,   -871,  -2830 },
    {    453,  -1396,  -3824 },
    {   1152,  -1913,  -3434 },
    {    192,  -2225,  -3434 },
    {   1778,  -2368,  -2830 },
    {    891,  -2741,  -2911 },
    {    -46,  -2961,  -2830 },
    {   1467,      0,  -3824 },
    {   2175,    505,  -3434 },
    {   2175,   -505,  -3434 },
    {   2802,    959,  -2830 },
    {   2882,      0,  -2911 },
    {   2802,   -959,  -2830 },
};

static u8 ClampLitChannelToByte(s32 value) {
    if (value <= 0) {
        return 0;
    }

    value = (value + 8) >> 4;
    if (value > 255) {
        value = 255;
    }
    return static_cast<u8>(value);
}

static s32 ComputeLightIntensity12(s32 normalX, s32 normalY, s32 normalZ, const HardwareLight* light) {
    if (!light) {
        return 0;
    }

    const s32 lightX = light->directionX >> 4;
    const s32 lightY = light->directionY >> 4;
    const s32 lightZ = light->directionZ >> 4;
    s64 dot = static_cast<s64>(normalX) * lightX;
    dot += static_cast<s64>(normalY) * lightY;
    dot += static_cast<s64>(normalZ) * lightZ;
    if (dot <= 0) {
        return 0;
    }

    dot >>= 12;
    if (dot > 4096) {
        dot = 4096;
    }
    return static_cast<s32>(dot);
}

static void ComputeRPStreeLitColour(u8 normalIndex, u8* outR, u8* outG, u8* outB) {
    const u32 ambient = GetCurrentPortAmbientLightColour();
    const RPStreeNormal& normal = s_rpStreeNormals[normalIndex];

    s32 accumR = static_cast<s32>(ambient & 0xFFu) << 4;
    s32 accumG = static_cast<s32>((ambient >> 8) & 0xFFu) << 4;
    s32 accumB = static_cast<s32>((ambient >> 16) & 0xFFu) << 4;

    for (s32 slot = 0; slot < 3; slot++) {
        const HardwareLight* light = GetCurrentPortHardwareLight(slot);
        const s32 intensity = ComputeLightIntensity12(normal.x, normal.y, normal.z, light);
        if (intensity <= 0 || !light) {
            continue;
        }

        accumR += ((static_cast<s32>(light->colour & 0xFFu) << 4) * intensity) >> 12;
        accumG += ((static_cast<s32>((light->colour >> 8) & 0xFFu) << 4) * intensity) >> 12;
        accumB += ((static_cast<s32>((light->colour >> 16) & 0xFFu) << 4) * intensity) >> 12;
    }

    if (outR) {
        *outR = ClampLitChannelToByte(accumR);
    }
    if (outG) {
        *outG = ClampLitChannelToByte(accumG);
    }
    if (outB) {
        *outB = ClampLitChannelToByte(accumB);
    }
}

const u8* tGeometry::GetVertexList() const {
    MARKFUNCTION(0x800A18A8);
    return vertexList;
}

void tGeometry::SetVertexList(const u8* verts) {
    MARKFUNCTION(0x800A18B4);
    vertexList = verts;
}

tPrimGeom::~tPrimGeom() {
    MARKFUNCTION(0x800A14C4);

    delete[] ownedRawData;
    ownedRawData = nullptr;
    ownedRawSize = 0;
    primList = nullptr;
    primListAlt = nullptr;
    primData48 = nullptr;
    primData4C = nullptr;
    gmFogWriteList = nullptr;
    polyData = nullptr;
    gmFogColourList = nullptr;
    primData5C = nullptr;
    loopVertData = nullptr;
    loopPrimData = nullptr;
}

tPrimGeom* tPrimGeom::Clone() const {
    MARKFUNCTION(0x800A1548);

    tPrimGeom* clone = new tPrimGeom();
    clone->geoType = geoType;
    clone->numLoops = numLoops;

    if (ownedRawData && ownedRawSize) {
        clone->ownedRawData = new u8[ownedRawSize];
        clone->ownedRawSize = ownedRawSize;
        memcpy(clone->ownedRawData, ownedRawData, ownedRawSize);

        auto remap = [&](const u8* sourcePtr) -> const u8* {
            if (!sourcePtr) return nullptr;
            u32 offset = static_cast<u32>(sourcePtr - ownedRawData);
            if (offset >= ownedRawSize) return nullptr;
            return clone->ownedRawData + offset;
        };

        clone->SetVertexList(remap(GetVertexList()));
        clone->primList = remap(primList);
        clone->primListAlt = remap(primListAlt);
        clone->primData48 = remap(primData48);
        clone->primData4C = remap(primData4C);
        clone->gmFogWriteList = remap(gmFogWriteList);
        clone->polyData = remap(polyData);
        clone->gmFogColourList = remap(gmFogColourList);
        clone->primData5C = remap(primData5C);
        clone->loopVertData = remap(loopVertData);
        clone->loopPrimData = remap(loopPrimData);
    }

    return clone;
}

int tPrimGeom::Display(const LVector* drawPos) {
    MARKFUNCTION(0x800A1860);
    return RP_ZCullGClip(this, drawPos);
}

int tPrimGeom::GetGeoType() const {
    MARKFUNCTION(0x800A1888);
    return geoType;
}

int tPrimGeom::GetEntityType() {
    MARKFUNCTION(0x800A1894);
    return 196609;
}

struct GMFogState {
    bool enabled = false;
    u16 nearDist = 0;
    u16 farDist = 0;
    u32 colour = 0;
};

static pddiPrimBuffer* BuildPrimBufferFromPrimGeom(const tPrimGeom* geom,
                                                   const LVector* drawPos,
                                                   const GMFogState* gmFogState,
                                                   bool* outUsesSemiTrans,
                                                   u8* outSemiTransMode) {
    if (outUsesSemiTrans) {
        *outUsesSemiTrans = false;
    }
    if (outSemiTransMode) {
        *outSemiTransMode = 0;
    }

    if (!geom || !geom->ownedRawData || !geom->ownedRawSize)
        return nullptr;

    const u8* verts = geom->GetVertexList();
    const u8* polys = geom->polyData;
    const u8* primStart = geom->primList;
    const u8* primEnd = geom->ownedRawData + geom->ownedRawSize;

    if (!verts || !polys || !primStart || !geom->loopVertData || !geom->loopPrimData)
        return nullptr;

    if (geom->numVerts == 0 || geom->numPolys == 0 || geom->numLoops == 0)
        return nullptr;

    // PSX path difference:
    // - RP_ZCullGClip rejects when all clip bits overlap after NCLIP.
    // - RP_ZCullGMFog only gates on NCLIP sign (no clip-bit overlap reject).
    const bool useClipOverlapReject = !(gmFogState && gmFogState->enabled);

    struct LoopInfo {
        u16 vertBase;
        u16 vertCount;
        u16 primCounts[4];
    };

    std::vector<LoopInfo> loops;
    loops.reserve(geom->numLoops);

    u16 vertAccum = 0;
    u32 expectedPolyCount = 0;
    for (u16 i = 0; i < geom->numLoops; i++) {
        const u8* loopVert = geom->loopVertData + i * 4;
        const u8* loopPrim = geom->loopPrimData + i * 8;

        LoopInfo info;
        info.vertBase = vertAccum;
        info.vertCount = p3dReadU16LE(loopVert + 0);
        info.primCounts[0] = p3dReadU16LE(loopPrim + 0);
        info.primCounts[1] = p3dReadU16LE(loopPrim + 2);
        info.primCounts[2] = p3dReadU16LE(loopPrim + 4);
        info.primCounts[3] = p3dReadU16LE(loopPrim + 6);
        loops.push_back(info);

        vertAccum = static_cast<u16>(vertAccum + info.vertCount);
        expectedPolyCount += info.primCounts[0] + info.primCounts[1] + info.primCounts[2] + info.primCounts[3];
    }

    ASSERT(vertAccum <= geom->numVerts);
    if (vertAccum > geom->numVerts)
        return nullptr;

    ASSERT(expectedPolyCount == geom->numPolys);
    if (expectedPolyCount != geom->numPolys) {
        LOG("[Geom] poly-count mismatch: geom=%p expected=%u numPolys=%u numLoops=%u gmFog=%u",
            geom,
            expectedPolyCount,
            geom->numPolys,
            geom->numLoops,
            (gmFogState && gmFogState->enabled) ? 1u : 0u);
        return nullptr;
    }

    struct Vert { f32 x, y, z, r, g, b, u, v, tpage, cba; };
    std::vector<Vert> vertBuf;
    std::vector<u16> idxBuf;
    u32 culledBackfaceCount = 0;
    u32 culledClipCount = 0;
    bool usesSemiTrans = false;
    bool hasSemiTransMode = false;
    u8 semiTransMode = 0;

    // PSX RP_ZCull* does primitive rejection in tPort space, not GL NDC.
    // GClip uses NCLIP + clip-bit overlap; GMFog uses NCLIP sign only.
    // Use the same tPort transform/clip convention as world culling.
    bool doPsxFaceCull = (drawPos && p3d::context && g_display);
    Mat4 viewMatrix;
    ChanProjectionState portState;
    s32 worldTx = 0;
    s32 worldTy = 0;
    s32 worldTz = 0;
    if (doPsxFaceCull) {
        viewMatrix = p3d::context->GetViewMatrix();
        portState = g_display->GetChanProjectionState();
        worldTx = drawPos->x;
        worldTy = drawPos->y;
        worldTz = drawPos->z;
    }

    struct PortProjectedVert {
        s32 sx;
        s32 sy;
        s32 vz;
        u32 clipCode;
    };

    auto transformToPort = [&](const Vert& v, s32* outX, s32* outY, s32* outZ) {
        f32 ox, oy, oz;
        Mat4TransformPoint(viewMatrix,
                           v.x + static_cast<f32>(worldTx),
                           v.y + static_cast<f32>(worldTy),
                           v.z + static_cast<f32>(worldTz),
                           ox, oy, oz);
        *outX = static_cast<s32>(ox);
        *outY = static_cast<s32>(-oy);
        *outZ = static_cast<s32>(-oz);
    };

    auto computeClipMask = [&](s32 sx, s32 sy) -> u32 {
        // PSX RP_ZCullGClip clip test (decomp):
        //   (SXY & 0x40004000) | ((0x01000200 - SXY) & 0x80008000 & 0xC000C000)
        // SXY values are in PSX screen space (nominal 320x240 centered at 160,120),
        // then tested against fixed 512x256 clip limits. Map host screen coords
        // into that PSX screen space before applying the mask formula.
        s32 psxSx = sx;
        s32 psxSy = sy;
        if (portState.width > 0) {
            psxSx = ((sx - portState.centerX) * 320) / portState.width + 160;
        }
        if (portState.height > 0) {
            psxSy = ((sy - portState.centerY) * 240) / portState.height + 120;
        }

        u32 packedSxy = (static_cast<u32>(static_cast<u16>(static_cast<s16>(psxSy))) << 16)
                      | static_cast<u16>(static_cast<s16>(psxSx));

        return (packedSxy & 0x40004000u)
             | ((0x01000200u - packedSxy) & 0x80008000u & 0xC000C000u);
    };

    auto projectToPortScreen = [&](const Vert& v, PortProjectedVert* out) {
        s32 vx, vy, vz;
        transformToPort(v, &vx, &vy, &vz);

        out->vz = vz;

        // Keep primitive rejection in screen space even when vz<=0 (PSX NCLIP
        // still evaluates SXY values). Guard only against divide-by-zero.
        const s32 denom = (vz == 0) ? 1 : vz;
        const f32 sxF = static_cast<f32>(portState.centerX) + (static_cast<f32>(vx) * portState.projectionDistanceX) / static_cast<f32>(denom);
        const f32 syF = static_cast<f32>(portState.centerY) + (static_cast<f32>(vy) * portState.projectionDistanceY) / static_cast<f32>(denom);

        if (sxF < -32768.0f) out->sx = -32768;
        else if (sxF > 32767.0f) out->sx = 32767;
        else out->sx = static_cast<s32>(sxF);

        if (syF < -32768.0f) out->sy = -32768;
        else if (syF > 32767.0f) out->sy = 32767;
        else out->sy = static_cast<s32>(syF);

        out->clipCode = computeClipMask(out->sx, out->sy);
    };

    enum class PsxCullResult : u8 {
        Keep = 0,
        Backface = 1,
        Clip = 2,
    };

    auto evaluatePsxCull = [&](const Vert& v0, const Vert& v1, const Vert& v2, const Vert* v3) -> PsxCullResult {
        if (!doPsxFaceCull) {
            return PsxCullResult::Keep;
        }

        PortProjectedVert p0, p1, p2;
        projectToPortScreen(v0, &p0);
        projectToPortScreen(v1, &p1);
        projectToPortScreen(v2, &p2);

        // PSX NCLIP result in MAC0: reject when MAC0 >= 0.
        s64 mac0 = static_cast<s64>(p0.sx) * static_cast<s64>(p1.sy - p2.sy)
                 + static_cast<s64>(p1.sx) * static_cast<s64>(p2.sy - p0.sy)
                 + static_cast<s64>(p2.sx) * static_cast<s64>(p0.sy - p1.sy);
        if (mac0 >= 0) {
            return PsxCullResult::Backface;
        }

        if (!useClipOverlapReject) {
            return PsxCullResult::Keep;
        }

        u32 commonClip = p0.clipCode & p1.clipCode & p2.clipCode;
        if (v3) {
            PortProjectedVert p3;
            projectToPortScreen(*v3, &p3);
            commonClip &= p3.clipCode;
        }

        if (commonClip == 0) {
            return PsxCullResult::Keep;
        }
        return PsxCullResult::Clip;
    };

    auto triArea2 = [](s32 ax, s32 ay, s32 bx, s32 by, s32 cx, s32 cy) -> s64 {
        return static_cast<s64>(bx - ax) * static_cast<s64>(cy - ay)
             - static_cast<s64>(by - ay) * static_cast<s64>(cx - ax);
    };

    auto chooseQuadDiag02 = [&](const Vert& v0, const Vert& v1, const Vert& v2, const Vert& v3) -> bool {
        if (!doPsxFaceCull) {
            return false;
        }

        PortProjectedVert p0, p1, p2, p3;
        projectToPortScreen(v0, &p0);
        projectToPortScreen(v1, &p1);
        projectToPortScreen(v2, &p2);
        projectToPortScreen(v3, &p3);

        s64 a012 = triArea2(p0.sx, p0.sy, p1.sx, p1.sy, p2.sx, p2.sy);
        if (a012 == 0) {
            return false;
        }

        s64 a132 = triArea2(p1.sx, p1.sy, p3.sx, p3.sy, p2.sx, p2.sy);
        s64 a023 = triArea2(p0.sx, p0.sy, p2.sx, p2.sy, p3.sx, p3.sy);

        bool stripSame = (a132 > 0) == (a012 > 0);
        bool diagSame = (a023 > 0) == (a012 > 0);

        if (diagSame && !stripSame) {
            return true;
        }
        if (stripSame && !diagSame) {
            return false;
        }

        s64 abs132 = (a132 < 0) ? -a132 : a132;
        s64 abs023 = (a023 < 0) ? -a023 : a023;
        return abs023 > abs132;
    };

    auto readVert = [&](u16 vertBase, u8 vi) -> Vert {
        u32 idx = vertBase + vi;
        ASSERT(idx < geom->numVerts);
        if (idx >= geom->numVerts)
            idx = 0;

        const u8* v = verts + idx * 8;
        Vert vert;
        vert.x = static_cast<f32>(p3dReadS16LE(v + 0));
        vert.y = static_cast<f32>(p3dReadS16LE(v + 2));
        vert.z = static_cast<f32>(p3dReadS16LE(v + 4));
        vert.r = 0.7f; vert.g = 0.7f; vert.b = 0.7f;
        vert.u = 0.0f; vert.v = 0.0f;
        vert.tpage = -1.0f; vert.cba = 0.0f;
        return vert;
    };

    std::vector<u8> gmFogPrimScratch;
    if (gmFogState && gmFogState->enabled && geom->gmFogWriteList && geom->gmFogColourList) {
        const u32 primRegionOff = static_cast<u32>(primStart - geom->ownedRawData);
        if (primRegionOff < geom->ownedRawSize) {
            const u32 primRegionSize = geom->ownedRawSize - primRegionOff;
            gmFogPrimScratch.assign(primStart, primStart + primRegionSize);

            auto writeFogRGB = [&](u32 linkOffset, u8 r, u8 g, u8 b) {
                if (linkOffset < 4)
                    return;

                u32 colorOff = linkOffset - 4;
                if (colorOff + 3 > gmFogPrimScratch.size())
                    return;

                gmFogPrimScratch[colorOff + 0] = r;
                gmFogPrimScratch[colorOff + 1] = g;
                gmFogPrimScratch[colorOff + 2] = b;
            };

            auto computeFoggedRGB = [&](u32 vertIndex, u32 baseColour, u8* outR, u8* outG, u8* outB) {
                (void)vertIndex;
                u8 r = static_cast<u8>(baseColour & 0xFF);
                u8 g = static_cast<u8>((baseColour >> 8) & 0xFF);
                u8 b = static_cast<u8>((baseColour >> 16) & 0xFF);

                *outR = r;
                *outG = g;
                *outB = b;
            };

            const u8* pairPtr = geom->gmFogWriteList;
            const u8* pairEnd = geom->ownedRawData + geom->ownedRawSize;
            const u32 colourOff = static_cast<u32>(geom->gmFogColourList - geom->ownedRawData);
            const bool hasColourTable = colourOff < geom->ownedRawSize && (geom->ownedRawSize - colourOff) >= static_cast<u32>(geom->numVerts) * 4u;

            if (pairPtr < pairEnd && hasColourTable) {
                u32 vertIndex = 0;
                bool stop = false;
                for (u16 loop = 0; loop < geom->numLoops && !stop; ++loop) {
                    u16 loopVertCount = p3dReadU16LE(geom->loopVertData + loop * 4);
                    for (u16 iv = 0; iv < loopVertCount; ++iv) {
                        if (vertIndex >= geom->numVerts) {
                            stop = true;
                            break;
                        }

                        const u32 baseColour = p3dReadU32LE(geom->gmFogColourList + vertIndex * 4);
                        u8 outR, outG, outB;
                        computeFoggedRGB(vertIndex, baseColour, &outR, &outG, &outB);

                        const u32 vertWord1 = p3dReadU32LE(verts + vertIndex * 8 + 4);
                        int remaining = static_cast<int>((vertWord1 >> 24) & 0xFF) - 2;

                        while (remaining >= 0) {
                            if (pairPtr + 4 > pairEnd) {
                                stop = true;
                                break;
                            }

                            const u32 pair = p3dReadU32LE(pairPtr);
                            pairPtr += 4;
                            writeFogRGB(pair & 0xFFFF, outR, outG, outB);
                            writeFogRGB((pair >> 16) & 0xFFFF, outR, outG, outB);
                            remaining -= 2;
                        }

                        if (stop)
                            break;

                        if (remaining == -1) {
                            if (pairPtr + 4 > pairEnd) {
                                stop = true;
                                break;
                            }

                            const u32 pair = p3dReadU32LE(pairPtr);
                            pairPtr += 4;
                            writeFogRGB(pair & 0xFFFF, outR, outG, outB);
                        }

                        ++vertIndex;
                    }
                }
            }

            primStart = gmFogPrimScratch.data();
            primEnd = primStart + gmFogPrimScratch.size();
        }
    }

    const u8* primCursor = primStart;
    u32 polyIdx = 0;

    auto processPrimitive = [&](u16 vertBase, int bucket) -> bool {
        if (polyIdx >= geom->numPolys) {
            ASSERT(false);
            return false;
        }

        // PSX RP_ZCullGClip / RP_ZCullGMFog advance by fixed packet stride per
        // bucket, independent of the OT tag's word-count byte.
        u32 pktSize = 0;
        if (bucket == 0) pktSize = 52;
        else if (bucket == 1) pktSize = 36;
        else if (bucket == 2) pktSize = 40;
        else if (bucket == 3) pktSize = 28;
        else {
            ASSERT(false);
            return false;
        }

        if (primCursor + pktSize > primEnd) {
            ASSERT(false);
            LOG("[Geom] packet overrun: geom=%p bucket=%d polyIdx=%u pktSize=%u rem=%u gmFog=%u",
                geom,
                bucket,
                polyIdx,
                pktSize,
                static_cast<u32>(primEnd - primCursor),
                (gmFogState && gmFogState->enabled) ? 1u : 0u);
            return false;
        }

        const u8* pkt = primCursor;
        const u8* poly = polys + polyIdx * 4;
        const u8 cmd = pkt[7];
        if ((cmd & 0x2u) != 0u) {
            usesSemiTrans = true;
        }
        u8 cmdBase = cmd & 0xFC;
        u8 vi0 = poly[0], vi1 = poly[1], vi2 = poly[2], vi3 = poly[3];

        if (bucket == 0) ASSERT(cmdBase == 0x3C || cmdBase == 0x2C);
        if (bucket == 1) ASSERT(cmdBase == 0x38 || cmdBase == 0x28);
        if (bucket == 2) ASSERT(cmdBase == 0x34 || cmdBase == 0x24);
        if (bucket == 3) ASSERT(cmdBase == 0x30 || cmdBase == 0x20);

        auto readRGB = [&](int byteOff) {
            if (byteOff + 3 > (int)pktSize)
                return std::make_tuple(0.5f, 0.5f, 0.5f);

            f32 r = std::min(1.0f, pkt[byteOff] / 128.0f);
            f32 g = std::min(1.0f, pkt[byteOff + 1] / 128.0f);
            f32 b = std::min(1.0f, pkt[byteOff + 2] / 128.0f);
            return std::make_tuple(r, g, b);
        };

        if (cmdBase == 0x3C) {
            if (pktSize < 52) {
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            Vert v0 = readVert(vertBase, vi0), v1 = readVert(vertBase, vi1);
            Vert v2 = readVert(vertBase, vi2), v3 = readVert(vertBase, vi3);
            const u16 tpage = p3dReadU16LE(pkt + 26);
            f32 tp = static_cast<f32>(tpage);
            f32 cb = static_cast<f32>(p3dReadU16LE(pkt + 14));
            if ((cmd & 0x2u) != 0u && !hasSemiTransMode) {
                semiTransMode = static_cast<u8>((tpage >> 5) & 3u);
                hasSemiTransMode = true;
            }

            auto [r0, g0, b0] = readRGB(4);
            auto [r1, g1, b1] = readRGB(16);
            auto [r2, g2, b2] = readRGB(28);
            auto [r3, g3, b3] = readRGB(40);
            v0.r = r0; v0.g = g0; v0.b = b0;
            v1.r = r1; v1.g = g1; v1.b = b1;
            v2.r = r2; v2.g = g2; v2.b = b2;
            v3.r = r3; v3.g = g3; v3.b = b3;

            v0.u = pkt[12]; v0.v = pkt[13]; v0.tpage = tp; v0.cba = cb;
            v1.u = pkt[24]; v1.v = pkt[25]; v1.tpage = tp; v1.cba = cb;
            v2.u = pkt[36]; v2.v = pkt[37]; v2.tpage = tp; v2.cba = cb;
            v3.u = pkt[48]; v3.v = pkt[49]; v3.tpage = tp; v3.cba = cb;

            const PsxCullResult cull = evaluatePsxCull(v0, v1, v2, &v3);
            if (cull != PsxCullResult::Keep) {
                if (cull == PsxCullResult::Backface) ++culledBackfaceCount;
                else ++culledClipCount;
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            u16 base = static_cast<u16>(vertBuf.size());
            vertBuf.push_back(v0); vertBuf.push_back(v1);
            vertBuf.push_back(v2); vertBuf.push_back(v3);
            idxBuf.push_back(base); idxBuf.push_back(base + 1); idxBuf.push_back(base + 2);
            if (chooseQuadDiag02(v0, v1, v2, v3)) {
                idxBuf.push_back(base); idxBuf.push_back(base + 2); idxBuf.push_back(base + 3);
            }
            else {
                idxBuf.push_back(base + 1); idxBuf.push_back(base + 3); idxBuf.push_back(base + 2);
            }
        }
        else if (cmdBase == 0x2C) {
            if (pktSize < 40) {
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            Vert v0 = readVert(vertBase, vi0), v1 = readVert(vertBase, vi1);
            Vert v2 = readVert(vertBase, vi2), v3 = readVert(vertBase, vi3);
            const u16 tpage = p3dReadU16LE(pkt + 22);
            f32 tp = static_cast<f32>(tpage);
            f32 cb = static_cast<f32>(p3dReadU16LE(pkt + 14));
            if ((cmd & 0x2u) != 0u && !hasSemiTransMode) {
                semiTransMode = static_cast<u8>((tpage >> 5) & 3u);
                hasSemiTransMode = true;
            }

            auto [r0, g0, b0] = readRGB(4);
            v0.r = r0; v0.g = g0; v0.b = b0;
            v1.r = r0; v1.g = g0; v1.b = b0;
            v2.r = r0; v2.g = g0; v2.b = b0;
            v3.r = r0; v3.g = g0; v3.b = b0;

            v0.u = pkt[12]; v0.v = pkt[13]; v0.tpage = tp; v0.cba = cb;
            v1.u = pkt[20]; v1.v = pkt[21]; v1.tpage = tp; v1.cba = cb;
            v2.u = pkt[28]; v2.v = pkt[29]; v2.tpage = tp; v2.cba = cb;
            v3.u = pkt[36]; v3.v = pkt[37]; v3.tpage = tp; v3.cba = cb;

            const PsxCullResult cull = evaluatePsxCull(v0, v1, v2, &v3);
            if (cull != PsxCullResult::Keep) {
                if (cull == PsxCullResult::Backface) ++culledBackfaceCount;
                else ++culledClipCount;
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            u16 base = static_cast<u16>(vertBuf.size());
            vertBuf.push_back(v0); vertBuf.push_back(v1);
            vertBuf.push_back(v2); vertBuf.push_back(v3);
            idxBuf.push_back(base); idxBuf.push_back(base + 1); idxBuf.push_back(base + 2);
            if (chooseQuadDiag02(v0, v1, v2, v3)) {
                idxBuf.push_back(base); idxBuf.push_back(base + 2); idxBuf.push_back(base + 3);
            }
            else {
                idxBuf.push_back(base + 1); idxBuf.push_back(base + 3); idxBuf.push_back(base + 2);
            }
        }
        else if (cmdBase == 0x38 || cmdBase == 0x28) {
            Vert v0 = readVert(vertBase, vi0), v1 = readVert(vertBase, vi1);
            Vert v2 = readVert(vertBase, vi2), v3 = readVert(vertBase, vi3);

            if (cmdBase == 0x38) {
                auto [r0, g0, b0] = readRGB(4);
                auto [r1, g1, b1] = readRGB(12);
                auto [r2, g2, b2] = readRGB(20);
                auto [r3, g3, b3] = readRGB(28);
                v0.r = r0; v0.g = g0; v0.b = b0;
                v1.r = r1; v1.g = g1; v1.b = b1;
                v2.r = r2; v2.g = g2; v2.b = b2;
                v3.r = r3; v3.g = g3; v3.b = b3;
            }
            else {
                auto [r0, g0, b0] = readRGB(4);
                v0.r = r0; v0.g = g0; v0.b = b0;
                v1.r = r0; v1.g = g0; v1.b = b0;
                v2.r = r0; v2.g = g0; v2.b = b0;
                v3.r = r0; v3.g = g0; v3.b = b0;
            }

            const PsxCullResult cull = evaluatePsxCull(v0, v1, v2, &v3);
            if (cull != PsxCullResult::Keep) {
                if (cull == PsxCullResult::Backface) ++culledBackfaceCount;
                else ++culledClipCount;
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            u16 base = static_cast<u16>(vertBuf.size());
            vertBuf.push_back(v0); vertBuf.push_back(v1);
            vertBuf.push_back(v2); vertBuf.push_back(v3);
            idxBuf.push_back(base); idxBuf.push_back(base + 1); idxBuf.push_back(base + 2);
            if (chooseQuadDiag02(v0, v1, v2, v3)) {
                idxBuf.push_back(base); idxBuf.push_back(base + 2); idxBuf.push_back(base + 3);
            }
            else {
                idxBuf.push_back(base + 1); idxBuf.push_back(base + 3); idxBuf.push_back(base + 2);
            }
        }
        else if (cmdBase == 0x34) {
            if (pktSize < 40) {
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            Vert v0 = readVert(vertBase, vi0), v1 = readVert(vertBase, vi1), v2 = readVert(vertBase, vi2);
            const u16 tpage = p3dReadU16LE(pkt + 26);
            f32 tp = static_cast<f32>(tpage);
            f32 cb = static_cast<f32>(p3dReadU16LE(pkt + 14));
            if ((cmd & 0x2u) != 0u && !hasSemiTransMode) {
                semiTransMode = static_cast<u8>((tpage >> 5) & 3u);
                hasSemiTransMode = true;
            }

            auto [r0, g0, b0] = readRGB(4);
            auto [r1, g1, b1] = readRGB(16);
            auto [r2, g2, b2] = readRGB(28);
            v0.r = r0; v0.g = g0; v0.b = b0;
            v1.r = r1; v1.g = g1; v1.b = b1;
            v2.r = r2; v2.g = g2; v2.b = b2;

            v0.u = pkt[12]; v0.v = pkt[13]; v0.tpage = tp; v0.cba = cb;
            v1.u = pkt[24]; v1.v = pkt[25]; v1.tpage = tp; v1.cba = cb;
            v2.u = pkt[36]; v2.v = pkt[37]; v2.tpage = tp; v2.cba = cb;

            const PsxCullResult cull = evaluatePsxCull(v0, v1, v2, nullptr);
            if (cull != PsxCullResult::Keep) {
                if (cull == PsxCullResult::Backface) ++culledBackfaceCount;
                else ++culledClipCount;
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            u16 base = static_cast<u16>(vertBuf.size());
            vertBuf.push_back(v0); vertBuf.push_back(v1); vertBuf.push_back(v2);
            idxBuf.push_back(base); idxBuf.push_back(base + 1); idxBuf.push_back(base + 2);
        }
        else if (cmdBase == 0x24) {
            if (pktSize < 32) {
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            Vert v0 = readVert(vertBase, vi0), v1 = readVert(vertBase, vi1), v2 = readVert(vertBase, vi2);
            const u16 tpage = p3dReadU16LE(pkt + 22);
            f32 tp = static_cast<f32>(tpage);
            f32 cb = static_cast<f32>(p3dReadU16LE(pkt + 14));
            if ((cmd & 0x2u) != 0u && !hasSemiTransMode) {
                semiTransMode = static_cast<u8>((tpage >> 5) & 3u);
                hasSemiTransMode = true;
            }

            auto [r0, g0, b0] = readRGB(4);
            v0.r = r0; v0.g = g0; v0.b = b0;
            v1.r = r0; v1.g = g0; v1.b = b0;
            v2.r = r0; v2.g = g0; v2.b = b0;

            v0.u = pkt[12]; v0.v = pkt[13]; v0.tpage = tp; v0.cba = cb;
            v1.u = pkt[20]; v1.v = pkt[21]; v1.tpage = tp; v1.cba = cb;
            v2.u = pkt[28]; v2.v = pkt[29]; v2.tpage = tp; v2.cba = cb;

            const PsxCullResult cull = evaluatePsxCull(v0, v1, v2, nullptr);
            if (cull != PsxCullResult::Keep) {
                if (cull == PsxCullResult::Backface) ++culledBackfaceCount;
                else ++culledClipCount;
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            u16 base = static_cast<u16>(vertBuf.size());
            vertBuf.push_back(v0); vertBuf.push_back(v1); vertBuf.push_back(v2);
            idxBuf.push_back(base); idxBuf.push_back(base + 1); idxBuf.push_back(base + 2);
        }
        else if (cmdBase == 0x30 || cmdBase == 0x20) {
            Vert v0 = readVert(vertBase, vi0), v1 = readVert(vertBase, vi1), v2 = readVert(vertBase, vi2);

            if (cmdBase == 0x30) {
                auto [r0, g0, b0] = readRGB(4);
                auto [r1, g1, b1] = readRGB(12);
                auto [r2, g2, b2] = readRGB(20);
                v0.r = r0; v0.g = g0; v0.b = b0;
                v1.r = r1; v1.g = g1; v1.b = b1;
                v2.r = r2; v2.g = g2; v2.b = b2;
            }
            else {
                auto [r0, g0, b0] = readRGB(4);
                v0.r = r0; v0.g = g0; v0.b = b0;
                v1.r = r0; v1.g = g0; v1.b = b0;
                v2.r = r0; v2.g = g0; v2.b = b0;
            }

            const PsxCullResult cull = evaluatePsxCull(v0, v1, v2, nullptr);
            if (cull != PsxCullResult::Keep) {
                if (cull == PsxCullResult::Backface) ++culledBackfaceCount;
                else ++culledClipCount;
                primCursor += pktSize;
                polyIdx++;
                return true;
            }

            u16 base = static_cast<u16>(vertBuf.size());
            vertBuf.push_back(v0); vertBuf.push_back(v1); vertBuf.push_back(v2);
            idxBuf.push_back(base); idxBuf.push_back(base + 1); idxBuf.push_back(base + 2);
        }
        else {
            ASSERT(false);
            LOG("[Geom] unknown cmd: geom=%p bucket=%d polyIdx=%u cmd=0x%02X gmFog=%u",
                geom,
                bucket,
                polyIdx,
                cmdBase,
                (gmFogState && gmFogState->enabled) ? 1u : 0u);
            return false;
        }

        primCursor += pktSize;
        polyIdx++;
        return true;
    };

    for (int loop = 0; loop < (int)loops.size(); loop++) {
        const LoopInfo& loopInfo = loops[loop];

        for (int bucket = 0; bucket < 4; bucket++) {
            for (u16 count = 0; count < loopInfo.primCounts[bucket]; count++) {
                if (!processPrimitive(loopInfo.vertBase, bucket)) {
                    LOG("[Geom] processPrimitive failed: geom=%p loop=%d bucket=%d polyIdx=%u gmFog=%u",
                        geom,
                        loop,
                        bucket,
                        polyIdx,
                        (gmFogState && gmFogState->enabled) ? 1u : 0u);
                    return nullptr;
                }
            }
        }
    }

    ASSERT(polyIdx == geom->numPolys);
    if (polyIdx != geom->numPolys)
        return nullptr;

    if (idxBuf.empty()) {
        return nullptr;
    }

    // Create pddiPrimBuffer through the device abstraction
    u32 format = PDDI_V_POSITION | PDDI_V_COLOUR | PDDI_V_UV | PDDI_V_TEXINFO;
    pddiPrimBufferDesc desc(PDDI_PRIM_TRIANGLES, format,
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

    return buffer;
}

static bool InitPrimGeomFromRawPrimData(tPrimGeom* geom, const u8* primData, u32 primSize) {
    if (!geom || !primData || primSize < 108) {
        return false;
    }

    geom->ownedRawData = new u8[primSize];
    geom->ownedRawSize = primSize;
    memcpy(geom->ownedRawData, primData, primSize);

    const u8* raw = geom->ownedRawData;

    u32 vertListOff = p3dReadU32LE(raw + 0x10) << 2;
    u32 primListOff = p3dReadU32LE(raw + 0x40) << 2;
    u32 primListAltOff = p3dReadU32LE(raw + 0x44) << 2;
    u32 primData48Off = p3dReadU32LE(raw + 0x48) << 2;
    u32 primData4COff = p3dReadU32LE(raw + 0x4C) << 2;
    u32 gmFogWriteOff = p3dReadU32LE(raw + 0x50) << 2;
    u32 polyDataOff = p3dReadU32LE(raw + 0x54) << 2;
    u32 gmFogColourOff = p3dReadU32LE(raw + 0x58) << 2;
    u32 primData5COff = p3dReadU32LE(raw + 0x5C) << 2;
    u32 loopVertOff = p3dReadU32LE(raw + 0x60) << 2;
    u32 loopPrimOff = p3dReadU32LE(raw + 0x68) << 2;

    geom->SetVertexList((vertListOff < primSize) ? (raw + vertListOff) : nullptr);
    geom->numVerts = p3dReadU16LE(raw + 0x14);
    geom->numPolys = p3dReadU16LE(raw + 0x16);
    geom->primList = (primListOff < primSize) ? (raw + primListOff) : nullptr;
    geom->primListAlt = (primListAltOff < primSize) ? (raw + primListAltOff) : nullptr;
    geom->primData48 = (primData48Off < primSize) ? (raw + primData48Off) : nullptr;
    geom->primData4C = (primData4COff < primSize) ? (raw + primData4COff) : nullptr;
    geom->gmFogWriteList = (gmFogWriteOff < primSize) ? (raw + gmFogWriteOff) : nullptr;
    geom->polyData = (polyDataOff < primSize) ? (raw + polyDataOff) : nullptr;
    geom->gmFogColourList = (gmFogColourOff < primSize) ? (raw + gmFogColourOff) : nullptr;
    geom->primData5C = (primData5COff < primSize) ? (raw + primData5COff) : nullptr;
    geom->loopVertData = (loopVertOff < primSize) ? (raw + loopVertOff) : nullptr;
    geom->geoType = p3dReadU16LE(raw + 0x64);
    geom->numLoops = p3dReadU16LE(raw + 0x66);
    geom->loopPrimData = (loopPrimOff < primSize) ? (raw + loopPrimOff) : nullptr;

    return geom->GetVertexList() && geom->primList && geom->polyData && geom->loopVertData && geom->loopPrimData;
}

tPrimGeom* CloneRawPrimGeom(const u8* primData, u32 primSize) {
    tPrimGeom* geom = new tPrimGeom();
    if (!InitPrimGeomFromRawPrimData(geom, primData, primSize)) {
        delete geom;
        return nullptr;
    }

    return geom;
}

pddiPrimBuffer* BuildPrimBufferFromRawPrimGeom(const u8* primData, u32 primSize) {
    tPrimGeom* geom = CloneRawPrimGeom(primData, primSize);
    if (!geom) {
        return nullptr;
    }

    pddiPrimBuffer* buffer = BuildPrimBufferFromPrimGeom(geom, nullptr, nullptr, nullptr, nullptr);
    delete geom;
    return buffer;
}

u32 RP_XformVertsLitCBF_CL(tPrimGeom* geometry, STreeJoint* joint, u32* fastCache, u16* scratch) {
    MARKFUNCTION(0x80084F14);

    if (!geometry || !joint || !geometry->GetVertexList()) {
        return 0;
    }

    const u8* vertexList = geometry->GetVertexList();
    const u32 startIndex = joint->primGeomStartIdx;
    const u32 endIndex = startIndex + joint->primGeomCount;
    for (u32 vertexIndex = startIndex; vertexIndex < endIndex; vertexIndex++) {
        const u8* vertex = vertexList + vertexIndex * 8;
        if (scratch) {
            u8 litR = 0;
            u8 litG = 0;
            u8 litB = 0;
            ComputeRPStreeLitColour(vertex[6], &litR, &litG, &litB);
            scratch[vertexIndex * 2 + 0] = static_cast<u16>((litG << 8) | litR);
            scratch[vertexIndex * 2 + 1] = static_cast<u16>(litB);
        }
    }

    (void)fastCache;
    return 0;
}

s32 RP_FixUpPolysCBF_CL(tPrimGeom* geometry, void* view, u32 loopIndex, u32 polyIndex) {
    MARKFUNCTION(0x8008500C);

    static bool warned = false;
    if (!warned) {
        LOG("[STree] RP_FixUpPolysCBF_CL ownership restored but implementation is still pending RPSTREECOL reversal");
        warned = true;
    }

    (void)geometry;
    (void)view;
    (void)loopIndex;
    return static_cast<s32>(polyIndex);
}

int RP_ZCullGClip(tGeometry* geometry, const LVector* drawPos) {
    MARKFUNCTION(0x800A0E14);

    tPrimGeom* primGeom = static_cast<tPrimGeom*>(geometry);
    bool usesSemiTrans = false;
    u8 semiTransMode = 0;
    pddiPrimBuffer* buffer = BuildPrimBufferFromPrimGeom(primGeom, drawPos, nullptr, &usesSemiTrans, &semiTransMode);
    if (!buffer)
        return 0;

    p3d::context->SetBlendMode(PDDI_BLEND_NONE);

    if (usesSemiTrans) {
        pddiBlendMode blendMode = PDDI_BLEND_ALPHA;
        switch (semiTransMode & 3u) {
            case 1: blendMode = PDDI_BLEND_ADD; break;
            case 2: blendMode = PDDI_BLEND_SUBTRACT; break;
            case 3: blendMode = PDDI_BLEND_PSX_QUARTER; break;
            default: break;
        }
        p3d::context->SetBlendMode(blendMode);
    }

    p3d::context->DrawPrimBuffer(buffer);

    if (usesSemiTrans) {
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    }

    buffer->Release();
    return 1;
}

int RP_ZCullGMFog(tGeometry* geometry, const LVector* drawPos, u16 fogNear, u16 fogFar, u32 fogColour) {
    MARKFUNCTION(0x800A18F4);

    GMFogState gmFogState;
    gmFogState.enabled = true;
    gmFogState.nearDist = fogNear;
    gmFogState.farDist = fogFar;
    gmFogState.colour = fogColour;

    tPrimGeom* primGeom = static_cast<tPrimGeom*>(geometry);
    bool usesSemiTrans = false;
    u8 semiTransMode = 0;
    pddiPrimBuffer* buffer = BuildPrimBufferFromPrimGeom(primGeom, drawPos, &gmFogState, &usesSemiTrans, &semiTransMode);
    if (!buffer)
        return 0;

    p3d::context->SetBlendMode(PDDI_BLEND_NONE);

    if (usesSemiTrans) {
        pddiBlendMode blendMode = PDDI_BLEND_ALPHA;
        switch (semiTransMode & 3u) {
            case 1: blendMode = PDDI_BLEND_ADD; break;
            case 2: blendMode = PDDI_BLEND_SUBTRACT; break;
            case 3: blendMode = PDDI_BLEND_PSX_QUARTER; break;
            default: break;
        }
        p3d::context->SetBlendMode(blendMode);
    }

    p3d::context->DrawPrimBuffer(buffer);

    if (usesSemiTrans) {
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    }

    buffer->Release();
    return 1;
}

