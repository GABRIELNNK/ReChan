// tim.h - PSX TIM file loader
// Converts PSX TIM image files to RGBA32 for PC rendering.
#pragma once

#include "gen/common.h"

class tTexture;
class pddiBaseShader;

// Loaded TIM image data
struct TimImage {
    s32 width = 0;      // pixel width
    s32 height = 0;     // pixel height
    u32* rgba = nullptr; // RGBA32 pixel data (width * height)

    ~TimImage() { delete[] rgba; }
};

// PSX TIM file loader
namespace Tim {
    // Load a PSX TIM file from disk, decode to RGBA32.
    // Returns nullptr on failure. Caller owns the result.
    TimImage* LoadFromFile(const char* path);

    // Decode a TIM from a memory buffer.
    // Returns nullptr on failure. Caller owns the result.
    TimImage* LoadFromMemory(const u8* data, u32 size);

    // Create a tTexture from a TimImage (uploads to GPU).
    // Caller owns the returned texture.
    tTexture* CreateTexture(const TimImage* img);
}

// 2D overlay drawing - all coordinates in normalized 0..1 range.
// Creates/caches an internal shader on first use.
namespace ScreenDraw {
    // Draw a texture filling the entire screen (opaque, no alpha).
    void DrawFullscreen(tTexture* tex);

    // Draw a textured quad with optional UV and color tint.
    // Default UV = full texture, default color = white (no tint).
    // PSX GPU neutral color = 128 (tex * 128/128 = 1.0x)
    void DrawQuad(tTexture* tex, f32 x, f32 y, f32 w, f32 h,
                  f32 u0 = 0.0f, f32 v0 = 0.0f, f32 u1 = 1.0f, f32 v1 = 1.0f,
                  u8 r = 128, u8 g = 128, u8 b = 128, u8 a = 255);

    // Draw a solid colored rectangle (no texture).
    void DrawColoredRect(f32 x, f32 y, f32 w, f32 h,
                         u8 r, u8 g, u8 b, u8 a);

    // Draw a fullscreen colored quad (used for fade transitions).
    void DrawColoredQuad(u8 r, u8 g, u8 b, u8 a);

    // Cleanup cached resources.
    void Shutdown();
}
