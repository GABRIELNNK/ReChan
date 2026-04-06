// world.cpp - Level world implementation
#include "common.h"
#include "gen/world.h"
#include "gen/ai.h"
#include "gen/charmgr.h"
#include "gen/database.h"
#include "gen/director.h"
#include "gen/geometry.h"
#include "gen/levelmgr.h"
#include "snd/rsevent.h"
#include "fe/loadanim.h"
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
    // Free level table data
    if (levelList) { delete[] levelList; levelList = nullptr; }
    if (highestPetal) { delete[] highestPetal; highestPetal = nullptr; }
    if (levelNames) {
        for (s32 i = 0; i < levelCount; i++)
            delete[] levelNames[i];
        delete[] levelNames;
        levelNames = nullptr;
    }
    if (petalNames) {
        for (s32 i = 0; i < levelCount; i++) {
            if (petalNames[i]) {
                s32 pc = levelList ? levelList[i * 2 + 1] : 0;
                for (s32 j = 0; j <= pc; j++)
                    delete[] petalNames[i][j];
                delete[] petalNames[i];
            }
        }
        delete[] petalNames;
        petalNames = nullptr;
    }
    if (petalSoundIDs) {
        for (s32 i = 0; i < levelCount; i++)
            delete[] petalSoundIDs[i];
        delete[] petalSoundIDs;
        petalSoundIDs = nullptr;
    }
}

// Simple tokenizer matching PSX GetNextToken__4Game
static bool GetNextToken(char* out, char** cursor, const char* delims) {
    char* p = *cursor;
    while (*p && strchr(delims, *p))
        p++;
    if (!*p) {
        *out = '\0';
        return false;
    }
    char* dst = out;
    while (*p && !strchr(delims, *p))
        *dst++ = *p++;
    *dst = '\0';
    *cursor = p;
    return true;
}

// PSX: LoadLevelNames__5World (WORLD.CPP:990, 0x80045700)
void World::LoadLevelNames() {
    // Local arrays matching PSX stack layout (max 16 levels, 16 petals each)
    char* tmpLevNames[16] = {};
    s32 tmpLevIDs[16] = {};
    s32 tmpPetalIdx[16][16] = {};
    s32 tmpSoundBytes[16][16] = {};
    char* tmpPetalNames[16][16] = {};
    s32 tmpPetalCounts[16] = {};

    // Read RTARGET/GAME_LN.TXT
    char filename[128];
    std::snprintf(filename, sizeof(filename), "RTARGET/GAME_LN.TXT");

    std::ifstream file(filename);
    if (!file)
        return;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    levelCount = 0;
    char* cursor = content.data();
    char token[128];
    s32 levIdx = -1;
    s32 petalSeq = -1;

    // PSX: tokenize with " \r\n\t", matching the comma-operator ++v4 pattern
    while (GetNextToken(token, &cursor, " \r\n\t")) {
        if (token[0] == 'L' || token[0] == 'l') {
            // Level line: "levNN"
            ++levIdx;
            tmpLevIDs[levIdx] = atoi(token + 3);
            GetNextToken(token, &cursor, "\r\n");
            s32 len = (s32)strlen(token);
            tmpLevNames[levIdx] = new char[len + 1];
            memcpy(tmpLevNames[levIdx], token, len + 1);
            petalSeq = -1;
            ++levelCount;
        } else {
            // Petal line: <index> <soundByte> <name>
            ++petalSeq;
            tmpPetalIdx[levIdx][petalSeq] = atoi(token);
            GetNextToken(token, &cursor, " \t");
            tmpSoundBytes[levIdx][petalSeq] = atoi(token);
            GetNextToken(token, &cursor, "\r\n");
            s32 len = (s32)strlen(token);
            tmpPetalNames[levIdx][petalSeq] = new char[len + 1];
            memcpy(tmpPetalNames[levIdx][petalSeq], token, len + 1);
            ++tmpPetalCounts[levIdx];
        }
    }

    // Build levelList: pairs of {levelID, petalCount}
    levelList = new s32[levelCount * 2];
    highestPetal = new s32[levelCount];
    for (s32 i = 0; i < levelCount; i++) {
        levelList[i * 2] = tmpLevIDs[i];
        levelList[i * 2 + 1] = tmpPetalCounts[i];
    }

    // Build levelNames
    levelNames = new char*[levelCount + 1];
    levelNames[levelCount] = nullptr;

    // Build petalNames sub-arrays (null-initialized)
    petalNames = new char**[levelCount + 1];
    petalNames[levelCount] = nullptr;
    for (s32 i = 0; i <= levelCount; i++) {
        if (i == levelCount) break;
        s32 pc = tmpPetalCounts[i];
        petalNames[i] = new char*[pc + 1];
        for (s32 j = 0; j <= pc; j++)
            petalNames[i][j] = nullptr;
    }

    // Copy level names, petal names, and compute highestPetal
    for (s32 i = 0; i < levelCount; i++) {
        s32 len = (s32)strlen(tmpLevNames[i]);
        levelNames[i] = new char[len + 1];
        memcpy(levelNames[i], tmpLevNames[i], len + 1);
        delete[] tmpLevNames[i];

        s32 pc = tmpPetalCounts[i];
        for (s32 j = 0; j < pc; j++) {
            s32 pidx = tmpPetalIdx[i][j];
            s32 nlen = (s32)strlen(tmpPetalNames[i][j]);
            petalNames[i][pidx] = new char[nlen + 1];
            memcpy(petalNames[i][pidx], tmpPetalNames[i][j], nlen + 1);
            delete[] tmpPetalNames[i][j];
        }

        highestPetal[i] = tmpPetalIdx[i][pc - 1];
    }

    // Build petalSoundIDs (u8 arrays, sequential order)
    petalSoundIDs = new u8*[levelCount];
    for (s32 i = 0; i < levelCount; i++) {
        s32 pc = tmpPetalCounts[i];
        petalSoundIDs[i] = new u8[pc];
        for (s32 j = 0; j < pc; j++)
            petalSoundIDs[i][j] = (u8)tmpSoundBytes[i][j];
    }
}

