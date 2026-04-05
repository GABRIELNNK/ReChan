// xclib.cpp - xclib .1 file parser and cell image decoder
// Reversed from PSX source files:
//   XCSOS.CPP  - xcSection, xcInventory, xcSectionMan
//   XCINV.CPP  - xcInventory item lookup and fix-up
//   XCCIMAGE.CPP - xcCellImage construction and VRAM upload
//   EXPAND.CPP - SquExpandData LZSS decompressor
//   XCHASH.CPP - xcHash DJB2 hash
//   XCFILE.CPP - xcReadFileLow file loading
#include "xclib/xclib.h"
#include "xcfile.h"
#include "fe/xcfont.h"
#include "p3d/texture.h"
#include "pc/tim.h"
#include <cstdio>
#include <cstring>

// ---- SQU LZSS Decompression (PSX: EXPAND.CPP:67, addr 0x80026D10) ----
// Command byte encoding:
//   0x00-0x7F: short back-ref: len=((cmd>>4)&7)+2, dist=(cmd&0xF)+1
//   0x80-0xBF: long back-ref:  len=(cmd&0x3F)+3, dist=nextByte+1
//   0xC0-0xFE: literal run:    count=(cmd&0x3F)+1, followed by raw bytes
//   0xFF:      end of stream

s32 SquExpandData(u8* dst, const u8* src) {
    u8* dstStart = dst;
    u8 cmd = *src++;

    for (;;) {
        if (cmd < 0x80) {
            // Short back-reference
            s32 length = ((cmd >> 4) & 7) + 2;
            s32 offset = (cmd & 0x0F) + 1;
            const u8* ref = dst - offset;
            for (s32 i = 0; i < length; i++)
                *dst++ = ref[i];
        }
        else if (cmd < 0xC0) {
            // Long back-reference
            u8 nextByte = *src++;
            s32 length = (cmd & 0x3F) + 3;
            s32 offset = nextByte + 1;
            const u8* ref = dst - offset;
            for (s32 i = 0; i < length; i++)
                *dst++ = ref[i];
        }
        else if (cmd == 0xFF) {
            // End marker
            break;
        }
        else {
            // Literal run
            s32 count = (cmd & 0x3F) + 1;
            std::memcpy(dst, src, count);
            dst += count;
            src += count;
        }
        cmd = *src++;
    }

    return (s32)(dst - dstStart);
}

// ---- xcHash (PSX: XCHASH.CPP:11, addr 0x80091310) ----

u32 xcHash(const char* str) {
    u32 hash = 0;
    while (*str) {
        hash = hash * 33 + (u8)*str;
        str++;
    }
    return hash;
}

// ---- xcInventory (PSX: XCINV.CPP) ----

// PSX: FixDataPointers__11xcInventoryUl (XCINV.CPP:34, 0x800913B0)
void xcInventory::FixDataPointers(u8* base) {
    xcInventoryItem* items = GetItems();
    for (u32 i = 0; i < itemCount; i++) {
        // Convert relative offset to absolute pointer (stored as u32 offset)
        items[i].dataOffset += (u32)(uintptr_t)base;
    }
}

// PSX: FindItem__11xcInventoryUl (XCINV.CPP:44, 0x80091420)
const xcInventoryItem* xcInventory::FindItem(u32 hash) const {
    const xcInventoryItem* items = GetItems();
    for (u32 i = 0; i < itemCount; i++) {
        if (items[i].hash == hash)
            return &items[i];
    }
    return nullptr;
}

// PSX: Sort__11xcInventory (XCINV.CPP:27, 0x80091370)
// Shaker sort by hash for binary search in FindItem.
// PSX uses GenShakerSort with a compare callback. We do a simple sort here.
void xcInventory::Sort() {
    xcInventoryItem* items = GetItems();
    u32 n = itemCount;
    if (n <= 1) return;

    // Shaker sort (bidirectional bubble sort) matching PSX GenShakerSort
    u32 start = 0, end = n - 1;
    bool swapped = true;
    while (swapped) {
        swapped = false;
        for (u32 i = start; i < end; i++) {
            if (items[i].hash > items[i + 1].hash) {
                xcInventoryItem tmp = items[i];
                items[i] = items[i + 1];
                items[i + 1] = tmp;
                swapped = true;
            }
        }
        if (!swapped) break;
        end--;
        swapped = false;
        for (u32 i = end; i > start; i--) {
            if (items[i - 1].hash > items[i].hash) {
                xcInventoryItem tmp = items[i - 1];
                items[i - 1] = items[i];
                items[i] = tmp;
                swapped = true;
            }
        }
        start++;
    }
}

