// skeleton.cpp - Pure3D v11.3 tSTree/tSJoint implementation
// PSX: TSTREE.CPP (Display, ComputeMatrices), STLOAD.CPP (tSTreeLoader)
#include "p3d/skeleton.h"
#include "p3d/hash.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "p3d/p3dmath.h"
#include <cstring>
#include <cstdlib>

// P3D chunk IDs for skeleton loading
static constexpr u16 CHUNK_STREE_JOINT   = 0x6121;
static constexpr u16 CHUNK_STREE_MAPPING = 0x4123;
static constexpr u16 CHUNK_REST_POSE     = 0x6125;

static u16 ReadU16(const u8* p) { return p[0] | (p[1] << 8); }
static u32 ReadU32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static s16 ReadS16(const u8* p) { return (s16)(p[0] | (p[1] << 8)); }

// Read PString: u8 length prefix + chars. Returns name hash.
static u32 ReadPString(const u8* data, u32 maxLen, u32& outBytesRead) {
    if (maxLen < 1) {
        outBytesRead = 0;
        return 0;
    }
    u8 len = data[0];
    if (1u + len > maxLen) {
        outBytesRead = 1;
        return 0;
    }
    char buf[256];
    u32 copyLen = (len < 255) ? len : 255;
    std::memcpy(buf, data + 1, copyLen);
    buf[copyLen] = '\0';
    outBytesRead = 1 + len;
    return p3dHash(buf);
}

STreeData::~STreeData() {
    if (joints) {
        for (u32 i = 0; i < numJoints; i++) {
            if (joints[i].meshBuffer) {
                joints[i].meshBuffer->Release();
                joints[i].meshBuffer = nullptr;
            }
        }
        std::free(joints);
        joints = nullptr;
    }
    if (jointOrderMap) {
        std::free(jointOrderMap);
        jointOrderMap = nullptr;
    }
}

void STreeData::ComputeWorldMatrices(Mat4* outMatrices) const {
    if (!joints || numJoints == 0) {
        return;
    }

    Mat4 matStack[32];
    s32 stackTop = 0;
    Mat4 current;

    for (u32 i = 0; i < numJoints; i++) {
        const STreeJoint& j = joints[i];

        if (j.flags & STF_PUSH_MATRIX) {
            if (stackTop < 32) {
                matStack[stackTop++] = current;
            }
        }

        if (j.flags & STF_LOAD_MATRIX) {
            // TODO: load from captured anim matrix buffer
        }

        if (j.flags & STF_TRANSFORM) {
            Mat4 local;
            p3dBuildRotMatrixYZX(j.rotationX, j.rotationY, j.rotationZ, local);
            local.SetTranslation((f32)j.translationX, (f32)j.translationY, (f32)j.translationZ);

            Mat4 temp = current;
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    current.m[r * 4 + c] =
                        local.m[r * 4 + 0] * temp.m[0 * 4 + c] +
                        local.m[r * 4 + 1] * temp.m[1 * 4 + c] +
                        local.m[r * 4 + 2] * temp.m[2 * 4 + c] +
                        local.m[r * 4 + 3] * temp.m[3 * 4 + c];
                }
            }
        }

        // 0x80: capture current matrix for later loadMatrix joints
        if (j.flags & STF_CAPTURE_MATRIX) {
            // TODO: store matrix for Repeat joint loading
        }

        outMatrices[i] = current;

        if (j.flags & STF_POP_MATRIX) {
            if (stackTop > 0) {
                current = matStack[--stackTop];
            }
        }
    }
}

