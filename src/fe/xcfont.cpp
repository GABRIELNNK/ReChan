// xcfont.cpp - xcFont reversed from PSX XCFONT.CPP
// PSX source: C:\CHAN\GAME\SRC\FE\XCFONT.CPP
// Font construction from raw .1 file data (PHO blocks).
// Raw data format per font:
//   [0]: "PHO\0" magic
//   [4]: u32 totalBlockSize
//   [8]: u32 nameSize
//   [12]: char name[nameSize]
//   [12+nameSize]: metric data:
//     +0: u8 lineHeight
//     +4: u32 imageCount
//     +8: image RECT/pixel data (repeat imageCount)
//     ...: u32 paletteCount
//     ...: palette entries (36 bytes each: clutX, clutY, 16 x u16 colors)
//     ...: u32 glyphCounts[imageCount * paletteCount]
//     ...: xciSpriteLetter[totalGlyphs] (8 bytes each)
#include "fe/xcfont.h"
#include "gen/common.h"
#include "xclib/xclib.h"
#include "pc/tim.h"
#include "p3d/texture.h"

// PSX: gp[56] global oxFontFile instance
oxFontFile* g_oxFontFile = nullptr;

// PSX ABGR1555 to RGBA8888 (shared with xclib.cpp)
static inline u32 psxColorToRGBA(u16 c) {
    if (c == 0) return 0; // transparent black
    u32 r = (c & 0x1F) << 3;
    u32 g = ((c >> 5) & 0x1F) << 3;
    u32 b = ((c >> 10) & 0x1F) << 3;
    return (255u << 24) | (b << 16) | (g << 8) | r;
}

// ---- xcFont ----