// ---- PSX ABGR1555 to RGBA8888 ----

static inline u32 psxColorToRGBA(u16 c) {
    if (c == 0) return 0; // transparent black
    u32 r = (c & 0x1F) << 3;
    u32 g = ((c >> 5) & 0x1F) << 3;
    u32 b = ((c >> 10) & 0x1F) << 3;
    return (255u << 24) | (b << 16) | (g << 8) | r;
}

// ---- xcCellImage ----

xcCellImage::~xcCellImage() {
    delete[] rgba;
    if (texture) {
        texture->Release();
    }
}

// CELL raw data header (32 bytes):
//   +0:  "CELL" magic (4)
//   +4:  u32 totalSize (including magic+size)
//   +8:  s16 hotX
//   +10: s16 hotY
//   +12: u16 pixelW (full image width)
//   +14: u16 pixelH (full image height)
//   +16: u8  cellW (cell tile width in pixels)
//   +17: u8  cellH (cell tile height in pixels)
//   +18: u8  bppCode (1=8bpp, 2=4bpp)
//   +19: u8  pad
//   +20: u16 compressedClutSize
//   +22: u16 clutEntries
//   +24: u16 cellsPerFrame (alloc hint)
//   +26: u16 paletteCount
//   +28: u32 frameCount
//   +32: [CLUT data] [frame data...]

