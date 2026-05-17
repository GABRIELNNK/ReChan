#pragma once
#include "core.h"

// Parses xclib .1 files (TITLE.1, FE.1, etc.) into inventories of images,
// strings, screens, and overlays. Cell images are decoded from LZSS-compressed
// indexed color data to RGBA32 for GL rendering.

class tTexture;
class xcFont;

// PSX: SquExpandData(u8* dst, const u8* src)
// Returns bytes written. Dst must be large enough (max 8192).
s32 SquExpandData(u8* dst, const u8* src);

// Reads file into allocated buffer.
bool xcReadFileLow(const char* path, u8** outData, u32* outSize);

// Same as xcReadFileLow on PC (PSX uses different allocator for "high" memory).
bool xcReadFileHigh(const char* path, u8** outData, u32* outSize);


// PSX: djb2 variant hash
u32 xcHash(const char* str);

// Registers a runtime-owned string and returns a token that can be stored
// in xcTextPrim string slots.
u32 xcRegisterRuntimeString(const char* str);

// Resolves a runtime string token. Returns nullptr if token is unknown.
const char* xcResolveRuntimeString(u32 token);

struct xcInventoryItem {
    u32 hash;       // xcHash of item name
    u32 dataOffset; // relative offset from file start (before fix-up)
};

// On-disk format: [tag:4][dataSize:4][itemCount:4][items:8*N][referenced data]
struct xcInventory {
    u32 tag;
    u32 dataSize;
    u32 itemCount;
    // items follow at +12, stride 8

    xcInventoryItem* GetItems() { return reinterpret_cast<xcInventoryItem*>(reinterpret_cast<u8*>(this) + 12); }
    const xcInventoryItem* GetItems() const { return reinterpret_cast<const xcInventoryItem*>(reinterpret_cast<const u8*>(this) + 12); }
    u32 GetSizeInBytes() const { return dataSize + 8; }

    void FixDataPointers(u8* base);

    // Sorts items by hash using shaker sort for binary search.
    void Sort();

    const xcInventoryItem* FindItem(u32 hash) const;
};

// Decoded cell image with RGBA pixel data for GL rendering.
// On PSX this is 32 bytes with VRAM upload; on PC we decode to RGBA and create a GL texture.
struct xcCellImage {
    s32 width = 0;          // full image width in pixels
    s32 height = 0;         // full image height in pixels
    u32* rgba = nullptr;    // decoded RGBA32 pixel data
    tTexture* texture = nullptr; // GL texture (created on first use)
    u8 bppCode = 0;         // 1=8bpp, 2=4bpp

    xcCellImage() = default;
    ~xcCellImage();

    // Decode cell from raw CELL data block (at the "CELL" magic offset)
    bool Decode(const u8* cellData);

    // Get or create GL texture from decoded RGBA data
    tTexture* GetTexture();
};

// CELL file header (32 bytes on disk)
// PSX: xcCellImage constructor reads this at the start of each CELL block
struct xcCellHeader {
    u8  magic[4];           // +0:  "CELL"
    u32 totalSize;          // +4:  total block size including header
    u16 tileCols;           // +8:  number of cell columns in the image
    u16 tileRows;           // +10: number of cell rows in the image
    u16 pixelW;             // +12: visible image width in pixels
    u16 pixelH;             // +14: visible image height in pixels
    u8  cellW;              // +16: cell tile width
    u8  cellH;              // +17: cell tile height
    u8  bppCode;            // +18: 1=8bpp, 2=4bpp
    u8  pad;                // +19
    u16 compressedClutSize; // +20: compressed CLUT size (0 = uncompressed)
    u16 clutEntries;        // +22: number of CLUT palette entries
    u16 cellsPerFrame;      // +24: cells per frame (tile count)
    u16 paletteCount;       // +26: trailing static tile count when frameCount == 0
    u32 frameCount;         // +28: number of compressed frame blocks
    // CLUT data follows at +32, then frame data
};

// PSX: xcPrimObj type field (byte +0)
enum xcPrimType : u8 {
    XC_PRIM_SPRITE = 9,
    XC_PRIM_TEXT = 10,
    XC_PRIM_POLYF4 = 16,
    XC_PRIM_POLYG4 = 17,
};

// PSX: justification flags in xcPrimHeader::flags (byte +1)
// Used by xcFontDC::PushJustTrans and xcImageDC::FindJust
enum xcJustify : u8 {
    XC_JUST_LEFT = 0,
    XC_JUST_RIGHT = 0x02,  // (flags & 2) - right-align
    XC_JUST_CENTER = 0x03,  // (flags & 3) == 3 - center
    XC_JUST_BOTTOM = 0x08,  // (flags & 8) - bottom-align
    XC_JUST_VCENTER = 0x0C,  // (flags & 0xC) == 0xC - vertical center
    XC_JUST_HMASK = 0x03,
    XC_JUST_VMASK = 0x0C,
};

