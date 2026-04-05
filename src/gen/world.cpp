// world.cpp - Level world implementation
#include "gen/world.h"
#include "gen/database.h"
#include "gen/director.h"
#include "gen/geometry.h"
#include "p3d/context.h"
#include "p3d/stream.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"

#include <fstream>
#include <filesystem>

// Global block manager pointer (PSX: gp scope, set by World)
BlockManager* g_blockManager = nullptr;

// DynamicThing physics globals (PSX: gp+1740, gp+1744)
s32 g_maxFallSpeed = 0x4000;
s32 g_dampingFactor = 0xCCCC;

// PSX BGR555 to RGBA8
static void PsxToRGBA(u16 c, u8& r, u8& g, u8& b, u8& a) {
    r = static_cast<u8>((c & 0x1F) << 3);
    g = static_cast<u8>(((c >> 5) & 0x1F) << 3);
    b = static_cast<u8>(((c >> 10) & 0x1F) << 3);
    a = (c == 0) ? 0 : 255;
}

void PsxVRAM::DecodePage(u16 tpage, u16 cba, u8* out) const {
    int tx = tpage & 0xF;
    int ty = (tpage >> 4) & 1;
    int depth = (tpage >> 7) & 3;

    int pageX = tx * 64;  // VRAM word column
    int pageY = ty * 256; // VRAM row

    int clutX = (cba & 0x3F) * 16;
    int clutY = (cba >> 6) & 0x1FF;

    if (depth == 0) {
        // 4-bit indexed: 16-color CLUT
        u16 clut[16];
        for (int i = 0; i < 16; i++)
            clut[i] = Get(clutX + i, clutY);

        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int wordX = pageX + x / 4;
                int wordY = pageY + y;
                if (wordX >= 1024 || wordY >= 512) continue;
                u16 word = Get(wordX, wordY);
                int nibble = (word >> ((x % 4) * 4)) & 0xF;
                u16 color = clut[nibble];
                int idx = (y * 256 + x) * 4;
                PsxToRGBA(color, out[idx], out[idx + 1], out[idx + 2], out[idx + 3]);
            }
        }
    } else if (depth == 1) {
        // 8-bit indexed: 256-color CLUT
        u16 clut[256];
        for (int i = 0; i < 256; i++)
            clut[i] = Get(clutX + i, clutY);

        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int wordX = pageX + x / 2;
                int wordY = pageY + y;
                if (wordX >= 1024 || wordY >= 512) continue;
                u16 word = Get(wordX, wordY);
                int byteIdx = (x & 1) ? ((word >> 8) & 0xFF) : (word & 0xFF);
                u16 color = clut[byteIdx];
                int idx = (y * 256 + x) * 4;
                PsxToRGBA(color, out[idx], out[idx + 1], out[idx + 2], out[idx + 3]);
            }
        }
    } else {
        // 15-bit direct color
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int wordX = pageX + x;
                int wordY = pageY + y;
                if (wordX >= 1024 || wordY >= 512) continue;
                u16 color = Get(wordX, wordY);
                int idx = (y * 256 + x) * 4;
                PsxToRGBA(color, out[idx], out[idx + 1], out[idx + 2], out[idx + 3]);
            }
        }
    }
}

static u16 ReadU16(const u8* p) { return static_cast<u16>(p[0] | (p[1] << 8)); }
static u32 ReadU32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static s16 ReadS16(const u8* p) { return static_cast<s16>(p[0] | (p[1] << 8)); }
static s32 ReadS32(const u8* p) { return static_cast<s32>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24)); }

// Database::Scan handles WDB parsing now (see database.cpp).