// PSX: LoadPermanent__5World (WORLD.CPP:1062, 0x80045D6C)
void World::LoadPermanent() {
    LoadLevelNames();

    // PSX: LoadLevel__12LevelManager() - empty stub on PSX
    // PSX: PurgeLevelP3DInventory__12LevelManager() - also empty stub

    // PSX: OpenCharacter(type=0), EnableCache(type=0, 1)
    if (g_characterManager) {
        g_characterManager->OpenCharacter(0);
        g_characterManager->EnableCache(0, 1);

        // PSX: allocate CharMgrCallback, LoadCharacter(type=0, callback), spin until done
        CharMgrCallback* callback = new CharMgrCallback();
        g_characterManager->LoadCharacter(0, callback);
        // PSX spins: while (!callback->done) rDoTaskList(rMainTaskList, 0);
        // PC: LoadCharacter is synchronous, callback already fired

        // PSX: LoadAnimation(type=0, animEnum=0, hash=124, callback), spin until done
        callback->done = 0;
        g_characterManager->LoadAnimation(0, 0, 124, callback);
        // PSX spins again - PC is synchronous

        // PSX: LoadAnimation(type=0, animEnum=0, hash=124, nullptr) - fire and forget
        g_characterManager->LoadAnimation(0, 0, 124, nullptr);

        // PSX: EnableCache(type=0, 0), delete callback
        g_characterManager->EnableCache(0, 0);
        delete callback;
    }

    // PSX: AddThingNoTagList("Jackie", 0, {0,0,0}, {0,0,0}, "JACKIELOHIER", nullptr)
    if (g_ai) {
        LVector zeroPos = {0, 0, 0};
        SVector zeroOrient = {0, 0, 0};
        g_ai->AddThingNoTagList("Jackie", 0, &zeroPos, &zeroOrient, "JACKIELOHIER", nullptr);
    }
}

