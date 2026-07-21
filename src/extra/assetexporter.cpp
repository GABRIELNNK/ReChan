#include "extra/assetexporter.h"
#include "extra/parameterdata.h"

#include "gen/charmgr.h"
#include "gen/levelmgr.h"
#include "gen/skeleton.h"
#include "gen/world.h"
#include "gen/psxcolor_helpers.h"
#include "pc/log.h"
#include "pc/tim.h"
#include "p3d/byteread.h"
#include "p3d/hash.h"
#include "p3d/texture.h"
#include "p3d/fileio.h"
#include "snd/adpcm.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>

const char* AssetEntry::CategoryName(AssetCategory cat) {
    switch (cat) {
        case AssetCategory::Texture:      return "Texture";
        case AssetCategory::SkeletalMesh: return "SkeletalMesh";
        case AssetCategory::StaticMesh:   return "StaticMesh";
        case AssetCategory::ETreeMesh:    return "ETreeMesh";
        case AssetCategory::Sound:        return "Sound";
        case AssetCategory::Data:         return "Data";
    }
    return "Unknown";
}

// Singleton

static AssetExporter s_assetExporterInstance;

AssetExporter& AssetExporter::Instance() {
    return s_assetExporterInstance;
}

// Catalog building

void AssetExporter::BuildCatalog() {
    if (m_catalogBuilt) return;

    m_entries.clear();

    ScanRCHARS();
    ScanTIMDirectory();
    ScanSoundDirectory();
    ScanLevelDirectory();
    ScanDataDirectory();

    // Sort by category then name
    std::sort(m_entries.begin(), m_entries.end(),
              [](const AssetEntry& a, const AssetEntry& b) {
        if (a.category != b.category)
            return (int)a.category < (int)b.category;
        return a.name < b.name;
    });

    LOG("[AssetExporter] Catalog built: %zu entries", m_entries.size());
    m_catalogBuilt = true;
}

void AssetExporter::ClearCatalog() {
    m_entries.clear();
    m_catalogBuilt = false;
}

// Directory scanners