void World::LoadTPGTextures(const u8* lcfData, u32 lcfSize) {
    vram.Clear();

    // Re-parse stream header to find TPG entries
    if (lcfSize < 4) return;
    u32 count = (lcfData[0] << 24) | (lcfData[1] << 16) | (lcfData[2] << 8) | lcfData[3];
    u32 pos = 4;
    for (u32 i = 0; i < count; i++) {
        if (pos + 16 > lcfSize) break;
        char magic[5] = {};
        memcpy(magic, lcfData + pos, 4);
        u32 size   = (lcfData[pos+4]<<24) | (lcfData[pos+5]<<16) | (lcfData[pos+6]<<8) | lcfData[pos+7];
        u32 offset = (lcfData[pos+8]<<24) | (lcfData[pos+9]<<16) | (lcfData[pos+10]<<8) | lcfData[pos+11];
        u32 extraLen = (lcfData[pos+12]<<24) | (lcfData[pos+13]<<16) | (lcfData[pos+14]<<8) | lcfData[pos+15];
        pos += 16;
        if (extraLen > 0)
            pos += (extraLen + 3) & ~3;

        if (strncmp(magic, ".TPG", 4) != 0) continue;
        if (offset + size > lcfSize || size < 6) continue;

        const u8* d = lcfData + offset;
        u16 rootId = ReadU16(d);
        u32 rootSize = ReadU32(d + 2);
        if (rootId != 0xFF04) continue;

        u32 cpos = 6;
        u32 cend = (rootSize < size) ? rootSize : size;
        while (cpos + 6 <= cend) {
            u16 chunkId = ReadU16(d + cpos);
            u32 chunkSize = ReadU32(d + cpos + 2);
            if (chunkSize < 6 || cpos + chunkSize > cend) break;

            if (chunkId == 0x6008) {
                u32 doff = cpos + 6;
                u32 dlen = chunkSize - 6;
                u32 p = doff;

                // PString: u8 len + chars
                if (p >= cpos + chunkSize) { cpos += chunkSize; continue; }
                u8 nameLen = d[p++];
                p += nameLen; // skip name

                // RECT16: s16 x, y, w, h + u32 type
                if (p + 12 > doff + dlen) { cpos += chunkSize; continue; }
                s16 rx = ReadS16(d + p); p += 2;
                s16 ry = ReadS16(d + p); p += 2;
                s16 rw = ReadS16(d + p); p += 2;
                s16 rh = ReadS16(d + p); p += 2;
                p += 4; // skip type

                // Upload raw pixel data to VRAM
                if (rw > 0 && rh > 0 && rw <= 1024 && rh <= 512 &&
                    p + rw * rh * 2 <= offset + size) {
                    vram.Upload(rx, ry, rw, rh, d + p);
                }
            }
            cpos += chunkSize;
        }
    }

    // Upload raw VRAM as R16UI texture for shader-side palette lookup
    if (vramHandle) {
        p3d::context->DestroyVRAMTexture(vramHandle);
        vramHandle = 0;
    }
    vramHandle = p3d::context->CreateVRAMTexture(1024, 512, vram.data);
    LOG("[World] Uploaded raw VRAM as R16UI (1024x512, handle=%u)", vramHandle);
}

World::World() = default;

World::~World() {
    Unload();
}

// PSX: LoadLevel__5WorldUl (WORLD.CPP:1389, 0x8004624C)
bool World::LoadLevelIndex(u32 levelIndex) {
    MARKFUNCTION(0x8004624C);

    targetLevelIndex = levelIndex;

    char levelPath[64];
    std::snprintf(levelPath, sizeof(levelPath), "RTARGET/LEV%02u.LCF", levelIndex + 1);
    if (!Load(levelPath)) {
        return false;
    }

    currentLevelIndex = targetLevelIndex;
    currentPetalIndex = targetPetalIndex;
    return true;
}

