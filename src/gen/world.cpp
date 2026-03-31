// world.cpp — Level world implementation
#include "gen/world.h"
#include "gen/assets.h"
#include "p3d/context.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstring>

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

// Parse a WDB node stream to extract DBVolume translations.
// Returns a vector of (block_number, tx, ty, tz) for volumes with subtype==0.
struct WDBVolume { u32 blockNum; s32 tx, ty, tz; };

static std::vector<WDBVolume> ParseWDBVolumes(const u8* d, u32 size) {
    std::vector<WDBVolume> vols;
    u32 pos = 0;
    while (pos + 4 <= size) {
        u32 tag = ReadU32(d + pos); pos += 4;
        if (tag >= 8 || tag == 0) continue;

        // Parse DBRoot common header
        if (pos + 36 > size) break;
        u16 typ = ReadU16(d + pos); pos += 4; // u16 + 2 padding
        u16 sub = ReadU16(d + pos); pos += 4;
        s32 tx = ReadS32(d + pos); pos += 4;
        s32 ty = ReadS32(d + pos); pos += 4;
        s32 tz = ReadS32(d + pos); pos += 4;
        pos += 12; // skip unk1, unk2, unk3
        if (pos + 4 > size) break;
        u32 nattr = ReadU32(d + pos); pos += 4;

        // Parse attribs: find attrib with id==15 (block number)
        u32 blockNum = 0;
        bool hasBlock = false;
        for (u32 i = 0; i < nattr; i++) {
            if (pos + 8 > size) break;
            u32 hdr = ReadU32(d + pos); pos += 4;
            u32 dlen = ReadU32(d + pos); pos += 4;
            u16 attrId = static_cast<u16>(hdr & 0xFFFF);
            u16 attrType = static_cast<u16>((hdr >> 16) & 0xFFFF);
            if (attrId == 15 && attrType == 1 && dlen >= 4 && pos + 4 <= size) {
                blockNum = ReadU32(d + pos);
                hasBlock = true;
            }
            pos += dlen;
        }

        // Tag-specific extra fields
        if (tag == 2) { // DBVolume
            if (pos + 12 <= size) {
                pos += 12; // skip sizeX, sizeZ, sizeY
            }
            if (sub == 0 && hasBlock) {
                vols.push_back({ blockNum, tx, ty, tz });
            }
        } else if (tag == 6) { // DBMesh
            if (pos + 4 <= size) {
                u32 fnlen = ReadU32(d + pos); pos += 4;
                pos += (fnlen + 3) & ~3;
            }
        }
        // tag 1 (DBPoint), 3-5: no extra fields after DBRoot
    }
    return vols;
}

void World::LoadTPGTextures(const u8* lcfData, u32 lcfSize) {
    mVRAM.Clear();

    // Re-parse stream header to find TPG entries
    if (lcfSize < 4) return;
    u32 count = (lcfData[0] << 24) | (lcfData[1] << 16) | (lcfData[2] << 8) | lcfData[3];
    u32 pos = 4;
    for (u32 i = 0; i < count; i++) {
        if (pos + 16 > lcfSize) break;
        char magic[5] = {};
        std::memcpy(magic, lcfData + pos, 4);
        u32 size   = (lcfData[pos+4]<<24) | (lcfData[pos+5]<<16) | (lcfData[pos+6]<<8) | lcfData[pos+7];
        u32 offset = (lcfData[pos+8]<<24) | (lcfData[pos+9]<<16) | (lcfData[pos+10]<<8) | lcfData[pos+11];
        u32 extraLen = (lcfData[pos+12]<<24) | (lcfData[pos+13]<<16) | (lcfData[pos+14]<<8) | lcfData[pos+15];
        pos += 16;
        if (extraLen > 0)
            pos += (extraLen + 3) & ~3;

        if (std::strncmp(magic, ".TPG", 4) != 0) continue;
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
                    mVRAM.Upload(rx, ry, rw, rh, d + p);
                }
            }
            cpos += chunkSize;
        }
    }

    // Upload raw VRAM as GL_R16UI texture for shader-side palette lookup
    if (mVRAMHandle) {
        glDeleteTextures(1, &mVRAMHandle);
        mVRAMHandle = 0;
    }
    glGenTextures(1, &mVRAMHandle);
    glBindTexture(GL_TEXTURE_2D, mVRAMHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, 1024, 512, 0,
                 GL_RED_INTEGER, GL_UNSIGNED_SHORT, mVRAM.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    RC_LOG("[World] Uploaded raw VRAM as GL_R16UI (1024x512, handle=%u)", mVRAMHandle);
}

World::World() = default;

World::~World() {
    Unload();
}