bool xcCellImage::Decode(const u8* cellData) {
    // Check magic
    if (std::memcmp(cellData, "CELL", 4) != 0) {
        LOG("[xcCellImage] Bad CELL magic");
        return false;
    }

    u32 totalSize = *(const u32*)(cellData + 4);
    u16 pixelW = *(const u16*)(cellData + 12);
    u16 pixelH = *(const u16*)(cellData + 14);
    u8  cellW = cellData[16];
    u8  cellH = cellData[17];
    u8  bpp = cellData[18];
    u16 compClut = *(const u16*)(cellData + 20);
    u16 clutEntries = *(const u16*)(cellData + 22);
    u16 cellsPerFr = *(const u16*)(cellData + 24);
    u32 frameCount = *(const u32*)(cellData + 28);

    bppCode = bpp;

    if (frameCount == 0 || clutEntries == 0) {
        LOG("[xcCellImage] No frames or no CLUT (frames=%u clut=%u)", frameCount, clutEntries);
        return false;
    }

    // Decode CLUT
    u32 clutDataSize = clutEntries * 2;
    const u8* clutSrc = cellData + 32;
    u8 clutBuf[1024]; // max 512 bytes for 256-entry CLUT, plus safety margin
    const u8* clutRaw;
    u32 clutAdvance;

    if (compClut < clutDataSize) {
        // SQU-compressed CLUT
        SquExpandData(clutBuf, clutSrc);
        clutRaw = clutBuf;
        clutAdvance = compClut;
    }
    else {
        clutRaw = clutSrc;
        clutAdvance = clutDataSize;
    }

    // Build RGBA palette
    u32 palette[256] = {};
    for (u32 i = 0; i < clutEntries && i < 256; i++) {
        u16 c = *(const u16*)(clutRaw + i * 2);
        palette[i] = psxColorToRGBA(c);
    }

    // Compute per-cell dimensions
    s32 vramW;
    if (bpp == 2) { // 4bpp
        vramW = cellW >> 2;
    }
    else if (bpp == 1) { // 8bpp
        vramW = cellW >> 1;
    }
    else {
        vramW = cellW;
    }

    s32 frameByteSz = vramW * cellH * 2;
    s32 frameDecompSz = frameByteSz * cellsPerFr;

    // Allocate output image
    width = cellsPerFr * cellW;
    height = frameCount * cellH;
    rgba = new u32[width * height];
    std::memset(rgba, 0, width * height * sizeof(u32));

    LOG("[xcCellImage] Decoding %dx%d, %u frames x %u cells, bpp=%u",
        width, height, frameCount, cellsPerFr, bpp);

    // Decompress and decode frames
    u8 expandBuf[16384]; // max 8192 per frame, with safety margin
    const u8* framePtr = clutSrc + clutAdvance;

    for (u32 fi = 0; fi < frameCount; fi++) {
        if (framePtr + 4 > cellData + totalSize) break;

        u32 compSize = *(const u32*)framePtr;
        const u8* frameSrc = framePtr + 4;
        const u8* pixelData;

        if (compSize < 8192) {
            SquExpandData(expandBuf, frameSrc);
            pixelData = expandBuf;
        }
        else {
            pixelData = frameSrc;
        }

        // Place cells into image grid
        for (u32 ci = 0; ci < cellsPerFr; ci++) {
            s32 cellDataOff = ci * frameByteSz;
            s32 baseX = ci * cellW;
            s32 baseY = fi * cellH;

            for (s32 cy = 0; cy < cellH; cy++) {
                for (s32 cx = 0; cx < cellW; cx++) {
                    u8 idx = 0;
                    if (bpp == 1) { // 8bpp
                        s32 byteOff = cellDataOff + cy * vramW * 2 + cx;
                        if (byteOff < frameDecompSz)
                            idx = pixelData[byteOff];
                    }
                    else if (bpp == 2) { // 4bpp
                        s32 byteOff = cellDataOff + cy * vramW * 2 + cx / 2;
                        if (byteOff < frameDecompSz) {
                            u8 b = pixelData[byteOff];
                            idx = (cx & 1) ? ((b >> 4) & 0x0F) : (b & 0x0F);
                        }
                    }

                    s32 px = baseX + cx;
                    s32 py = baseY + cy;
                    if (px < width && py < height) {
                        rgba[py * width + px] = palette[idx];
                    }
                }
            }
        }

        framePtr += 4 + compSize;
    }

    return true;
}

tTexture* xcCellImage::GetTexture() {
    if (texture) return texture;
    if (!rgba || width <= 0 || height <= 0) return nullptr;

    texture = new tTexture();
    texture->Create(width, height, 32, 8, rgba);
    return texture;
}

// ---- xcSection ----

xcSection::~xcSection() {
    if (cells) {
        for (s32 i = 0; i < numCells; i++)
            delete cells[i];
        delete[] cells;
    }
    delete[] rawData;
}

// PSX: xcSection::Init (XCSOS.CPP:295, 0x8005EBD8)
bool xcSection::Init(u8* data, u32 size, xcSectionMan* man) {
    MARKFUNCTION(0x8005EBD8);
    rawData = data;
    rawSize = size;
    sectionMan = man;

    if (size < 16) {
        LOG("[xcSection] File too small (%u bytes)", size);
        return false;
    }

    // Validate .1 file format: check outerTag and innerTag
    // PSX doesn't validate (no MMU, garbage reads don't crash), but PC must.
    // Valid .1 files have outerTag=1, innerTag=2 based on all known assets.
    u32 outerTag = *(u32*)(data);
    u32 innerTag = *(u32*)(data + 8);
    if (outerTag != 1 || innerTag != 2) {
        LOG("[xcSection] Not a .1 file (outerTag=%u innerTag=%u), skipping inventories", outerTag, innerTag);
        return true; // data is still stored for text-based subclasses (e.g. gameMenu)
    }

    // PSX: FixUpPointers -> FixInventories -> LoadCells -> FreeDiscardableData -> FixScreenAndOverlayandDO
    // PC: FreeDiscardableData omitted - PSX shrinks rawData after cells are decoded to VRAM,
    // but PC still needs rawData for string/overlay/screen access by offset (no FixDataPointers).
    FixInventories();
    LoadCells();
    FixScreenAndOverlayandDO();
    return true;
}

