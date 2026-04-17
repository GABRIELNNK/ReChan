#include "common.h"
#include "ai/player.h"
#include "gen/world.h"
#include "gen/ai.h"
#include "gen/camera.h"
#include "gen/charmgr.h"
#include "gen/database.h"
#include "gen/display.h"
#include "gen/director.h"
#include "gen/game.h"
#include "gen/geometry.h"
#include "gen/levelmgr.h"
#include "gen/model.h"
#include "snd/rsevent.h"
#include "fe/hdmenu.h"
#include "fe/hud.h"
#include "fe/loadanim.h"
#include "p3d/hash.h"
#include "p3d/context.h"
#include "p3d/stream.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "ai/colfight.h"
#include "pc/log.h"

#include "gen/uvdata.h"

#include <fstream>
#include <filesystem>
#include <unordered_map>

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

static u16 ReadU16(const u8* p) { return static_cast<u16>(p[0] | (p[1] << 8)); }
static u32 ReadU32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static s16 ReadS16(const u8* p) { return static_cast<s16>(p[0] | (p[1] << 8)); }
static s32 ReadS32(const u8* p) { return static_cast<s32>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24)); }

struct GeoMaterialInfo {
    u8 primCmd = 0;
    u16 cba = 0;
    u16 tpage = 0;
};

struct GeoVertex {
    f32 x;
    f32 y;
    f32 z;
    f32 r;
    f32 g;
    f32 b;
    f32 u;
    f32 v;
    f32 tpage;
    f32 cba;
};

static void DecodePackedUV(u16 packed, GeoVertex& vertex, const GeoMaterialInfo& material) {
    vertex.u = static_cast<f32>(packed & 0xFF);
    vertex.v = static_cast<f32>((packed >> 8) & 0xFF);
    vertex.tpage = static_cast<f32>(material.tpage);
    vertex.cba = static_cast<f32>(material.cba);
}

