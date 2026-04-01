// block.cpp — Block class reversed from PSX BLOCK.CPP
// Original: C:\CHAN\GAME\SRC\GEN\BLOCK.CPP
#include "gen/block.h"
#include "gen/geometry.h"
#include "p3d/context.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include <cstring>

// DBVolume::FindAttrib — search for attribute by id
const DBAttrib* DBVolume::FindAttrib(u32 id) const {
    for (u32 i = 0; i < numAttribs; i++) {
        if (attribs[i].id == id) return &attribs[i];
    }
    return nullptr;
}

Block::Block() {
    MARKFUNCTION(0x80052B64); // __5Block
    std::memset(this, 0, sizeof(Block));
}

Block::~Block() {
    Destroy();
}

void Block::Destroy() {
    if (primBuffer) {
        primBuffer->Release();
        primBuffer = nullptr;
    }
}

// Init__5BlockPC8DBVolume (BLOCK.CPP:288)
void Block::Init(const DBVolume* vol) {
    MARKFUNCTION(0x80052BB0);

    // Copy position from DBVolume (+28,+32,+36)
    posX = vol->pos.x;
    posY = vol->pos.y;
    posZ = vol->pos.z;

    // Set dimensions from corners
    SetDimension(&vol->cornerA, &vol->cornerB);

    // Default LOD values
    lodNearX = 5; lodFarX = 1;
    lodNearY = 5; lodFarY = 1;
    lodNearZ = 5; lodFarZ = 1;
    lodExtra0 = 0;
    lodExtra1 = 0;
    unk38 = 0;
    unk40 = 0;
    fogMinDist = 0;
    fogMaxDist = 0;
    fogColor = 0;
    unk56 = 0;
    unk60 = 0;

    // Attrib 15: block number
    {
        const DBAttrib* a = vol->FindAttrib(15);
        if (a) blockNum = static_cast<u16>(a->value);
    }

    // Attrib 10: LOD near (all axes)
    {
        const DBAttrib* a = vol->FindAttrib(10);
        if (a && a->value != 0) {
            lodNearX = static_cast<u8>(a->value);
            lodNearY = static_cast<u8>(a->value);
            lodNearZ = static_cast<u8>(a->value);
        }
    }

    // Attrib 11: LOD far (all axes)
    {
        const DBAttrib* a = vol->FindAttrib(11);
        if (a && a->value != 0) {
            lodFarX = static_cast<u8>(a->value);
            lodFarY = static_cast<u8>(a->value);
            lodFarZ = static_cast<u8>(a->value);
        }
    }

    // Attrib 12: LOD extra 0
    {
        const DBAttrib* a = vol->FindAttrib(12);
        if (a && a->value != 0) {
            lodExtra0 = static_cast<u8>(a->value);
        }
    }

    // Attrib 13: LOD extra 1
    {
        const DBAttrib* a = vol->FindAttrib(13);
        if (a && a->value != 0) {
            lodExtra1 = static_cast<u8>(a->value);
        }
    }

    // Attrib 16: unk38
    {
        const DBAttrib* a = vol->FindAttrib(16);
        if (a) unk38 = static_cast<u16>(a->value);
    }

    // Attrib 17: unk40
    {
        const DBAttrib* a = vol->FindAttrib(17);
        if (a) unk40 = static_cast<u16>(a->value);
    }

    // Attrib 20: fog min distance
    {
        const DBAttrib* a = vol->FindAttrib(20);
        if (a) fogMinDist = static_cast<u16>(a->value);
    }

    // Attrib 21: fog max distance
    {
        const DBAttrib* a = vol->FindAttrib(21);
        if (a) fogMaxDist = static_cast<u16>(a->value);
    }

    // Attrib 22: fog color
    {
        const DBAttrib* a = vol->FindAttrib(22);
        if (a) fogColor = a->value;
    }

    // Attrib 23: unk56
    {
        const DBAttrib* a = vol->FindAttrib(23);
        if (a) unk56 = static_cast<u16>(a->value);
    }

    // Attrib 24: unk60
    {
        const DBAttrib* a = vol->FindAttrib(24);
        if (a) unk60 = a->value;
    }

    // Attrib 25: override lodNearY, lodNearZ
    {
        const DBAttrib* a = vol->FindAttrib(25);
        if (a) {
            lodNearY = static_cast<u8>(a->value);
            lodNearZ = static_cast<u8>(a->value);
        }
    }

    // Attrib 26: override lodFarY, lodFarZ
    {
        const DBAttrib* a = vol->FindAttrib(26);
        if (a) {
            lodFarY = static_cast<u8>(a->value);
            lodFarZ = static_cast<u8>(a->value);
        }
    }

    // Attrib 27: override lodNearZ only
    {
        const DBAttrib* a = vol->FindAttrib(27);
        if (a) lodNearZ = static_cast<u8>(a->value);
    }

    // Attrib 28: override lodFarZ only
    {
        const DBAttrib* a = vol->FindAttrib(28);
        if (a) lodFarZ = static_cast<u8>(a->value);
    }

    // Attrib 99: script info (string attrib)
    {
        const DBAttrib* a = vol->FindAttrib(99);
        if (a) {
            if (a->strValue) {
                hasScript = 1;
                scriptId = static_cast<u16>(static_cast<s8>(vol->scriptAxis));
            } else {
                hasScript = 0;
                scriptId = 0;
            }
        }
    }
}