// PSX: xcSection::FixInventories (XCSOS.CPP:446, 0x8005EE50)
// PSX flow:
//   1. Check for 5th font inventory after the 4 main ones; if present, FixDataPointers + LoadFonts
//   2. Assign images, strings, screens, overlays by chaining through file data
//   3. FixDataPointers on images, overlays, strings, screens (converts offsets to absolute addrs)
//   4. Sort images, strings, and conditionally screens (if flags != 0)
//
// On PC (64-bit) we do NOT call FixDataPointers because we can't store 64-bit
// pointers in 32-bit dataOffset fields. Instead, all access manually adds rawData.
// We DO call Sort for hash-based binary search compatibility.
void xcSection::FixInventories() {
    u8* base = rawData;
    u8* inner = base + 8;

    // PSX: check for 5th font inventory after the 4 main inventories
    // outerEnd = base + outerDataSize + 8
    // innerEnd = inner + innerDataSize + 4
    // fontStart = innerEnd + 4
    u32 outerDataSize = *(u32*)(base + 4);
    u32 innerDataSize = *(u32*)(inner + 4);
    u8* outerEnd = base + outerDataSize + 8;
    u8* innerEnd = inner + innerDataSize + 4;
    u8* fontStart = innerEnd + 4; // potential 5th inventory

    if (outerEnd >= fontStart + 8) {
        // There's room for a 5th inventory
        u32 fontDataSize = *(u32*)(fontStart + 4);
        if (fontDataSize > 0) {
            // Check if it's a font inventory: skip ahead to the inventory header at innerEnd+12
            xcInventory* fontInv = reinterpret_cast<xcInventory*>(innerEnd + 12);
            if (fontInv->tag == 4 && fontInv->itemCount > 0) {
                // PSX: FixDataPointers(fontInv, base) + LoadFonts(sectionMan, fontInv)
                if (sectionMan) {
                    sectionMan->LoadFonts(fontInv, base);
                }
            }
        }
    }

    // Chain 4 main inventories starting at offset 16
    u8* ptr = base + 16;
    images = reinterpret_cast<xcInventory*>(ptr);
    ptr += images->GetSizeInBytes();

    strings = reinterpret_cast<xcInventory*>(ptr);
    ptr += strings->GetSizeInBytes();

    screens = reinterpret_cast<xcInventory*>(ptr);
    ptr += screens->GetSizeInBytes();

    overlays = reinterpret_cast<xcInventory*>(ptr);

    // PSX: Sort images and strings for binary search
    images->Sort();
    strings->Sort();

    // PSX: Sort screens only if field4 (flags) != 0
    // field4 is stored at xcSection+4 on PSX. On PC we don't have this field
    // but the standard Init path always uses flags=1 from oxScreenManager.
    // For safety, always sort screens.
    screens->Sort();

    LOG("[xcSection] Inventories: images=%u strings=%u screens=%u overlays=%u",
        images->itemCount, strings->itemCount, screens->itemCount, overlays->itemCount);
}

// PSX: xcSection::LoadCells (XCSOS.CPP:500, 0x8005EFA4)
void xcSection::LoadCells() {
    if (!images || images->itemCount == 0) {
        numCells = 0;
        return;
    }

    numCells = (s32)images->itemCount;
    cells = new xcCellImage * [numCells];
    std::memset(cells, 0, numCells * sizeof(xcCellImage*));

    const xcInventoryItem* items = images->GetItems();
    for (s32 i = 0; i < numCells; i++) {
        u32 offset = items[i].dataOffset;
        if (offset + 32 > rawSize) {
            LOG("[xcSection] Cell %d offset 0x%X out of bounds", i, offset);
            continue;
        }

        const u8* cellData = rawData + offset;
        if (std::memcmp(cellData, "CELL", 4) != 0) {
            LOG("[xcSection] Cell %d at 0x%X: not a CELL block", i, offset);
            continue;
        }

        xcCellImage* cell = new xcCellImage();
        if (cell->Decode(cellData)) {
            cells[i] = cell;
            LOG("[xcSection] Cell %d: %dx%d decoded", i, cell->width, cell->height);
        }
        else {
            delete cell;
        }
    }
}

