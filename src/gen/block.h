// block.h = Block class reversed from PSX BLOCK.CPP
// Original: C:\CHAN\GAME\SRC\GEN\BLOCK.CPP
#pragma once

#include "core.h"

class pddiPrimBuffer;
struct DBVolume;

// LVector = PSX 3D integer vector (tagLVector)
struct LVector {
    s32 x, y, z;
};

// DBAttrib = attribute from WDB database node
struct DBAttrib {
    u16 id;
    u16 type;
    u32 value;
    const char* strValue;
};

// Block = reversed from PSX Block class (104 bytes on PSX)
// Source: C:\CHAN\GAME\SRC\GEN\BLOCK.CPP
struct Block {
    // +0: parsed flag (1 if data has prims, 0 otherwise)
    u32 parsed;
    // +4,+8,+12: world position from DBVolume
    s32 posX;
    s32 posY;
    s32 posZ;
    // +16,+20,+24: dimensions (abs corner deltas)
    s32 dimX;
    s32 dimY;
    s32 dimZ;
    // +28: block number from attrib 15
    u16 blockNum;
    // +30-35: per-axis LOD near/far (attribs 10,11,25,26,27,28)
    u8 lodNearX;
    u8 lodFarX;
    u8 lodNearY;
    u8 lodFarY;
    u8 lodNearZ;
    u8 lodFarZ;
    // +36-37: LOD extra (attribs 12,13)
    u8 lodExtra0;
    u8 lodExtra1;
    // +38,+40: unknown (attribs 16,17)
    u16 unk38;
    u16 unk40;
    // +42,+44: script info (attrib 99)
    u16 hasScript;
    u16 scriptId;
    // +46: padding
    u16 pad46;
    // +48,+50: fog distance range (attribs 20,21)
    u16 fogMinDist;
    u16 fogMaxDist;
    // +52: fog color (attrib 22)
    u32 fogColor;
    // +56: unknown (attrib 23)
    u16 unk56;
    u16 pad58;
    // +60: unknown (attrib 24)
    u32 unk60;
    // +64: data pointer (BLK entry data)
    const u8* data;
    // +68: tPrimGeom / pddiPrimBuffer (PC replacement)
    pddiPrimBuffer* primBuffer;
    // +72: collision sector pointer
    void* collision;
    // +76: texture page animation frame counter
    s32 texPageFrame;
    // +80-100: half-extents (computed in SetDimension)
    s32 halfExtNegX;
    s32 halfExtNegY;
    s32 halfExtNegZ;
    s32 halfExtPosX;
    s32 halfExtPosY;
    s32 halfExtPosZ;

    Block();
    ~Block();

    void Init(const DBVolume* vol);                            // 0x80052BB0
    void SetDimension(const LVector* a, const LVector* b);     // 0x80052EA0
    void Parse(u32 size, const u8* blkData);                    // 0x80052F80
    void Unload();                                              // 0x80052FF0
    bool PointInBlock(const LVector* pt) const;                 // 0x80053024
    u32 GetNextBlockNumber() const;                             // 0x800530B0
    u32 GetPrevBlockNumber() const;                             // 0x800530D4
    bool Draw(const LVector* drawPos);                          // 0x800530F8
    void LoadPrim(const u8* primData, u32 primSize);            // 0x8005328C
    void Destroy();
};
