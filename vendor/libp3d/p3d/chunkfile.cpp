// chunkfile.cpp = P3D chunk file reader implementation
#include "p3d/chunkfile.h"
#include <cassert>
#include <stdexcept>

static constexpr u32 CHUNK_HEADER_SIZE = 6; // u16 id + u32 totalSize

// tChunkFile

tChunkFile::tChunkFile(const u8* data, u32 size)
    : buf(data), bufSize(size), pos(0), stackTop(-1) {}

bool tChunkFile::ChunksRemaining() {
    if (stackTop < 0)
        return pos + CHUNK_HEADER_SIZE <= bufSize;

    return pos + CHUNK_HEADER_SIZE <= stack[stackTop].endPos;
}

u16 tChunkFile::BeginChunk() {
    assert(stackTop < STACK_SIZE - 1);

    u16 id = ReadU16();
    u32 totalSize = ReadU32();
    u32 dataLength = totalSize - CHUNK_HEADER_SIZE;

    int next = ++stackTop;
    stack[next].id = id;
    stack[next].dataLength = dataLength;
    stack[next].startPos = pos;
    stack[next].endPos = pos + dataLength;

    return id;
}

void tChunkFile::EndChunk() {
    assert(stackTop >= 0);

    // Skip any unread data in this chunk
    pos = stack[stackTop].endPos;
    --stackTop;
}

u16 tChunkFile::GetCurrentID() const {
    assert(stackTop >= 0);
    return stack[stackTop].id;
}

u32 tChunkFile::GetCurrentDataLength() const {
    assert(stackTop >= 0);
    return stack[stackTop].dataLength;
}

u8 tChunkFile::GetByte() {
    assert(pos + 1 <= bufSize);
    return buf[pos++];
}

u16 tChunkFile::GetUShort() {
    return ReadU16();
}

u32 tChunkFile::GetUInt() {
    return ReadU32();
}

s32 tChunkFile::GetInt() {
    return static_cast<s32>(ReadU32());
}

f32 tChunkFile::GetFloat() {
    u32 bits = ReadU32();
    f32 result;
    std::memcpy(&result, &bits, sizeof(f32));
    return result;
}

std::string tChunkFile::GetPString() {
    u8 len = GetByte();
    std::string s(reinterpret_cast<const char*>(buf + pos), len);
    pos += len;
    return s;
}

void tChunkFile::GetData(void* dst, u32 count) {
    assert(pos + count <= bufSize);
    std::memcpy(dst, buf + pos, count);
    pos += count;
}

void tChunkFile::Skip(u32 bytes) {
    pos += bytes;
}

u16 tChunkFile::ReadU16() {
    assert(pos + 2 <= bufSize);
    u16 v = static_cast<u16>(buf[pos])
        | (static_cast<u16>(buf[pos + 1]) << 8);
    pos += 2;
    return v;
}

u32 tChunkFile::ReadU32() {
    assert(pos + 4 <= bufSize);
    u32 v = static_cast<u32>(buf[pos])
        | (static_cast<u32>(buf[pos + 1]) << 8)
        | (static_cast<u32>(buf[pos + 2]) << 16)
        | (static_cast<u32>(buf[pos + 3]) << 24);
    pos += 4;
    return v;
}

// Tree parser

// Only these chunk IDs are known to contain child chunks.
// All others have raw payload data.
static bool IsContainerChunk(u16 id) {
    switch (id) {
        case ChunkID::TexturePage:   // 0xFF04
        case ChunkID::P3DContainer:  // 0x6000
            return true;
        default:
            return false;
    }
}

static tChunk ParseOneChunk(const u8* data, u32 size, u32& offset) {
    tChunk chunk;
    chunk.id = static_cast<u16>(data[offset])
        | (static_cast<u16>(data[offset + 1]) << 8);
    offset += 2;

    chunk.totalSize = static_cast<u32>(data[offset])
        | (static_cast<u32>(data[offset + 1]) << 8)
        | (static_cast<u32>(data[offset + 2]) << 16)
        | (static_cast<u32>(data[offset + 3]) << 24);
    offset += 4;

    u32 payloadSize = chunk.totalSize - CHUNK_HEADER_SIZE;
    u32 payloadEnd = offset + payloadSize;

    if (IsContainerChunk(chunk.id) && payloadSize >= CHUNK_HEADER_SIZE) {
        while (offset + CHUNK_HEADER_SIZE <= payloadEnd) {
            chunk.children.push_back(ParseOneChunk(data, size, offset));
        }
    }
    else {
        chunk.data.assign(data + offset, data + payloadEnd);
        offset = payloadEnd;
    }

    return chunk;
}

tChunk ParseChunkTree(const u8* data, u32 size) {
    u32 offset = 0;
    return ParseOneChunk(data, size, offset);
}

std::vector<tChunk> ParseAllChunks(const u8* data, u32 size) {
    std::vector<tChunk> chunks;
    u32 offset = 0;
    while (offset + CHUNK_HEADER_SIZE <= size) {
        chunks.push_back(ParseOneChunk(data, size, offset));
    }
    return chunks;
}