xcCellImage* xcSection::GetCell(s32 index) const {
    if (index < 0 || index >= numCells) return nullptr;
    return cells[index];
}

xcCellImage* xcSection::FindCell(u32 hash) const {
    if (!images) return nullptr;
    const xcInventoryItem* items = images->GetItems();
    for (s32 i = 0; i < numCells; i++) {
        if (items[i].hash == hash)
            return cells[i];
    }
    return nullptr;
}

// PSX: FixScreenAndOverlayandDO__9xcSection (XCSOS.CPP:545, 0x8005F0B0)
// On PSX this converts overlay/screen/prim offsets to absolute pointers.
// On PC (64-bit) we can't store 64-bit pointers in 32-bit fields,
// so we keep hash-based lookup at draw time instead.
void xcSection::FixScreenAndOverlayandDO() {
    MARKFUNCTION(0x8005F0B0);
    // PSX (208 bytes, 10 blocks):
    // Loop 1: for each overlay inventory item:
    //   FixDataPointers__9xcOverlay - converts prim dataOffset fields to absolute PSX pointers
    //   FindNamedData__9xcOverlay   - for each prim:
    //     xcSprite (type 9):  replaces image hashes at +48 with cell image pointers
    //     xcTextObj (type 10): replaces string hashes at +56 with string pointers,
    //                          replaces font hash at +48 with xcFont pointer
    // Loop 2: for each screen inventory item:
    //   FindNamedData__8xcScreen - replaces overlay hashes at +4 with overlay data pointers
    //
    // PC: all of these store 32-bit PSX pointers in 32-bit fields.
    // On 64-bit PC, we cannot store pointers in 32-bit fields.
    // Instead, GotoScreen/Draw do hash-based lookup at runtime.
}

// PSX: GotoScreen__9xcSectionP8xcScreen (XCSOS.CPP:332, 0x8005EC40)
void xcSection::GotoScreen(xcScreenData* scr) {
    MARKFUNCTION(0x8005EC40);
    if (currentScreen)
        UnloadOverlays();

    // PSX: LoadOverlays__8xcScreen - sets visibility on screen's overlays
    if (scr) {
        u32* refs = reinterpret_cast<u32*>(reinterpret_cast<u8*>(scr) + 4);
        for (u32 i = 0; i < scr->overlayCount; i++) {
            xcOverlayData* ovl = FindOverlay(refs[i]);
            if (ovl) ovl->visibility = 1;
        }
    }
    currentScreen = scr;
}

// PSX: UnloadOverlays__9xcSection (XCSOS.CPP:346, 0x8005EC8C)
// Hides ALL overlays in the section (not just current screen's).
void xcSection::UnloadOverlays() {
    MARKFUNCTION(0x8005EC8C);
    if (!overlays) return;
    const xcInventoryItem* items = overlays->GetItems();
    for (u32 i = 0; i < overlays->itemCount; i++) {
        xcOverlayData* ovl = reinterpret_cast<xcOverlayData*>(rawData + items[i].dataOffset);
        ovl->visibility = 0;
    }
}

// PSX: FindImage__9xcSectionUl (XCSOS.CPP:373, 0x8005ED9C)
xcCellImage* xcSection::FindImage(u32 hash) const {
    MARKFUNCTION(0x8005ED9C);
    return FindCell(hash);
}

// PSX: Draw__9xcSection (XCSOS.CPP:355, 0x8005ED04)
void xcSection::Draw() {
    MARKFUNCTION(0x8005ED04);
    if (!overlays) return;

    u32 ovlCount = overlays->itemCount;
    const xcInventoryItem* ovlItems = overlays->GetItems();

    // PSX iterates overlays backward into an ordering table (OT).
    // On PC we draw immediately, so iterate forward: earlier prims draw
    // first (background), later prims draw on top (text).
    for (u32 i = 0; i < ovlCount; i++) {
        u32 offset = ovlItems[i].dataOffset;
        if (offset + 8 > rawSize) continue;

        xcOverlayData* ovl = reinterpret_cast<xcOverlayData*>(rawData + offset);
        if (ovl->visibility == 0) continue;

        u8* ovlBase = rawData + offset;
        u32 primCount = ovl->primCount;

        for (u32 pi = 0; pi < primCount; pi++) {
            u32 primOffset = *(u32*)(ovlBase + 8 + pi * 8 + 4);
            if (primOffset + 4 > rawSize) continue;

            u8* primData = rawData + primOffset;
            DrawPrimObj(primData);
        }
    }
}