// PSX: __6xcFontPv (XCFONT.CPP:92, 0x800915A0)
// Constructs xcFont from raw PHO data.
xcFont::xcFont(const u8* rawData) {
    MARKFUNCTION(0x800915A0);

    memset(charTable, 0xFF, sizeof(charTable));
    sprites = nullptr;
    extended = nullptr;
    numSprites = 0;
    numExtended = 0;
    textures = nullptr;
    numTextures = 0;
    texWidths = nullptr;
    texHeights = nullptr;

    // rawData points to PHO block: [magic(4)][totalSize(4)][nameSize(4)][name...]
    u32 nameSize = *(const u32*)(rawData + 8);
    const u8* metric = rawData + 12 + nameSize;

    // +0: lineHeight (u8)
    lineHeight = metric[0];

    // +4: imageCount (u32)
    u32 imageCount = *(const u32*)(metric + 4);

    // Parse image RECTs and pixel data
    struct ImageRect {
        s16 x, y, w, h;
        const u8* pixelData;
        u32 pixelBytes;
    };
    ImageRect* imageRects = new ImageRect[imageCount > 0 ? imageCount : 1];

    const u8* pos = metric + 8;
    for (u32 i = 0; i < imageCount; i++) {
        imageRects[i].x = *(const s16*)(pos + 0);
        imageRects[i].y = *(const s16*)(pos + 2);
        imageRects[i].w = *(const s16*)(pos + 4);
        imageRects[i].h = *(const s16*)(pos + 6);
        pos += 8; // past RECT header
        u32 pixBytes = 4 * (((u32)(imageRects[i].w * 2 * imageRects[i].h) + 3) >> 2);
        imageRects[i].pixelData = pos;
        imageRects[i].pixelBytes = pixBytes;
        pos += pixBytes;
    }

    // Parse palettes
    u32 paletteCount = *(const u32*)pos;
    pos += 4;

    struct PaletteEntry {
        s16 clutX, clutY;
        const u8* colorData; // 16 x u16 = 32 bytes
    };
    PaletteEntry* palettes = new PaletteEntry[paletteCount > 0 ? paletteCount : 1];
    for (u32 i = 0; i < paletteCount; i++) {
        palettes[i].clutX = *(const s16*)(pos + 0);
        palettes[i].clutY = *(const s16*)(pos + 2);
        palettes[i].colorData = pos + 4;
        pos += 36; // 4 header + 32 CLUT data
    }

    // Parse glyph count table: imageCount * paletteCount entries
    u32 totalEntries = imageCount * paletteCount;
    const u32* glyphCounts = reinterpret_cast<const u32*>(pos);
    u32 totalGlyphs = 0;
    for (u32 i = 0; i < totalEntries; i++) {
        totalGlyphs += glyphCounts[i];
    }
    pos += totalEntries * 4;

    // PC: create decoded textures (one per image+palette combo)
    numTextures = (s32)totalEntries;
    textures = new tTexture*[numTextures];
    texWidths = new s32[numTextures];
    texHeights = new s32[numTextures];
    for (s32 t = 0; t < numTextures; t++) {
        textures[t] = nullptr;
        texWidths[t] = 0;
        texHeights[t] = 0;
    }

    s32 texIdx = 0;
    for (u32 img = 0; img < imageCount; img++) {
        for (u32 pal = 0; pal < paletteCount; pal++) {
            s32 tw = 0, th = 0;
            textures[texIdx] = DecodeImageWithPalette(
                imageRects[img].pixelData,
                imageRects[img].w, imageRects[img].h,
                palettes[pal].colorData,
                &tw, &th);
            texWidths[texIdx] = tw;
            texHeights[texIdx] = th;
            texIdx++;
        }
    }

    // Allocate sprite letters
    numSprites = (s32)totalGlyphs;
    sprites = new xcSpriteLetter[totalGlyphs > 0 ? totalGlyphs : 1];
    memset(sprites, 0, (totalGlyphs > 0 ? totalGlyphs : 1) * sizeof(xcSpriteLetter));

    // Parse glyph data: nested loop matching PSX constructor
    // xciSpriteLetter raw format (8 bytes):
    //   [0]: u8 x_offset (pixels within texture)
    //   [2]: u8 y_offset (pixels from image top)
    //   [4]: u8 width (glyph pixel width)
    //   [6]: u16 charCode
    const u8* glyphData = pos;
    s32 sprIdx = 0;
    s32 countTableIdx = 0;

    for (u32 img = 0; img < imageCount; img++) {
        for (u32 pal = 0; pal < paletteCount; pal++) {
            u32 nGlyphs = glyphCounts[countTableIdx];
            s32 tIdx = (s32)(img * paletteCount + pal);

            for (u32 g = 0; g < nGlyphs; g++) {
                const u8* gd = glyphData + (sprIdx + g) * 8;
                u8 xOff = gd[0];
                u8 yOff = gd[2];
                u8 gWidth = gd[4];
                u16 charCode = *(const u16*)(gd + 6);

                xcSpriteLetter& spr = sprites[sprIdx + g];
                spr.u0 = xOff;
                spr.v0 = yOff;
                spr.u1 = xOff + gWidth;
                spr.v1 = yOff + lineHeight;
                spr.w = gWidth;
                spr.h = lineHeight;
                spr.charCode = charCode;
                spr.texIdx = (u8)tIdx;
            }
            sprIdx += nGlyphs;
            countTableIdx++;
        }
    }

    // PSX: GenShakerSort by charCode
    // Simple sort since counts are small
    for (s32 i = 0; i < numSprites - 1; i++) {
        for (s32 j = i + 1; j < numSprites; j++) {
            if (sprites[i].charCode > sprites[j].charCode) {
                xcSpriteLetter tmp = sprites[i];
                sprites[i] = sprites[j];
                sprites[j] = tmp;
            }
        }
    }

    // Build charTable[256]: for chars < 255, map charCode -> sprite index
    // PSX: sets charTable[charCode] = spriteIndex for chars < 0xFF
    // PSX: chars >= 0xFF go into extended array
    for (s32 i = 0; i < numSprites; i++) {
        if (sprites[i].charCode >= 0xFF) {
            // First extended char found
            extended = &sprites[i];
            numExtended = numSprites - i;
            break;
        }
        if (sprites[i].charCode < 256) {
            charTable[sprites[i].charCode] = (u8)i;
        }
    }

    // PSX: spaceWidth from FindLetter(' ')
    const xcSpriteLetter* spaceLetter = FindLetter((u8)' ');
    if (spaceLetter) {
        spaceWidth = spaceLetter->w;
    } else {
        spaceWidth = 4; // PSX default when space not found
    }

    delete[] imageRects;
    delete[] palettes;

    LOG("[xcFont] Constructed: %d glyphs, lineHeight=%d, spaceWidth=%d",
           numSprites, lineHeight, spaceWidth);
}

