#include "gen/common.h"
#include "gen/psxcolor_helpers.h"
#include "pc/tim.h"
#include "gen/config.h"
#include "p3d/texture.h"
#include "p3d/shader.h"
#include "p3d/matrix.h"
#include "p3d/context.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "pddi/pddishad.h"
#include "pddi/pdditex.h"
#include "xclib/xcfile.h"

// TIM file structures
struct TimHeader {
    u32 magic;  // 0x10
    u32 flags;  // bits 0-2: depth (0=4bit,1=8bit,2=15bit,3=24bit), bit 3: has CLUT
};

struct TimSection {
    u32 size;   // total section size in bytes (including this field)
    u16 x, y;   // VRAM destination
    u16 w, h;   // dimensions (in 16-bit units for image data)
};

TimImage* Tim::LoadFromFile(const char* path) {
    u8* data = nullptr;
    u32 fileSize = 0;
    if (!xcReadFileLow(path, &data, &fileSize)) {
        LOG("[Tim] Failed to open: %s", path);
        return nullptr;
    }

    TimImage* img = Tim::LoadFromMemory(data, fileSize);
    delete[] data;

    if (img) {
        LOG("[Tim] Loaded %s: %dx%d", path, img->width, img->height);
    }
    else {
        LOG("[Tim] Failed to decode: %s", path);
    }
    return img;
}

TimImage* Tim::LoadFromMemory(const u8* data, u32 fileSize) {
    if (!data || fileSize < 8)
        return nullptr;

    const TimHeader* hdr = reinterpret_cast<const TimHeader*>(data);
    if (hdr->magic != 0x10)
        return nullptr;

    u32 depth = hdr->flags & 0x07;
    bool hasClut = (hdr->flags & 0x08) != 0;
    u32 pos = 8;

    // Parse optional CLUT
    u16* clut = nullptr;
    s32 clutColors = 0;
    if (hasClut) {
        if (pos + 12 > (u32)fileSize) { return nullptr; }
        const TimSection* cs = reinterpret_cast<const TimSection*>(data + pos);
        clutColors = cs->w * cs->h;
        clut = new u16[clutColors];
        memcpy(clut, data + pos + 12, clutColors * 2);
        pos += cs->size;
    }

    // Parse image section
    if (pos + 12 > (u32)fileSize) {
        delete[] clut;
        return nullptr;
    }
    const TimSection* is = reinterpret_cast<const TimSection*>(data + pos);
    u16 iw = is->w; // in 16-bit halfword units
    u16 ih = is->h;
    const u8* imgData = data + pos + 12;

    // Calculate pixel dimensions
    s32 pixW, pixH;
    pixH = ih;
    switch (depth) {
        case 0: pixW = iw * 4; break;  // 4bpp: 4 pixels per halfword
        case 1: pixW = iw * 2; break;  // 8bpp: 2 pixels per halfword
        case 2: pixW = iw;     break;  // 16bpp: 1 pixel per halfword
        case 3: pixW = (iw * 2) / 3; break; // 24bpp
        default:
            delete[] clut;
            return nullptr;
    }

    TimImage* img = new TimImage();
    img->width = pixW;
    img->height = pixH;
    img->rgba = new u32[pixW * pixH];
    memset(img->rgba, 0, pixW * pixH * 4);

    // Decode pixels
    switch (depth) {
        case 0:
        { // 4bpp indexed
            if (!clut) break;
            for (s32 y = 0; y < pixH; y++) {
                const u8* row = imgData + y * iw * 2;
                for (s32 x = 0; x < pixW; x++) {
                    u8 byte = row[x / 2];
                    u8 idx = (x & 1) ? (byte >> 4) : (byte & 0x0F);
                    if (idx < clutColors)
                        img->rgba[y * pixW + x] = PsxAbgr1555ToRgba8888(clut[idx]);
                }
            }
            break;
        }
        case 1:
        { // 8bpp indexed
            if (!clut) break;
            for (s32 y = 0; y < pixH; y++) {
                const u8* row = imgData + y * iw * 2;
                for (s32 x = 0; x < pixW; x++) {
                    u8 idx = row[x];
                    if (idx < clutColors)
                        img->rgba[y * pixW + x] = PsxAbgr1555ToRgba8888(clut[idx]);
                }
            }
            break;
        }
        case 2:
        { // 16bpp direct color
            for (s32 y = 0; y < pixH; y++) {
                const u16* row = reinterpret_cast<const u16*>(imgData + y * iw * 2);
                for (s32 x = 0; x < pixW; x++) {
                    img->rgba[y * pixW + x] = PsxAbgr1555ToRgba8888(row[x]);
                }
            }
            break;
        }
        case 3:
        { // 24bpp direct color
            for (s32 y = 0; y < pixH; y++) {
                const u8* row = imgData + y * iw * 2;
                for (s32 x = 0; x < pixW; x++) {
                    s32 off = x * 3;
                    u8 r = row[off + 0];
                    u8 g = row[off + 1];
                    u8 b = row[off + 2];
                    img->rgba[y * pixW + x] = (255u << 24) | (b << 16) | (g << 8) | r;
                }
            }
            break;
        }
    }

    delete[] clut;
    return img;
}