// PSX: Draw__9xcPrimObj (XCDO.CPP:110, 0x800AE430)
void xcSection::DrawPrimObj(u8* primData) {
    auto* hdr = reinterpret_cast<xcPrimHeader*>(primData);
    if (hdr->subtype == 5) return;

    switch (hdr->type) {
        case 16:
        {
            auto* prim = reinterpret_cast<xcPolyF4Prim*>(primData);
            s16 x0, y0, x1, y1;
            prim->GetBounds(x0, y0, x1, y1);

            f32 nx = PSX_NORM_X(x0);
            f32 ny = 1.0f - PSX_NORM_Y(y1);
            f32 nw = PSX_NORM_W(x1 - x0);
            f32 nh = PSX_NORM_H(y1 - y0);

            ScreenDraw::DrawColoredRect(nx, ny, nw, nh, prim->r, prim->g, prim->b, 255);
            break;
        }
        case 17:
        {
            auto* prim = reinterpret_cast<xcPolyG4Prim*>(primData);
            s16 x0, y0, x1, y1;
            prim->GetBounds(x0, y0, x1, y1);
            u8 r, g, b;
            prim->GetAvgColor(r, g, b);

            f32 nx = PSX_NORM_X(x0);
            f32 ny = 1.0f - PSX_NORM_Y(y1);
            f32 nw = PSX_NORM_W(x1 - x0);
            f32 nh = PSX_NORM_H(y1 - y0);

            ScreenDraw::DrawColoredRect(nx, ny, nw, nh, r, g, b, 255);
            break;
        }
        case 9:
        {
            auto* prim = reinterpret_cast<xcSpritePrim*>(primData);
            if (prim->numImages == 0) break;

            xcCellImage* cell = FindCell(prim->GetImageHash());
            if (!cell) break;
            tTexture* tex = cell->GetTexture();
            if (!tex) break;

            s32 sx = prim->mtx.GetX();
            s32 sy = prim->mtx.GetY();
            f32 nx = PSX_NORM_X(sx);
            f32 ny = 1.0f - PSX_NORM_Y(sy + cell->height);
            f32 nw = PSX_NORM_W(cell->width);
            f32 nh = PSX_NORM_H(cell->height);

            ScreenDraw::DrawQuad(tex, nx, ny, nw, nh,
                                 0.0f, 0.0f, 1.0f, 1.0f,
                                 prim->colorR, prim->colorG, prim->colorB, prim->colorA);
            break;
        }
        case 10:
        {
            auto* prim = reinterpret_cast<xcTextPrim*>(primData);
            if (prim->numStrings == 0) break;

            const char* str = FindString(prim->GetStringHash());
            if (!str) break;

            xcFont* font = nullptr;
            if (sectionMan) font = sectionMan->FindFont(prim->fontHash);
            if (!font) break;

            // Map from 512x240 overlay space to 320x240 font space
            s32 fontX = prim->mtx.GetX() * PSX_FONT_WIDTH / PSX_SCREEN_WIDTH;
            s32 fontY = prim->mtx.GetY() * PSX_FONT_HEIGHT / PSX_SCREEN_HEIGHT;

            // Center text horizontally at the given X position
            s32 textW = font->MeasureText(str);
            fontX -= textW / 2;

            font->DrawText(str, fontX, fontY, prim->GetColor());
            static int textDbg = 0;
            if (textDbg++ < 10)
                LOG("[DrawText] str='%s' colorR=%d G=%d B=%d A=%d raw=0x%08X",
                    str, prim->colorR, prim->colorG, prim->colorB, prim->colorA, prim->GetColor());
            break;
        }
        default:
            break;
    }
}