// PSX: _._6xcFont (XCFONT.CPP:204, 0x800919BC)
xcFont::~xcFont() {
    MARKFUNCTION(0x800919BC);
    delete[] sprites;
    sprites = nullptr;
    extended = nullptr;
    if (textures) {
        for (s32 i = 0; i < numTextures; i++) {
            if (textures[i]) textures[i]->Release();
        }
        delete[] textures;
        textures = nullptr;
    }
    delete[] texWidths;
    delete[] texHeights;
    texWidths = nullptr;
    texHeights = nullptr;
}

// PSX: ReloadData__6xcFontPv (XCFONT.CPP:268, 0x80091B20)
// On PSX, reloads image/palette data to VRAM without rebuilding sprites.
// On PC, we need to re-decode the textures from the new raw data.
void xcFont::ReloadData(const u8* rawData) {
    MARKFUNCTION(0x80091B20);

    u32 nameSize = *(const u32*)(rawData + 8);
    const u8* metric = rawData + 12 + nameSize;

    lineHeight = metric[0];
    u32 imageCount = *(const u32*)(metric + 4);

    // Parse image RECTs
    const u8* pos = metric + 8;
    struct ImageRect { s16 x, y, w, h; const u8* pixelData; };
    ImageRect* imageRects = new ImageRect[imageCount > 0 ? imageCount : 1];
    for (u32 i = 0; i < imageCount; i++) {
        imageRects[i].x = *(const s16*)(pos); imageRects[i].y = *(const s16*)(pos + 2);
        imageRects[i].w = *(const s16*)(pos + 4); imageRects[i].h = *(const s16*)(pos + 6);
        pos += 8;
        u32 pixBytes = 4 * (((u32)(imageRects[i].w * 2 * imageRects[i].h) + 3) >> 2);
        imageRects[i].pixelData = pos;
        pos += pixBytes;
    }

    u32 paletteCount = *(const u32*)pos;
    pos += 4;

    struct PalEntry { s16 cx, cy; const u8* colorData; };
    PalEntry* pals = new PalEntry[paletteCount > 0 ? paletteCount : 1];
    for (u32 i = 0; i < paletteCount; i++) {
        pals[i].cx = *(const s16*)(pos); pals[i].cy = *(const s16*)(pos + 2);
        pals[i].colorData = pos + 4;
        pos += 36;
    }

    // Re-decode textures
    s32 texIdx = 0;
    for (u32 img = 0; img < imageCount; img++) {
        for (u32 pal = 0; pal < paletteCount; pal++) {
            if (texIdx < numTextures) {
                if (textures[texIdx]) { textures[texIdx]->Release(); textures[texIdx] = nullptr; }
                textures[texIdx] = DecodeImageWithPalette(
                    imageRects[img].pixelData,
                    imageRects[img].w, imageRects[img].h,
                    pals[pal].colorData,
                    &texWidths[texIdx], &texHeights[texIdx]);
            }
            texIdx++;
        }
    }

    delete[] imageRects;
    delete[] pals;
}