bool World::Load(const std::string& lcfPath) {
    Unload();

    // Read LCF file from disc (PC equivalent of Stream::Open + disc read)
    std::ifstream file(lcfPath, std::ios::binary | std::ios::ate);
    if (!file) {
        LOG("[World] Failed to open: %s", lcfPath.c_str());
        return false;
    }
    auto fileSize = file.tellg();
    file.seekg(0);
    streamData.resize(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(streamData.data()), fileSize);
    file.close();

    u32 dataSize = static_cast<u32>(streamData.size());
    const u8* data = streamData.data();

    // Parse stream header (PSX Stream::Open reads this from disc)
    auto entries = ParseStreamHeader(data, dataSize);
    if (entries.empty()) {
        LOG("[World] No stream entries in: %s", lcfPath.c_str());
        streamData.clear();
        return false;
    }

    // Load TPG textures into VRAM (PSX HandleTPGChunk)
    LoadTPGTextures(data, dataSize);

    // Count BLK and WDB entries from stream header
    u32 blkCount = 0;
    u32 wdbCount = 0;
    for (const auto& e : entries) {
        if (strncmp(e.magic, ".BLK", 4) == 0) blkCount++;
        if (strncmp(e.magic, ".WDB", 4) == 0) wdbCount++;
    }
    LOG("[World] Found %u BLK, %u WDB entries in %s", blkCount, wdbCount, lcfPath.c_str());

    // Parse WDB entries using Database::Scan (PSX HandleWDBChunk)
    // Each WDB has block numbers starting from 0 - they are local to that WDB's
    // BLK group. Count BLK entries between WDB entries to compute the base offset.
    std::vector<DBVolume*> blockVolumes(blkCount, nullptr);
    // We need the Database alive so DBVolume pointers remain valid
    Database db;
    {
        // Build list of (wdbIndex, blkBaseOffset) pairs
        // LCF stream interleaves: WDB#0 BLK*N0 WDB#1 BLK*N1 WDB#2 BLK*N2 ...
        struct WDBGroup { u32 entryIdx; u32 blkBase; u32 blocksBefore; };
        std::vector<WDBGroup> wdbGroups;
        u32 blkAccum = 0;
        for (u32 i = 0; i < entries.size(); i++) {
            if (strncmp(entries[i].magic, ".WDB", 4) == 0) {
                wdbGroups.push_back({i, blkAccum, 0});
            } else if (strncmp(entries[i].magic, ".BLK", 4) == 0) {
                blkAccum++;
            }
        }

        // Count existing block volumes before each scan so we can
        // attribute newly added volumes to the correct WDB group
        u32 prevCount = 0;
        for (size_t gi = 0; gi < wdbGroups.size(); gi++) {
            wdbGroups[gi].blocksBefore = prevCount;
            const auto& e = entries[wdbGroups[gi].entryIdx];
            if (e.offset + e.size > dataSize) continue;
            db.Scan(data + e.offset, e.size);

            // Count how many block volumes are in the database now
            u32 curCount = 0;
            DBRoot* v = db.GetFirstBlock();
            while (v) { curCount++; v = static_cast<DBRoot*>(v->next); }

            // The new volumes from this WDB group are indices [prevCount..curCount)
            // Their attrib 15 values are local, offset by blkBase
            u32 idx = 0;
            v = db.GetFirstBlock();
            while (v) {
                if (idx >= wdbGroups[gi].blocksBefore) {
                    DBVolume* dbVol = static_cast<DBVolume*>(v);
                    const DBAttrib* a = dbVol->FindAttrib(15);
                    if (a) {
                        u32 globalIdx = wdbGroups[gi].blkBase + a->value;
                        if (globalIdx < blkCount) {
                            blockVolumes[globalIdx] = dbVol;
                        }
                    }
                }
                idx++;
                v = static_cast<DBRoot*>(v->next);
            }
            prevCount = curCount;

            LOG("[World] WDB group at entry %u: blkBase=%u, %u volumes",
                   wdbGroups[gi].entryIdx, wdbGroups[gi].blkBase, curCount - wdbGroups[gi].blocksBefore);
        }

        u32 totalParsed = 0;
        for (auto* v : blockVolumes) { if (v) totalParsed++; }
        LOG("[World] Parsed %u DBVolumes from %u WDB entries", totalParsed, (u32)wdbGroups.size());
    }

    // Initialize blocks from volumes (PSX _LoadBlocksFunc - Block::Init)
    blockMgr.LoadBlocksFunc(blockVolumes);

    // Parse BLK data into blocks (PSX LoadBlocks - LoadSingleBlockAndParse - Parse)
    std::vector<const u8*> blkPtrs;
    std::vector<u32> blkSizes;
    for (const auto& e : entries) {
        if (strncmp(e.magic, ".BLK", 4) != 0) continue;
        if (e.offset + e.size > dataSize) {
            blkPtrs.push_back(nullptr);
            blkSizes.push_back(0);
        } else {
            blkPtrs.push_back(data + e.offset);
            blkSizes.push_back(e.size);
        }
    }
    blockMgr.LoadBlocks(0, blkPtrs.data(), blkSizes.data(), blkCount);

    LOG("[World] Loaded %u blocks", blockMgr.GetNumBlocks());

    // Debug: log ALL block positions and compute level AABB
    s32 minX = 0x7FFFFFFF, minY = 0x7FFFFFFF, minZ = 0x7FFFFFFF;
    s32 maxX = -0x7FFFFFFF, maxY = -0x7FFFFFFF, maxZ = -0x7FFFFFFF;
    for (u32 i = 0; i < blockMgr.GetNumBlocks(); i++) {
        Block* b = blockMgr.GetBlock(i);
        if (!b) continue;
        if (i < 10) LOG("[World] Block %u: pos=(%d,%d,%d) dim=(%d,%d,%d) parsed=%d",
                           i, b->posX, b->posY, b->posZ, b->dimX, b->dimY, b->dimZ, b->parsed);
        s32 bMinX = b->posX + b->halfExtNegX, bMaxX = b->posX + b->halfExtPosX;
        s32 bMinY = b->posY + b->halfExtNegY, bMaxY = b->posY + b->halfExtPosY;
        s32 bMinZ = b->posZ + b->halfExtNegZ, bMaxZ = b->posZ + b->halfExtPosZ;
        if (bMinX < minX) minX = bMinX; if (bMaxX > maxX) maxX = bMaxX;
        if (bMinY < minY) minY = bMinY; if (bMaxY > maxY) maxY = bMaxY;
        if (bMinZ < minZ) minZ = bMinZ; if (bMaxZ > maxZ) maxZ = bMaxZ;
    }
    levelMin = { minX, minY, minZ };
    levelMax = { maxX, maxY, maxZ };
    LOG("[World] Level AABB: min=(%d,%d,%d) max=(%d,%d,%d)",
           minX, minY, minZ, maxX, maxY, maxZ);
    LOG("[World] Level size: (%d, %d, %d)",
           maxX - minX, maxY - minY, maxZ - minZ);

    return blockMgr.GetNumBlocks() > 0;
}

