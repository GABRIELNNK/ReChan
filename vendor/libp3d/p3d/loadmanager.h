// loadmanager.h — tChunkHandler + tP3DFileHandler
#pragma once

#include "core.h"
#include <vector>

class tChunkFile;
class tInventory;

// Handler for a specific chunk type
class tChunkHandler {
public:
    virtual ~tChunkHandler() = default;
    virtual u16  GetChunkID() = 0;
    virtual void LoadChunk(tChunkFile* file, tInventory* store) = 0;
};

// Manages chunk handlers and coordinates loading
class tP3DFileHandler {
public:
    ~tP3DFileHandler();

    void           AddHandler(tChunkHandler* handler);
    tChunkHandler* GetHandler(u16 chunkID);

    // Load all chunks from a raw P3D data buffer into the inventory
    void LoadFromMemory(const u8* data, u32 size, tInventory* store);

private:
    std::vector<tChunkHandler*> handlers;
};
