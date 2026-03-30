// stream.cpp — Stream and RR archive parser implementation
#include "p3d/stream.h"
#include <cstring>

// Read a big-endian u32
static u32 ReadBE32(const u8* p) {
    return (static_cast<u32>(p[0]) << 24)
        | (static_cast<u32>(p[1]) << 16)
        | (static_cast<u32>(p[2]) << 8)
        | static_cast<u32>(p[3]);
}

// Read a little-endian u32
static u32 ReadLE32(const u8* p) {
    return  static_cast<u32>(p[0])
        | (static_cast<u32>(p[1]) << 8)
        | (static_cast<u32>(p[2]) << 16)
        | (static_cast<u32>(p[3]) << 24);
}

std::vector<tStreamEntry> ParseStreamHeader(const u8* data, u32 dataSize) {
    std::vector<tStreamEntry> entries;

    if (dataSize < 4)
        return entries;

    u32 count = ReadBE32(data);
    if (count > 10000)  // sanity
        return entries;

    u32 pos = 4;
    for (u32 i = 0; i < count; ++i) {
        if (pos + 16 > dataSize)
            break;

        tStreamEntry e;
        e.magic[0] = static_cast<char>(data[pos + 0]);
        e.magic[1] = static_cast<char>(data[pos + 1]);
        e.magic[2] = static_cast<char>(data[pos + 2]);
        e.magic[3] = static_cast<char>(data[pos + 3]);
        e.magic[4] = '\0';

        e.size = ReadBE32(data + pos + 4);
        e.offset = ReadBE32(data + pos + 8);
        u32 extraLen = ReadBE32(data + pos + 12);
        pos += 16;

        if (extraLen > 0) {
            u32 aligned = (extraLen + 3) & ~3u;
            if (pos + aligned <= dataSize) {
                e.extra.assign(data + pos, data + pos + extraLen);
            }
            pos += aligned;
        }

        entries.push_back(std::move(e));
    }

    return entries;
}

std::vector<tRREntry> ParseRRHeader(const u8* data, u32 dataSize) {
    std::vector<tRREntry> entries;

    if (dataSize < 8)
        return entries;

    // Check magic "PX"
    if (data[0] != 'P' || data[1] != 'X')
        return entries;

    u32 headerSize = ReadLE32(data + 4);
    u32 numEntries = headerSize / 8;

    for (u32 i = 1; i < numEntries; ++i)  // skip entry 0
    {
        u32 base = i * 8;
        if (base + 8 > dataSize)
            break;

        tRREntry e;
        e.index = i;
        e.offset = ReadLE32(data + base);
        u32 sizeFlags = ReadLE32(data + base + 4);
        e.size = sizeFlags >> 8;
        e.flags = static_cast<u8>(sizeFlags & 0xFF);

        if (e.size > 0)
            entries.push_back(e);
    }

    return entries;
}
