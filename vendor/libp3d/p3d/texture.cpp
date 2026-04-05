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

#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdio>

// tTexture

tTexture::tTexture() = default;

tTexture::~tTexture() {
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
}

bool tTexture::Create(int width, int height, int bpp, int alphaDepth,
                      const void* rgba) {
    if (!p3d::device) return false;
    if (texture) {
        delete texture;
    }
    texture = p3d::device->NewTexture();
    texture->SetData(width, height, bpp, alphaDepth, rgba);
    return true;
}

int tTexture::GetWidth() const { return texture ? texture->GetWidth() : 0; }
int tTexture::GetHeight() const { return texture ? texture->GetHeight() : 0; }

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
    // Pending CLUT from previous sub-chunk
    struct PendingCLUT {
        std::vector<u8> rgba;  // decoded palette as RGBA8 (numColors * 4)
        int numColors;
    };
    PendingCLUT pendingClut;
    bool hasClut = false;

    while (file->ChunksRemaining()) {
        u16 id = file->BeginChunk();

        if (id == 0x6008) {
            u32 len = file->GetCurrentDataLength();
            std::vector<u8> buf(len);
            file->GetData(buf.data(), len);

            TexChunkHeader hdr;
            if (ParseTexChunkHeader(buf.data(), len, hdr)) {
                if (hdr.isClut) {
                    // Decode CLUT: ABGR1555 entries
                    int numColors = hdr.rw * std::max<int>(hdr.rh, 1);
                    pendingClut.numColors = numColors;
                    pendingClut.rgba.resize(numColors * 4);

                    for (int i = 0; i < numColors; ++i) {
                        if (static_cast<u32>(i * 2 + 2) <= hdr.rawSize) {
                            u16 c16 = hdr.rawData[i * 2] | (hdr.rawData[i * 2 + 1] << 8);
                            PsxColorToRGBA(c16,
                                           pendingClut.rgba[i * 4 + 0],
                                           pendingClut.rgba[i * 4 + 1],
                                           pendingClut.rgba[i * 4 + 2],
                                           pendingClut.rgba[i * 4 + 3]);
                        }
                    }
                    hasClut = true;
                }
                else if (hasClut) {
                    // Determine bpp from CLUT size
                    int bpp, actualW, actualH;
                    if (pendingClut.numColors <= 16) {
                        bpp = 4;
                        actualW = hdr.rw * 4;
                        actualH = hdr.rh;
                    }
                    else if (pendingClut.numColors <= 256) {
                        bpp = 8;
                        actualW = hdr.rw * 2;
                        actualH = hdr.rh;
                    }
                    else {
                        hasClut = false;
                        file->EndChunk();
                        continue;
                    }

                    if (actualW <= 0 || actualH <= 0) {
                        hasClut = false;
                        file->EndChunk();
                        continue;
                    }

                    // Decode pixels using the pending CLUT
                    u32 pixels = static_cast<u32>(actualW) * actualH;
                    std::vector<u8> rgba(pixels * 4);

                    if (bpp == 4) {
                        for (u32 i = 0; i < pixels; i += 2) {
                            u32 byteIdx = i / 2;
                            if (byteIdx >= hdr.rawSize) break;
                            u8 byte = hdr.rawData[byteIdx];
                            u8 lo = byte & 0x0F;
                            u8 hi = (byte >> 4) & 0x0F;

                            if (lo * 4 + 3 < static_cast<int>(pendingClut.rgba.size()))
                                std::memcpy(&rgba[i * 4], &pendingClut.rgba[lo * 4], 4);
                            if (i + 1 < pixels && hi * 4 + 3 < static_cast<int>(pendingClut.rgba.size()))
                                std::memcpy(&rgba[(i + 1) * 4], &pendingClut.rgba[hi * 4], 4);
                        }
                    }
                    else // bpp == 8
                    {
                        for (u32 i = 0; i < pixels; ++i) {
                            if (i >= hdr.rawSize) break;
                            u8 idx = hdr.rawData[i];
                            if (idx * 4 + 3 < static_cast<int>(pendingClut.rgba.size()))
                                std::memcpy(&rgba[i * 4], &pendingClut.rgba[idx * 4], 4);
                        }
                    }

                    auto* tex = new tTexture();
                    tex->SetName(hdr.name.c_str());
                    tex->Create(actualW, actualH, bpp, 1, rgba.data());
                    store->Store(tex);
                    tex->Release();

                    hasClut = false;
                }
            }
        }

        file->EndChunk();
    }
}
