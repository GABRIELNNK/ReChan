// loadmanager.cpp — tP3DFileHandler implementation
#include "p3d/loadmanager.h"
#include "p3d/chunkfile.h"
#include "p3d/inventory.h"

tP3DFileHandler::~tP3DFileHandler() {
    for (auto* h : handlers)
        delete h;
}

void tP3DFileHandler::AddHandler(tChunkHandler* handler) {
    handlers.push_back(handler);
}

tChunkHandler* tP3DFileHandler::GetHandler(u16 chunkID) {
    for (auto* h : handlers) {
        if (h->GetChunkID() == chunkID)
            return h;
    }
    return nullptr;
}

void tP3DFileHandler::LoadFromMemory(const u8* data, u32 size, tInventory* store) {
    tChunkFile file(data, size);

    while (file.ChunksRemaining()) {
        u16 id = file.BeginChunk();

        tChunkHandler* handler = GetHandler(id);
        if (handler)
            handler->LoadChunk(&file, store);

        file.EndChunk();
    }
}