bool World::Load(const std::string& lcfPath) {
    Unload();

    auto stream = Assets::LoadStream(lcfPath);
    if (stream.entries.empty()) {
        RC_ERR("[World] Failed to load stream: %s", lcfPath.c_str());
        return false;
    }

    // Load TPG textures into VRAM and decode page
    LoadTPGTextures(stream.data.data(), static_cast<u32>(stream.data.size()));

    auto blkSpans = Assets::FilterStreamEntries(stream, ".BLK");
    RC_LOG("[World] Found %u BLK entries in %s", (u32)blkSpans.size(), lcfPath.c_str());

    for (const auto& span : blkSpans) {
        BlockMesh mesh = ParseBLK(span.ptr, span.size);
        if (mesh.vao != 0) {
            mBlocks.push_back(mesh);
        }
    }

    // Parse WDB entries to extract per-block world translations
    auto wdbSpans = Assets::FilterStreamEntries(stream, ".WDB");
    RC_LOG("[World] Found %u WDB entries", (u32)wdbSpans.size());
    u32 blkBase = 0;
    for (const auto& wdb : wdbSpans) {
        auto vols = ParseWDBVolumes(wdb.ptr, wdb.size);
        for (const auto& v : vols) {
            u32 globalIdx = blkBase + v.blockNum;
            if (globalIdx < mBlocks.size()) {
                mBlocks[globalIdx].tx = v.tx;
                mBlocks[globalIdx].ty = v.ty;
                mBlocks[globalIdx].tz = v.tz;
            }
        }
        blkBase += static_cast<u32>(vols.size());
        RC_LOG("[World]   WDB section: %u volumes, blkBase now %u", (u32)vols.size(), blkBase);
    }

    RC_LOG("[World] Loaded %u blocks", (u32)mBlocks.size());
    return !mBlocks.empty();
}

void World::Render() {
    p3d::context->SetVRAMHandle(mVRAMHandle);

    for (auto& block : mBlocks) {
        if (block.lod != mTargetLOD) continue;
        Mat4 world;
        world.m[12] = static_cast<f32>(block.tx);
        world.m[13] = static_cast<f32>(block.ty);
        world.m[14] = static_cast<f32>(block.tz);
        p3d::context->SetWorldMatrix(world);
        p3d::context->DrawPrimBuffer(PDDI_PRIM_TRIANGLES, block.vao, block.indexCount);
    }

    p3d::context->SetVRAMHandle(0);
}

void World::Unload() {
    for (auto& block : mBlocks)
        block.Destroy();
    mBlocks.clear();
    if (mVRAMHandle) {
        glDeleteTextures(1, &mVRAMHandle);
        mVRAMHandle = 0;
    }
}

// OrbitCamera

void FreeCamera::Update(void* window, f32 dt) {
    auto* win = static_cast<GLFWwindow*>(window);

    // Mouse rotation on LMB
    double mx, my;
    glfwGetCursorPos(win, &mx, &my);
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (mHasPrev) {
            f32 dx = static_cast<f32>(mx - mLastMX);
            f32 dy = static_cast<f32>(my - mLastMY);
            yaw   -= dx * sensitivity;
            pitch -= dy * sensitivity;
            if (pitch >  1.55f) pitch =  1.55f;
            if (pitch < -1.55f) pitch = -1.55f;
        }
        mHasPrev = true;
    } else {
        mHasPrev = false;
    }
    mLastMX = mx;
    mLastMY = my;

    // Speed boost with shift
    f32 spd = speed * dt;
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) spd *= 4.0f;

    // Forward/back/strafe
    f32 sy = std::sin(yaw), cy_ = std::cos(yaw);
    f32 sp = std::sin(pitch), cp = std::cos(pitch);
    // Forward direction
    f32 fx = sy * cp, fy = sp, fz = cy_ * cp;
    // Right direction
    f32 rx = -cy_, rz = sy;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) { x += fx * spd; y += fy * spd; z += fz * spd; }
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) { x -= fx * spd; y -= fy * spd; z -= fz * spd; }
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) { x -= rx * spd; z -= rz * spd; }
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) { x += rx * spd; z += rz * spd; }
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) y += spd;
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) y -= spd;
}

void FreeCamera::Apply() {
    f32 sy = std::sin(yaw), cy_ = std::cos(yaw);
    f32 sp = std::sin(pitch), cp = std::cos(pitch);
    f32 tx = x + sy * cp;
    f32 ty = y + sp;
    f32 tz = z + cy_ * cp;

    Mat4 view = LookAt(x, y, z, tx, ty, tz, 0, 1, 0);
    Mat4 proj = Perspective(0.7f, 4.0f / 3.0f, 10.0f, 100000.0f);

    p3d::context->SetViewMatrix(view);
    p3d::context->SetProjectionMatrix(proj);
}