// PSX: FindLetter__6xcFontCFUc (XCFONT.CPP:319, 0x80091C64)
const xcSpriteLetter* xcFont::FindLetter(u8 ch) const {
    MARKFUNCTION(0x80091C64);
    u8 idx = charTable[ch];
    if (idx == 0xFF) return nullptr;
    if ((s32)idx >= numSprites) return nullptr;
    return &sprites[idx];
}

// PSX: FindLetter__6xcFontCFUs (XCFONT.CPP:254, 0x80091AC8)
const xcSpriteLetter* xcFont::FindLetter(u16 ch) const {
    MARKFUNCTION(0x80091AC8);
    if (ch < 0xFF) return FindLetter((u8)ch);
    // PSX: binary search on extended letter table
    if (!extended || numExtended == 0) return nullptr;
    // Binary search by charCode
    s32 lo = 0, hi = numExtended - 1;
    while (lo <= hi) {
        s32 mid = (lo + hi) / 2;
        u16 midCode = extended[mid].charCode;
        if (midCode == ch) return &extended[mid];
        if (midCode < ch) lo = mid + 1;
        else hi = mid - 1;
    }
    return nullptr;
}

// PC: Draw text at 512x240 overlay coordinates with xcJustify flags
// PSX: xcFontDC::Draw + PushJustTrans + MakePolys (XCFONTDC.CPP)
void xcFont::DrawText(const char* text, s32 screenX, s32 screenY,
                      u32 color, u8 justify, s32 lineSpacing) const {
    if (!text || !sprites || numTextures == 0) return;

    u8 cr = (u8)(color & 0xFF);
    u8 cg = (u8)((color >> 8) & 0xFF);
    u8 cb = (u8)((color >> 16) & 0xFF);
    u8 ca = (u8)((color >> 24) & 0xFF);

    // Vertical justification
    if (justify & XC_JUST_BOTTOM) {
        s32 numLines = 1;
        for (const char* p = text; *p; p++) {
            if (*p == '\n') numLines++;
        }
        s32 totalH = (s32)lineHeight * numLines + lineSpacing * (numLines - 1);
        if ((justify & XC_JUST_VMASK) == XC_JUST_VCENTER)
            screenY -= totalH / 2;
        else
            screenY -= totalH;
    }

    // Horizontal justification per line (PSX PushJustTrans)
    f32 curX = (f32)screenX;
    f32 curY = (f32)screenY;

    if (justify & XC_JUST_RIGHT) {
        s32 lineW = MeasureText(text);
        if ((justify & XC_JUST_HMASK) == XC_JUST_CENTER)
            curX -= (f32)(lineW / 2);
        else
            curX -= (f32)lineW;
    }

    for (const char* p = text; *p; p++) {
        if (*p == '\n') {
            curY += (f32)((s32)lineHeight + lineSpacing);
            curX = (f32)screenX;
            if (justify & XC_JUST_RIGHT) {
                s32 lineW = MeasureText(p + 1);
                if ((justify & XC_JUST_HMASK) == XC_JUST_CENTER)
                    curX -= (f32)(lineW / 2);
                else
                    curX -= (f32)lineW;
            }
            continue;
        }
        if (*p == ' ') {
            curX += (f32)spaceWidth;
            continue;
        }

        const xcSpriteLetter* spr = FindLetter((u8)*p);
        if (!spr) {
            curX += (f32)spaceWidth;
            continue;
        }

        s32 tIdx = spr->texIdx;
        if (tIdx >= numTextures || !textures[tIdx]) {
            curX += (f32)spr->w;
            continue;
        }

        f32 tw = (f32)texWidths[tIdx];
        f32 th = (f32)texHeights[tIdx];
        if (tw <= 0 || th <= 0) { curX += (f32)spr->w; continue; }

        // Normalize in 512x240 overlay space (same as sprites)
        f32 nx = SCALE_NORM_X(curX);
        f32 ny = 1.0f - SCALE_NORM_Y(curY + (f32)spr->h);
        f32 nw = SCALE_NORM_X((f32)spr->w);
        f32 nh = SCALE_NORM_Y((f32)spr->h);

        f32 u0 = (f32)spr->u0 / tw;
        f32 v0 = (f32)spr->v0 / th;
        f32 u1 = (f32)spr->u1 / tw;
        f32 v1 = (f32)spr->v1 / th;

        ScreenDraw::DrawQuad(textures[tIdx], nx, ny, nw, nh, u0, v0, u1, v1, cr, cg, cb, ca);
        curX += (f32)spr->w;
    }
}

