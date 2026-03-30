// assets.cpp — Filesystem layer implementation
#include "gen/assets.h"
#include <fstream>
#include <cstring>

namespace Assets {
    static std::filesystem::path sRoot;

    void SetRoot(const std::filesystem::path& root) {
        sRoot = root;
    }

    const std::filesystem::path& Root() {
        return sRoot;
    }

    std::vector<u8> ReadFile(const std::filesystem::path& relativePath) {
        auto fullPath = sRoot / relativePath;
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file)
            return {};

        auto size = file.tellg();
        file.seekg(0);
        std::vector<u8> data(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }

    std::vector<std::string> ListFiles(const std::filesystem::path& relativeDir,
                                       const std::string& extension) {
        std::vector<std::string> results;
        auto fullDir = sRoot / relativeDir;

        if (!std::filesystem::exists(fullDir))
            return results;

        for (const auto& entry : std::filesystem::directory_iterator(fullDir)) {
            if (!entry.is_regular_file())
                continue;
            if (!extension.empty() && entry.path().extension() != extension)
                continue;
            results.push_back(entry.path().filename().string());
        }
        return results;
    }

    StreamFile LoadStream(const std::string& relativePath) {
        StreamFile sf;
        sf.data = ReadFile(relativePath);
        if (!sf.data.empty())
            sf.entries = ParseStreamHeader(sf.data.data(), static_cast<u32>(sf.data.size()));
        return sf;
    }

    RRFile LoadRR(const std::string& relativePath) {
        RRFile rf;
        rf.data = ReadFile(relativePath);
        if (!rf.data.empty())
            rf.entries = ParseRRHeader(rf.data.data(), static_cast<u32>(rf.data.size()));
        return rf;
    }

    std::vector<DataSpan> FilterStreamEntries(const StreamFile& stream, const char* magic) {
        std::vector<DataSpan> spans;
        for (const auto& entry : stream.entries) {
            if (std::strncmp(entry.magic, magic, 4) == 0) {
                if (entry.offset + entry.size <= static_cast<u32>(stream.data.size())) {
                    spans.push_back({ stream.data.data() + entry.offset, entry.size });
                }
            }
        }
        return spans;
    }

} // namespace Assets