void World::Render(const LVector* camPos) {
    p3d::context->SetVRAMHandle(vramHandle);
    DrawEverythingHandler(camPos);
    p3d::context->SetVRAMHandle(0);
}

// TransformVector - PC equivalent of PSX tPort::TransformVector
// Multiplies world-space point by the current view matrix (GTE rotation + translation)
static void TransformVector(const Mat4& vm, s32 inX, s32 inY, s32 inZ,
                            s32* outX, s32* outY, s32* outZ) {
    f32 fx = static_cast<f32>(inX);
    f32 fy = static_cast<f32>(inY);
    f32 fz = static_cast<f32>(inZ);
    *outX = static_cast<s32>(vm.m[0] * fx + vm.m[4] * fy + vm.m[8]  * fz + vm.m[12]);
    *outY = static_cast<s32>(vm.m[1] * fx + vm.m[5] * fy + vm.m[9]  * fz + vm.m[13]);
    *outZ = static_cast<s32>(vm.m[2] * fx + vm.m[6] * fy + vm.m[10] * fz + vm.m[14]);
}

// chanp3dClipCode - PC equivalent of PSX chanp3dClipCode
// Computes 6-bit clip code for a view-space point against the frustum
// bit 0: left, bit 1: right, bit 2: bottom, bit 3: top, bit 4: near, bit 5: far
static u32 chanp3dClipCode(const Mat4& pm, s32 vx, s32 vy, s32 vz) {
    f32 fx = static_cast<f32>(vx);
    f32 fy = static_cast<f32>(vy);
    f32 fz = static_cast<f32>(vz);
    // Transform to homogeneous clip space
    f32 cx = pm.m[0] * fx + pm.m[4] * fy + pm.m[8]  * fz + pm.m[12];
    f32 cy = pm.m[1] * fx + pm.m[5] * fy + pm.m[9]  * fz + pm.m[13];
    f32 cz = pm.m[2] * fx + pm.m[6] * fy + pm.m[10] * fz + pm.m[14];
    f32 cw = pm.m[3] * fx + pm.m[7] * fy + pm.m[11] * fz + pm.m[15];
    u32 code = 0;
    if (cx < -cw) code |= 0x01; // left
    if (cx >  cw) code |= 0x02; // right
    if (cy < -cw) code |= 0x04; // bottom
    if (cy >  cw) code |= 0x08; // top
    if (cz < -cw) code |= 0x10; // near
    if (cz >  cw) code |= 0x20; // far
    return code;
}