s32 xcFont::MeasureText(const char* text) const {
    if (!text || !sprites)
        return 0;
    s32 width = 0;
    for (const char* p = text; *p; p++) {
        if (*p == '\n') break;
        if (*p == ' ') { width += spaceWidth; continue; }
        const xcSpriteLetter* spr = FindLetter((u8)*p);
        if (!spr) { width += spaceWidth; continue; }
        width += spr->w;
    }
    return width;
}

// PC: Decode PSX 4bpp image data with CLUT palette to RGBA texture
// rectW is in 16-bit VRAM words (4bpp: 4 pixels per word)
// rectH is in scanlines
// paletteData: 16 x u16 PSX colors (32 bytes)
tTexture* xcFont::DecodeImageWithPalette(const u8* pixelData,
    s32 rectW, s32 rectH, const u8* paletteData, s32* outW, s32* outH) {

    // 4bpp: each 16-bit VRAM word = 4 pixels
    s32 pixelWidth = rectW * 4;
    s32 pixelHeight = rectH;

    *outW = pixelWidth;
    *outH = pixelHeight;

    // Build RGBA palette from 16 PSX colors
    u32 palette[16];
    for (s32 i = 0; i < 16; i++) {
        u16 c = *(const u16*)(paletteData + i * 2);
        palette[i] = psxColorToRGBA(c);
    }
    LOG("[xcFont] palette: %04X %04X %04X %04X %04X %04X %04X %04X",
        *(const u16*)(paletteData+0), *(const u16*)(paletteData+2),
        *(const u16*)(paletteData+4), *(const u16*)(paletteData+6),
        *(const u16*)(paletteData+8), *(const u16*)(paletteData+10),
        *(const u16*)(paletteData+12), *(const u16*)(paletteData+14));
    LOG("[xcFont] RGBA: %08X %08X %08X %08X %08X %08X %08X %08X",
        palette[0], palette[1], palette[2], palette[3],
        palette[4], palette[5], palette[6], palette[7]);

    // Decode 4bpp indexed to RGBA
    u32* rgba = new u32[pixelWidth * pixelHeight];
    memset(rgba, 0, pixelWidth * pixelHeight * sizeof(u32));

    s32 srcIdx = 0;
    for (s32 y = 0; y < pixelHeight; y++) {
        for (s32 x = 0; x < pixelWidth; x += 2) {
            if (srcIdx >= rectW * rectH * 2) break; // safety
            u8 byte = pixelData[srcIdx++];
            u8 lo = byte & 0x0F;
            u8 hi = (byte >> 4) & 0x0F;
            rgba[y * pixelWidth + x] = palette[lo];
            if (x + 1 < pixelWidth)
                rgba[y * pixelWidth + x + 1] = palette[hi];
        }
    }

    tTexture* tex = new tTexture();
    tex->Create(pixelWidth, pixelHeight, 32, 8, rgba);
    delete[] rgba;
    return tex;
}

// ---- oxFontFile methods ----

