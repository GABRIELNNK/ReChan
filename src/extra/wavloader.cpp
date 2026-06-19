#include "extra/wavloader.h"

#ifdef MOD_LOADER

#include "pc/log.h"
#include <cstring>
#include <fstream>
#include <vector>

// Minimal RIFF WAV parser — avoids pulling in miniaudio for simple PCM loading.
// For compressed formats the caller should use miniaudio directly.

struct RiffHeader {
    char chunkID[4];
    u32 chunkSize;
    char format[4];
};

struct FmtChunk {
    char subchunkID[4];
    u32 subchunkSize;
    u16 audioFormat;
    u16 numChannels;
    u32 sampleRate;
    u32 byteRate;
    u16 blockAlign;
    u16 bitsPerSample;
};

struct DataChunkHeader {
    char subchunkID[4];
    u32 subchunkSize;
};

WAVAudioBuffer WAVLoader::LoadFromFile(const char* path) {
    WAVAudioBuffer result;

    if (!path || !path[0]) {
        LOG("[WAVLoader] Invalid path");
        return result;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOG("[WAVLoader] Cannot open: %s", path);
        return result;
    }

    // Read RIFF header
    RiffHeader riff;
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
    if (!file || std::memcmp(riff.chunkID, "RIFF", 4) != 0 ||
        std::memcmp(riff.format, "WAVE", 4) != 0) {
        LOG("[WAVLoader] Not a valid WAV file: %s", path);
        return result;
    }

    FmtChunk fmt = {};
    bool foundFmt = false;

    // Scan chunks until we find fmt and data
    while (file.good()) {
        char chunkID[4] = {};
        u32 chunkSize = 0;
        file.read(chunkID, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (!file) break;

        if (std::memcmp(chunkID, "fmt ", 4) == 0) {
            u32 readSize = chunkSize < sizeof(FmtChunk) - 8 ? chunkSize : sizeof(FmtChunk) - 8;
            fmt.subchunkSize = readSize;
            file.read(reinterpret_cast<char*>(&fmt.audioFormat), readSize);
            foundFmt = true;

            // Skip remaining fmt bytes if chunk is larger than expected
            if (chunkSize > readSize) {
                file.seekg(static_cast<std::streamoff>(chunkSize - readSize), std::ios::cur);
            }
        }
        else if (std::memcmp(chunkID, "data", 4) == 0) {
            if (!foundFmt) {
                LOG("[WAVLoader] data chunk before fmt chunk: %s", path);
                return result;
            }

            if (fmt.audioFormat != 1) {
                LOG("[WAVLoader] Only PCM format supported (format=%u): %s", fmt.audioFormat, path);
                return result;
            }

            result.sampleRate = fmt.sampleRate;
            result.channels = fmt.numChannels;
            result.bitsPerSample = fmt.bitsPerSample;
            result.size = chunkSize;
            result.data = new u8[chunkSize];

            file.read(reinterpret_cast<char*>(result.data), chunkSize);
            if (!file) {
                LOG("[WAVLoader] Failed to read PCM data: %s", path);
                delete[] result.data;
                result.data = nullptr;
                result.size = 0;
                return result;
            }

            LOG("[WAVLoader] Loaded: %s (%u Hz, %u ch, %u-bit, %u bytes)",
                path, result.sampleRate, result.channels, result.bitsPerSample, result.size);
            return result;
        }
        else {
            // Skip unknown chunk
            file.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);
        }
    }

    LOG("[WAVLoader] No data chunk found: %s", path);
    return result;
}

#endif // MOD_LOADER