// PSX: LoadLevel__5WorldUl (WORLD.CPP:1389, 0x8004624C)
bool World::LoadLevelIndex(u32 levelIndex) {
    MARKFUNCTION(0x8004624C);

    // PSX: clamp levelIndex to valid range
    if (levelCount > 0 && levelIndex >= (u32)levelCount)
        levelIndex = (u32)(levelCount - 1);

    u32 prevLevel = currentLevelIndex;
    currentLevelIndex = levelIndex;
    previousLevelIndex = prevLevel;

    targetLevelIndex = levelIndex;

    // PSX: EstimateLoadTime, StartLogo, FillMeter(100)
    StartLogo("RUNFIRST.TIM");
    FillMeter(100);

    // PSX: rSPrintf(v8, "%slev%02d.lcf", "RTARGET\\", levelList[levelIndex * 2])
    char levelPath[64];
    s32 levNum = (levelList && levelCount > 0) ? levelList[levelIndex * 2] : (s32)(levelIndex + 1);
    std::snprintf(levelPath, sizeof(levelPath), "RTARGET/LEV%02d.LCF", levNum);
    if (!Load(levelPath)) {
        StopLogo();
        return false;
    }

    currentPetalIndex = targetPetalIndex;

    // PSX: rsEvent(4, petalSoundIDs[levelIndex][targetPetalIndex] - 1, 0, 0)
    if (petalSoundIDs && levelIndex < (u32)levelCount) {
        s32 soundLocation = (s32)petalSoundIDs[levelIndex][targetPetalIndex] - 1;
        rsEvent(RS_SET_LOCATION, soundLocation, 0, 0);
    }

    // PSX: ExecuteLoadCallbacks -> cameraLoadFunc -> SetupPaths
    // (handled by gsQueueLevelLoad on PC)

    // PSX: Construct__5World (WORLD.CPP:1399, 0x80046E80)
    // On PSX this is a separate function called after LoadLevel.
    // It initializes fighting collision, effects, populates AI entities,
    // loads backgrounds, resets Director, and sets up the level script.
    // We inline the steps we can handle here.

    // PSX: Init__17FightingCollision, InsertHumanoid (player)
    // TODO: FightingCollision not yet reversed

    // PSX: CheckpointInfo, WorldEffects, PWorldEffects, ParticleSystem
    // TODO: not yet reversed

    // PSX: Populate__2AI(0) - spawn entities from WDB database
    if (g_ai) {
        g_ai->Populate();
    }

    // PSX: LoadBG, InitBG - background rendering
    // TODO: BackG not yet reversed

    // PSX: ScoreManager::SetPar
    // TODO: not yet reversed

    // PSX: Director->Reset() then Director->SetScript()
    if (g_director) {
        g_director->Reset();
        g_director->SetScript();
    }

    // PSX: SetupModelAmbientLighting, ProcessSwitches
    // TODO: not yet reversed

    // PSX: rsEvent(5, 0, 0, 0) - start music for current location
    rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);

    // PSX: StopLogo after load completes
    StopLogo();
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
    // Use the global Database (created in Game::InternalOpen) so entities
    // remain accessible to AI::Populate after this function returns.
    g_database->Close();

    // Scan all WDB entries into the database
    for (const auto& e : entries) {
        if (strncmp(e.magic, ".WDB", 4) != 0) continue;
        if (e.offset + e.size > dataSize) continue;
        g_database->Scan(data + e.offset, e.size);
    }

    // PSX _LoadBlocksFunc: iterate GetFirstBlock linked list to build volume list.
    // PSX iterates the database block list sequentially - no index mapping needed.
    std::vector<DBVolume*> blockVolumes;
    for (DBRoot* v = g_database->GetFirstBlock(); v; v = static_cast<DBRoot*>(v->next)) {
        blockVolumes.push_back(static_cast<DBVolume*>(v));
    }
    LOG("[World] Parsed %u block volumes from WDB", (u32)blockVolumes.size());

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

void World::UploadToVRAM(s16 x, s16 y, s16 w, s16 h, const u8* raw) {
    vram.Upload(x, y, w, h, raw);
}

void World::RefreshVRAMTexture() {
    if (vramHandle) {
        p3d::context->DestroyVRAMTexture(vramHandle);
        vramHandle = 0;
    }
    vramHandle = p3d::context->CreateVRAMTexture(1024, 512, vram.data);
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

// PSX: UnloadPetal__5World (WORLD.CPP:1176, 0x80045F34)
void World::UnloadPetal() {
    MARKFUNCTION(0x80045F34);

    if (g_director) {
        g_director->PurgeAnims();
    }

    if (g_levelManager) {
        g_levelManager->PurgePetal();
    }

    rsEvent(RS_UNLOAD_LEVEL, 0, 0, 0);
}

// PSX: LoadPetal__5WorldUl (WORLD.CPP:1222, 0x8004604C)
void World::LoadPetal(u32 petalIndex) {
    MARKFUNCTION(0x8004604C);

    // PSX: if current level is DestSelect (lev07), save as previous
    if (levelList && currentLevelIndex < (u32)levelCount) {
        if (levelList[currentLevelIndex * 2] == 7)
            previousLevelIndex = currentLevelIndex;
    }

    // PSX: EstimateLoadTime, StartLogo, FillMeter(100)
    StartLogo("RUNFIRST.TIM");
    FillMeter(100);

    // PSX: rsEvent(4, petalSoundIDs[currentLevelIndex][petalIndex] - 1, 0, 0)
    if (petalSoundIDs && currentLevelIndex < (u32)levelCount) {
        s32 soundLocation = (s32)petalSoundIDs[currentLevelIndex][petalIndex] - 1;
        rsEvent(RS_SET_LOCATION, soundLocation, 0, 0);
    }

    targetPetalIndex = petalIndex;
    currentPetalIndex = petalIndex;

    if (g_levelManager) {
        g_levelManager->LoadPetal();
    }

    rsEvent(RS_LEVEL_BEGIN, 0, 0, 0);

    // PSX: StopLogo after load completes
    StopLogo();
}

// PSX: ResetLevel__5World (WORLD.CPP:1918, 0x80046DE0)
void World::ResetLevel() {
    MARKFUNCTION(0x80046DE0);

    // PSX also resets checkpoint validity and dead pool state here.
    if (g_director) {
        g_director->LevelReset();
    }
}