// PSX: GetPrimObj__9xcOverlayUl11xcChunkEnum (XCSOS.CPP:109)
// Finds a prim object by hash and expected type within this overlay.
u8* xcOverlayData::GetPrimObj(u32 hash, u8 type, u8* rawData) const {
    const u8* ovlBase = reinterpret_cast<const u8*>(this);
    for (u32 i = 0; i < primCount; i++) {
        u32 itemHash = *(u32*)(ovlBase + 8 + i * 8);
        u32 itemOffset = *(u32*)(ovlBase + 8 + i * 8 + 4);
        if (itemHash == hash) {
            u8* prim = rawData + itemOffset;
            if (prim[0] == type)
                return prim;
        }
    }
    return nullptr;
}

// PSX: GetTextObj__9xcOverlayUl (XCSOS.CPP:148, 0x8005E9D0)
u8* xcOverlayData::GetTextObj(u32 hash, u8* rawData) const {
    return GetPrimObj(hash, 10, rawData); // type 10 = xcTextObj
}

// PSX: GetSprite__9xcOverlayUl (XCSOS.CPP:136, 0x8005E9B0)
u8* xcOverlayData::GetSprite(u32 hash, u8* rawData) const {
    return GetPrimObj(hash, 9, rawData); // type 9 = xcSprite
}

// Find raw overlay data by hash
xcOverlayData* xcSection::FindOverlay(u32 hash) const {
    if (!overlays) return nullptr;
    const xcInventoryItem* items = overlays->GetItems();
    for (u32 i = 0; i < overlays->itemCount; i++) {
        if (items[i].hash == hash) {
            return reinterpret_cast<xcOverlayData*>(rawData + items[i].dataOffset);
        }
    }
    return nullptr;
}

// Find raw screen data by hash
xcScreenData* xcSection::FindScreen(u32 hash) const {
    if (!screens) return nullptr;
    const xcInventoryItem* items = screens->GetItems();
    for (u32 i = 0; i < screens->itemCount; i++) {
        if (items[i].hash == hash) {
            return reinterpret_cast<xcScreenData*>(rawData + items[i].dataOffset);
        }
    }
    return nullptr;
}

// PSX: FindString__9xcSectionUl (XCSOS.CPP, 0x8005EB30)
const char* xcSection::FindString(u32 hash) const {
    MARKFUNCTION(0x8005EB30);
    if (!strings) return nullptr;
    const xcInventoryItem* item = strings->FindItem(hash);
    if (!item) return nullptr;
    return reinterpret_cast<const char*>(rawData + item->dataOffset);
}

// Show all overlays referenced by a screen
void xcSection::ShowScreen(u32 hash) {
    xcScreenData* scr = FindScreen(hash);
    if (!scr) return;

    u32* refs = reinterpret_cast<u32*>(reinterpret_cast<u8*>(scr) + 4);
    for (u32 i = 0; i < scr->overlayCount; i++) {
        xcOverlayData* ovl = FindOverlay(refs[i]);
        if (ovl) ovl->visibility = 1;
    }
}

// Hide all overlays referenced by a screen
void xcSection::HideScreen(u32 hash) {
    xcScreenData* scr = FindScreen(hash);
    if (!scr) return;

    u32* refs = reinterpret_cast<u32*>(reinterpret_cast<u8*>(scr) + 4);
    for (u32 i = 0; i < scr->overlayCount; i++) {
        xcOverlayData* ovl = FindOverlay(refs[i]);
        if (ovl) ovl->visibility = 0;
    }
}

// Hide all overlays
void xcSection::HideAllOverlays() {
    if (!overlays) return;
    const xcInventoryItem* items = overlays->GetItems();
    for (u32 i = 0; i < overlays->itemCount; i++) {
        xcOverlayData* ovl = reinterpret_cast<xcOverlayData*>(rawData + items[i].dataOffset);
        ovl->visibility = 0;
    }
}