void AssetExporter::ScanRCHARS() {
    const std::string rcharsDir = p3d::io::ResolvePath("RCHARS");
    if (!p3d::io::DirExists(rcharsDir)) {
        LOG("[AssetExporter] RCHARS/ directory not found");
        return;
    }

    // g_charNameTable has 29 entries (types 0-28)
    for (s32 type = 0; type < 29; type++) {
        const char* name = g_charNameTable[type];
        if (!name || !name[0]) continue;

        char rrPath[256];
        std::snprintf(rrPath, sizeof(rrPath), "RCHARS/%s.RR", name);

        const std::string resolvedRrPath = p3d::io::ResolvePath(rrPath);
        if (!p3d::io::FileExists(resolvedRrPath)) continue;

        u32 crc = p3dHash(name);

        // STree (skeletal mesh) — resource index 0 in most .RR files
        {
            AssetEntry entry;
            entry.name = name;
            std::transform(entry.name.begin(), entry.name.end(), entry.name.begin(),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
            entry.category = AssetCategory::SkeletalMesh;
            entry.filePath = resolvedRrPath;
            entry.crc = crc;
            entry.resourceIndex = 0;
            m_entries.push_back(std::move(entry));
        }

    }
}

void AssetExporter::ScanRCHARS_Textures() {
    const std::string rcharsDir = p3d::io::ResolvePath("RCHARS");
    if (!p3d::io::DirExists(rcharsDir)) return;

    for (s32 type = 0; type < 29; type++) {
        const char* name = g_charNameTable[type];
        if (!name || !name[0]) continue;

        char rrPath[256];
        std::snprintf(rrPath, sizeof(rrPath), "RCHARS/%s.RR", name);
        const std::string resolvedRrPath = p3d::io::ResolvePath(rrPath);
        if (!p3d::io::FileExists(resolvedRrPath)) continue;

        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                       [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

        // Character texture pages are at resource indices 3 and 5 (P3D 0xFF04 TexturePage)
        for (int resIdx : {3, 5}) {
            char texName[256];
            std::snprintf(texName, sizeof(texName), "%s_tex%d", lowerName.c_str(), resIdx);

            AssetEntry entry;
            entry.name = texName;
            entry.category = AssetCategory::Texture;
            entry.filePath = resolvedRrPath;
            entry.crc = p3dHash(texName);
            entry.resourceIndex = resIdx;
            m_entries.push_back(std::move(entry));
        }
    }
}

void AssetExporter::ScanLevelDirectory() {
    const char* levelDirs[] = { "rtarget", "RTARGET", nullptr };

    for (int d = 0; levelDirs[d]; d++) {
        if (!p3d::io::DirExists(levelDirs[d])) continue;

        for (const p3d::io::DirEntryInfo& entry : p3d::io::ListDirectory(levelDirs[d], /*recursive=*/false)) {
            if (entry.isDirectory) continue;

            std::string ext = std::filesystem::path(entry.name).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

            if (ext != ".lcf" && ext != ".gcf") continue;

            std::string stem = std::filesystem::path(entry.name).stem().string();
            std::transform(stem.begin(), stem.end(), stem.begin(),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

            u32 crc = p3dHash(stem.c_str());

            AssetEntry asset;
            asset.name = stem;
            asset.category = AssetCategory::StaticMesh;
            asset.filePath = p3d::io::NormalizeSeparators(entry.fullPath);
            asset.crc = crc;
            m_entries.push_back(std::move(asset));
        }
        break; // aliases differ only by case on the supported Windows build
    }
}

void AssetExporter::ScanTIMDirectory() {
    const char* timDirs[] = { "tim", "TIM", nullptr };

    for (int d = 0; timDirs[d]; d++) {
        if (!p3d::io::DirExists(timDirs[d])) continue;

        for (const p3d::io::DirEntryInfo& entry : p3d::io::ListDirectory(timDirs[d], /*recursive=*/true)) {
            if (entry.isDirectory) continue;

            std::string ext = std::filesystem::path(entry.name).extension().string();
            // Case-insensitive .tim check
            bool isTim = ext.size() == 4 &&
                std::tolower(static_cast<unsigned char>(ext[1])) == 't' &&
                std::tolower(static_cast<unsigned char>(ext[2])) == 'i' &&
                std::tolower(static_cast<unsigned char>(ext[3])) == 'm';

            if (!isTim) continue;

            std::string stem = std::filesystem::path(entry.name).stem().string();
            std::transform(stem.begin(), stem.end(), stem.begin(),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

            u32 crc = p3dHash(stem.c_str());

            AssetEntry asset;
            asset.name = stem;
            asset.category = AssetCategory::Texture;
            asset.filePath = p3d::io::NormalizeSeparators(entry.fullPath);
            asset.crc = crc;
            m_entries.push_back(std::move(asset));
        }
        break;
    }
}

void AssetExporter::ScanSoundDirectory() {
    const char* soundDirs[] = { "sound", "SOUND", nullptr };

    for (int d = 0; soundDirs[d]; d++) {
        if (!p3d::io::DirExists(soundDirs[d])) continue;

        for (const p3d::io::DirEntryInfo& entry : p3d::io::ListDirectory(soundDirs[d], /*recursive=*/true)) {
            if (entry.isDirectory) continue;

            std::string ext = std::filesystem::path(entry.name).extension().string();
            std::string lowerExt = ext;
            std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

            if (lowerExt != ".wax" && lowerExt != ".vag" && lowerExt != ".str" &&
                lowerExt != ".wap" && lowerExt != ".fag" && lowerExt != ".clp"
                && lowerExt != ".dlg") continue;

            std::string stem = std::filesystem::path(entry.name).stem().string();
            std::transform(stem.begin(), stem.end(), stem.begin(),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

            u32 crc = p3dHash(stem.c_str());

            AssetEntry asset;
            asset.name = stem;
            asset.category = AssetCategory::Sound;
            asset.filePath = p3d::io::NormalizeSeparators(entry.fullPath);
            asset.crc = crc;
            m_entries.push_back(std::move(asset));
        }
        break;
    }
}

void AssetExporter::ScanDataDirectory() {
    // Intentionally empty. JSON is only emitted for typed WDB game parameters,
    // which are exported as part of each level. Arbitrary FE/XC/SCR binaries are
    // not mislabeled as editable JSON.
}

// File I/O helpers

std::vector<u8> AssetExporter::ReadFileBytes(const std::string& path) {
    auto data = p3d::io::ReadFile(p3d::io::ResolvePath(path));
    return data ? std::move(*data) : std::vector<u8>{};
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb/stb_image_write.h"

static u32 HashAssetBytes(const u8* data, size_t size) {
    u32 hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static u32 PngCrc32(const u8* data, size_t size) {
    u32 crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static void AppendBE32(std::vector<u8>& bytes, u32 value) {
    bytes.push_back(static_cast<u8>(value >> 24));
    bytes.push_back(static_cast<u8>(value >> 16));
    bytes.push_back(static_cast<u8>(value >> 8));
    bytes.push_back(static_cast<u8>(value));
}

static bool WriteTaggedPng(const std::string& path, int width, int height, const void* rgba, int stride) {
    if (!rgba || width <= 0 || height <= 0
        || !stbi_write_png(path.c_str(), width, height, 4, rgba, stride)) return false;
    auto pngData = p3d::io::ReadFile(path);
    if (!pngData) return false;
    std::vector<u8> png = std::move(*pngData);
    if (png.size() < 33) return false;

    const u32 hash = HashAssetBytes(static_cast<const u8*>(rgba), static_cast<size_t>(height) * stride);
    char hashText[16];
    std::snprintf(hashText, sizeof(hashText), "%08x", hash);
    const std::string payload = std::string("rechanBaseline") + '\0' + hashText;
    std::vector<u8> chunk;
    AppendBE32(chunk, static_cast<u32>(payload.size()));
    const size_t crcStart = chunk.size();
    chunk.insert(chunk.end(), { 't', 'E', 'X', 't' });
    chunk.insert(chunk.end(), payload.begin(), payload.end());
    AppendBE32(chunk, PngCrc32(chunk.data() + crcStart, chunk.size() - crcStart));
    png.insert(png.begin() + 33, chunk.begin(), chunk.end());

    return p3d::io::WriteFile(path, png);
}

bool AssetExporter::ConvertTIMtoPNG(const std::vector<u8>& rawData,
                                    const std::string& outputPath,
                                    int* outWidth, int* outHeight) {
    TimImage* img = Tim::LoadFromMemory(rawData.data(), static_cast<u32>(rawData.size()));
    if (!img) {
        return false;
    }

    if (outWidth) *outWidth = img->width;
    if (outHeight) *outHeight = img->height;

    std::string dir = outputPath.substr(0, outputPath.find_last_of("/\\"));
    if (!dir.empty()) {
        p3d::io::CreateDirectories(dir);
    }

    const bool result = WriteTaggedPng(outputPath, img->width, img->height,
                                       reinterpret_cast<const void*>(img->rgba), img->width * 4);
    delete img;

    if (!result) {
        LOG("[AssetExporter] Failed to write PNG: %s", outputPath.c_str());
        return false;
    }

    LOG("[AssetExporter] Exported PNG: %s", outputPath.c_str());
    return true;
}

// --- Texture chunk pipeline ---

std::vector<TexChunkExport> AssetExporter::ParseP3DTexChunks(const u8* data, u32 size) {
    std::vector<TexChunkExport> result;
    if (!data || size < 6) return result;
    if (p3dReadU16LE(data) != 0xFF04) return result;

    u32 rootSize = p3dReadU32LE(data + 2);
    u32 end = std::min(rootSize, size);
    u32 pos = 6;

    while (pos + 6 <= end) {
        u16 id = p3dReadU16LE(data + pos);
        u32 chunkSize = p3dReadU32LE(data + pos + 2);
        if (chunkSize < 6 || pos + chunkSize > end) break;

        if (id == 0x6008) {
            u32 p = pos + 6;
            u32 dend = pos + chunkSize;

            if (p >= dend) { pos += chunkSize; continue; }
            u8 nameLen = data[p++];
            if (p + nameLen + 12 > dend) { pos += chunkSize; continue; }
            std::string texName(reinterpret_cast<const char*>(data + p), nameLen);
            p += nameLen;
            while (!texName.empty() && (texName.back() == ' ' || texName.back() == '\0'))
                texName.pop_back();
            if (texName.empty()) { pos += chunkSize; continue; }

            s16 rx = p3dReadS16LE(data + p); p += 2;
            s16 ry = p3dReadS16LE(data + p); p += 2;
            s16 rw = p3dReadS16LE(data + p); p += 2;
            s16 rh = p3dReadS16LE(data + p); p += 2;
            p += 4;

            u32 pixelBytes = (u32)rw * (u32)rh * 2;
            if (rw <= 0 || rh <= 0 || rw > 1024 || rh > 512 || p + pixelBytes > dend) {
                pos += chunkSize; continue;
            }

            TexChunkExport tex;
            tex.manifest.name = texName;
            tex.manifest.rx = rx;
            tex.manifest.ry = ry;
            tex.manifest.rw = rw;
            tex.manifest.rh = rh;
            // Assume depth=2 (16-bit direct) for standalone decode path
            tex.manifest.tx = static_cast<u8>(rx / 64);
            tex.manifest.ty = static_cast<u8>(ry / 256);
            tex.manifest.px = static_cast<s16>(rx - tex.manifest.tx * 64);
            tex.manifest.py = static_cast<s16>(ry - tex.manifest.ty * 256);
            tex.manifest.pw = rw;
            tex.manifest.ph = rh;
            tex.rgba8.resize((u32)rw * (u32)rh * 4);

            const u16* src = reinterpret_cast<const u16*>(data + p);
            u8* dst = tex.rgba8.data();
            u32 npx = (u32)rw * (u32)rh;
            for (u32 pxi = 0; pxi < npx; pxi++) {
                u16 v = src[pxi];
                dst[pxi * 4 + 0] = static_cast<u8>((v & 0x1F) << 3);
                dst[pxi * 4 + 1] = static_cast<u8>(((v >> 5) & 0x1F) << 3);
                dst[pxi * 4 + 2] = static_cast<u8>(((v >> 10) & 0x1F) << 3);
                dst[pxi * 4 + 3] = (v == 0) ? 0 : 0xFF;
            }

            result.push_back(std::move(tex));
        }
        pos += chunkSize;
    }
    return result;
}

// Scan a 0xFF04 P3D container for its 0x6122 (mapped) or 0x6120 (flat) STree
// chunk and parse it. No VRAM/world side effects, so safe to call from the
// export path while the game is running without disturbing live rendering.
static STreeData* ParseCharacterSkeleton(const u8* data, u32 size) {
    if (!data || size < 6 || p3dReadU16LE(data) != 0xFF04) return nullptr;
    u32 end = std::min(p3dReadU32LE(data + 2), size);
    u32 pos = 6;
    while (pos + 6 <= end) {
        u16 id = p3dReadU16LE(data + pos);
        u32 chunkSize = p3dReadU32LE(data + pos + 2);
        if (chunkSize < 6 || pos + chunkSize > end) break;
        if (id == 0x6122) return ParseSTreeChunk(data + pos + 6, chunkSize - 6, true);
        if (id == 0x6120) return ParseSTreeChunk(data + pos + 6, chunkSize - 6, false);
        pos += chunkSize;
    }
    return nullptr;
}

// Upload all 0x6008 chunks from a 0xFF04 P3D container to vram; record raw metadata.
// px/py/pw/ph/tx/ty are left zero — caller fills them after determining tpage depth.
static void UploadP3DTexToVRAM(const u8* data, u32 size, PsxVRAM& vram,
                               std::vector<TexManifest>& metaOut) {
    if (!data || size < 6 || p3dReadU16LE(data) != 0xFF04) return;
    u32 end = std::min(p3dReadU32LE(data + 2), size);
    u32 pos = 6;
    while (pos + 6 <= end) {
        u16 id = p3dReadU16LE(data + pos);
        u32 chunkSize = p3dReadU32LE(data + pos + 2);
        if (chunkSize < 6 || pos + chunkSize > end) break;
        if (id == 0x6008) {
            u32 p = pos + 6;
            u32 dend = pos + chunkSize;
            if (p >= dend) { pos += chunkSize; continue; }
            u8 nameLen = data[p++];
            if (p + nameLen + 12 > dend) { pos += chunkSize; continue; }
            std::string texName(reinterpret_cast<const char*>(data + p), nameLen);
            p += nameLen;
            // PStrings may be padded with trailing nulls/spaces for alignment
            // (matches ParseP3DStreamFull's stripping in skeleton.cpp) — a
            // mismatched " CLUT" suffix check or clutMap lookup otherwise
            // silently fails whenever a name carries trailing padding.
            while (!texName.empty() && (texName.back() == ' ' || texName.back() == '\0'))
                texName.pop_back();
            if (texName.empty()) { pos += chunkSize; continue; }
            s16 rx = p3dReadS16LE(data + p); p += 2;
            s16 ry = p3dReadS16LE(data + p); p += 2;
            s16 rw = p3dReadS16LE(data + p); p += 2;
            s16 rh = p3dReadS16LE(data + p); p += 2;
            p += 4;
            u32 pixelBytes = (u32)rw * (u32)rh * 2;
            if (rw <= 0 || rh <= 0 || rw > 1024 || rh > 512 || p + pixelBytes > dend) {
                pos += chunkSize; continue;
            }
            vram.Upload(rx, ry, rw, rh, data + p);
            TexManifest m;
            m.name = texName; m.rx = rx; m.ry = ry; m.rw = rw; m.rh = rh;
            metaOut.push_back(std::move(m));
        }
        pos += chunkSize;
    }
}

std::vector<TexManifest> AssetExporter::ExportTexChunks(
    const std::vector<TexChunkExport>& chunks, const std::string& outputDir) {
    p3d::io::CreateDirectories(outputDir);
    std::vector<TexManifest> manifests;

    for (const auto& chunk : chunks) {
        std::string pngPath = outputDir + "/" + chunk.manifest.name + ".png";
        if (WriteTaggedPng(pngPath, chunk.manifest.pw, chunk.manifest.ph,
            chunk.rgba8.data(), chunk.manifest.pw * 4)) {
            manifests.push_back(chunk.manifest);
            LOG("[AssetExporter] Tex: %s (%dx%d)", pngPath.c_str(),
                chunk.manifest.pw, chunk.manifest.ph);
        }
    }

    return manifests;
}

// --- GLB writer ---

static void GlbWriteU32(std::vector<u8>& buf, u32 v) {
    buf.push_back(v & 0xFF);
    buf.push_back((v >> 8) & 0xFF);
    buf.push_back((v >> 16) & 0xFF);
    buf.push_back((v >> 24) & 0xFF);
}
static void GlbWriteF32(std::vector<u8>& buf, float v) {
    u32 bits; memcpy(&bits, &v, 4); GlbWriteU32(buf, bits);
}
static void BufWriteU32(std::vector<u8>& buf, u32 v) {
    buf.push_back((u8)(v & 0xFF));
    buf.push_back((u8)((v >> 8) & 0xFF));
    buf.push_back((u8)((v >> 16) & 0xFF));
    buf.push_back((u8)((v >> 24) & 0xFF));
}

std::vector<MatPrimitive> AssetExporter::GroupByMaterial(
    const std::vector<PrimGeomVertex>& verts,
    const std::vector<u32>& indices,
    const std::vector<TexManifest>& manifests) {
    std::map<std::string, int> groupIdx;
    std::vector<MatPrimitive> groups;

    auto getGroup = [&](const std::string& name) -> MatPrimitive& {
        auto it = groupIdx.find(name);
        if (it == groupIdx.end()) {
            groupIdx[name] = (int)groups.size();
            groups.emplace_back();
            groups.back().textureName = name;
        }
        return groups[groupIdx[name]];
    };

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const PrimGeomVertex& v0 = verts[indices[i]];
        const PrimGeomVertex& v1 = verts[indices[i + 1]];
        const PrimGeomVertex& v2 = verts[indices[i + 2]];

        std::string texName;
        int mIdx = -1;

        if (v0.tpage >= 0.0f) {
            u16 tp = static_cast<u16>(static_cast<u32>(v0.tpage));
            u8  cur_tx = tp & 0xF;
            u8  cur_ty = (tp >> 4) & 1;
            int pu = static_cast<int>(v0.u);
            int pv = static_cast<int>(v0.v);
            for (int mi = 0; mi < (int)manifests.size(); mi++) {
                const TexManifest& m = manifests[mi];
                if (m.tx == cur_tx && m.ty == cur_ty &&
                    pu >= m.px && pu < m.px + m.pw &&
                    pv >= m.py && pv < m.py + m.ph) {
                    texName = m.name; mIdx = mi; break;
                }
            }
        }

        auto remapUV = [&](const PrimGeomVertex& v, float& ou, float& ov) {
            if (mIdx < 0) { ou = ov = 0.0f; return; }
            const TexManifest& m = manifests[mIdx];
            ou = (m.pw > 0) ? (static_cast<float>(v.u) - m.px) / static_cast<float>(m.pw) : 0.0f;
            ov = (m.ph > 0) ? (static_cast<float>(v.v) - m.py) / static_cast<float>(m.ph) : 0.0f;
        };

        float u0, uv0, u1, uv1, u2, uv2;
        remapUV(v0, u0, uv0);
        remapUV(v1, u1, uv1);
        remapUV(v2, u2, uv2);

        MatPrimitive& g = getGroup(texName);

        auto push = [&](const PrimGeomVertex& v, float nu, float nv) {
            PrimGeomVertex pv = v; pv.u = nu; pv.v = nv;
            g.indices.push_back(static_cast<u32>(g.verts.size()));
            g.verts.push_back(pv);
        };
        push(v0, u0, uv0);
        push(v1, u1, uv1);
        push(v2, u2, uv2);
    }
    return groups;
}

bool AssetExporter::WriteGLB(const std::string& path, const std::string& meshName,
                             const std::vector<MatPrimitive>& prims,
                             const std::string& textureUriPrefix,
                             const STreeData* skeleton) {
    std::vector<const MatPrimitive*> valid;
    for (const auto& p : prims) {
        if (!p.verts.empty() && p.indices.size() >= 3) valid.push_back(&p);
    }
    if (valid.empty()) return false;

    struct PrimBin {
        u32 posOff, posBytes, uvOff, uvBytes, colOff, colBytes, idxOff, idxBytes;
        u32 jointOff = 0, jointBytes = 0, weightOff = 0, weightBytes = 0;
        u32 vCount, iCount;
        float mnPos[3], mxPos[3];
    };

    std::vector<u8> bin;
    std::vector<PrimBin> pb;
    const bool skinned = skeleton && skeleton->joints && skeleton->numJoints > 0;

    for (const auto* pp : valid) {
        PrimBin b;
        b.vCount = (u32)pp->verts.size();
        b.iCount = (u32)pp->indices.size();
        b.mnPos[0] = b.mxPos[0] = pp->verts[0].x / 4096.0f;
        b.mnPos[1] = b.mxPos[1] = pp->verts[0].y / 4096.0f;
        b.mnPos[2] = b.mxPos[2] = pp->verts[0].z / 4096.0f;

        b.posOff = (u32)bin.size();
        for (const auto& v : pp->verts) {
            float x = v.x / 4096.0f, y = v.y / 4096.0f, z = v.z / 4096.0f;
            GlbWriteF32(bin, x); GlbWriteF32(bin, y); GlbWriteF32(bin, z);
            if (x < b.mnPos[0]) b.mnPos[0] = x; if (x > b.mxPos[0]) b.mxPos[0] = x;
            if (y < b.mnPos[1]) b.mnPos[1] = y; if (y > b.mxPos[1]) b.mxPos[1] = y;
            if (z < b.mnPos[2]) b.mnPos[2] = z; if (z > b.mxPos[2]) b.mxPos[2] = z;
        }
        b.posBytes = (u32)bin.size() - b.posOff;

        b.uvOff = (u32)bin.size();
        for (const auto& v : pp->verts) { GlbWriteF32(bin, v.u); GlbWriteF32(bin, v.v); }
        b.uvBytes = (u32)bin.size() - b.uvOff;

        b.colOff = (u32)bin.size();
        for (const auto& v : pp->verts) {
            GlbWriteF32(bin, v.r); GlbWriteF32(bin, v.g); GlbWriteF32(bin, v.b);
        }
        b.colBytes = (u32)bin.size() - b.colOff;

        if (skinned) {
            b.jointOff = (u32)bin.size();
            for (const auto& v : pp->verts) {
                const u16 joint = static_cast<u16>(std::min<u32>(v.jointIndex, skeleton->numJoints - 1));
                bin.push_back(static_cast<u8>(joint & 0xFFu)); bin.push_back(static_cast<u8>(joint >> 8));
                for (int component = 1; component < 4; ++component) { bin.push_back(0); bin.push_back(0); }
            }
            b.jointBytes = (u32)bin.size() - b.jointOff;

            b.weightOff = (u32)bin.size();
            for (size_t vertexIndex = 0; vertexIndex < pp->verts.size(); ++vertexIndex) {
                GlbWriteF32(bin, 1.0f); GlbWriteF32(bin, 0.0f);
                GlbWriteF32(bin, 0.0f); GlbWriteF32(bin, 0.0f);
            }
            b.weightBytes = (u32)bin.size() - b.weightOff;
        }

        b.idxOff = (u32)bin.size();
        for (u32 idx : pp->indices) GlbWriteU32(bin, idx);
        b.idxBytes = (u32)bin.size() - b.idxOff;

        pb.push_back(b);
    }

    u32 inverseBindOffset = 0;
    u32 inverseBindBytes = 0;
    std::vector<Mat4> jointWorld;
    if (skinned) {
        jointWorld.resize(skeleton->numJoints);
        skeleton->ComputeWorldMatrices(jointWorld.data());
        inverseBindOffset = static_cast<u32>(bin.size());
        for (const Mat4& engineWorld : jointWorld) {
            Mat4 world = engineWorld;
            world.m[12] /= 4096.0f; world.m[13] /= 4096.0f; world.m[14] /= 4096.0f;
            Mat4 inverse;
            // Character joints contain rigid transforms: inverse rotation is
            // transpose, inverse translation is -R^T t.
            inverse.m[0] = world.m[0]; inverse.m[1] = world.m[4]; inverse.m[2] = world.m[8];
            inverse.m[4] = world.m[1]; inverse.m[5] = world.m[5]; inverse.m[6] = world.m[9];
            inverse.m[8] = world.m[2]; inverse.m[9] = world.m[6]; inverse.m[10] = world.m[10];
            inverse.m[12] = -(inverse.m[0] * world.m[12] + inverse.m[4] * world.m[13] + inverse.m[8] * world.m[14]);
            inverse.m[13] = -(inverse.m[1] * world.m[12] + inverse.m[5] * world.m[13] + inverse.m[9] * world.m[14]);
            inverse.m[14] = -(inverse.m[2] * world.m[12] + inverse.m[6] * world.m[13] + inverse.m[10] * world.m[14]);
            for (float value : inverse.m) GlbWriteF32(bin, value);
        }
        inverseBindBytes = static_cast<u32>(bin.size()) - inverseBindOffset;
    }
    while (bin.size() % 4) bin.push_back(0);
    u32 binSize = (u32)bin.size();
    const u32 baselineHash = HashAssetBytes(bin.data(), bin.size());
    char baselineText[16];
    std::snprintf(baselineText, sizeof(baselineText), "%08x", baselineHash);

    // Unique texture list
    std::vector<std::string> texNames;
    std::map<std::string, int> texIdx;
    for (const auto* pp : valid) {
        if (!pp->textureName.empty() && texIdx.find(pp->textureName) == texIdx.end()) {
            texIdx[pp->textureName] = (int)texNames.size();
            texNames.push_back(pp->textureName);
        }
    }

    auto esc = [](const std::string& s) {
        std::string o; for (char c : s) { if (c == '"') o += "\\\""; else o += c; } return o;
    };
    auto ff = [](float v) { char buf[32]; snprintf(buf, sizeof(buf), "%.6g", v); return std::string(buf); };

    std::string J;
    J.reserve(8192);
    J += "{\"asset\":{\"version\":\"2.0\",\"generator\":\"rechan\"},\"scene\":0,";
    J += "\"scenes\":[{\"name\":\"Scene\",\"nodes\":[0";
    if (skinned) for (u32 joint = 0; joint < skeleton->numJoints; ++joint) J += "," + std::to_string(joint + 1);
    J += "]}],";
    J += "\"nodes\":[{\"name\":\"" + esc(meshName) + "\",\"mesh\":0";
    if (skinned) J += ",\"skin\":0";
    J += "}";
    if (skinned) {
        // Blender round-trips per-node custom properties (bones/objects have
        // a real Custom Properties slot) but has no Blender data-block that
        // corresponds to a glTF *skin*, so anything placed on the skin's own
        // extras gets silently dropped on export. Carry the jointOrderMap on
        // joint 0's extras instead, where it's proven to survive.
        std::string jointOrderMapJson;
        if (skeleton->jointOrderMap && skeleton->numMapEntries > 0) {
            jointOrderMapJson = ",\"rchJointOrderMap\":[";
            for (u32 param = 0; param < skeleton->numMapEntries; ++param) {
                const u32 jointIdx = skeleton->jointOrderMap[param];
                const u32 nameUID = (jointIdx < skeleton->numJoints)
                    ? skeleton->joints[jointIdx].nameUID : 0;
                jointOrderMapJson += std::to_string(nameUID);
                if (param + 1 < skeleton->numMapEntries) jointOrderMapJson += ',';
            }
            jointOrderMapJson += "]";
        }

        for (u32 joint = 0; joint < skeleton->numJoints; ++joint) {
            const STreeJoint& source = skeleton->joints[joint];
            std::string boneName = source.name[0] ? source.name
                : ("Joint_" + std::to_string(joint));
            char extras[192];
            std::snprintf(extras, sizeof(extras),
                          "{\"rchNameUID\":%u,\"rchFlags\":%u,\"rchCapture\":%d,"
                          "\"rchTx\":%d,\"rchTy\":%d,\"rchTz\":%d,"
                          "\"rchRx\":%d,\"rchRy\":%d,\"rchRz\":%d",
                          source.nameUID, source.flags, source.captureBufferIdx,
                          source.translationX, source.translationY, source.translationZ,
                          source.rotationX, source.rotationY, source.rotationZ);
            std::string extrasJson = extras;
            if (joint == 0) extrasJson += jointOrderMapJson;
            extrasJson += "}";
            Mat4 world = jointWorld[joint];
            world.m[12] /= 4096.0f; world.m[13] /= 4096.0f; world.m[14] /= 4096.0f;
            J += ",{\"name\":\"" + esc(boneName) + "\",\"extras\":" + extrasJson + ",\"matrix\":[";
            for (int component = 0; component < 16; ++component) {
                J += ff(world.m[component]);
                if (component != 15) J += ',';
            }
            J += "]}";
        }
    }
    J += "],";
    J += "\"meshes\":[{\"name\":\"" + esc(meshName)
        + "\",\"extras\":{\"rechanBaseline\":\"" + baselineText + "\"},\"primitives\":[";
    const u32 attributesPerPrimitive = skinned ? 6u : 4u;
    for (size_t i = 0; i < valid.size(); i++) {
        u32 ba = (u32)i * attributesPerPrimitive;
        J += "{\"attributes\":{\"POSITION\":" + std::to_string(ba)
            + ",\"TEXCOORD_0\":" + std::to_string(ba + 1)
            + ",\"COLOR_0\":" + std::to_string(ba + 2);
        if (skinned) {
            J += ",\"JOINTS_0\":" + std::to_string(ba + 3)
                + ",\"WEIGHTS_0\":" + std::to_string(ba + 4);
        }
        J += "},\"indices\":" + std::to_string(ba + attributesPerPrimitive - 1)
            + ",\"material\":" + std::to_string(i) + "}";
        if (i + 1 < valid.size()) J += ",";
    }
    J += "]}],";

    J += "\"materials\":[";
    for (size_t i = 0; i < valid.size(); i++) {
        const std::string& tn = valid[i]->textureName;
        J += "{\"name\":\"" + esc(tn.empty() ? "untextured" : tn) + "\",\"pbrMetallicRoughness\":{";
        if (!tn.empty() && texIdx.count(tn))
            J += "\"baseColorTexture\":{\"index\":" + std::to_string(texIdx[tn]) + "},";
        else
            J += "\"baseColorFactor\":[0.7,0.7,0.7,1.0],";
        J += "\"metallicFactor\":0.0,\"roughnessFactor\":1.0},\"doubleSided\":true}";
        if (i + 1 < valid.size()) J += ",";
    }
    J += "]";

    if (skinned) {
        J += ",\"skins\":[{\"name\":\"" + esc(meshName) + "_skin\",\"joints\":[";
        for (u32 joint = 0; joint < skeleton->numJoints; ++joint) {
            J += std::to_string(joint + 1);
            if (joint + 1 < skeleton->numJoints) J += ',';
        }
        J += "]";
        J += ",\"inverseBindMatrices\":" + std::to_string(valid.size() * attributesPerPrimitive) + "}]";
    }

    if (!texNames.empty()) {
        J += ",\"textures\":[";
        for (size_t i = 0; i < texNames.size(); i++) {
            J += "{\"name\":\"" + esc(texNames[i]) + "\",\"source\":" + std::to_string(i) + "}";
            if (i + 1 < texNames.size()) J += ",";
        }
        J += "],\"images\":[";
        for (size_t i = 0; i < texNames.size(); i++) {
            J += "{\"uri\":\"" + esc(textureUriPrefix + texNames[i] + ".png") + "\"}";
            if (i + 1 < texNames.size()) J += ",";
        }
        J += "]";
    }

    J += ",\"accessors\":[";
    for (size_t i = 0; i < valid.size(); i++) {
        const PrimBin& b = pb[i]; u32 bv = (u32)i * attributesPerPrimitive;
        J += "{\"bufferView\":" + std::to_string(bv) + ",\"componentType\":5126,\"count\":" + std::to_string(b.vCount)
            + ",\"type\":\"VEC3\",\"min\":[" + ff(b.mnPos[0]) + "," + ff(b.mnPos[1]) + "," + ff(b.mnPos[2])
            + "],\"max\":[" + ff(b.mxPos[0]) + "," + ff(b.mxPos[1]) + "," + ff(b.mxPos[2]) + "]},";
        J += "{\"bufferView\":" + std::to_string(bv + 1) + ",\"componentType\":5126,\"count\":" + std::to_string(b.vCount) + ",\"type\":\"VEC2\"},";
        J += "{\"bufferView\":" + std::to_string(bv + 2) + ",\"componentType\":5126,\"count\":" + std::to_string(b.vCount) + ",\"type\":\"VEC3\"},";
        if (skinned) {
            J += "{\"bufferView\":" + std::to_string(bv + 3) + ",\"componentType\":5123,\"count\":" + std::to_string(b.vCount) + ",\"type\":\"VEC4\"},";
            J += "{\"bufferView\":" + std::to_string(bv + 4) + ",\"componentType\":5126,\"count\":" + std::to_string(b.vCount) + ",\"type\":\"VEC4\"},";
        }
        J += "{\"bufferView\":" + std::to_string(bv + attributesPerPrimitive - 1) + ",\"componentType\":5125,\"count\":" + std::to_string(b.iCount) + ",\"type\":\"SCALAR\"}";
        if (i + 1 < valid.size()) J += ",";
    }
    if (skinned) {
        J += ",{\"bufferView\":" + std::to_string(valid.size() * attributesPerPrimitive)
            + ",\"componentType\":5126,\"count\":" + std::to_string(skeleton->numJoints)
            + ",\"type\":\"MAT4\"}";
    }
    J += "],\"bufferViews\":[";
    for (size_t i = 0; i < valid.size(); i++) {
        const PrimBin& b = pb[i];
        J += "{\"buffer\":0,\"byteOffset\":" + std::to_string(b.posOff) + ",\"byteLength\":" + std::to_string(b.posBytes) + ",\"target\":34962},";
        J += "{\"buffer\":0,\"byteOffset\":" + std::to_string(b.uvOff) + ",\"byteLength\":" + std::to_string(b.uvBytes) + ",\"target\":34962},";
        J += "{\"buffer\":0,\"byteOffset\":" + std::to_string(b.colOff) + ",\"byteLength\":" + std::to_string(b.colBytes) + ",\"target\":34962},";
        if (skinned) {
            J += "{\"buffer\":0,\"byteOffset\":" + std::to_string(b.jointOff) + ",\"byteLength\":" + std::to_string(b.jointBytes) + ",\"target\":34962},";
            J += "{\"buffer\":0,\"byteOffset\":" + std::to_string(b.weightOff) + ",\"byteLength\":" + std::to_string(b.weightBytes) + ",\"target\":34962},";
        }
        J += "{\"buffer\":0,\"byteOffset\":" + std::to_string(b.idxOff) + ",\"byteLength\":" + std::to_string(b.idxBytes) + ",\"target\":34963}";
        if (i + 1 < valid.size()) J += ",";
    }
    if (skinned) {
        J += ",{\"buffer\":0,\"byteOffset\":" + std::to_string(inverseBindOffset)
            + ",\"byteLength\":" + std::to_string(inverseBindBytes) + "}";
    }
    J += "],\"buffers\":[{\"byteLength\":" + std::to_string(binSize) + "}]}";

    while (J.size() % 4) J += ' ';
    u32 jsonSize = (u32)J.size();
    u32 totalSize = 12 + 8 + jsonSize + 8 + binSize;

    p3d::io::CreateDirectories(std::filesystem::path(path).parent_path().string());

    std::vector<u8> glb;
    glb.reserve(totalSize);
    glb.insert(glb.end(), { 'g', 'l', 'T', 'F' });
    BufWriteU32(glb, 2);
    BufWriteU32(glb, totalSize);
    BufWriteU32(glb, jsonSize);
    BufWriteU32(glb, 0x4E4F534A);
    glb.insert(glb.end(), J.c_str(), J.c_str() + jsonSize);
    BufWriteU32(glb, binSize);
    BufWriteU32(glb, 0x004E4942);
    glb.insert(glb.end(), bin.begin(), bin.end());

    if (!p3d::io::WriteFile(path, glb)) return false;

    LOG("[AssetExporter] GLB: %s (%zu prim(s))", path.c_str(), valid.size());
    return true;
}

// --- Character export (RR → GLB + named PNGs) ---

bool AssetExporter::ExportCharacter(const std::vector<u8>& rawData,
                                    const std::string& outputDir,
                                    const std::string& charName) {
    if (rawData.size() < 16) return false;
    const u8* rrData = rawData.data();
    const u32 rrSize = (u32)rawData.size();

    u32 entryCount = p3dReadU32LE(rrData + 8) / 8;
    if (entryCount < 2 || entryCount > 4096) return false;

    auto getRes = [&](u32 idx, const u8*& rdata, u32& rsize) -> bool {
        if (idx >= entryCount) return false;
        u32 off = p3dReadU32LE(rrData + idx * 8);
        u32 sz = p3dReadU32LE(rrData + idx * 8 + 4) >> 8;
        if (off == 0 || sz == 0 || off + sz > rrSize) return false;
        rdata = rrData + off; rsize = sz; return true;
    };

    // Raw tPrimGeom mesh data lives at resource index 2, paired with the
    // textures+skeleton P3D container at resource index 3 (see CharMgr::
    // LoadCharacter: rrIdx = resourceSlot*2+3, mesh = rrIdx-1). Resource 0
    // is unrelated to the mesh, so scanning it (as before) always misses.
    std::vector<PrimGeomVertex> verts; std::vector<u32> idxs;
    STreeData* exportSkeleton = nullptr;
    {
        const u8* rd0; u32 rs0;
        if (getRes(2, rd0, rs0)) {
            // Some characters (e.g. the player) pack tPrimGeom data
            // contiguously past resource 2's own declared size, into
            // resource 3's space (CharMgr combines both sizes for this
            // case). Extend the scan window up to resource 3's offset to
            // capture the full vertex/poly/prim tables.
            if (3 < entryCount) {
                u32 off2 = p3dReadU32LE(rrData + 2 * 8);
                u32 off3 = p3dReadU32LE(rrData + 3 * 8);
                if (off3 > off2) rs0 = std::max(rs0, off3 - off2);
            }
            // The "PRIM" magic sits at offset +12 into the tPrimGeom block
            // (see IsCharacterPrimGeom in charmgr.cpp), so the block itself
            // starts 12 bytes before the matched text, not at the match.
            const u8* primData = nullptr; u32 primSize = 0;
            for (u32 i = 12; i + 0x18 <= rs0; i++) {
                if (rd0[i] == 'P' && rd0[i + 1] == 'R' && rd0[i + 2] == 'I' && rd0[i + 3] == 'M') {
                    const u8* blockStart = rd0 + i - 12;
                    u16 nv = p3dReadU16LE(blockStart + 0x14), np = p3dReadU16LE(blockStart + 0x16);
                    if (nv > 0 && nv < 65500 && np>0 && np < 65500) { primData = blockStart; primSize = rs0 - (i - 12); break; }
                }
            }
            if (primData) {
                // tPrimGeom vertices are stored per-joint in that joint's
                // local space. Without applying each joint's world matrix,
                // every body part renders on top of its own joint origin and
                // the whole character collapses into a ball at (0,0,0).
                // Resource 3 (the same container used for textures above)
                // also holds the STree skeleton; resource 8 is the idle
                // animation used to seed a sensible bind pose.
                STreeData* skeleton = nullptr;
                Mat4* jointMatrices = nullptr;
                std::vector<u32> vertJointMap;
                const u8* rd3; u32 rs3;
                if (getRes(3, rd3, rs3)) {
                    skeleton = ParseCharacterSkeleton(rd3, rs3);
                }
                if (skeleton && skeleton->numJoints > 0) {
                    const u8* idleBuf; u32 idleSize;
                    if (getRes(8, idleBuf, idleSize)) {
                        ApplyAnimFrame0(skeleton, idleBuf, idleSize);
                    }
                    jointMatrices = new Mat4[skeleton->numJoints];
                    skeleton->ComputeWorldMatrices(jointMatrices);

                    u16 numVertsHdr = p3dReadU16LE(primData + 0x14);
                    vertJointMap.assign(numVertsHdr, 0u);
                    for (u32 j = 0; j < skeleton->numJoints; j++) {
                        const STreeJoint& jt = skeleton->joints[j];
                        if (!(jt.flags & STF_HAS_MESH)) continue;
                        for (u32 v = 0; v < jt.primGeomCount; v++) {
                            u32 vi = jt.primGeomStartIdx + v;
                            if (vi < numVertsHdr) vertJointMap[vi] = j;
                        }
                    }
                }

                if (!ExtractPrimGeomVerts(primData, primSize, verts, idxs,
                    skeleton ? &vertJointMap : nullptr,
                    jointMatrices)) {
                    LOG("[AssetExporter] ExtractPrimGeomVerts failed: %s", charName.c_str());
                    verts.clear(); idxs.clear();
                }

                delete[] jointMatrices;
                exportSkeleton = skeleton;
            }
            else {
                LOG("[AssetExporter] No tPrimGeom: %s (texture-only)", charName.c_str());
            }
        }
    }

    // Colour depth is a VRAM-page-level property, so it's consistent across
    // every CLUT used on a given page — collect it from any vertex that
    // references that page.
    std::map<std::pair<u8, u8>, int> pageDepth;
    for (const auto& v : verts) {
        if (v.tpage < 0.0f) continue;
        u16 tp = static_cast<u16>(static_cast<u32>(v.tpage));
        u8 ttx = tp & 0xF, tty = (tp >> 4) & 1;
        pageDepth[{ttx, tty}] = (tp >> 7) & 3;
    }

    // Resources 3 and 5 are each an *independent* virtual VRAM snapshot with
    // their own self-contained coordinate space — they are NOT two regions
    // of one shared VRAM. Resource 5's chunks commonly reuse the very same
    // raw (rx,ry) addresses as resource 3's (both are similarly-laid-out
    // texture sets), so uploading both into a single shared PsxVRAM lets
    // resource 5 silently clobber resource 3's pixel/CLUT data wherever they
    // overlap — producing visible noise in the resulting decode. Process
    // each resource against its own fresh VRAM and CLUT map instead. Only
    // resource 3 is actually paired with the loaded mesh (resource 2), so
    // geometry-vote-based CLUT resolution only makes sense there; resource 5
    // resolves purely via its own "{name} CLUT" stream pairing.
    std::vector<u8> page(256 * 256 * 4);
    std::vector<TexChunkExport> decoded;
    for (u32 ri : { 3u, 5u }) {
        const u8* rd; u32 rs;
        if (!getRes(ri, rd, rs)) continue;

        PsxVRAM vram;
        std::vector<TexManifest> rawMeta;
        UploadP3DTexToVRAM(rd, rs, vram, rawMeta);

        // Build CLUT fallback map from CLUT chunks (rh==1, named "{tex} CLUT")
        std::map<std::string, std::pair<u16, int>> clutMap;
        for (const auto& m : rawMeta) {
            if (m.rh != 1) continue;
            int dep = (m.rw == 256) ? 1 : 0;
            u16 cba = static_cast<u16>(m.rx / 16) | static_cast<u16>(m.ry << 6);
            std::string base = m.name;
            if (base.size() > 5 && base.substr(base.size() - 5) == " CLUT")
                base.erase(base.size() - 5);
            clutMap[base] = { cba, dep };
        }

        for (auto& m : rawMeta) {
            if (m.rh == 1) continue;  // skip CLUT entries
            u8 ttx = static_cast<u8>(m.rx / 64), tty = static_cast<u8>(m.ry / 256);

            auto depthIt = pageDepth.find({ ttx, tty });
            int depth = (depthIt != pageDepth.end()) ? depthIt->second : 2;
            int ppw = (depth == 0) ? 4 : (depth == 1) ? 2 : 1;
            s16 px = static_cast<s16>((m.rx - ttx * 64) * ppw);
            s16 py = static_cast<s16>(m.ry - tty * 256);
            s16 pw = static_cast<s16>(m.rw * ppw);
            s16 ph = m.rh;

            // Find the CLUT most commonly referenced by vertices whose UV
            // falls within this chunk's own pixel rect on this page — only
            // meaningful for resource 3 (paired with the loaded mesh).
            std::map<u16, int> cbaVotes;
            if (ri == 3) {
                for (const auto& v : verts) {
                    if (v.tpage < 0.0f) continue;
                    u16 vtp = static_cast<u16>(static_cast<u32>(v.tpage));
                    u8 vtx = vtp & 0xF, vty = (vtp >> 4) & 1;
                    if (vtx != ttx || vty != tty) continue;
                    int vu = static_cast<int>(v.u), vv = static_cast<int>(v.v);
                    if (vu < px || vu >= px + pw || vv < py || vv >= py + ph) continue;
                    cbaVotes[static_cast<u16>(static_cast<u32>(v.cba))]++;
                }
            }

            u16 cb; bool haveCb = false;
            if (!cbaVotes.empty()) {
                int bestCount = -1;
                for (const auto& kv : cbaVotes) {
                    if (kv.second > bestCount) { bestCount = kv.second; cb = kv.first; haveCb = true; }
                }
            }
            if (!haveCb) {
                auto clit = clutMap.find(m.name);
                if (clit != clutMap.end()) {
                    cb = clit->second.first;
                    depth = clit->second.second;
                    ppw = (depth == 0) ? 4 : (depth == 1) ? 2 : 1;
                    px = static_cast<s16>((m.rx - ttx * 64) * ppw);
                    py = static_cast<s16>(m.ry - tty * 256);
                    pw = static_cast<s16>(m.rw * ppw);
                }
                else {
                    cb = 0;
                }
            }
            u16 tp = static_cast<u16>(ttx | (tty << 4) | (depth << 7));

            m.tx = ttx; m.ty = tty;
            m.px = px; m.py = py; m.pw = pw; m.ph = ph;
            vram.DecodePage(tp, cb, page.data());
            TexChunkExport tex;
            tex.manifest = m;
            tex.rgba8.resize(static_cast<size_t>(m.pw) * m.ph * 4);
            for (int row = 0; row < m.ph; row++) {
                memcpy(tex.rgba8.data() + row * m.pw * 4,
                       page.data() + static_cast<size_t>(m.py + row) * 256 * 4 + m.px * 4,
                       static_cast<size_t>(m.pw) * 4);
            }
            decoded.push_back(std::move(tex));
        }
    }

    auto manifests = ExportTexChunks(decoded, outputDir);
    if (verts.empty()) {
        LOG("[AssetExporter] No geometry for %s (textures only)", charName.c_str());
        delete exportSkeleton;
        return !manifests.empty();
    }
    auto groups = GroupByMaterial(verts, idxs, manifests);
    const bool wrote = WriteGLB(outputDir + "/" + charName + ".glb", charName,
                                groups, "", exportSkeleton) || !manifests.empty();
    delete exportSkeleton;
    return wrote;
}


#include "snd/rsdformat.h"
#include "snd/soundid.h"

static const char* kRsdSampleNames[] = {
    "land_heavy",   // 0
    "land_med",      // 1
    "land_light",    // 2
    "footstep_cloth",// 3
    "footstep_wood", // 4
    "footstep_metal",// 5
    "footstep_concrete", // 6
    "footstep_gravel",   // 7
    "silent",        // 8
    "impact_heavy",  // 9
    "impact_med",    // 10
    "impact_light",  // 11
    "jingle_0",      // 12
    "jingle_1",      // 13
    "jingle_2",      // 14
    "woosh_heavy",   // 15
    "woosh_med",     // 16
    "land_soft0",    // 17
    "land_soft1",    // 18
    "land_med0",     // 19
    "land_med1",     // 20
    "fall_heavy0",   // 21
    "fall_heavy1",   // 22
    "fall_med0",     // 23
    "footstep_soft0",// 24
    "menu_beep",     // 25
    "punch_heavy",   // 26
    "footstep0",     // 27
    "footstep1",     // 28
    "whoosh_med",    // 29
    "impact_hard",   // 30
    "block",         // 31
    "punch_lite0",   // 32
    "punch_lite1",   // 33
    "punch_med0",    // 34
    "kick_lite0",    // 35
    "hit_heavy0",    // 36
    "hit_heavy1",    // 37
    "hit_med0",      // 38
    "hit_med1",      // 39
    "hit_lite0",     // 40
    "hit_lite1",     // 41
    "stun_heavy",    // 42
    "jingle_3",      // 43
    "jingle_4",      // 44
    "back_beep",     // 45
    "woosh_lite0",   // 46
    "fall_med0",     // 47
    "env_step0",     // 48
    "env_step1",     // 49
    "env_step2",     // 50
    "env_step3",     // 51
    "env_step4",     // 52
    "env_step5",     // 53
    "env_step6",     // 54
    "break0",        // 55
    "env_hit0",      // 56
    "back_beep_l",   // 57
    "env_hit1",      // 58
    "env_hit2",      // 59
    "env_amb0",      // 60
    "env_amb1",      // 61
    "break1",        // 62
    "env_amb2",      // 63
    "env_amb3",      // 64
    "env_amb4",      // 65
    "confirm",       // 66
    "hit_heavy2",    // 67
    "explode",       // 68
    "select_beep",   // 69
    "env_amb5",      // 70
};

static bool WritePCMWav(const std::string& path, const std::vector<s16>& pcm, u32 sampleRate, u16 channels) {
    const u32 dataSize = static_cast<u32>(pcm.size() * sizeof(s16));
    const u16 bitsPerSample = 16;
    const u32 byteRate = sampleRate * channels * (bitsPerSample / 8);
    const u16 blockAlign = channels * (bitsPerSample / 8);

    std::vector<u8> buf;
    buf.reserve(44 + dataSize);
    auto writeBytes = [&](const void* data, size_t size) {
        const u8* p = reinterpret_cast<const u8*>(data);
        buf.insert(buf.end(), p, p + size);
    };
    auto writeU32 = [&](u32 v) { writeBytes(&v, 4); };
    auto writeU16 = [&](u16 v) { writeBytes(&v, 2); };

    writeBytes("RIFF", 4);
    writeU32(36 + dataSize);
    writeBytes("WAVE", 4);
    writeBytes("fmt ", 4);
    writeU32(16);
    writeU16(1);
    writeU16(channels);
    writeU32(sampleRate);
    writeU32(byteRate);
    writeU16(blockAlign);
    writeU16(bitsPerSample);
    writeBytes("data", 4);
    writeU32(dataSize);
    writeBytes(pcm.data(), dataSize);

    return p3d::io::WriteFile(path, buf);
}

bool AssetExporter::ConvertWAVtoWAV(const std::vector<u8>& rawData,
                                    const std::string& outputPath,
                                    const std::string& sourceExt) {
    std::string dir = outputPath.substr(0, outputPath.find_last_of("/\\"));
    if (!dir.empty()) {
        p3d::io::CreateDirectories(dir);
    }

    std::string lowerExt = sourceExt;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

    // None of these formats carry a reliable magic number: VAG's "VAGp" tag
    // is solid, but WAX's tag values (16/17/32) are just small integers that
    // collide with CLP/FAG's "[u32 count][u32 offsets...]" header shape
    // (e.g. a CLP with exactly 16 clips reads identically to a WAX
    // TAG_ADPCM_BLOCK chunk), and CLP vs FAG are mutually ambiguous by
    // content alone. Trust the file extension whenever the caller knows it;
    // only fall back to content-sniffing for direct/standalone calls.
    bool isVag = false, isWax = false, isClp = false, isFag = false;
    if (lowerExt == ".vag") {
        isVag = true;
    }
    else if (lowerExt == ".wax") {
        isWax = true;
    }
    else if (lowerExt == ".clp") {
        isClp = true;
    }
    else if (lowerExt == ".fag") {
        isFag = true;
    }
    else {
        isVag = rawData.size() >= 4 &&
            rawData[0] == 'V' && rawData[1] == 'A' && rawData[2] == 'G' && rawData[3] == 'p';

        isWax = !isVag && rawData.size() >= 8 &&
            (*(const u32*)(rawData.data()) == RsdFormat::TAG_HEADER ||
             *(const u32*)(rawData.data()) == RsdFormat::TAG_ADPCM_BLOCK ||
             *(const u32*)(rawData.data()) == RsdFormat::TAG_SAMPLE_TABLE);

        if (!isVag && !isWax && rawData.size() >= 4) {
            isClp = [&]() {
                const u32 numClips = *(const u32*)(rawData.data());
                return numClips >= 1 && numClips <= 256 &&
                    rawData.size() >= 4u + numClips * 8u;
            }();

            // FAG detection: must not be VAG/WAX/CLP, and first u32 must be a reasonable song count (1-64)
            // AND the header structure must be valid (numSongs * 8 + 4 <= fileSize)
            if (!isClp && rawData.size() >= 8) {
                const u32 numSongs = *(const u32*)(rawData.data());
                if (numSongs >= 1 && numSongs <= 64 &&
                    rawData.size() >= 4 + numSongs * 8 + 2) {
                    isFag = true;
                }
            }
        }
    }

    if (isFag) {
        RsdFormat::FagTrack track = RsdFormat::LoadFag(rawData.data(), static_cast<u32>(rawData.size()));
        if (!track.pcmData.empty()) {
            return WritePCMWav(outputPath, track.pcmData, track.sampleRate, track.channels);
        }
    }

    if (isClp) {
        RsdFormat::ClpBank bank = RsdFormat::LoadClp(rawData.data(), static_cast<u32>(rawData.size()));

        std::string bankDir = outputPath;
        const size_t extPos2 = bankDir.rfind(".wav");
        if (extPos2 != std::string::npos) {
            bankDir.erase(extPos2);
        }
        p3d::io::CreateDirectories(bankDir);

        s32 exported = 0;
        for (u32 i = 0; i < static_cast<u32>(bank.clips.size()); i++) {
            if (bank.clips[i].pcmData.empty()) continue;
            char clipPath[512];
            std::snprintf(clipPath, sizeof(clipPath), "%s/clip%02u.wav", bankDir.c_str(), i);
            if (WritePCMWav(clipPath, bank.clips[i].pcmData, 11025, 1)) {
                exported++;
            }
        }

        LOG("[AssetExporter] Exported %d clips from CLP: %s", exported, outputPath.c_str());
        return exported > 0;
    }

    if (isWax) {
        RsdFormat::WaxBank bank = RsdFormat::LoadWax(rawData.data(), static_cast<u32>(rawData.size()));

        // Strip .wav extension to get the bank folder path
        std::string bankDir = outputPath;
        const size_t extPos = bankDir.rfind(".wav");
        if (extPos != std::string::npos) {
            bankDir.erase(extPos);
        }

        p3d::io::CreateDirectories(bankDir);

        s32 exported = 0;
        const u32 numSamples = static_cast<u32>(bank.pcmSamples.size());
        for (u32 i = 0; i < numSamples; i++) {
            if (bank.pcmSamples[i].empty()) continue;

            u32 sampleRate = 11025;
            if (i < bank.sampleDescs.size()) {
                const u16 spuPitch = static_cast<u16>(bank.sampleDescs[i].params & 0xFFFF);
                if (spuPitch > 0) {
                    sampleRate = 44100u * spuPitch / 4096u;
                }
            }

            char samplePath[512];
            std::snprintf(samplePath, sizeof(samplePath), "%s/sample%02u.wav", bankDir.c_str(), i);

            if (WritePCMWav(samplePath, bank.pcmSamples[i], sampleRate, 1)) {
                exported++;
            }
        }

        LOG("[AssetExporter] Exported %d WAVs from bank: %s", exported, outputPath.c_str());
        return exported > 0;
    }

    if (isVag) {
        if (rawData.size() <= 64) return false;

        const u32 vagDataLen = (static_cast<u32>(rawData[0x0C]) << 24) |
            (static_cast<u32>(rawData[0x0D]) << 16) |
            (static_cast<u32>(rawData[0x0E]) << 8) |
            (static_cast<u32>(rawData[0x0F]));
        const u32 adpcmSize = std::min(vagDataLen, static_cast<u32>(rawData.size() - 64));

        std::vector<s16> pcm = SpuAdpcm::Decode(rawData.data() + 64, adpcmSize, false);
        if (pcm.empty()) return false;

        return WritePCMWav(outputPath, pcm, 22050, 1);
    }

    // Raw ADPCM fallback
    std::vector<s16> pcm = SpuAdpcm::Decode(rawData.data(), static_cast<u32>(rawData.size()), false);
    if (pcm.size() < 100) return false;

    return WritePCMWav(outputPath, pcm, 11025, 1);
}

// Conversion: Data → P3D chunk tree JSON

static void WriteChunkTreeJSON(std::ostringstream& out, const u8* d, u32 size, int depth) {
    static const char kHex[] = "0123456789abcdef";
    auto hexPreview = [&](u32 off, u32 len) {
        for (u32 i = off; i < off + len && i < size; i++)
            out << kHex[d[i] >> 4] << kHex[d[i] & 0xF];
    };

    if (size < 6) {
        out << "{\"_size\":" << size << ",\"_format\":\"unknown\",\"_preview\":\"";
        hexPreview(0, std::min(size, 16u)); out << "\"}"; return;
    }

    u16 id = p3dReadU16LE(d);
    u32 chunkSize = p3dReadU32LE(d + 2);

    char idStr[12]; snprintf(idStr, sizeof(idStr), "0x%04x", id);
    out << "{\"id\":\"" << idStr << "\",\"size\":" << chunkSize;

    if (chunkSize >= 6 && chunkSize <= size && depth < 8) {
        // Try parsing sub-chunks
        u32 pos = 6;
        bool valid = true;
        std::vector<std::pair<u32, u32>> children;
        while (pos + 6 <= chunkSize) {
            u32 sub = p3dReadU32LE(d + pos + 2);
            if (sub < 6 || pos + sub > chunkSize) { valid = false; break; }
            children.push_back({ pos, sub });
            pos += sub;
        }
        if (valid && pos == chunkSize && !children.empty()) {
            out << ",\"children\":[";
            for (size_t ci = 0; ci < children.size(); ci++) {
                WriteChunkTreeJSON(out, d + children[ci].first, children[ci].second, depth + 1);
                if (ci + 1 < children.size()) out << ",";
            }
            out << "]";
        }
        else {
            out << ",\"_preview\":\""; hexPreview(6, std::min(chunkSize - 6, 32u)); out << "\"";
        }
    }
    else if (chunkSize > 6) {
        out << ",\"_preview\":\""; hexPreview(6, std::min(chunkSize - 6, 32u)); out << "\"";
    }
    out << "}";
}

bool AssetExporter::ConvertDataToJSON(const std::vector<u8>& rawData,
                                      const std::string& outputPath) {
    std::string dir = outputPath.substr(0, outputPath.find_last_of("/\\"));
    if (!dir.empty()) p3d::io::CreateDirectories(dir);

    std::ostringstream out;
    const u8* d = rawData.data();
    const u32 sz = (u32)rawData.size();
    out << "{\"_size\":" << sz << ",\"_data\":";
    WriteChunkTreeJSON(out, d, sz, 0);
    out << "}\n";

    if (!p3d::io::WriteTextFile(outputPath, out.str())) return false;

    LOG("[AssetExporter] Data JSON: %s", outputPath.c_str());
    return true;
}

// LCF export

// Parse a big-endian stream header and return (magic, size, offset) triples.
struct LcfEntry { char magic[5]; u32 size; u32 offset; };
static std::vector<LcfEntry> ParseLcfHeader(const u8* data, u32 dataSize) {
    std::vector<LcfEntry> result;
    if (dataSize < 4) return result;
    u32 count = (u32)data[0] << 24 | (u32)data[1] << 16 | (u32)data[2] << 8 | data[3];
    u32 pos = 4;
    for (u32 i = 0; i < count && pos + 16 <= dataSize; i++) {
        LcfEntry e;
        memcpy(e.magic, data + pos, 4); e.magic[4] = '\0';
        e.size = (u32)data[pos + 4] << 24 | (u32)data[pos + 5] << 16 | (u32)data[pos + 6] << 8 | data[pos + 7];
        e.offset = (u32)data[pos + 8] << 24 | (u32)data[pos + 9] << 16 | (u32)data[pos + 10] << 8 | data[pos + 11];
        u32 extraLen = (u32)data[pos + 12] << 24 | (u32)data[pos + 13] << 16 | (u32)data[pos + 14] << 8 | data[pos + 15];
        pos += 16;
        if (extraLen > 0) pos += (extraLen + 3) & ~3u;
        if (e.offset + e.size <= dataSize) result.push_back(e);
    }
    return result;
}

static bool ExportDialogWavs(const std::vector<u8>& rawData, const std::string& outputDir) {
    constexpr u32 sectorSize = 2048;
    if (rawData.size() < 6) return false;
    const u32 headerSize = p3dReadU16LE(rawData.data());
    const u32 characterCount = p3dReadU16LE(rawData.data() + 2);
    const u32 dataBase = (headerSize + sectorSize - 1) & ~(sectorSize - 1);
    if (headerSize < 4 + characterCount * 2 || dataBase >= rawData.size()) return false;

    p3d::io::CreateDirectories(outputDir);
    u32 exported = 0;
    for (u32 character = 0; character < characterCount; ++character) {
        const u32 characterInfo = p3dReadU16LE(rawData.data() + 4 + character * 2);
        if (characterInfo >= headerSize) continue;
        const u32 dialogCount = rawData[characterInfo];
        const u32 dialogList = characterInfo + 1;
        const u32 streamTable = dialogList + dialogCount;
        const u32 clipInfoTable = streamTable + dialogCount * 2;
        if (clipInfoTable + dialogCount * 2 > headerSize) continue;

        for (u32 dialogIndex = 0; dialogIndex < dialogCount; ++dialogIndex) {
            const u32 dialogId = rawData[dialogList + dialogIndex];
            const u32 streamSector = p3dReadU16LE(rawData.data() + streamTable + dialogIndex * 2);
            const u32 clipInfo = p3dReadU16LE(rawData.data() + clipInfoTable + dialogIndex * 2);
            if (clipInfo >= headerSize) continue;
            const u32 clipCount = rawData[clipInfo];
            if (clipInfo + 1 + clipCount > headerSize) continue;

            u32 clipStart = dataBase + streamSector * sectorSize;
            for (u32 variant = 0; variant < clipCount; ++variant) {
                const u32 sectors = rawData[clipInfo + 1 + variant] & 0x3Fu;
                const u32 clipSize = sectors * sectorSize;
                if (clipSize&& clipStart < rawData.size() && clipStart + clipSize <= rawData.size()) {
                    std::vector<s16> pcm = SpuAdpcm::Decode(rawData.data() + clipStart, clipSize, true);
                    if (!pcm.empty()) {
                        char fileName[64];
                        std::snprintf(fileName, sizeof(fileName), "c%02u_d%03u_v%02u.wav",
                                      character, dialogId, variant);
                        if (WritePCMWav(outputDir + "/" + fileName, pcm, 11025, 1)) ++exported;
                    }
                }
                clipStart += clipSize;
            }
        }
    }
    LOG("[AssetExporter] Exported %u dialog WAVs to: %s", exported, outputDir.c_str());
    return exported > 0;
}

struct ExportGeoMaterial {
    u8 primCmd = 0;
    u16 cba = 0;
    u16 tpage = 0;
};

static bool ExtractDynGeoForExport(
    const u8* geoData,
    u32 geoSize,
    const std::unordered_map<u32, ExportGeoMaterial>& materials,
    std::vector<PrimGeomVertex>& outVerts,
    std::vector<u32>& outIndices,
    u32* outProcessedPolys,
    u32* outTotalPolys) {
    if (!geoData || geoSize < 0x58) return false;
    const u32 vertListOff = p3dReadU32LE(geoData + 0x10) << 2;
    const u16 numVerts = p3dReadU16LE(geoData + 0x14);
    const u16 numPolys = p3dReadU16LE(geoData + 0x16);
    if (outProcessedPolys) *outProcessedPolys = 0;
    if (outTotalPolys) *outTotalPolys = numPolys;
    const u32 polyListOff = p3dReadU32LE(geoData + 0x40) << 2;
    const u32 colourListOff = p3dReadU32LE(geoData + 0x44) << 2;
    if (!numVerts || !numPolys
        || vertListOff + static_cast<u32>(numVerts) * 8u > geoSize
        || polyListOff + static_cast<u32>(numPolys) * 24u > geoSize) return false;

    const u8* vertices = geoData + vertListOff;
    const u8* polygons = geoData + polyListOff;
    const u8* colours = (colourListOff && colourListOff + static_cast<u32>(numVerts) * 4u <= geoSize)
        ? geoData + colourListOff : nullptr;

    auto makeVertex = [&](u16 index, const ExportGeoMaterial& material, u16 uv) {
        PrimGeomVertex vertex = {};
        if (index >= numVerts) return vertex;
        const u8* source = vertices + static_cast<u32>(index) * 8u;
        vertex.x = static_cast<f32>(p3dReadS16LE(source));
        vertex.y = static_cast<f32>(p3dReadS16LE(source + 2));
        vertex.z = static_cast<f32>(p3dReadS16LE(source + 4));
        vertex.r = vertex.g = vertex.b = 0.85f;
        if (colours) {
            const u32 colour = p3dReadU32LE(colours + static_cast<u32>(index) * 4u);
            vertex.r = static_cast<f32>(colour & 0xFFu) / 128.0f;
            vertex.g = static_cast<f32>((colour >> 8) & 0xFFu) / 128.0f;
            vertex.b = static_cast<f32>((colour >> 16) & 0xFFu) / 128.0f;
        }
        const u8 command = static_cast<u8>(material.primCmd & 0xFCu);
        const bool textured = command == 0x24 || command == 0x2C
            || command == 0x34 || command == 0x3C;
        if (textured) {
            vertex.u = static_cast<f32>(uv & 0xFFu);
            vertex.v = static_cast<f32>((uv >> 8) & 0xFFu);
            vertex.tpage = static_cast<f32>(material.tpage);
            vertex.cba = static_cast<f32>(material.cba);
        }
        else {
            vertex.tpage = -1.0f;
        }
        return vertex;
    };

    for (u32 polygonIndex = 0; polygonIndex < numPolys; ++polygonIndex) {
        const u8* polygon = polygons + polygonIndex * 24u;
        const u32 materialHash = p3dReadU32LE(polygon);
        auto materialIt = materials.find(materialHash);
        if (materialIt == materials.end()) continue;
        const ExportGeoMaterial& material = materialIt->second;
        const u8 command = static_cast<u8>(material.primCmd & 0xFCu);
        const bool quad = command == 0x28 || command == 0x2C
            || command == 0x38 || command == 0x3C;
        const bool triangle = command == 0x20 || command == 0x24
            || command == 0x30 || command == 0x34;
        if (!quad && !triangle) continue;
        if (outProcessedPolys) ++*outProcessedPolys;

        PrimGeomVertex v[4];
        for (u32 i = 0; i < (quad ? 4u : 3u); ++i) {
            v[i] = makeVertex(p3dReadU16LE(polygon + 8u + i * 2u), material,
                              p3dReadU16LE(polygon + 16u + i * 2u));
        }
        const u32 base = static_cast<u32>(outVerts.size());
        outVerts.push_back(v[0]); outVerts.push_back(v[1]); outVerts.push_back(v[2]);
        outIndices.push_back(base); outIndices.push_back(base + 1); outIndices.push_back(base + 2);
        if (quad) {
            outVerts.push_back(v[3]);
            outIndices.push_back(base + 1); outIndices.push_back(base + 3); outIndices.push_back(base + 2);
        }
    }
    return !outIndices.empty();
}

// --- Level export (LCF → GLB + named PNGs) ---

bool AssetExporter::ExportLevel(const std::vector<u8>& rawData,
                                const std::string& outputDir,
                                const std::string& levelName) {
    if (rawData.size() < 4) return false;
    const u8* data = rawData.data();
    const u32 dataSize = (u32)rawData.size();

    auto entries = ParseLcfHeader(data, dataSize);
    if (entries.empty()) return false;

    u32 parameterSet = 0;
    for (const auto& entry : entries) {
        if (strncmp(entry.magic, ".WDB", 4) != 0) continue;
        const std::string fileName = outputDir + "/parameters_petal"
            + (parameterSet < 10 ? "0" : "") + std::to_string(parameterSet++) + ".json";
        ExportWDBParameters(data + entry.offset, entry.size, fileName);
    }

    // Pass 1: upload all .TPG texture chunks to VRAM, collect raw metadata
    PsxVRAM vram;
    std::vector<TexManifest> rawMeta;
    for (const auto& e : entries) {
        if (strncmp(e.magic, ".TPG", 4) != 0 || e.size < 6) continue;
        UploadP3DTexToVRAM(data + e.offset, e.size, vram, rawMeta);
    }

    // Pass 2: extract each named geometry independently. Runtime replacement
    // happens at the 0x6002 resource boundary, so a merged level GLB cannot be
    // mapped back safely.
    struct NamedLevelGeometry {
        std::string name;
        std::vector<PrimGeomVertex> verts;
        std::vector<u32> indices;
    };
    std::vector<NamedLevelGeometry> geometries;
    std::vector<PrimGeomVertex> allVerts; // texture page discovery only

    for (u32 i = 0; i + 1 < (u32)entries.size(); i++) {
        bool isPerm = (strncmp(entries[i].magic, ".RCI", 4) == 0 ||
                       strncmp(entries[i].magic, ".PCI", 4) == 0);
        if (!isPerm) continue;

        const u8* permData = data + entries[i].offset;
        const u32 permSize = entries[i].size;
        const u8* p3dData = data + entries[i + 1].offset;
        const u32 p3dSize = entries[i + 1].size;
        std::unordered_map<u32, ExportGeoMaterial> materials;

        if (p3dSize < 6 || p3dReadU16LE(p3dData) != 0xFF04) continue;
        u32 end = std::min(p3dReadU32LE(p3dData + 2), p3dSize);
        u32 pos = 6, permCursor = 0;

        while (pos + 6 <= end) {
            u16 id = p3dReadU16LE(p3dData + pos);
            u32 csz = p3dReadU32LE(p3dData + pos + 2);
            if (csz < 6 || pos + csz > end) break;

            if (csz < 14) { pos += csz; continue; }
            const u8* chunkBody = p3dData + pos + 6;
            u32 chunkPermSize = p3dReadU32LE(chunkBody + 4);
            if (id == 0x6001 && permCursor + chunkPermSize <= permSize) {
                const u32 materialCount = p3dReadU32LE(chunkBody);
                if (materialCount > 0) {
                    const u32 recordSize = chunkPermSize / materialCount;
                    if (recordSize >= 24) {
                        for (u32 materialIndex = 0; materialIndex < materialCount; ++materialIndex) {
                            const u8* record = permData + permCursor + materialIndex * recordSize;
                            const u32 materialHash = p3dReadU32LE(record);
                            const u32 textureInfo = p3dReadU32LE(record + 20);
                            materials[materialHash] = ExportGeoMaterial{
                                static_cast<u8>((p3dReadU32LE(record + 16) >> 24) & 0xFFu),
                                static_cast<u16>(textureInfo & 0xFFFFu),
                                static_cast<u16>(textureInfo >> 16)
                            };
                        }
                    }
                }
            }
            else if (id == 0x6002 && permCursor + chunkPermSize <= permSize) {
                std::string geoName;
                const u32 nameCount = p3dReadU32LE(chunkBody);
                u32 namePos = 8;
                if (nameCount > 0 && namePos < csz - 6) {
                    const u8 nameLen = chunkBody[namePos++];
                    if (nameLen > 0 && namePos + nameLen <= csz - 6) {
                        geoName.assign(reinterpret_cast<const char*>(chunkBody + namePos), nameLen);
                        std::transform(geoName.begin(), geoName.end(), geoName.begin(),
                                       [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
                    }
                }
                const u8* perm = permData + permCursor;
                std::vector<PrimGeomVertex> bv;
                std::vector<u32> bi;
                u32 processedPolys = 0;
                u32 totalPolys = 0;
                if (ExtractDynGeoForExport(perm, chunkPermSize, materials, bv, bi,
                    &processedPolys, &totalPolys)) {
                    for (const auto& vertex : bv) allVerts.push_back(vertex);
                    if (geoName.empty()) {
                        char fallback[32];
                        std::snprintf(fallback, sizeof(fallback), "geo_%08x", p3dReadU32LE(perm));
                        geoName = fallback;
                    }
                    if (processedPolys != totalPolys) {
                        LOG("[AssetExporter] Geometry incomplete: %s (%u/%u polygons)",
                            geoName.c_str(), processedPolys, totalPolys);
                    }
                    geometries.push_back(NamedLevelGeometry{ geoName, std::move(bv), std::move(bi) });
                }
            }
            if (id == 0x6001 || id == 0x6002) {
                permCursor += chunkPermSize;
            }
            pos += csz;
        }
    }

    // A page can contain many textures using different CLUTs. Pair each named
    // index chunk with its own "{name} CLUT" chunk, then use geometry UV votes
    // when available. A single page-wide CBA causes the classic wrong-palette
    // noise seen in exports.
    std::map<std::string, std::pair<u16, int>> clutMap;
    for (const auto& m : rawMeta) {
        if (m.rh != 1 || (m.rw != 16 && m.rw != 256)) continue;
        std::string base = m.name;
        std::string lower = base;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
        if (lower.size() >= 5 && lower.substr(lower.size() - 5) == " clut") {
            base.erase(base.size() - 5);
        }
        clutMap[base] = {
            static_cast<u16>((m.rx / 16) | (m.ry << 6)),
            m.rw == 256 ? 1 : 0
        };
    }

    std::vector<u8> page(256 * 256 * 4);
    std::vector<TexChunkExport> decoded;
    for (auto& m : rawMeta) {
        if (m.rh == 1 && (m.rw == 16 || m.rw == 256)) continue;
        u8 ttx = static_cast<u8>(m.rx / 64), tty = static_cast<u8>(m.ry / 256);
        std::map<std::pair<u16, u16>, int> materialVotes;
        for (const auto& vertex : allVerts) {
            if (vertex.tpage < 0.0f) continue;
            const u16 candidateTp = static_cast<u16>(static_cast<u32>(vertex.tpage));
            if ((candidateTp & 0xFu) != ttx || ((candidateTp >> 4) & 1u) != tty) continue;
            const int candidateDepth = (candidateTp >> 7) & 3;
            const int candidatePpw = candidateDepth == 0 ? 4 : candidateDepth == 1 ? 2 : 1;
            const int px = (m.rx - ttx * 64) * candidatePpw;
            const int py = m.ry - tty * 256;
            const int pw = m.rw * candidatePpw;
            const int u = static_cast<int>(vertex.u), v = static_cast<int>(vertex.v);
            if (u >= px && u < px + pw && v >= py && v < py + m.rh) {
                materialVotes[{ candidateTp, static_cast<u16>(static_cast<u32>(vertex.cba)) }]++;
            }
        }

        u16 tp = 0, cb = 0;
        int bestVotes = 0;
        for (const auto& vote : materialVotes) {
            if (vote.second > bestVotes) {
                tp = vote.first.first;
                cb = vote.first.second;
                bestVotes = vote.second;
            }
        }
        if (!bestVotes) {
            auto clut = clutMap.find(m.name);
            if (clut == clutMap.end()) continue;
            cb = clut->second.first;
            tp = static_cast<u16>(ttx | (tty << 4) | (clut->second.second << 7));
        }
        int depth = (tp >> 7) & 3;
        int ppw = (depth == 0) ? 4 : (depth == 1) ? 2 : 1;
        m.tx = ttx; m.ty = tty;
        m.px = static_cast<s16>((m.rx - ttx * 64) * ppw);
        m.py = static_cast<s16>(m.ry - tty * 256);
        m.pw = static_cast<s16>(m.rw * ppw);
        m.ph = m.rh;
        vram.DecodePage(tp, cb, page.data());
        TexChunkExport tex;
        tex.manifest = m;
        tex.rgba8.resize(static_cast<size_t>(m.pw) * m.ph * 4);
        for (int row = 0; row < m.ph; row++) {
            memcpy(tex.rgba8.data() + row * m.pw * 4,
                   page.data() + static_cast<size_t>(m.py + row) * 256 * 4 + m.px * 4,
                   static_cast<size_t>(m.pw) * 4);
        }
        decoded.push_back(std::move(tex));
    }

    auto manifests = ExportTexChunks(decoded, outputDir + "/textures");

    if (geometries.empty()) {
        LOG("[AssetExporter] No geometry: %s", levelName.c_str());
        return !manifests.empty();
    }

    bool wroteGeometry = false;
    for (const auto& geometry : geometries) {
        auto groups = GroupByMaterial(geometry.verts, geometry.indices, manifests);
        wroteGeometry |= WriteGLB(outputDir + "/geometry/" + geometry.name + ".glb",
                                  geometry.name, groups, "../textures/");
    }
    return wroteGeometry || !manifests.empty();
}


// Export API

bool AssetExporter::ExportEntry(const AssetEntry& entry, const char* outputDir) {
    std::vector<u8> rawData = ReadFileBytes(entry.filePath);
    if (rawData.empty()) {
        LOG("[AssetExporter] Cannot read: %s", entry.filePath.c_str());
        return false;
    }

    std::string lowerPath = entry.filePath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                   [](char c) { return (char)std::tolower((unsigned char)c); });

    switch (entry.category) {
        case AssetCategory::Texture:
        {
            // RR-embedded P3D texture container: export named PNGs
            if (entry.resourceIndex > 0 && lowerPath.find(".rr") != std::string::npos) {
                if (rawData.size() >= 16) {
                    u32 entryCount = p3dReadU32LE(rawData.data() + 8) / 8;
                    u32 ri = (u32)entry.resourceIndex;
                    if (ri < entryCount) {
                        u32 off = p3dReadU32LE(rawData.data() + ri * 8);
                        u32 sz = p3dReadU32LE(rawData.data() + ri * 8 + 4) >> 8;
                        if (off > 0 && sz > 6 && off + sz <= rawData.size()) {
                            // charName is everything before "_tex"
                            std::string charName = entry.name;
                            auto tpos = charName.find("_tex");
                            if (tpos != std::string::npos) charName.erase(tpos);
                            std::string texDir = std::string(outputDir) + "/characters/" + charName;
                            auto chunks = ParseP3DTexChunks(rawData.data() + off, sz);
                            auto ms = ExportTexChunks(chunks, texDir);
                            return !ms.empty();
                        }
                    }
                }
                return false;
            }
            // TIM file
            std::string texPath = std::string(outputDir) + "/textures/" + entry.name + ".png";
            return ConvertTIMtoPNG(rawData, texPath);
        }
        case AssetCategory::SkeletalMesh:
        case AssetCategory::ETreeMesh:
        {
            std::string charDir = std::string(outputDir) + "/characters/" + entry.name;
            return ExportCharacter(rawData, charDir, entry.name);
        }
        case AssetCategory::StaticMesh:
        {
            std::string lvlDir = std::string(outputDir) + "/levels/" + entry.name;
            return ExportLevel(rawData, lvlDir, entry.name);
        }
        case AssetCategory::Sound:
        {
            std::string sndPath = std::string(outputDir) + "/sounds/" + entry.name + ".wav";
            std::string ext = std::filesystem::path(entry.filePath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
            if (ext == ".dlg") {
                return ExportDialogWavs(rawData, std::string(outputDir) + "/sounds/dialog");
            }
            return ConvertWAVtoWAV(rawData, sndPath, ext);
        }
        case AssetCategory::Data:
        {
            return false;
        }
    }
    return false;
}

int AssetExporter::ExportAll(const char* outputDir,
                             AssetCategory filterCategory,
                             bool filterByCategory,
                             ExportProgressCallback progress) {
    int exported = 0;
    int total = 0;

    for (const auto& entry : m_entries) {
        if (filterByCategory && entry.category != filterCategory) continue;
        total++;
    }

    int current = 0;
    for (const auto& entry : m_entries) {
        if (filterByCategory && entry.category != filterCategory) continue;

        if (progress) {
            progress(current, total, entry.name.c_str());
        }

        if (ExportEntry(entry, outputDir)) {
            exported++;
        }

        current++;
    }

    LOG("[AssetExporter] Batch export done: %d/%d succeeded", exported, total);
    return exported;
}

int AssetExporter::ExportAllCategories(const char* outputDir,
                                       ExportProgressCallback progress) {
    const int exported = ExportAll(outputDir, AssetCategory::Texture, false, progress);
    p3d::io::InvalidateResolveCache();
    return exported;
}