// PSX: xcPrimObj base header (4 bytes), followed by type-specific data.
// PSX: Draw__9xcPrimObj dispatches by type byte[0].
struct xcPrimHeader {
    u8 type;        // +0: xcPrimType
    u8 flags;       // +1: xcJustify bits
    u8 subtype;     // +2: OT layer / clipping (5=clip, skip drawing)
    u8 pad;         // +3
};

// PSX 3x3 matrix in 16.16 fixed-point (36 bytes)
// Used by xcSprite and xcTextObj for position/scale/rotation.
// Translation: x = m[0][2] >> 16, y = m[1][2] >> 16
struct xcMatrix33 {
    s32 m[3][3];    // row-major, 16.16 fixed-point

    s32 GetX() const { return m[0][2] >> 16; }
    s32 GetY() const { return m[1][2] >> 16; }
    void SetY(s32 y) { m[1][2] = y << 16; }
};

// PSX: xcSprite (XCDO.CPP:221, 0x800AE76C) - 52+ bytes
// Textured sprite with position matrix and cell image reference.
struct xcSpritePrim {
    xcPrimHeader hdr;       // +0
    xcMatrix33 mtx;         // +4: position/transform matrix
    u8 colorR;              // +40
    u8 colorG;              // +41
    u8 colorB;              // +42
    u8 colorA;              // +43
    u8 numImages;           // +44: number of image hash entries
    u8 paletteIdx;          // +45: current palette/frame index
    u8 pad0;                // +46
    u8 pad1;                // +47

    u32* ImageHashes() { return reinterpret_cast<u32*>(reinterpret_cast<u8*>(this) + 48); }
    u32 GetImageHash() const {
        s32 idx = (paletteIdx < numImages) ? paletteIdx : 0;
        return reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(this) + 48)[idx];
    }
    u32 GetColor() const { return *(const u32*)(reinterpret_cast<const u8*>(this) + 40); }
};

// PSX: xcTextObj (XCDO.CPP:241, 0x800AE828) - 60+ bytes
// Text string rendered with xcFont at matrix position.
struct xcTextPrim {
    xcPrimHeader hdr;       // +0
    xcMatrix33 mtx;         // +4: position/transform matrix
    u8 colorR;              // +40
    u8 colorG;              // +41
    u8 colorB;              // +42
    u8 colorA;              // +43
    u8 numStrings;          // +44: number of string hash entries
    u8 paletteIdx;          // +45: current string index
    u8 pad0;                // +46
    u8 pad1;                // +47
    u32 fontHash;           // +48: xcFont hash
    u32 lineSpacing;        // +52: inter-line spacing for multiline text

    u32* StringHashes() { return reinterpret_cast<u32*>(reinterpret_cast<u8*>(this) + 56); }
    u32 GetStringHash() const {
        s32 idx = (paletteIdx < numStrings) ? paletteIdx : 0;
        return reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(this) + 56)[idx];
    }
    u32 GetColor() const { return *(const u32*)(reinterpret_cast<const u8*>(this) + 40); }
};

// PSX: POLY_F4 (GPU command 0x28) - flat-colored quad
struct xcPolyF4Prim {
    xcPrimHeader hdr;       // +0
    u8 r, g, b, code;      // +4: color + GPU command code
    s16 x0, y0;            // +8: vertex 0
    s16 x1, y1;            // +12: vertex 1
    s16 x2, y2;            // +16: vertex 2
    s16 x3, y3;            // +20: vertex 3

    void GetBounds(s16& minX, s16& minY, s16& maxX, s16& maxY) const {
        minX = (x0 < x3) ? x0 : x3;
        maxX = (x0 > x3) ? x0 : x3;
        minY = (y0 < y3) ? y0 : y3;
        maxY = (y0 > y3) ? y0 : y3;
    }
};

// PSX: POLY_G4 (GPU command 0x38) - Gouraud-shaded quad
struct xcPolyG4Prim {
    xcPrimHeader hdr;       // +0
    u8 r0, g0, b0, code;   // +4
    s16 x0, y0;            // +8
    u8 r1, g1, b1, pad1;   // +12
    s16 x1, y1;            // +16
    u8 r2, g2, b2, pad2;   // +20
    s16 x2, y2;            // +24
    u8 r3, g3, b3, pad3;   // +28
    s16 x3, y3;            // +32

    void GetBounds(s16& minX, s16& minY, s16& maxX, s16& maxY) const {
        minX = (x0 < x3) ? x0 : x3;
        maxX = (x0 > x3) ? x0 : x3;
        minY = (y0 < y3) ? y0 : y3;
        maxY = (y0 > y3) ? y0 : y3;
    }
    void GetAvgColor(u8& r, u8& g, u8& b) const {
        r = (u8)(((u32)r0 + r1 + r2 + r3) / 4);
        g = (u8)(((u32)g0 + g1 + g2 + g3) / 4);
        b = (u8)(((u32)b0 + b1 + b2 + b3) / 4);
    }
};

// Item entry within an xcOverlayData - references a prim object.
struct xcOverlayItem {
    u32 hash;
    u32 dataOffset;  // offset from file base to prim data
};

