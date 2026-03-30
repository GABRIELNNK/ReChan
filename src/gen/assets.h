// assets.h — Filesystem layer for PSX disc assets
#ifndef ASSETS_H
#define ASSETS_H

#include "core.h"
#include "p3d/stream.h"
#include "p3d/chunkfile.h"
#include <string>
#include <vector>
#include <filesystem>

namespace Assets {
    // Set/get the root path (typically "assets/" next to the exe)
    void SetRoot(const std::filesystem::path& root);
    const std::filesystem::path& Root();

    // Read a raw file from the assets directory
    std::vector<u8> ReadFile(const std::filesystem::path& relativePath);

    // List files in a subdirectory, optionally filtered by extension
    std::vector<std::string> ListFiles(const std::filesystem::path& relativeDir,
                                       const std::string& extension = "");

    // ─── High-level disc asset access ────────────────────────────────

    // Load and parse a Stream file (.LCF/.GCF), returning its entries + raw data
    struct StreamFile {
        std::vector<u8> data;                   // full file contents
        std::vector<tStreamEntry> entries;       // parsed header
    };
    StreamFile LoadStream(const std::string& relativePath);

    // Load an RR character archive, returning entries + raw data
    struct RRFile {
        std::vector<u8> data;
        std::vector<tRREntry> entries;
    };
    RRFile LoadRR(const std::string& relativePath);

    // Extract Stream entries matching a magic tag (e.g. ".TPG")
    struct DataSpan {
        const u8* ptr;
        u32 size;
    };
    std::vector<DataSpan> FilterStreamEntries(const StreamFile& stream,
                                              const char* magic);
}

#endif // ASSETS_H