static pddiPrimBuffer* ParseDynGeoPrims(
    const u8* geoData,
    u32 geoSize,
    const std::unordered_map<u32, GeoMaterialInfo>& materials)
{
    if (!geoData || geoSize < 0x58) {
        return nullptr;
    }

    u32 vertListOff = ReadU32(geoData + 0x10) << 2;
    u16 numVerts = ReadU16(geoData + 0x14);
    u16 numPolys = ReadU16(geoData + 0x16);
    u32 polyListOff = ReadU32(geoData + 0x40) << 2;

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

    std::vector<GeoVertex> vertBuf;
    std::vector<u16> idxBuf;

    auto makeVertex = [&](u16 index) -> GeoVertex {
        GeoVertex vertex = {};
        if (index >= numVerts) {
            return vertex;
        }

        const u8* src = verts + index * 8;
        vertex.x = static_cast<f32>(ReadS16(src + 0));
        vertex.y = static_cast<f32>(ReadS16(src + 2));
        vertex.z = static_cast<f32>(ReadS16(src + 4));
        vertex.r = 0.85f;
        vertex.g = 0.85f;
        vertex.b = 0.85f;
        vertex.u = 0.0f;
        vertex.v = 0.0f;
        vertex.tpage = -1.0f; // TEMP: force untextured to test geometry
        vertex.cba = 0.0f;
        return vertex;
    };

    for (u16 polyIndex = 0; polyIndex < numPolys; polyIndex++) {
        const u8* poly = polys + polyIndex * 24;
        u32 materialHash = ReadU32(poly + 0);
        auto materialIt = materials.find(materialHash);
        if (materialIt == materials.end()) {
            continue;
        }

        const GeoMaterialInfo& material = materialIt->second;
        u8 primCmd = static_cast<u8>(material.primCmd & 0xFD);

        GeoVertex v0 = makeVertex(ReadU16(poly + 8));
        GeoVertex v1 = makeVertex(ReadU16(poly + 10));
        GeoVertex v2 = makeVertex(ReadU16(poly + 12));
        GeoVertex v3 = makeVertex(ReadU16(poly + 14));

        if (primCmd == 0x34 || primCmd == 0x24 || primCmd == 0x3C || primCmd == 0x2C) {
            DecodePackedUV(ReadU16(poly + 16), v0, material);
            DecodePackedUV(ReadU16(poly + 18), v1, material);
            DecodePackedUV(ReadU16(poly + 20), v2, material);
            if (primCmd == 0x3C || primCmd == 0x2C) {
                DecodePackedUV(ReadU16(poly + 22), v3, material);
            }
        }

        u16 base = static_cast<u16>(vertBuf.size());
        switch (primCmd) {
            case 0x30:
            case 0x20:
            case 0x34:
            case 0x24:
                vertBuf.push_back(v0);
                vertBuf.push_back(v1);
                vertBuf.push_back(v2);
                idxBuf.push_back(base + 0);
                idxBuf.push_back(base + 1);
                idxBuf.push_back(base + 2);
                break;

            case 0x38:
            case 0x28:
            case 0x3C:
            case 0x2C:
                vertBuf.push_back(v0);
                vertBuf.push_back(v1);
                vertBuf.push_back(v2);
                vertBuf.push_back(v3);
                idxBuf.push_back(base + 0);
                idxBuf.push_back(base + 1);
                idxBuf.push_back(base + 2);
                idxBuf.push_back(base + 1);
                idxBuf.push_back(base + 3);
                idxBuf.push_back(base + 2);
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
    if (ReadU16(p3dData) != 0xFF04) {
        return;
    }

        std::unordered_map<u32, GeoMaterialInfo> materials;

        u32 rootSize = ReadU32(p3dData + 2);
        u32 chunkEnd = (rootSize < p3dSize) ? rootSize : p3dSize;
        u32 chunkPos = 6;
        u32 permCursor = 0;

        while (chunkPos + 6 <= chunkEnd) {
            u16 chunkId = ReadU16(p3dData + chunkPos);
            u32 chunkSize = ReadU32(p3dData + chunkPos + 2);
            if (chunkSize < 6 || chunkPos + chunkSize > chunkEnd) {
                break;
            }

            const u8* chunkBody = p3dData + chunkPos + 6;

            if (chunkId == 0x6001 || chunkId == 0x6002) {
                u32 nameCount = ReadU32(chunkBody + 0);
                u32 chunkPermSize = ReadU32(chunkBody + 4);
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
                                info.primCmd = static_cast<u8>((ReadU32(record + 16) >> 24) & 0xFF);
                                u32 texInfo = ReadU32(record + 20);
                                info.cba = static_cast<u16>(texInfo & 0xFFFF);
                                info.tpage = static_cast<u16>(texInfo >> 16);

                                u32 materialHash = ReadU32(record + 0);
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
                        u32 modelHash = ReadU32(permData + permCursor + 0);
                        if (!g_levelManager->FindModel(static_cast<s32>(modelHash))) {
                            pddiPrimBuffer* buffer = ParseDynGeoPrims(
                                permData + permCursor,
                                chunkPermSize,
                                materials);
                            if (buffer) {
                                OriginalGeo* original = new OriginalGeo();
                                original->nameCRC = modelHash ? modelHash : p3dHash(names[0].c_str());
                                original->SetStoreID(static_cast<s8>(storeId));
                                original->meshBuffer = buffer;
                                original->bboxMin[0] = ReadS32(permData + permCursor + 0x18);
                                original->bboxMin[1] = ReadS32(permData + permCursor + 0x1C);
                                original->bboxMin[2] = ReadS32(permData + permCursor + 0x20);
                                original->bboxMax[0] = ReadS32(permData + permCursor + 0x24);
                                original->bboxMax[1] = ReadS32(permData + permCursor + 0x28);
                                original->bboxMax[2] = ReadS32(permData + permCursor + 0x2C);
                                g_levelManager->AddOriginal(original, 0);
                                LOG("[World] Loaded Geo model '%s' (hash 0x%08X, store %d)",
                                    names[0].c_str(), original->nameCRC, storeId);
                            }
                        }
                    }
                }

                permCursor += chunkPermSize;
            }

            else if (chunkId == 0x8C20 || chunkId == 0x8C21) {
                LoadUVPrimData(chunkId, chunkBody, chunkSize - 6,
                               permData, permCursor, permSize);
            }

            else if (chunkId == 0x8C30 || chunkId == 0x8C31) {
                LoadCBVPrimData(chunkId, chunkBody, chunkSize - 6,
                                permData, permCursor, permSize);
            }

            else if (chunkId == 0x6008 && world) {
                u32 p = 0;
                u32 bodyLen = chunkSize - 6;
                if (bodyLen < 1) { chunkPos += chunkSize; continue; }
                u8 nameLen = chunkBody[p++];
                p += nameLen;
                if (p + 12 > bodyLen) { chunkPos += chunkSize; continue; }
                s16 rx = ReadS16(chunkBody + p); p += 2;
                s16 ry = ReadS16(chunkBody + p); p += 2;
                s16 rw = ReadS16(chunkBody + p); p += 2;
                s16 rh = ReadS16(chunkBody + p); p += 2;
                p += 4; // skip type
                if (rw > 0 && rh > 0 && rw <= 1024 && rh <= 512 &&
                    p + (u32)(rw * rh * 2) <= bodyLen) {
                    world->UploadToVRAM(rx, ry, rw, rh, chunkBody + p);
                    LOG("[GeoTex] VRAM upload: x=%d y=%d w=%d h=%d", rx, ry, rw, rh);
                }
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
    for (u32 i = rangeStart; i < rangeEnd; i++) {
        if (strncmp(entries[i].magic, permMagic, 4) != 0) {
            continue;
        }

            u32 pairIndex = rangeEnd;
            for (u32 j = i + 1; j < rangeEnd; j++) {
                if (strncmp(entries[j].magic, p3dMagic, 4) == 0) {
                    pairIndex = j;
                    break;
                }
                if (strncmp(entries[j].magic, permMagic, 4) == 0) {
                    break;
                }
            }

            if (pairIndex == rangeEnd) {
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
        u16 rootId = ReadU16(d);
        u32 rootSize = ReadU32(d + 2);
        if (rootId != 0xFF04) continue;

        u32 cpos = 6;
        u32 cend = (rootSize < size) ? rootSize : size;
        while (cpos + 6 <= cend) {
            u16 chunkId = ReadU16(d + cpos);
            u32 chunkSize = ReadU32(d + cpos + 2);
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
                s16 rx = ReadS16(d + p); p += 2;
                s16 ry = ReadS16(d + p); p += 2;
                s16 rw = ReadS16(d + p); p += 2;
                s16 rh = ReadS16(d + p); p += 2;
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

World::World() = default;

World::~World() {
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

    // PSX: LoadLevel__12LevelManager() - empty stub on PSX
    // PSX: PurgeLevelP3DInventory__12LevelManager() - also empty stub

    // PSX: OpenCharacter(type=0), EnableCache(type=0, 1)
    if (g_characterManager) {
        g_characterManager->OpenCharacter(0);
        g_characterManager->EnableCache(0, 1);

        // PSX: allocate CharMgrCallback, LoadCharacter(type=0, callback), spin until done
        CharMgrCallback* callback = new CharMgrCallback();
        g_characterManager->LoadCharacter(0, callback);
        // PSX spins: while (!callback->done) rDoTaskList(rMainTaskList, 0);
        // PC: LoadCharacter is synchronous, callback already fired

        // PSX: LoadAnimation(type=0, animEnum=0, hash=124, callback), spin until done
        callback->done = 0;
        g_characterManager->LoadAnimation(0, 0, 124, callback);
        // PSX spins again - PC is synchronous

        // PSX: LoadAnimation(type=0, animEnum=0, hash=124, nullptr) - fire and forget
        g_characterManager->LoadAnimation(0, 0, 124, nullptr);

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

    // PSX: Init__17FightingCollision, InsertHumanoid (player)
    FightingCollision::Init();
    if (Player::s_player) {
        FightingCollision::InsertHumanoid(static_cast<Humanoid*>(Player::s_player));
    }

    // PSX: CheckpointInfo
    u32 startBlockNum = 0;
    bool hasCheckpoint = false;
    // TODO: CheckpointInfo::IsValid() not yet reversed
    // if (checkpoint.IsValid()) { startBlockNum = checkpoint.field24; hasCheckpoint = true; }

    // PSX: WorldEffects, PWorldEffects, ParticleSystem
    // TODO: not yet reversed

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

    // PSX: PopulateWEffects
    // TODO: not yet reversed

    // PSX: ScoreManager::SetPar
    // TODO: not yet reversed

    // PSX: Director->Reset() then Director->SetScript()
    if (g_director) {
        g_director->Reset();
        g_director->SetScript();
    }

    // PSX: SetupModelAmbientLighting, ProcessSwitches
    // TODO: not yet reversed

    // PSX: Close__8Database(0)
    // TODO: database close not yet implemented

    // PSX: AllocBlockPool__12BlockManager(0) - allocate block node pool
    // PC: blocks already allocated by LoadBlocksFunc

    // PSX: LoadBlocks__12BlockManagerUl(0, startBlockNum)
    // PC: blocks already parsed by Load(). The active/draw lists were built in LoadBlocks.
    // Pop ulateBlock is called by LoadBlocks on PSX. On PC we call it here.
    if (g_ai) {
        g_ai->PopulateBlock();
    }

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
                && previousLevelIndex != currentLevelIndex) {
                // PSX: additional check: (!v10 || MEMORY[0x24] != 11)
                doShowLevel = true;
            }
            if (g_destSelectReturnPosValid) {
                doShowLevel = true;
            }

            LVector returnPos;
            if (doShowLevel && g_destSelectReturnPosValid) {
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

            LVector playerDelta = {
                returnPos.x - player->pos.x,
                returnPos.y - player->pos.y,
                returnPos.z - player->pos.z,
            };

            player->homePos = returnPos;
            player->pos = returnPos;

            g_destSelectReturnPosValid = false;

            if (g_display) {
                Camera* cam = g_display->GetCamera();
                if (cam) {
                    const LVector& camPos = cam->GetPosition();
                    cam->SetPosition(camPos.x + playerDelta.x,
                                     camPos.y + playerDelta.y,
                                     camPos.z + playerDelta.z);
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

    // Parse only this petal's BLK data into blocks
    std::vector<const u8*> blkPtrs;
    std::vector<u32> blkSizes;
    for (u32 i = petalStart; i < petalEnd; i++) {
        if (strncmp(entries[i].magic, ".BLK", 4) != 0) continue;
        if (entries[i].offset + entries[i].size > dataSize) {
            blkPtrs.push_back(nullptr);
            blkSizes.push_back(0);
        }
        else {
            blkPtrs.push_back(data + entries[i].offset);
            blkSizes.push_back(entries[i].size);
        }
    }
    blockMgr.LoadBlocks(0, blkPtrs.data(), blkSizes.data(), blkCount);

    LOG("[World] Loaded %u blocks", blockMgr.GetNumBlocks());

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
// Multiplies world-space point by the current view matrix (GTE rotation + translation)
static void TransformVector(const Mat4& vm, s32 inX, s32 inY, s32 inZ,
                            s32* outX, s32* outY, s32* outZ) {
    f32 ox, oy, oz;
    Mat4TransformPoint(vm, (f32)inX, (f32)inY, (f32)inZ, ox, oy, oz);
    *outX = static_cast<s32>(ox);
    *outY = static_cast<s32>(oy);
    *outZ = static_cast<s32>(oz);
}

// chanp3dClipCode - PC equivalent of PSX chanp3dClipCode
// Computes 6-bit clip code for a view-space point against the frustum
// bit 0: left, bit 1: right, bit 2: bottom, bit 3: top, bit 4: near, bit 5: far
static u32 chanp3dClipCode(const Mat4& pm, s32 vx, s32 vy, s32 vz) {
    f32 fx = static_cast<f32>(vx);
    f32 fy = static_cast<f32>(vy);
    f32 fz = static_cast<f32>(vz);
    // Transform to homogeneous clip space
    f32 cx = pm.m[0] * fx + pm.m[4] * fy + pm.m[8] * fz + pm.m[12];
    f32 cy = pm.m[1] * fx + pm.m[5] * fy + pm.m[9] * fz + pm.m[13];
    f32 cz = pm.m[2] * fx + pm.m[6] * fy + pm.m[10] * fz + pm.m[14];
    f32 cw = pm.m[3] * fx + pm.m[7] * fy + pm.m[11] * fz + pm.m[15];
    u32 code = 0;
    if (cx < -cw) code |= 0x01; // left
    if (cx > cw) code |= 0x02; // right
    if (cy < -cw) code |= 0x04; // bottom
    if (cy > cw) code |= 0x08; // top
    if (cz < -cw) code |= 0x10; // near
    if (cz > cw) code |= 0x20; // far
    return code;
}

// vecLengthSquared - PC equivalent of PSX vecLengthSquared
// Returns squared length of view-space vector (with >>8 shift to prevent overflow)
static s32 vecLengthSquared(s32 x, s32 y, s32 z) {
    s32 sx = x >> 8;
    s32 sy = y >> 8;
    s32 sz = z >> 8;
    return sx * sx + sy * sy + sz * sz;
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
    // PC: all blocks always loaded, no demand loading needed.

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
        if (!block || !block->primBuffer) continue;

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

            // PSX: Draw__5BlockRC10tagLVector(block, &localPos)
            entry.block->Draw(&localPos);
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

    // Get view and projection matrices (PC equivalent of GTE state)
    const Mat4& vm = p3d::context->GetViewMatrix();
    const Mat4& pm = p3d::context->GetProjectionMatrix();

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
        clipCodes[0] = chanp3dClipCode(pm, tvx, tvy, tvz);
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
        clipCodes[1] = chanp3dClipCode(pm, tvx, tvy, tvz);
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
        clipCodes[2] = chanp3dClipCode(pm, tvx, tvy, tvz);
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
        clipCodes[3] = chanp3dClipCode(pm, tvx, tvy, tvz);
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
        clipCodes[4] = chanp3dClipCode(pm, tvx, tvy, tvz);
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
        clipCodes[5] = chanp3dClipCode(pm, tvx, tvy, tvz);
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
        clipCodes[6] = chanp3dClipCode(pm, tvx, tvy, tvz);
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
        clipCodes[7] = chanp3dClipCode(pm, tvx, tvy, tvz);
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

    // PSX: v1 = gp[96] (seamDivisor)
    // These are set during level initialization - using reasonable PSX defaults
    s32 seamDivisor = 4096; // gp+0x60
    s32 seamLimit = 8;      // gp+0x64

    if (seamDivisor == 0) return;

    // PSX: a3 = (signX * dx) / seamDivisor
    s32 rawX = (signX * dx) / seamDivisor;
    s32 rawY = (signY * dy) / seamDivisor;
    s32 rawZ = (signZ * dz) / seamDivisor;

    // PSX: offset = (-sign) * (raw + 1) - pushes block position toward camera
    s32 offX = (-signX) * (rawX + 1);
    s32 offY = (-signY) * (rawY + 1);
    s32 offZ = (-signZ) * (rawZ + 1);

    // PSX: clamp each to Â±seamLimit (gp[100])
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
    UnloadUVPrimData();
    UnloadCBVPrimData();

    blockMgr.InternalClose();
    if (g_ai) {
        g_ai->UnPopulate(0);
    }
    streamData.clear();
    if (vramHandle && p3d::context) {
        p3d::context->DestroyVRAMTexture(vramHandle);
        vramHandle = 0;
    }
}

// PSX: UnloadPetal__5World (WORLD.CPP:1176, 0x80045F34)
void World::UnloadPetal() {
    MARKFUNCTION(0x80045F34);

    // PSX: Unload__10UVPrimData, Unload__11CBVPrimData (0x80045F90, 0x80045F98)
    UnloadUVPrimData();
    UnloadCBVPrimData();

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

    // PSX: rsEvent(4, petalSoundIDs[currentLevelIndex][petalIndex] - 1, 0, 0)
    if (petalSoundIDs && currentLevelIndex < (u32)levelCount) {
        s32 soundLocation = (s32)petalSoundIDs[currentLevelIndex][petalIndex] - 1;
        rsEvent(RS_SET_LOCATION, soundLocation, 0, 0);
    }

    targetPetalIndex = petalIndex;
    currentPetalIndex = petalIndex;

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

        u32 petalStart = wdbIndices[pi];
        u32 petalEnd = (pi + 1 < (u32)wdbIndices.size())
                           ? wdbIndices[pi + 1]
                           : (u32)entries.size();

        LOG("[World] LoadPetal %u: entries %u-%u", pi, petalStart, petalEnd - 1);

        // Scan this petal's WDB
        g_database->Close();
        for (u32 i = petalStart; i < petalEnd; i++) {
            if (strncmp(entries[i].magic, ".WDB", 4) != 0) continue;
            if (entries[i].offset + entries[i].size > dataSize) continue;
            g_database->Scan(data + entries[i].offset, entries[i].size);
        }

        LoadGeoPairsInRange(this, entries, data, dataSize, petalStart, petalEnd, ".PCI", ".PCP", 2);

        // Refresh VRAM GL texture after Geo texture uploads
        RefreshVRAMTexture();

        // Build block volumes
        std::vector<DBVolume*> blockVolumes;
        for (DBRoot* v = g_database->GetFirstBlock(); v; v = static_cast<DBRoot*>(v->next)) {
            blockVolumes.push_back(static_cast<DBVolume*>(v));
        }
        blockMgr.LoadBlocksFunc(blockVolumes);

        // Parse BLK data
        u32 blkCount = 0;
        std::vector<const u8*> blkPtrs;
        std::vector<u32> blkSizes;
        for (u32 i = petalStart; i < petalEnd; i++) {
            if (strncmp(entries[i].magic, ".BLK", 4) != 0) continue;
            blkCount++;
            if (entries[i].offset + entries[i].size > dataSize) {
                blkPtrs.push_back(nullptr);
                blkSizes.push_back(0);
            }
            else {
                blkPtrs.push_back(data + entries[i].offset);
                blkSizes.push_back(entries[i].size);
            }
        }
        blockMgr.LoadBlocks(0, blkPtrs.data(), blkSizes.data(), blkCount);

        LOG("[World] LoadPetal: loaded %u blocks", blockMgr.GetNumBlocks());
    }

    // PSX: AI::Populate for new petal entities
    if (g_ai) {
        g_ai->Populate();
    }

    if (g_director) {
        g_director->Reset();
        g_director->SetScript();
    }

    rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);

    // PSX: StopLogo after load completes
    StopLogo();
}

// PSX: ResetLevel__5World (WORLD.CPP:1918, 0x80046DE0)
void World::ResetLevel() {
    MARKFUNCTION(0x80046DE0);

    // PSX also resets checkpoint validity and dead pool state here.
    if (g_director) {
        g_director->LevelReset();
    }
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