// SetDimension__5BlockRC10tagLVectorT1 (BLOCK.CPP:442)
void Block::SetDimension(const LVector* a, const LVector* b) {
    MARKFUNCTION(0x80052EA0);

    s32 dx = a->x - b->x;
    s32 dy = a->y - b->y;
    s32 dz = a->z - b->z;

    dimX = (dx >= 0) ? dx : -dx;
    dimY = (dy >= 0) ? dy : -dy;
    dimZ = (dz >= 0) ? dz : -dz;

    // Negative half-extents
    halfExtNegX = -(dimX / 2);
    halfExtNegY = -(dimY / 2);
    halfExtNegZ = -(dimZ / 2);

    // Positive half-extents
    halfExtPosX = dimX / 2;
    halfExtPosY = dimY / 2;
    halfExtPosZ = dimZ / 2;
}

// Parse__5BlockUlPc (BLOCK.CPP:482)
// PSX: stores data ptr, checks prim offset, calls collision loader + LoadPrim
// PC: skip collision, call LoadPrim to build pddiPrimBuffer
void Block::Parse(u32 size, const u8* blkData) {
    MARKFUNCTION(0x80052F80);

    data = blkData;

    // PSX checks word at data+16 to determine if prim data exists
    if (size >= 20) {
        u32 check = blkData[16] | (blkData[17] << 8) |
                    (blkData[18] << 16) | (blkData[19] << 24);
        parsed = (check != 0) ? 1 : 0;
    } else {
        parsed = 0;
    }

    // PSX: collision = AsynchLoad__15CollisionSector(blockNum, data + *(data+20))
    // PC: skip collision loading

    // PSX: primGeom = LoadPrim(data + 24)
    // PC: build pddiPrimBuffer from tPrimGeom GPU packets
    if (size > 24) {
        LoadPrim(blkData + 24, size - 24);
    }
}

// Unload__5Block (BLOCK.CPP:514)
// PSX: unloads collision sector, clears data and collision pointers
void Block::Unload() {
    MARKFUNCTION(0x80052FF0);

    // PSX: Unload__15CollisionSector(collision)
    collision = nullptr;
    data = nullptr;
}

// PointInBlock__C5BlockRC10tagLVector (BLOCK.CPP:524)
// PSX: checks point against collision sector bounding box (+4..+24)
// PC: uses block position + half-extents (equivalent bounds)
bool Block::PointInBlock(const LVector* pt) const {
    MARKFUNCTION(0x80053024);

    // PSX reads bounding box from collision sector:
    //   collision[4]=negX, collision[8]=negY, collision[12]=negZ
    //   collision[16]=posX, collision[20]=posY, collision[24]=posZ
    // PC equivalent: posX + halfExtNeg/Pos
    s32 minX = posX + halfExtNegX;
    s32 maxX = posX + halfExtPosX;
    s32 minY = posY + halfExtNegY;
    s32 maxY = posY + halfExtPosY;
    s32 minZ = posZ + halfExtNegZ;
    s32 maxZ = posZ + halfExtPosZ;

    if (pt->x < minX) return false;
    if (pt->x > maxX) return false;
    if (pt->y < minY) return false;
    if (pt->y > maxY) return false;
    if (pt->z < minZ) return false;
    if (pt->z > maxZ) return false;
    return true;
}

// GetNextBlockNumber__C5Block (BLOCK.CPP:531)
// PSX: reads unk40 as s16; if > 0 return unk40-1, else return blockNum+1
u32 Block::GetNextBlockNumber() const {
    MARKFUNCTION(0x800530B0);

    s16 val = static_cast<s16>(unk40);
    if (val > 0) return static_cast<u32>(val - 1);
    return static_cast<u32>(blockNum + 1);
}

// GetPrevBlockNumber__C5Block (BLOCK.CPP:549)
// PSX: reads unk40 as s16; if < 0 return ~unk40, else return blockNum-1
u32 Block::GetPrevBlockNumber() const {
    MARKFUNCTION(0x800530D4);

    s16 val = static_cast<s16>(unk40);
    if (val < 0) return static_cast<u32>(~val);
    return static_cast<u32>(blockNum - 1);
}

// Draw__5BlockRC10tagLVector (BLOCK.CPP:600)
// PSX: calls SyncCamView, TransMatrix with drawPos, iterates texPageFrames,
//      updates UVPrimData/CBVPrimData, then RP_ZCullGClip or RP_ZCullGMFog.
// PC: set world matrix from drawPos, draw the prim buffer.
bool Block::Draw(const LVector* drawPos) {
    MARKFUNCTION(0x800530F8);

    if (!primBuffer) return false;

    // TransMatrix: set world translation from the passed position
    // (DrawEverythingHandler passes block pos modified by OffsetToPreventSeams)
    // PSX: gte_SetTransMatrix with drawPos directly, no axis flips
    Mat4 world;
    world.m[12] = static_cast<f32>(drawPos->x);
    world.m[13] = static_cast<f32>(drawPos->y);
    world.m[14] = static_cast<f32>(drawPos->z);
    p3d::context->SetWorldMatrix(world);

    // PSX: Update UVPrimData/CBVPrimData, then RP_ZCullGClip/RP_ZCullGMFog
    // PC: direct draw
    p3d::context->DrawPrimBuffer(primBuffer);

    return true;
}

// LoadPrim__5BlockPv (BLOCK.CPP:668)
// PSX: constructs tEntity on prim data, patches vtable, fixes up 12
//      word-offset pointers to absolute (<<2 + base), patches OT linked list.
// PC: parses GPU packet data to build pddiPrimBuffer.
void Block::LoadPrim(const u8* primData, u32 primSize) {
    MARKFUNCTION(0x8005328C);

    primBuffer = ParseBLKPrims(primData, primSize);
}
