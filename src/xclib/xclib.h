// xclib.h - xclib .1 file parser and cell image decoder
// Reversed from PSX XCSOS.CPP, XCINV.CPP, XCCIMAGE.CPP, XCFILE.CPP, EXPAND.CPP
// Parses xclib .1 files (TITLE.1, FE.1, etc.) into inventories of images,
// strings, screens, and overlays. Cell images are decoded from LZSS-compressed
// indexed color data to RGBA32 for GL rendering.
#pragma once

#include "gen/common.h"

class tTexture;
class xcFont;

// SQU LZSS Decompression (EXPAND.CPP:67)
// PSX: SquExpandData(u8* dst, const u8* src)
// Returns bytes written. Dst must be large enough (max 8192).
s32 SquExpandData(u8* dst, const u8* src);

// xcReadFileLow / xcReadFileHigh (XCFILE.CPP)

// PSX: xcReadFileLow__FPCcPPvPUl (XCFILE.CPP:76, 0x800911C0)
// Reads file into allocated buffer.
bool xcReadFileLow(const char* path, u8** outData, u32* outSize);

// PSX: xcReadFileHigh__FPCcPPvPUl (XCFILE.CPP:105, 0x80091268)
// Same as xcReadFileLow on PC (PSX uses different allocator for "high" memory).
bool xcReadFileHigh(const char* path, u8** outData, u32* outSize);

// xcHash (XCHASH.CPP:11)

// PSX: djb2 variant hash
u32 xcHash(const char* str);

//xcInventoryItem (XCINV.CPP)

struct xcInventoryItem {
    u32 hash;       // xcHash of item name
    u32 dataOffset; // relative offset from file start (before fix-up)
};

// xcInventory (XCINV.CPP)

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

    // PSX: Sort__11xcInventory (XCINV.CPP:27, 0x80091370)
    // Sorts items by hash using shaker sort for binary search.
    void Sort();

    const xcInventoryItem* FindItem(u32 hash) const;
};

// xcCellImage (XCCIMAGE.CPP:132)

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

// xcPrimObj (XCDO.CPP)
// PSX: xcPrimObj base header (4 bytes), followed by type-specific data.
// All prim objects begin with this header.
// PSX: Draw__9xcPrimObj dispatches by type byte[0].
struct xcPrimHeader {
    u8 type;        // +0: prim type (9=sprite, 10=text, 16=POLY_F4, 17=POLY_G4)
    u8 flags;       // +1: flags/layer info
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
    u32 pad2;               // +52

    u32* StringHashes() { return reinterpret_cast<u32*>(reinterpret_cast<u8*>(this) + 56); }
    u32 GetStringHash() const {
        s32 idx = (paletteIdx < numStrings) ? paletteIdx : 0;
        return reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(this) + 56)[idx];
    }
    // Packed RGBA as u32 (little-endian: R in low byte)
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

// xcOverlayData (XCSOS.CPP:55)

// Raw overlay data within .1 file (cast from rawData + offset).
// PSX: xcOverlay (C:\devsys\psx\xclib\psx\SRC\XCSOS.CPP)
// Layout:
//   +0: visibility (u32) - 0=hidden, non-zero=visible
//   +4: primCount (u32) - number of display objects
//   +8: items[N] stride 8: { hash(u32), dataOffset(u32) }
struct xcOverlayData {
    u32 visibility;
    u32 primCount;
    // items follow at +8: { hash(u32), dataOffset(u32) } * primCount

    // PSX: GetPrimObj__9xcOverlayUl11xcChunkEnum (XCSOS.CPP:109)
    // Finds a prim object by hash and type within this overlay.
    u8* GetPrimObj(u32 hash, u8 type, u8* rawData) const;

    // PSX: GetTextObj__9xcOverlayUl (XCSOS.CPP:148, 0x8005E9D0)
    u8* GetTextObj(u32 hash, u8* rawData) const;

    // PSX: GetSprite__9xcOverlayUl (XCSOS.CPP:136, 0x8005E9B0)
    u8* GetSprite(u32 hash, u8* rawData) const;
};

// xcScreenData (XCSOS.CPP:250)

// Raw screen data within .1 file (cast from rawData + offset).
// PSX: xcScreen (C:\devsys\psx\xclib\psx\SRC\XCSOS.CPP)
// Layout:
//   +0: overlayCount (u32)
//   +4: overlayRef[N] (u32 each) - overlay hashes
struct xcScreenData {
    u32 overlayCount;
    // overlay hashes follow at +4
};

// xcSection (XCSOS.CPP:295)

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

    // PSX: Init__9xcSectionPUcP12xcSectionManUl (XCSOS.CPP:295, 0x8005EBD8)
    bool Init(u8* data, u32 size, xcSectionMan* man);

    // Get a decoded cell image by index
    xcCellImage* GetCell(s32 index) const;

    // Get a decoded cell image by hash name
    xcCellImage* FindCell(u32 hash) const;

    // PSX: FixScreenAndOverlayandDO__9xcSection (XCSOS.CPP:545, 0x8005F0B0)
    void FixScreenAndOverlayandDO();

    // PSX: Draw__9xcSection (XCSOS.CPP:355, 0x8005ED04)
    void Draw();

    // PSX: GotoScreen__9xcSectionP8xcScreen (XCSOS.CPP:332, 0x8005EC40)
    void GotoScreen(xcScreenData* scr);

    // PSX: UnloadOverlays__9xcSection (XCSOS.CPP:346, 0x8005EC8C)
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

    // Internal: draw a single prim object at the given raw data pointer
    void DrawPrimObj(u8* primData);
};

//xcSectionMan

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

    // PSX: __12xcSectionMan (XCSOS.CPP:572, 0x8005F180)
    xcSectionMan() { MARKFUNCTION(0x8005F180); }
    ~xcSectionMan();

    // PSX: LoadSection__12xcSectionManPCcUl (XCSOS.CPP:609)
    bool LoadSection(const char* filename);

    // PSX: FreeSection__12xcSectionMan (XCSOS.CPP:598, 0x8005F1EC)
    void FreeSection();

    // PSX: LoadFonts__12xcSectionManP11xcInventory (XCSOS.CPP:676, 0x8005F3B4)
    void LoadFonts(xcInventory* fontInv, u8* fileBase);

    // PSX: DeleteFonts__12xcSectionMan (XCSOS.CPP:653, 0x8005F300)
    void DeleteFonts();

    // PSX: FindFont__12xcSectionManUl (XCSOS.CPP:580, 0x8005F190)
    xcFont* FindFont(u32 hash);

    // PSX: FindFont__12xcSectionManPCc (XCSOS.CPP:591, 0x8005F1B8)
    xcFont* FindFont(const char* name);
};
