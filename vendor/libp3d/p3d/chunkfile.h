// chunkfile.h = P3D chunk file reader
//
// PSX P3D chunk format: 16-bit IDs, 6-byte headers (u16 id + u32 totalSize).
// totalSize includes the header. Chunks can nest.
//
#pragma once

#include "core.h"
#include <vector>
#include <string>
#include <cstring>

// PSX P3D chunk IDs (16-bit)
namespace ChunkID {
    // File wrappers
    constexpr u16 TexturePage = 0xFF04;  // .TPG texture page container

    // PSX-specific chunks
    constexpr u16 P3DContainer = 0x6000;
    constexpr u16 Texture = 0x6008;
    constexpr u16 Shader = 0x6100;
    constexpr u16 ShaderParam = 0x6101;

    // Geometry chunks
    constexpr u16 Geometry = 0x6200;
    constexpr u16 PrimGroup = 0x6201;
    constexpr u16 VertexList = 0x6202;
    constexpr u16 NormalList = 0x6203;
    constexpr u16 UVList = 0x6204;
    constexpr u16 ColourList = 0x6205;
    constexpr u16 IndexList = 0x6206;

    // Tool/debug markers
    constexpr u16 Separator = 0x7001;  // P3D_ALIGN = section separator

    // Bitmap chunks
    constexpr u16 BitmapRaw = 0xA001;
}

// Parsed chunk in memory (recursive tree)
struct tChunk {
    u16 id;
    u32 totalSize;
    std::vector<u8> data;              // payload bytes (excluding header + children)
    std::vector<tChunk> children;

    const u8* PayloadData() const { return data.data(); }
    u32 PayloadSize() const { return static_cast<u32>(data.size()); }
};

// Sequential chunk reader with BeginChunk/EndChunk API
class tChunkFile {
public:
    tChunkFile(const u8* data, u32 size);

    // Navigation
    bool ChunksRemaining();
    u16 BeginChunk();
    void EndChunk();

    u16 GetCurrentID() const;
    u32 GetCurrentDataLength() const;

    // Data readers
    u8 GetByte();
    u16 GetUShort();
    u32 GetUInt();
    s32 GetInt();
    f32 GetFloat();
    std::string GetPString();  // Pascal string: u8 length + chars

    void GetData(void* buf, u32 count);
    void Skip(u32 bytes);

    // Legacy names
    s32 GetLong() { return GetInt(); }
    u16 GetUWord() { return GetUShort(); }

private:
    static constexpr int STACK_SIZE = 32;

    struct Frame {
        u16 id;
        u32 dataLength;   // payload length (totalSize - 6)
        u32 startPos;     // position right after header
        u32 endPos;       // startPos + dataLength
    };

    const u8* buf;
    u32 bufSize;
    u32 pos;
    Frame stack[STACK_SIZE];
    int stackTop;

    u16 ReadU16();
    u32 ReadU32();
};

// Parse an entire chunk tree from raw bytes (convenience for bulk loading)
tChunk ParseChunkTree(const u8* data, u32 size);

// Parse all top-level chunks from a buffer
std::vector<tChunk> ParseAllChunks(const u8* data, u32 size);