// Raw overlay data within .1 file (cast from rawData + offset).
struct xcOverlayData {
    u32 visibility;
    u32 primCount;
    // xcOverlayItem items[primCount] follows at +8

    xcOverlayItem* GetItems() { return reinterpret_cast<xcOverlayItem*>(this + 1); }
    const xcOverlayItem* GetItems() const { return reinterpret_cast<const xcOverlayItem*>(this + 1); }

    // PSX: GetPrimObj__9xcOverlayUl11xcChunkEnum (XCSOS.CPP:109)
    u8* GetPrimObj(u32 hash, u8 type, u8* rawData) const;
    u8* GetPrimObj(const char* name, u8 type, u8* rawData) const;

    // PSX: GetTextObj__9xcOverlayUl (XCSOS.CPP:148, 0x8005E9D0)
    u8* GetTextObj(u32 hash, u8* rawData) const;
    u8* GetTextObj(const char* name, u8* rawData) const;

    // PSX: GetSprite__9xcOverlayUl (XCSOS.CPP:136, 0x8005E9B0)
    u8* GetSprite(u32 hash, u8* rawData) const;
    u8* GetSprite(const char* name, u8* rawData) const;
};

// Raw screen data within .1 file (cast from rawData + offset).
struct xcScreenData {
    u32 overlayCount;
    // u32 overlayRefs[overlayCount] follows at +4

    u32* GetOverlayRefs() { return reinterpret_cast<u32*>(this + 1); }
    const u32* GetOverlayRefs() const { return reinterpret_cast<const u32*>(this + 1); }
};

// Parsed xclib .1 file. Contains inventories referencing cell images,
// strings, screens, and overlays within the loaded file data.
// PSX layout (32 bytes):
//   +0:  rawData (u8*)     - base of loaded data
//   +4:  field4 (u32)      - flags from Init
//   +8:  strings (xcInventory*)
//   +12: screens (xcInventory*)
//   +16: images (xcInventory*)
//   +20: overlays (xcInventory*)
//   +24: currentScreen (xcScreenData*)
//   +28: sectionMan (xcSectionMan*)
struct xcSectionMan; // forward
struct xcSection {
    u8* rawData = nullptr;              // +0: loaded file data (owned)
    u32 rawSize = 0;                    // PC only: file size
    xcInventory* images = nullptr;      // +16: image inventory
    xcInventory* strings = nullptr;     // +8:  string inventory
    xcInventory* screens = nullptr;     // +12: screen inventory
    xcInventory* overlays = nullptr;    // +20: overlay inventory
    xcScreenData* currentScreen = nullptr; // +24: active screen
    xcSectionMan* sectionMan = nullptr; // +28: back-pointer

    // PC only: decoded cell images
    xcCellImage** cells = nullptr;
    s32 numCells = 0;

    xcSection() = default;
    ~xcSection();

    bool Init(u8* data, u32 size, xcSectionMan* man);

    // Get a decoded cell image by index
    xcCellImage* GetCell(s32 index) const;

    // Get a decoded cell image by hash name
    xcCellImage* FindCell(u32 hash) const;

    void FixScreenAndOverlayandDO();
    void Draw();
    void GotoScreen(xcScreenData* scr);
    void UnloadOverlays();

    // Find raw overlay data by hash
    xcOverlayData* FindOverlay(u32 hash) const;

    // Find raw screen data by hash
    xcScreenData* FindScreen(u32 hash) const;

    // PSX: FindString__9xcSectionUl (XCSOS.CPP:423)
    const char* FindString(u32 hash) const;

    // PSX: FindImage__9xcSectionUl (XCSOS.CPP:373)
    xcCellImage* FindImage(u32 hash) const;

    // Show all overlays referenced by a screen (set visibility=1)
    void ShowScreen(u32 hash);

    // Hide all overlays referenced by a screen (set visibility=0)
    void HideScreen(u32 hash);

    // Hide all overlays (set all visibility=0)
    void HideAllOverlays();

private:
    void FixInventories();
    void LoadCells();
    void DrawPrimObj(u8* primData);
};

// PSX: xcSectionMan (8 bytes)
// PSX layout:
//   +0: fonts (xcInventory*) - fonts inventory (sorted by hash)
//   +4: section (xcSection*)
struct xcSectionMan {
    xcInventory* fonts = nullptr;   // +0: fonts inventory (copied from file, sorted)
    xcSection* section = nullptr;   // +4: section

    // PC: xcFont objects indexed parallel to fonts inventory items
    // On PSX, xcFont pointers are stored in 32-bit inventory item dataOffset fields.
    // On 64-bit PC we store them separately.
    xcFont** fontObjects = nullptr;
    s32 numFontObjects = 0;

    xcSectionMan() { MARKFUNCTION(0x8005F180); }
    ~xcSectionMan();
    bool LoadSection(const char* filename);
    void FreeSection();
    void LoadFonts(xcInventory* fontInv, u8* fileBase);
    void DeleteFonts();
    xcFont* FindFont(u32 hash);
    xcFont* FindFont(const char* name);
};
