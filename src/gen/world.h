// world.h — Level world: loads BLK blocks from an LCF stream
#ifndef WORLD_H
#define WORLD_H

#include "core.h"
#include "gen/geometry.h"
#include <vector>
#include <string>
#include <cstring>

// PSX VRAM simulation (1024x512 16-bit words), heap-allocated
struct PsxVRAM {
    u16* data; // [y * 1024 + x], 1024x512

    PsxVRAM() : data(new u16[1024 * 512]()) {}
    ~PsxVRAM() { delete[] data; }
    PsxVRAM(const PsxVRAM&) = delete;
    PsxVRAM& operator=(const PsxVRAM&) = delete;

    u16  Get(int x, int y) const { return data[y * 1024 + x]; }
    void Set(int x, int y, u16 v) { data[y * 1024 + x] = v; }

    void Clear() { std::memset(data, 0, 1024 * 512 * sizeof(u16)); }

    void Upload(s16 x, s16 y, s16 w, s16 h, const u8* raw) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int vx = x + col, vy = y + row;
                if (vx >= 0 && vx < 1024 && vy >= 0 && vy < 512) {
                    int idx = (row * w + col) * 2;
                    Set(vx, vy, static_cast<u16>(raw[idx] | (raw[idx + 1] << 8)));
                }
            }
        }
    }

    // Decode a 256x256 texture page to RGBA8 (out must be 256*256*4 bytes)
    void DecodePage(u16 tpage, u16 cba, u8* rgbaOut) const;
};

class World {
public:
    World();
    ~World();

    bool Load(const std::string& lcfPath);
    void Render();
    void Unload();

    u32 GetBlockCount() const { return static_cast<u32>(mBlocks.size()); }
    void SetTargetLOD(u16 lod) { mTargetLOD = lod; }
    u16 GetTargetLOD() const { return mTargetLOD; }

private:
    std::vector<BlockMesh> mBlocks;
    PsxVRAM mVRAM;
    u32 mVRAMHandle = 0; // raw GL_R16UI texture of VRAM
    u16 mTargetLOD = 3;  // default LOD level to render

    void LoadTPGTextures(const u8* lcfData, u32 lcfSize);
};

// Free camera for level viewing (WASD + LMB drag)
struct FreeCamera {
    f32 x = 3240, y = -192, z = -648; // position (level center)
    f32 yaw   = 0.0f;   // radians, 0 = looking along +Z
    f32 pitch = 0.0f;
    f32 speed = 3000.0f;
    f32 sensitivity = 0.003f;

    // Update from keyboard + mouse (call each frame with dt)
    void Update(void* window, f32 dt);

    // Build view + projection matrices and set them on the context
    void Apply();

private:
    double mLastMX = 0, mLastMY = 0;
    bool mHasPrev = false;
};

#endif // WORLD_H