// vecLengthSquared - PC equivalent of PSX vecLengthSquared
// Returns squared length of view-space vector (with >>8 shift to prevent overflow)
static s32 vecLengthSquared(s32 x, s32 y, s32 z) {
    s32 sx = x >> 8;
    s32 sy = y >> 8;
    s32 sz = z >> 8;
    return sx * sx + sy * sy + sz * sz;
}

// DrawEverythingHandler__FP7Handler (GAME.CPP:2211)
// Reversed from PSX: builds draw list from block pool, selection-sorts by distSq,
// applies OffsetToPreventSeams, renders each block with Draw.
void World::DrawEverythingHandler(const LVector* camPos) {
    MARKFUNCTION(0x8002A98C);

    u32 numBlocks = blockMgr.GetNumBlocks();
    if (numBlocks == 0) return;

    // Build draw entry array: {Block*, distSq, zDepth}
    // PSX uses 12-byte entries: [block_ptr, distSq, zDepth]
    struct DrawEntry {
        Block* block;
        s32 distSq;
        s32 zDepth;
    };
    std::vector<DrawEntry> drawList;
    drawList.reserve(numBlocks);

    for (u32 i = 0; i < numBlocks; i++) {
        Block* block = blockMgr.GetBlock(i);
        if (!block || !block->primBuffer) continue;

        s32 distSq, zDepth;
        computeBlockToPointDistances(block, camPos, &distSq, &zDepth);
        drawList.push_back({ block, distSq, zDepth });
    }

    u32 count = static_cast<u32>(drawList.size());
    if (count == 0) return;

    // Selection sort by distSq ascending (nearest first)
    // PSX: inner loop finds minimum, swaps 12-byte entries
    for (u32 i = 0; i < count - 1; i++) {
        u32 minIdx = i;
        for (u32 j = i + 1; j < count; j++) {
            if (drawList[j].distSq < drawList[minIdx].distSq) {
                minIdx = j;
            }
        }
        if (minIdx != i) {
            DrawEntry tmp = drawList[i];
            drawList[i] = drawList[minIdx];
            drawList[minIdx] = tmp;
        }
    }

    // PSX: find maxZDepth across all entries, add 64, clamp to 0xFFFF
    // PSX: count entries with positive distSq (s6 index)
    // PSX: EnterLayer on tView (OT bucket management - handled by z-buffer on PC)

    // Render each block in sorted order
    for (u32 i = 0; i < count; i++) {
        DrawEntry& entry = drawList[i];

        // Skip culled blocks (distSq == -1 from frustum test)
        if (entry.distSq < 0) continue;

        // Copy block->pos to local and apply OffsetToPreventSeams
        // PSX: reads block+4/+8/+12 (posX/Y/Z) to stack local
        LVector localPos;
        localPos.x = entry.block->posX;
        localPos.y = entry.block->posY;
        localPos.z = entry.block->posZ;
        OffsetToPreventSeams(localPos, *camPos);

        // PSX: profile begin(10), DrawLoop(blockMgr+52, blockNum) - entity list 1
        // PSX: profile end(10), begin(11), DrawLoop(blockMgr+76, blockNum) - entity list 2
        // PSX: DrawLoop(blockMgr+64, blockNum) - entity list 3
        // PSX: profile end(11), begin(12), DrawLoop(blockMgr+88, blockNum) - entity list 4
        // PSX: profile end(12), begin(13), DrawEffects(blockNum)
        // PSX: profile end(13)

        // PSX: profile begin(9), Draw(block, &localPos), profile end(9)
        entry.block->Draw(&localPos);

        // PSX: ExitLayer on tView if current layer == 2
    }

    // PSX: DebugDrawSector, ExitLayer(2), profile end(7)
}

