// loadmanager.cpp — tP3DFileHandler implementation
#include "p3d/loadmanager.h"
#include "p3d/chunkfile.h"
#include "p3d/inventory.h"

static bool IsP3DContainerChunk(u16 chunkID) {
    return chunkID == ChunkID::TexturePage || chunkID == ChunkID::P3DContainer;
}

static void LoadChunkTree(tP3DFileHandler* handlerSet, tChunkFile* file, tInventory* store) {
    while (file->ChunksRemaining()) {
        const u16 id = file->BeginChunk();

        if (tChunkHandler* handler = handlerSet->GetHandler(id)) {
            handler->LoadChunk(file, store);
        } else if (IsP3DContainerChunk(id)) {
            LoadChunkTree(handlerSet, file, store);
        }

        file->EndChunk();
    }
}

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
    LoadChunkTree(this, &file, store);
}
