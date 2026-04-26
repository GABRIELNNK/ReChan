// texture.cpp — tTexture + tTextureLoader implementation
//
// PSX 0x6008 format: PString name, RECT16 vram, u32 type, raw data.
// Textures come in CLUT+pixel pairs within a 0xFF04 TexturePage.
// 4bpp: w = rw*4, 8bpp: w = rw*2.
//
#include "p3d/texture.h"
#include "p3d/context.h"
#include "p3d/chunkfile.h"
#include "p3d/inventory.h"
#include "pddi/pddidev.h"
#include "pddi/pdditex.h"

#include <string>
#include <cstring>

namespace p3d {
    RawTextureUploadCallback rawTextureUploader = nullptr;
}

// tTexture

tTexture::tTexture() = default;

tTexture::~tTexture() {
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
    delete[] textureData;
    textureData = nullptr;
    textureDataSize = 0;
}

bool tTexture::Create(int width, int height, int bpp, int alphaDepth,
                      const void* rgba) {
    if (!p3d::device) return false;
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
    texture = p3d::device->NewTexture();
    texture->SetData(width, height, bpp, alphaDepth, rgba);
    return true;
}

int tTexture::GetWidth() const { return texture ? texture->GetWidth() : 0; }
int tTexture::GetHeight() const { return texture ? texture->GetHeight() : 0; }

void tTexture::SetTextureData(const u8* data, u32 size) {
    delete[] textureData;
    textureData = nullptr;
    textureDataSize = 0;

    if (!data || size == 0) {
        return;
    }

    textureData = new u8[size];
    std::memcpy(textureData, data, size);
    textureDataSize = size;
}

void tTexture::SetTextureRect(s16 x, s16 y, s16 w, s16 h) {
    textureRect.x = x;
    textureRect.y = y;
    textureRect.w = w;
    textureRect.h = h;
}

// 0x6008 sub-chunk parser

struct TexChunkHeader {
    std::string name;
    s16 rx, ry, rw, rh;   // VRAM rect
    u32 type;
    const u8* rawData;
    u32 rawSize;
    bool isClut;
};

static bool ParseTexChunkHeader(const u8* data, u32 size, TexChunkHeader& out) {
    if (!data || size < 1) return false;

    u32 pos = 0;
    u8 nameLen = data[pos++];
    if (pos + nameLen + 12 > size) return false;

    out.name.assign(reinterpret_cast<const char*>(data + pos), nameLen);
    pos += nameLen;

    // Trim trailing whitespace/nulls
    while (!out.name.empty() && (out.name.back() == ' ' || out.name.back() == '\0'))
        out.name.pop_back();

    // RECT16: x, y, w, h (s16 LE)
    auto readS16 = [&]() -> s16 {
        s16 v = static_cast<s16>(data[pos] | (data[pos + 1] << 8));
        pos += 2;
        return v;
    };
    out.rx = readS16();
    out.ry = readS16();
    out.rw = readS16();
    out.rh = readS16();

    // u32 type
    out.type = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
    pos += 4;

    out.rawData = data + pos;
    out.rawSize = size - pos;
    out.isClut = out.name.find("CLUT") != std::string::npos;

    return true;
}

// tTextureLoader

void tTextureLoader::LoadChunk(tChunkFile* file, tInventory* store) {
    if (!store) {
        return;
    }

    const u32 len = file->GetCurrentDataLength();
    std::string payload(len, '\0');
    file->GetData(payload.data(), len);

    TexChunkHeader hdr;
    if (!ParseTexChunkHeader(reinterpret_cast<const u8*>(payload.data()), len, hdr)) {
        return;
    }

    auto* tex = new tTexture();
    tex->SetName(hdr.name.c_str());
    tex->SetTextureRect(hdr.rx, hdr.ry, hdr.rw, hdr.rh);
    tex->SetTextureType(hdr.type);
    tex->SetTextureData(hdr.rawData, hdr.rawSize);
    store->Store(tex);
    tex->Release();
}
