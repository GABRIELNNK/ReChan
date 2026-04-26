#pragma once
#include "core.h"

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

// 2D overlay drawing
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

    // Draw a Gouraud-shaded quad with 4 per-vertex colors (PSX POLYG4).
    void DrawGouraudQuad(f32 x0, f32 y0, u8 r0, u8 g0, u8 b0, u8 a0,
                         f32 x1, f32 y1, u8 r1, u8 g1, u8 b1, u8 a1,
                         f32 x2, f32 y2, u8 r2, u8 g2, u8 b2, u8 a2,
                         f32 x3, f32 y3, u8 r3, u8 g3, u8 b3, u8 a3);

    // Cleanup cached resources.
    void Shutdown();
}