// ---- xcSectionMan ----

xcSectionMan::~xcSectionMan() {
    DeleteFonts();
    FreeSection();
}

bool xcSectionMan::LoadSection(const char* filename) {
    FreeSection();

    FILE* f = std::fopen(filename, "rb");
    if (!f) {
        LOG("[xcSectionMan] Failed to open: %s", filename);
        return false;
    }

    std::fseek(f, 0, SEEK_END);
    u32 size = (u32)std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    u8* data = new u8[size];
    std::fread(data, 1, size, f);
    std::fclose(f);

    section = new xcSection();
    if (!section->Init(data, size, this)) {
        delete section;
        section = nullptr;
        return false;
    }

    LOG("[xcSectionMan] Loaded %s (%u bytes)", filename, size);
    return true;
}

void xcSectionMan::FreeSection() {
    delete section;
    section = nullptr;
}

// PSX: LoadFonts__12xcSectionManP11xcInventory (XCSOS.CPP:676, 0x8005F3B4)
// PSX: copies font inventory, sorts, creates xcFont objects for each item.
// PSX stores xcFont* in the 32-bit item dataOffset field.
// PC: stores xcFont* in separate fontObjects array (64-bit pointers).
void xcSectionMan::LoadFonts(xcInventory* fontInv, u8* fileBase) {
    MARKFUNCTION(0x8005F3B4);
    DeleteFonts();

    if (!fontInv || fontInv->itemCount == 0) return;

    // PSX: copy font inventory into sectionMan->fonts
    u32 invSize = fontInv->GetSizeInBytes();
    u8* copy = new u8[invSize];
    std::memcpy(copy, fontInv, invSize);
    fonts = reinterpret_cast<xcInventory*>(copy);

    // PSX: Sort(sectionMan->fonts)
    fonts->Sort();

    // PSX: for each font item, allocate xcFont(276 bytes) and construct from raw data
    // On PSX: item->dataOffset gets replaced with xcFont* (32-bit).
    // On PC: we store xcFont* in fontObjects[i] and use the copied inventory for hash lookup.
    u32 count = fonts->itemCount;
    numFontObjects = (s32)count;
    fontObjects = new xcFont * [count];

    xcInventoryItem* items = fonts->GetItems();
    // Also need original (unsorted) inventory to resolve data offsets before sorting changed order.
    // Since we sorted the copy, the items' hashes are sorted but dataOffsets still hold file offsets.
    for (u32 i = 0; i < count; i++) {
        // Resolve raw data pointer: fileBase + item's original dataOffset
        const u8* rawData = fileBase + items[i].dataOffset;
        fontObjects[i] = new xcFont(rawData);
    }

    LOG("[xcSectionMan] LoadFonts: %u fonts loaded", count);
}

// PSX: DeleteFonts__12xcSectionMan (XCSOS.CPP:653, 0x8005F300)
void xcSectionMan::DeleteFonts() {
    MARKFUNCTION(0x8005F300);
    if (fontObjects) {
        for (s32 i = 0; i < numFontObjects; i++) {
            delete fontObjects[i];
        }
        delete[] fontObjects;
        fontObjects = nullptr;
        numFontObjects = 0;
    }
    if (fonts) {
        delete[] reinterpret_cast<u8*>(fonts);
        fonts = nullptr;
    }
}

// PSX: FindFont__12xcSectionManUl (XCSOS.CPP:580, 0x8005F190)
xcFont* xcSectionMan::FindFont(u32 hash) {
    MARKFUNCTION(0x8005F190);
    if (!fonts || !fontObjects) return nullptr;
    const xcInventoryItem* items = fonts->GetItems();
    for (u32 i = 0; i < fonts->itemCount; i++) {
        if (items[i].hash == hash)
            return fontObjects[i];
    }
    return nullptr;
}

// PSX: FindFont__12xcSectionManPCc (XCSOS.CPP:591, 0x8005F1B8)
xcFont* xcSectionMan::FindFont(const char* name) {
    MARKFUNCTION(0x8005F1B8);
    u32 hash = xcHash(name);
    return FindFont(hash);
}
