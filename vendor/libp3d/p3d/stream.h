// stream.h — Stream (.LCF/.GCF) and RR (.RR) archive readers
//
// Stream: big-endian header, entries with 4-char magic + size + offset.
// RR (PETLATL): little-endian "PX" header, 8-byte entries.
//
#pragma once

#include "core.h"
#include <string>
#include <vector>

// A single entry in a Stream archive
struct tStreamEntry {
    char     magic[5];  // null-terminated 4-char tag
    u32      size;
    u32      offset;
    std::vector<u8> extra;
};

// A single entry in an RR archive
struct tRREntry {
    u32 index;
    u32 offset;
    u32 size;
    u8  flags;
};

// Parse a Stream file header from raw file data.
// Returns the list of entries. The raw data pointer is needed
// to later read entry payloads at their offsets.
std::vector<tStreamEntry> ParseStreamHeader(const u8* data, u32 dataSize);

// Parse an RR file header. Entry 0 is skipped (overlaps magic).
std::vector<tRREntry> ParseRRHeader(const u8* data, u32 dataSize);

// Read the payload for a stream entry from the file data
inline const u8* GetStreamEntryData(const u8* fileData, const tStreamEntry& entry) {
    return fileData + entry.offset;
}
