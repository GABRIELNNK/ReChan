#pragma once
#include "core.h"
#include "fe/oxscrmgr.h"

class tTexture;

// xcSpriteLetter (16 bytes on PSX) - stores glyph UV/size within texture
// PSX layout: POLY_FT4 UV coords + CLUT/TPage
// PC: u0/v0/u1/v1 are pixel coords relative to decoded texture; charCode for lookup
struct xcSpriteLetter {
    u8 u0;         // texture U left
    u8 v0;         // texture V top
    u8 u1;         // texture U right (exclusive)
    u8 v1;         // texture V bottom (exclusive)
    u8 w;          // glyph pixel width (u1 - u0)
    u8 h;          // glyph pixel height
    u8 pad0;
    u8 pad1;
    u8 pad2;
    u8 pad3;
    u16 charCode;  // character code
    u8 texIdx;     // which font texture this glyph uses (image*palCount + pal)
    u8 pad4;
    u16 pad5;
};

// xcFont (276 bytes on PSX)
// PSX: xcFont at XCFONT.CPP:92 (0x800915A0)
// PSX layout:
//   +0:    spaceWidth (u8)
//   +1:    lineHeight (u8)
//   +2:    charTable[256] (u8) - ASCII -> glyph index (0xFF = not found)
//   +258:  padding
//   +260:  sprites (xcSpriteLetter*) - glyph array
//   +264:  extendedLetters (xcSpriteLetter*) - for chars > 255
//   +268:  numSprites (s32)
//   +272:  numExtended (s32)
class xcFont {
public:
    u8 spaceWidth = 0;                // +0
    u8 lineHeight = 0;                // +1
    u8 charTable[256] = {};           // +2: ASCII -> glyph index (0xFF = not found)
    xcSpriteLetter* sprites = nullptr; // +260: sorted glyph array
    xcSpriteLetter* extended = nullptr; // +264: extended chars > 255
    s32 numSprites = 0;               // +268: total sprite count
    s32 numExtended = 0;              // +272: extended sprite count

    // PC: decoded textures (one per image*palette combo)
    tTexture** textures = nullptr;
    s32 numTextures = 0;
    s32* texWidths = nullptr;
    s32* texHeights = nullptr;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float wrapX = 0.0f;  // max line width in screen pixels; 0 = disabled

    xcFont(const u8* rawData);
    ~xcFont();
    void ReloadData(const u8* rawData);
    const xcSpriteLetter* FindLetter(u8 ch) const;
    const xcSpriteLetter* FindLetter(u16 ch) const;

    void SetScale(float x, float y);
    void SetWrapX(float val) { wrapX = val; }
    s32 CountWrappedLines(const char* text, f32 maxWidth) const;
    void DrawText(const char* text, f32 screenX, f32 screenY,
                  u32 color = 0xFFFFFFFF, u8 justify = 0, s32 lineSpacing = 0) const;

    // PC: Measure text width in 512-space pixels
    f32 MeasureText(const char* text) const;

private:
    void WrapText(const char* text, f32 maxWidth, char* out, s32 outSize) const;
    // PC: decode PSX raw image+palette data to RGBA texture
    static tTexture* DecodeImageWithPalette(const u8* pixelData,
        s32 rectW, s32 rectH, const u8* paletteData, s32* outW, s32* outH);
};

// oxFontFile (48 bytes on PSX) - inherits oxScreenManager
// PSX source: OXSCRMGR.CPP
// PSX layout: same as oxScreenManager (48 bytes), no extra fields.
// PSX: gp[56] holds the global instance.
// FontInit creates sectionMan, loads FONTS.1, extracts font inventory.
// The sectionMan is shared with feMenuMgr, gameMenu, HUD as their parent.
class oxFontFile : public oxScreenManager {
public:
    oxFontFile() = default;
    ~oxFontFile() override = default;

    void FontInit(const char* path);
    void ReloadFont(const char* path);
    xcFont* FindFont(const char* name);

    const char** GetScreenNames() override { return nullptr; }
    void SelfInit() override {}
};

// PSX: gp[56] - global oxFontFile instance
extern oxFontFile* g_oxFontFile;