// PSX: FontInit__10oxFontFilePc (OXSCRMGR.CPP:284, 0x80040998)
// PSX: creates sectionMan, loads font file, finds font inventory,
// FixDataPointers + LoadFonts to create xcFont objects, frees file data.
void oxFontFile::FontInit(const char* path) {
    MARKFUNCTION(0x80040998);

    // PSX: allocate and construct new xcSectionMan (8 bytes)
    sectionMan = new xcSectionMan();

    // PSX: xcReadFileLow(path, &data, &size)
    u8* data = nullptr;
    u32 size = 0;
    xcReadFileLow(path, &data, &size);
    if (!data) return;

    // PSX: parse file header to find font inventory
    // outerEnd = data + data[4] + 8
    // innerEnd = data + 8 + data[12] + 4
    // fontContainer = innerEnd + 4
    u32 outerDS = *(u32*)(data + 4);
    u32 innerDS = *(u32*)(data + 12);
    u8* outerEnd = data + outerDS + 8;
    u8* innerEnd = data + 8 + innerDS + 4;
    u8* fontContainer = innerEnd + 4;

    if (outerEnd >= fontContainer) {
        u32 fontContDS = *(u32*)(fontContainer + 4);
        if (fontContDS > 0) {
            xcInventory* fontInv = reinterpret_cast<xcInventory*>(innerEnd + 12);
            if (fontInv->tag == 4 && fontInv->itemCount > 0) {
                // PSX: FixDataPointers(fontInv, data) - converts offsets to absolute ptrs
                // PC: we pass fileBase to LoadFonts which computes absolute ptrs
                sectionMan->LoadFonts(fontInv, data);
            }
        }
    }

    // PSX: rPFree(data)
    delete[] data;
}

// PSX: ReloadFont__10oxFontFilePc (OXSCRMGR.CPP:317, 0x80040A8C)
// PSX: reads file via xcReadFileHigh, finds font inventory,
// FixDataPointers, iterates font items calling ReloadData on existing xcFont objects.
void oxFontFile::ReloadFont(const char* path) {
    MARKFUNCTION(0x80040A8C);

    // If no sectionMan yet, do FontInit instead
    if (!sectionMan) {
        FontInit(path);
        return;
    }

    u8* data = nullptr;
    u32 size = 0;
    xcReadFileHigh(path, &data, &size);
    if (!data) return;

    // Parse file header to find font inventory (same as FontInit)
    u32 outerDS = *(u32*)(data + 4);
    u32 innerDS = *(u32*)(data + 12);
    u8* outerEnd = data + outerDS + 8;
    u8* innerEnd = data + 8 + innerDS + 4;
    u8* fontContainer = innerEnd + 4;

    if (outerEnd >= fontContainer) {
        u32 fontContDS = *(u32*)(fontContainer + 4);
        if (fontContDS > 0) {
            xcInventory* fontInv = reinterpret_cast<xcInventory*>(innerEnd + 12);
            if (fontInv->tag == 4 && fontInv->itemCount > 0) {
                // PSX: FixDataPointers on the new file's font inventory
                // Then iterate font items, calling ReloadData on each existing xcFont
                if (sectionMan->fonts && sectionMan->fontObjects) {
                    xcInventoryItem* oldItems = sectionMan->fonts->GetItems();
                    xcInventoryItem* newItems = fontInv->GetItems();
                    u32 count = sectionMan->fonts->itemCount;
                    if (count > fontInv->itemCount) count = fontInv->itemCount;

                    for (u32 i = 0; i < count; i++) {
                        if (sectionMan->fontObjects[i]) {
                            const u8* rawData = data + newItems[i].dataOffset;
                            sectionMan->fontObjects[i]->ReloadData(rawData);
                        }
                    }
                }
            }
        }
    }

    delete[] data;
}

// PSX: FindFont__10oxFontFilePc (OXSCRMGR.CPP:355, 0x80040BA0)
xcFont* oxFontFile::FindFont(const char* name) {
    MARKFUNCTION(0x80040BA0);
    // PSX: delegates to sectionMan->FindFont(name)
    if (!sectionMan) return nullptr;
    return sectionMan->FindFont(name);
}
