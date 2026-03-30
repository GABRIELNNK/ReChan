// texture.h — tTexture + tTextureLoader
#ifndef P3D_TEXTURE_H
#define P3D_TEXTURE_H

#include "p3d/entity.h"
#include "p3d/loadmanager.h"

class pddiTexture;

// P3D texture entity
class tTexture : public tEntity {
public:
    tTexture();
    ~tTexture() override;

    bool Create(int width, int height, int bpp, int alphaDepth, const void* rgba);

    pddiTexture* GetTexture() const { return mTexture; }
    int GetWidth() const;
    int GetHeight() const;

private:
    pddiTexture* mTexture = nullptr;
};

// Chunk handler for 0xFF04 TexturePage
class tTextureLoader : public tChunkHandler {
public:
    u16  GetChunkID() override { return 0xFF04; }
    void LoadChunk(tChunkFile* file, tInventory* store) override;
};

// Convert PSX ABGR1555 colour to RGBA8888
inline void PsxColorToRGBA(u16 c, u8& r, u8& g, u8& b, u8& a) {
    r = static_cast<u8>((c & 0x1F) << 3);
    g = static_cast<u8>(((c >> 5) & 0x1F) << 3);
    b = static_cast<u8>(((c >> 10) & 0x1F) << 3);
    a = (c == 0) ? 0 : 255;
}

#endif // P3D_TEXTURE_H