// Parse a single 0x6121 joint sub-chunk
static bool ParseJointChunk(const u8* data, u32 dataSize, STreeJoint* out) {
    u32 p = 0;

    // PString -> nameUID
    u32 nameBytes = 0;
    out->nameUID = ReadPString(data + p, dataSize - p, nameBytes);
    p += nameBytes;

    // Long -> flags (PSX stores pre-computed traversal flags here)
    if (p + 4 > dataSize) {
        return false;
    }
    out->flags = ReadU32(data + p);
    p += 4;

    // Word -> primGeomStartIdx
    if (p + 2 > dataSize) {
        return false;
    }
    out->primGeomStartIdx = ReadU16(data + p);
    p += 2;

    // Word -> primGeomCount
    if (p + 2 > dataSize) {
        return false;
    }
    out->primGeomCount = ReadU16(data + p);
    p += 2;

    // Word -> polyStartIdx
    if (p + 2 > dataSize) {
        return false;
    }
    out->polyStartIdx = ReadU16(data + p);
    p += 2;

    // Long -> extraMemOffset (skip - PSX memory management)
    if (p + 4 > dataSize) {
        return false;
    }
    p += 4;

    // Initialize runtime fields
    out->translationX = 0;
    out->translationY = 0;
    out->translationZ = 0;
    out->rotationX = 0;
    out->rotationY = 0;
    out->rotationZ = 0;
    out->restPoseRotX = 0;
    out->restPoseRotY = 0;
    out->restPoseRotZ = 0;
    out->meshBuffer = nullptr;

    // Look for optional 0x6125 rest-pose sub-chunk
    while (p + 6 <= dataSize) {
        u16 subId = ReadU16(data + p);
        u32 subSize = ReadU32(data + p + 2);
        if (subSize < 6 || p + subSize > dataSize) {
            break;
        }
        if (subId == CHUNK_REST_POSE) {
            u32 sp = p + 6;
            if (sp + 6 <= p + subSize) {
                out->restPoseRotX = ReadS16(data + sp);
                out->restPoseRotY = ReadS16(data + sp + 2);
                out->restPoseRotZ = ReadS16(data + sp + 4);
            }
        }
        p += subSize;
    }

    return true;
}

STreeData* ParseSTreeChunk(const u8* chunkData, u32 chunkDataSize, bool isMapped) {
    u32 p = 0;

    // PString -> skeleton name
    u32 nameBytes = 0;
    ReadPString(chunkData + p, chunkDataSize - p, nameBytes);
    p += nameBytes;

    // Word -> numJoints
    if (p + 2 > chunkDataSize) {
        return nullptr;
    }
    s16 numJoints = ReadS16(chunkData + p);
    p += 2;
    if (numJoints <= 0 || numJoints > 256) {
        return nullptr;
    }

    // PString -> material name (skip)
    u32 matBytes = 0;
    ReadPString(chunkData + p, chunkDataSize - p, matBytes);
    p += matBytes;

    // Long -> extraMemSize (skip)
    if (p + 4 > chunkDataSize) {
        return nullptr;
    }
    p += 4;

    STreeData* tree = new STreeData();
    tree->numJoints = (u32)numJoints;
    tree->joints = (STreeJoint*)std::calloc(numJoints, sizeof(STreeJoint));

    // Parse sub-chunks
    u32 jointIdx = 0;
    while (p + 6 <= chunkDataSize) {
        u16 subId = ReadU16(chunkData + p);
        u32 subSize = ReadU32(chunkData + p + 2);
        if (subSize < 6 || p + subSize > chunkDataSize) {
            break;
        }

        if (subId == CHUNK_STREE_MAPPING && isMapped) {
            u32 mp = p + 6;
            if (mp + 4 <= p + subSize) {
                u32 mapCount = ReadU32(chunkData + mp);
                mp += 4;
                tree->numMapEntries = mapCount;
                tree->jointOrderMap = (u32*)std::malloc(mapCount * sizeof(u32));
                for (u32 m = 0; m < mapCount && mp + 4 <= p + subSize; m++) {
                    tree->jointOrderMap[m] = ReadU32(chunkData + mp);
                    mp += 4;
                }
            }
        } else if (subId == CHUNK_STREE_JOINT) {
            if (jointIdx < (u32)numJoints) {
                ParseJointChunk(chunkData + p + 6, subSize - 6, &tree->joints[jointIdx]);
                jointIdx++;
            }
        }

        p += subSize;
    }

    // If no mapping was parsed, create identity mapping
    if (!tree->jointOrderMap) {
        tree->numMapEntries = tree->numJoints;
        tree->jointOrderMap = (u32*)std::malloc(tree->numJoints * sizeof(u32));
        for (u32 i = 0; i < tree->numJoints; i++) {
            tree->jointOrderMap[i] = i;
        }
    }

    return tree;
}
