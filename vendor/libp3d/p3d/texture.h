// texture.h — tTexture + tTextureLoader
#pragma once

#include "p3d/entity.h"
#include "p3d/loadmanager.h"

class pddiTexture;

namespace p3d {
using RawTextureUploadCallback = void (*)(s16 x, s16 y, s16 w, s16 h, const u8* raw);
extern RawTextureUploadCallback rawTextureUploader;
}

struct tTextureRect {
    s16 x = 0;
    s16 y = 0;
    s16 w = 0;
    s16 h = 0;
};

// P3D texture entity
class tTexture : public tEntity {
public:
    tTexture();
    ~tTexture() override;

    static tTexture* LoadFromImagePath(const char* path, s32* outWidth = nullptr, s32* outHeight = nullptr);

    bool Create(int width, int height, int bpp, int alphaDepth, const void* rgba);

    pddiTexture* GetTexture() const { return texture; }
    int GetWidth() const;
    int GetHeight() const;
    u8* GetTextureData() const { return textureData; }
    void SetTextureData(const u8* data, u32 size);
    tTextureRect* GetTextureRect() { return &textureRect; }
    const tTextureRect* GetTextureRect() const { return &textureRect; }
    void SetTextureRect(s16 x, s16 y, s16 w, s16 h);
    u32 GetTextureType() const { return textureType; }
    void SetTextureType(u32 type) { textureType = type; }
    u32 GetTextureDataSize() const { return textureDataSize; }

private:
    pddiTexture* texture = nullptr;
    u8* textureData = nullptr;
    u32 textureDataSize = 0;
    tTextureRect textureRect;
    u32 textureType = 0;
};

// Chunk handler for 0xFF04 TexturePage
class tTextureLoader : public tChunkHandler {
public:
    u16  GetChunkID() override { return 0x6008; }
    void LoadChunk(tChunkFile* file, tInventory* store) override;
};

// Convert PSX ABGR1555 colour to RGBA8888
inline void PsxColorToRGBA(u16 c, u8& r, u8& g, u8& b, u8& a) {
    r = static_cast<u8>((c & 0x1F) << 3);
    g = static_cast<u8>(((c >> 5) & 0x1F) << 3);
    b = static_cast<u8>(((c >> 10) & 0x1F) << 3);
    a = (c == 0) ? 0 : 255;
}