tTexture* Tim::CreateTexture(const TimImage* img) {
    if (!img || !img->rgba) return nullptr;
    tTexture* tex = new tTexture();
    tex->Create(img->width, img->height, 32, 8, img->rgba);
    return tex;
}

// ScreenDraw

static pddiBaseShader* s_screenShader = nullptr;

static void EnsureShader() {
    if (s_screenShader) return;
    s_screenShader = p3d::device->NewShader("simple");
}

// Internal: begin 2D overlay rendering (saves projection, sets ortho).
static Mat4 BeginOverlay() {
    EnsureShader();
    Mat4 prev = p3d::context->GetProjectionMatrix();

    p3d::context->ResolveForOverlayPass();
    p3d::context->SetProjectionMatrix(Ortho(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f));
    p3d::context->EnableZBuffer(false);
    p3d::context->SetCullMode(PDDI_CULL_NONE);
    p3d::context->SetMultisampleEnabled(false);

    return prev;
}

// Internal: end 2D overlay rendering (restores previous state).
static void EndOverlay(const Mat4& prev) {
    p3d::context->SetProjectionMatrix(prev);
    p3d::context->EnableZBuffer(true);
    p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    p3d::context->SetMultisampleEnabled(true);
}

void ScreenDraw::DrawFullscreen(tTexture* tex) {
    if (!tex) return;
    Mat4 prev = BeginOverlay();
    p3d::context->SetBlendMode(PDDI_BLEND_NONE);

    s_screenShader->SetTexture(0, tex->GetTexture());
    s_screenShader->SetColour(0, pddiColour(255, 255, 255, 255));
    p3d::context->DrawQuad(s_screenShader, SCALE_AND_CENTER_X(0.0f), 0.0f, SCREEN_SCALE_X(DEFAULT_SCREEN_WIDTH), SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f);

    EndOverlay(prev);
}

void ScreenDraw::DrawQuad(tTexture* tex, f32 x, f32 y, f32 w, f32 h,
                          f32 u0, f32 v0, f32 u1, f32 v1,
                          u8 r, u8 g, u8 b, u8 a) {
    if (!tex)
        return;
    Mat4 prev = BeginOverlay();
    p3d::context->SetBlendMode(PDDI_BLEND_ALPHA);

    s_screenShader->SetTexture(0, tex->GetTexture());
    s_screenShader->SetColour(0, pddiColour(r, g, b, a));
    p3d::context->DrawQuad(s_screenShader, x, y, w, h, u0, v0, u1, v1);

    EndOverlay(prev);
}

void ScreenDraw::DrawColoredQuad(u8 r, u8 g, u8 b, u8 a) {
    DrawColoredRect(0.0f, 0.0f, (f32)SCREEN_WIDTH, (f32)SCREEN_HEIGHT, r, g, b, a);
}

void ScreenDraw::DrawColoredRect(f32 x, f32 y, f32 w, f32 h,
                                 u8 r, u8 g, u8 b, u8 a) {
    static tTexture* s_colorTex = nullptr;
    static u32 s_lastColor = 0;

    u32 color = (255u << 24) | ((u32)b << 16) | ((u32)g << 8) | (u32)r;
    if (!s_colorTex || s_lastColor != color) {
        if (s_colorTex) s_colorTex->Release();
        s_colorTex = new tTexture();
        s_colorTex->Create(1, 1, 32, 8, &color);
        s_lastColor = color;
    }

    Mat4 prev = BeginOverlay();
    p3d::context->SetBlendMode(PDDI_BLEND_ALPHA);

    s_screenShader->SetTexture(0, s_colorTex->GetTexture());
    s_screenShader->SetColour(0, pddiColour(255, 255, 255, a));
    p3d::context->DrawQuad(s_screenShader, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);

    EndOverlay(prev);
}

void ScreenDraw::DrawGouraudQuad(f32 x0, f32 y0, u8 r0, u8 g0, u8 b0, u8 a0,
                                 f32 x1, f32 y1, u8 r1, u8 g1, u8 b1, u8 a1,
                                 f32 x2, f32 y2, u8 r2, u8 g2, u8 b2, u8 a2,
                                 f32 x3, f32 y3, u8 r3, u8 g3, u8 b3, u8 a3) {
    Mat4 prev = BeginOverlay();
    p3d::context->SetBlendMode(PDDI_BLEND_ALPHA);

    p3d::context->DrawGouraudQuad(
        x0, y0, r0 / 255.0f, g0 / 255.0f, b0 / 255.0f, a0 / 255.0f,
        x1, y1, r1 / 255.0f, g1 / 255.0f, b1 / 255.0f, a1 / 255.0f,
        x2, y2, r2 / 255.0f, g2 / 255.0f, b2 / 255.0f, a2 / 255.0f,
        x3, y3, r3 / 255.0f, g3 / 255.0f, b3 / 255.0f, a3 / 255.0f);

    EndOverlay(prev);
}

void ScreenDraw::Shutdown() {
    if (s_screenShader) {
        s_screenShader->Release();
        s_screenShader = nullptr;
    }
}
