#pragma once
#include "core.h"
#include "gen/geometry.h"
#include <functional>
#include <string>
#include <vector>

struct STreeData;

// Asset Exporter catalogs all game assets on disk and can export them to
// modern formats (PNG, GLB, WAV, JSON). Works independently of MOD_LOADER.
// Loads/unloads assets in isolation — no impact on gameplay.

enum class AssetCategory {
    Texture,       // TIM files → export as PNG
    SkeletalMesh,  // Character STree → export as GLB
    StaticMesh,    // Static Geo → export as GLB
    ETreeMesh,     // Export tree → export as GLB
    Sound,         // Sound effects → export as WAV
    Data,          // Scripts/configs → export as JSON
};

struct AssetEntry {
    std::string name;          // human-readable: "jackie_body", "barrel_01"
    AssetCategory category;
    std::string filePath;      // disk path: "RCHARS/JACKIE.RR"
    u32 crc = 0;               // p3dHash(name)
    int resourceIndex = 0;     // index within multi-resource file (RR files)
    bool isLoaded = false;     // currently in game memory?

    static const char* CategoryName(AssetCategory cat);
};

// Named texture chunk from a PSX 0x6008 sub-chunk.
struct TexManifest {
    std::string name;
    s16 rx = 0, ry = 0, rw = 0, rh = 0;  // VRAM word coords
    s16 px = 0, py = 0, pw = 0, ph = 0;  // pixel bounds within decoded 256×256 page
    u8  tx = 0, ty = 0;                   // page address: tp & 0xF, (tp>>4)&1
};

// Decoded texture chunk ready for PNG export.
struct TexChunkExport {
    TexManifest manifest;
    std::vector<u8> rgba8;  // rw * rh * 4 bytes, RGBA8
};

// Per-material triangle group with UV-remapped vertices.
struct MatPrimitive {
    std::vector<PrimGeomVertex> verts;
    std::vector<u32> indices;
    std::string textureName;  // "" = untextured
};

// Callback type for progress reporting during batch exports.
// Parameters: current item index, total items, asset name
using ExportProgressCallback = std::function<void(int current, int total, const char* name)>;

class AssetExporter {
public:
    AssetExporter() = default;
    ~AssetExporter() = default;

    static AssetExporter& Instance();

    // Build the catalog by scanning game directories on disk.
    // Safe to call multiple times — only rescans if dirty.
    void BuildCatalog();

    // Clear the catalog.
    void ClearCatalog();

    [[nodiscard]] const std::vector<AssetEntry>& GetEntries() const { return m_entries; }

    // Export a single entry to export/{category}/{name}/
    bool ExportEntry(const AssetEntry& entry, const char* outputDir = "export");

    // Batch export: export all entries, optionally filtered by category.
    int ExportAll(const char* outputDir = "export",
                  AssetCategory filterCategory = AssetCategory::Texture,
                  bool filterByCategory = false,
                  ExportProgressCallback progress = nullptr);

    int ExportAllCategories(const char* outputDir = "export",
                            ExportProgressCallback progress = nullptr);

    AssetExporter(const AssetExporter&) = delete;
    AssetExporter& operator=(const AssetExporter&) = delete;

    void ScanRCHARS();
    void ScanRCHARS_Textures();
    void ScanTIMDirectory();
    void ScanSoundDirectory();
    void ScanLevelDirectory();
    void ScanDataDirectory();

    static std::vector<u8> ReadFileBytes(const std::string& path);

    // TIM file → PNG
    static bool ConvertTIMtoPNG(const std::vector<u8>& rawData,
                                const std::string& outputPath,
                                int* outWidth = nullptr,
                                int* outHeight = nullptr);

    // PSX sound formats → WAV. sourceExt (e.g. ".fag", ".clp"), when known,
    // disambiguates FAG vs CLP — their headers are both "[u32 count][u32
    // offsets...]" and are not reliably distinguishable from content alone.
    static bool ConvertWAVtoWAV(const std::vector<u8>& rawData,
                                const std::string& outputPath,
                                const std::string& sourceExt = "");

    // Data file → JSON chunk tree
    static bool ConvertDataToJSON(const std::vector<u8>& rawData,
                                  const std::string& outputPath);

    // Parse 0xFF04 P3D container → named 0x6008 texture chunks, decoded to RGBA8
    static std::vector<TexChunkExport> ParseP3DTexChunks(const u8* data, u32 size);

    // Write each chunk as {name}.png to outputDir. No sidecar manifest is
    // produced; filename and containing scope are the mod identity.
    // Returns manifests for the chunks that were successfully written.
    static std::vector<TexManifest> ExportTexChunks(const std::vector<TexChunkExport>& chunks,
                                                    const std::string& outputDir);

    // Group raw verts/indices by texture name, remapping UVs from page-relative
    // (0-255) to normalized [0,1] within the covering manifest PNG.
    static std::vector<MatPrimitive> GroupByMaterial(
        const std::vector<PrimGeomVertex>& verts,
        const std::vector<u32>& indices,
        const std::vector<TexManifest>& manifests);

    // Write binary GLB (JSON + BIN chunks) for a multi-material mesh.
    static bool WriteGLB(const std::string& path, const std::string& meshName,
                         const std::vector<MatPrimitive>& prims,
                         const std::string& textureUriPrefix = "",
                         const STreeData* skeleton = nullptr);

    // Export RR character: mesh → GLB, texture resources → named PNGs, all in outputDir.
    static bool ExportCharacter(const std::vector<u8>& rawData,
                                const std::string& outputDir,
                                const std::string& charName);

    // Export LCF level: geometry → merged GLB, TPG textures → named PNGs, all in outputDir.
    static bool ExportLevel(const std::vector<u8>& rawData,
                            const std::string& outputDir,
                            const std::string& levelName);

    std::vector<AssetEntry> m_entries;
    bool m_catalogBuilt = false;
};