// computeBlockToPointDistances (GAME.CPP:1976)
// Reversed from PSX: builds 8 bounding box corners + center (9 points),
// transforms each through view matrix, computes clip codes + view-space distance,
// tests 13 clip code pairs for frustum culling.
// a0=block, a1=point, a2=outDistSq, a3=outZDepth
void World::computeBlockToPointDistances(const Block* block, const LVector* point,
                                         s32* outDistSq, s32* outZDepth) {
    MARKFUNCTION(0x8002A238);

    // PSX: reads bounding box from tPrimGeom virtual call: *(*(block->primGeom+8)+20)()
    // PC: uses block half-extent fields (same bounding box data, different access path)
    // s3 equivalent: bbox[0]=negX, [1]=negY, [2]=negZ, [3]=posX, [4]=posY, [5]=posZ
    s32 bbox[6] = {
        block->halfExtNegX, block->halfExtNegY, block->halfExtNegZ,
        block->halfExtPosX, block->halfExtPosY, block->halfExtPosZ
    };

    // s5 = &block->posX (block position at offset +4)
    const s32* pos = &block->posX; // pos[0]=X, pos[1]=Y, pos[2]=Z

    // Get view and projection matrices (PC equivalent of GTE state)
    const Mat4& vm = p3d::context->GetViewMatrix();
    const Mat4& pm = p3d::context->GetProjectionMatrix();

    s32 minDistSq = 0; // s7
    s32 maxZDepth = 0; // s6
    u32 clipCodes[8];
    s32 tvx, tvy, tvz; // transformed view-space coords

    // Build and process 8 bounding box corners
    // PSX corner pattern: (bbox[negX/posX], bbox[negY/posY], bbox[negZ/posZ]) + blockPos
    // Corner 0: pos + (negX, negY, negZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[1], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz; // save pre-project coords
        clipCodes[0] = chanp3dClipCode(pm, tvx, tvy, tvz);
        minDistSq = vecLengthSquared(svx, svy, svz);
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 1: pos + (posX, negY, negZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[1], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[1] = chanp3dClipCode(pm, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 2: pos + (negX, posY, negZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[4], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[2] = chanp3dClipCode(pm, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 3: pos + (posX, posY, negZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[4], wz = pos[2] + bbox[2];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[3] = chanp3dClipCode(pm, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 4: pos + (negX, negY, posZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[1], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[4] = chanp3dClipCode(pm, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 5: pos + (posX, negY, posZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[1], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[5] = chanp3dClipCode(pm, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 6: pos + (negX, posY, posZ)
    {
        s32 wx = pos[0] + bbox[0], wy = pos[1] + bbox[4], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[6] = chanp3dClipCode(pm, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Corner 7: pos + (posX, posY, posZ)
    {
        s32 wx = pos[0] + bbox[3], wy = pos[1] + bbox[4], wz = pos[2] + bbox[5];
        TransformVector(vm, wx, wy, wz, &tvx, &tvy, &tvz);
        s32 svx = tvx, svy = tvy, svz = tvz;
        clipCodes[7] = chanp3dClipCode(pm, tvx, tvy, tvz);
        s32 d = vecLengthSquared(svx, svy, svz);
        if (d < minDistSq) minDistSq = d;
        s32 z = svz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Center (9th point): just blockPos, no bbox offset
    // PSX: TransformVector + vecLengthSquared only (no ProjectVector/chanp3dClipCode)
    {
        TransformVector(vm, pos[0], pos[1], pos[2], &tvx, &tvy, &tvz);
        s32 d = vecLengthSquared(tvx, tvy, tvz);
        if (d < minDistSq) minDistSq = d;
        s32 z = tvz;
        if (z > 0xFFFF) z = 0xFFFF;
        if (z > maxZDepth) maxZDepth = z;
    }

    // Frustum cull test: 13 specific clip code pairs ANDed
    // If ANY pair ANDs to 0 - visible (at least one edge straddles a frustum plane)
    // If ALL pairs are non-zero - fully culled
    // PSX pairs: (0,1)(0,2)(0,4)(1,3)(1,5)(2,3)(2,6)(3,7)(4,5)(4,6)(5,7)(6,7)(0,7)
    if ((clipCodes[0] & clipCodes[1]) != 0 &&
        (clipCodes[0] & clipCodes[2]) != 0 &&
        (clipCodes[0] & clipCodes[4]) != 0 &&
        (clipCodes[1] & clipCodes[3]) != 0 &&
        (clipCodes[1] & clipCodes[5]) != 0 &&
        (clipCodes[2] & clipCodes[3]) != 0 &&
        (clipCodes[2] & clipCodes[6]) != 0 &&
        (clipCodes[3] & clipCodes[7]) != 0 &&
        (clipCodes[4] & clipCodes[5]) != 0 &&
        (clipCodes[4] & clipCodes[6]) != 0 &&
        (clipCodes[5] & clipCodes[7]) != 0 &&
        (clipCodes[6] & clipCodes[7]) != 0 &&
        (clipCodes[0] & clipCodes[7]) != 0) {
        // All 13 pairs non-zero - block is fully outside the frustum
        *outDistSq = -1;
        return;
    }

    // Visible - output minimum distance and maximum z-depth
    *outDistSq = minDistSq;
    *outZDepth = maxZDepth;
}

// OffsetToPreventSeams__FR10tagLVectorRC10tagLVector (GAME.CPP:2482)
// Reversed from PSX func_8002AF94: computes per-axis sign of (pos - camPos),
// then offset = -sign * (sign * delta / divisor + 1), clamped to Â±limit.
// Modifies pos in-place.
void World::OffsetToPreventSeams(LVector& pos, const LVector& camPos) {
    MARKFUNCTION(0x8002AF88);

    // PSX: t1 = &pos, v1 = pos.x, v0 = camPos.x
    s32 dx = pos.x - camPos.x; // sp[0]
    s32 dy = pos.y - camPos.y; // sp[4]
    s32 dz = pos.z - camPos.z; // sp[8]

    // Compute sign per axis: -1, 0, or +1 - sp[16], sp[20], sp[24]
    s32 signX = (dx < 0) ? -1 : (dx > 0) ? 1 : 0;
    s32 signY = (dy < 0) ? -1 : (dy > 0) ? 1 : 0;
    s32 signZ = (dz < 0) ? -1 : (dz > 0) ? 1 : 0;

    // PSX: v1 = gp[96] (seamDivisor)
    // These are set during level initialization - using reasonable PSX defaults
    s32 seamDivisor = 4096; // gp+0x60
    s32 seamLimit = 8;      // gp+0x64

    if (seamDivisor == 0) return;

    // PSX: a3 = (signX * dx) / seamDivisor
    s32 rawX = (signX * dx) / seamDivisor;
    s32 rawY = (signY * dy) / seamDivisor;
    s32 rawZ = (signZ * dz) / seamDivisor;

    // PSX: offset = (-sign) * (raw + 1) - pushes block position toward camera
    s32 offX = (-signX) * (rawX + 1);
    s32 offY = (-signY) * (rawY + 1);
    s32 offZ = (-signZ) * (rawZ + 1);

    // PSX: clamp each to Â±seamLimit (gp[100])
    if (offX < -seamLimit) offX = -seamLimit;
    else if (offX > seamLimit) offX = seamLimit;
    if (offY < -seamLimit) offY = -seamLimit;
    else if (offY > seamLimit) offY = seamLimit;
    if (offZ < -seamLimit) offZ = -seamLimit;
    else if (offZ > seamLimit) offZ = seamLimit;

    // PSX: add offsets to position
    pos.x += offX;
    pos.y += offY;
    pos.z += offZ;
}

void World::Unload() {
    blockMgr.InternalClose();
    streamData.clear();
    if (vramHandle && p3d::context) {
        p3d::context->DestroyVRAMTexture(vramHandle);
        vramHandle = 0;
    }
}

// PSX: ResetLevel__5World (WORLD.CPP:1918, 0x80046DE0)
void World::ResetLevel() {
    MARKFUNCTION(0x80046DE0);

    // PSX also resets checkpoint validity and dead pool state here.
    if (g_director) {
        g_director->LevelReset();
    }
}

