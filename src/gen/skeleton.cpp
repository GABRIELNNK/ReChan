// skeleton.cpp - game-side skeleton loading utilities
// ParseP3DStreamFull: extracts textures + skeleton from P3D stream
// BuildPerJointMeshes: builds pddiPrimBuffer from tPrimGeom + skeleton
#include "common.h"
#include "gen/skeleton.h"
#include "gen/model.h"
#include "gen/game.h"
#include "gen/world.h"
#include "p3d/context.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include <cstring>
#include <cstdlib>
#include <vector>

static u16 ReadU16(const u8* p) { return p[0] | (p[1] << 8); }
static u32 ReadU32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static s16 ReadS16(const u8* p) { return (s16)(p[0] | (p[1] << 8)); }

// P3D chunk IDs
static constexpr u16 CHUNK_P3D_CONTAINER = 0xFF04;
static constexpr u16 CHUNK_P3D_TEXTURE   = 0x6008;
static constexpr u16 CHUNK_STREE         = 0x6120;
static constexpr u16 CHUNK_MAPPED_STREE  = 0x6122;

STreeData* ParseP3DStreamFull(const u8* data, u32 size) {
    if (!data || size < 6) {
        return nullptr;
    }

    World* world = g_game ? g_game->GetWorld() : nullptr;
    STreeData* skeleton = nullptr;

    u16 rootId = ReadU16(data);
    u32 rootSize = ReadU32(data + 2);
    if (rootId != CHUNK_P3D_CONTAINER) {
        return nullptr;
    }

    u32 cpos = 6;
    u32 cend = (rootSize < size) ? rootSize : size;

    while (cpos + 6 <= cend) {
        u16 chunkId = ReadU16(data + cpos);
        u32 chunkSize = ReadU32(data + cpos + 2);
        if (chunkSize < 6 || cpos + chunkSize > cend) {
            break;
        }

        if (chunkId == CHUNK_P3D_TEXTURE && world) {
            u32 tp = cpos + 6;
            u32 tend = cpos + chunkSize;

            if (tp < tend) {
                u8 nameLen = data[tp++];
                tp += nameLen;
            }

            if (tp + 12 <= tend) {
                s16 rx = ReadS16(data + tp); tp += 2;
                s16 ry = ReadS16(data + tp); tp += 2;
                s16 rw = ReadS16(data + tp); tp += 2;
                s16 rh = ReadS16(data + tp); tp += 2;
                tp += 4; // type

                if (rw > 0 && rh > 0 && rw <= 1024 && rh <= 512 &&
                    tp + (u32)(rw * rh * 2) <= tend) {
                    world->UploadToVRAM(rx, ry, rw, rh, data + tp);
                }
            }
        } else if (chunkId == CHUNK_MAPPED_STREE) {
            if (!skeleton) {
                skeleton = ParseSTreeChunk(data + cpos + 6, chunkSize - 6, true);
            }
        } else if (chunkId == CHUNK_STREE) {
            if (!skeleton) {
                skeleton = ParseSTreeChunk(data + cpos + 6, chunkSize - 6, false);
            }
        }

        cpos += chunkSize;
    }

    if (world) {
        world->RefreshVRAMTexture();
    }

    if (skeleton) {
        LOG("[Skeleton] Parsed STree: %u joints, %u map entries",
            skeleton->numJoints, skeleton->numMapEntries);
    }

    return skeleton;
}

// PSX tTransformAnim raw binary header layout (40 bytes):
//   +0:  uid (u32)
//   +4:  refCount (s32)
//   +8:  vtable slot (u32, unused pre-relocation)
//   +12: numFrames (s32)
//   +16: targetType (s32)
//   +20: targetNameUID (u32)
//   +24: numRotChannels (s32)
//   +28: numTransChannels (s32)
//   +32: rotChannelArrayOff (u32, DWORD offset)
//   +36: transChannelArrayOff (u32, DWORD offset)
//
// Channel layout (pre-relocation):
//   +0: jointParamIndex (u32) - index into jointOrderMap
//   +4: keyType (u32) - identifies key list type
//   +8+: type-specific data
//
// Key types for rotation channels:
//   11 = tStatic3DOFKeyList: +8/+12/+16 = static X/Y/Z values (s32, written as s16 to joint)
//    5 = tJoint3DOFangle: +8=numKeys, +12=keyTimesOff, +16=keyValuesOff (packed u32)
//    3 = tJoint1DOFangle: +8=numKeys, +12=keyTimesOff, +20=keyValuesOff (s16)
//
// Key types for translation channels:
//   12 = tStatic3DOFKeyList: +8/+12/+16 = static X/Y/Z values (s32)
//    8 = tJoint3DOFlpPSX: +8=numKeys, +12=keyTimesOff, +16=keyValuesOff (3 x s16)

void ApplyAnimFrame0(STreeData* skeleton, const u8* rawAnimData, u32 rawAnimSize) {
    if (!skeleton || !rawAnimData || rawAnimSize < 40) {
        return;
    }

    s32 numRotCh = (s32)ReadU32(rawAnimData + 24);
    s32 numTransCh = (s32)ReadU32(rawAnimData + 28);
    u32 rotArrayDwordOff = ReadU32(rawAnimData + 32);
    u32 transArrayDwordOff = ReadU32(rawAnimData + 36);

    u32 rotArrayByteOff = rotArrayDwordOff * 4;
    u32 transArrayByteOff = transArrayDwordOff * 4;

    if (rotArrayByteOff + (u32)numRotCh * 4 > rawAnimSize) {
        LOG("[Anim] Rotation channel array out of bounds");
        return;
    }
    if (transArrayByteOff + (u32)numTransCh * 4 > rawAnimSize) {
        LOG("[Anim] Translation channel array out of bounds");
        return;
    }

    // Process rotation channels
    for (s32 i = 0; i < numRotCh; i++) {
        u32 chDwordOff = ReadU32(rawAnimData + rotArrayByteOff + i * 4);
        u32 chByteOff = chDwordOff * 4;
        if (chByteOff + 20 > rawAnimSize) {
            continue;
        }

        const u8* ch = rawAnimData + chByteOff;
        u32 jointParam = ReadU32(ch + 0);
        u32 keyType = ReadU32(ch + 4);

        // Map animation parameter index to actual joint index
        if (jointParam >= skeleton->numMapEntries) {
            continue;
        }
        u32 jointIdx = skeleton->jointOrderMap[jointParam];
        if (jointIdx >= skeleton->numJoints) {
            continue;
        }
        STreeJoint& joint = skeleton->joints[jointIdx];

        if (keyType == 11) {
            // tStatic3DOFKeyList: values at +8, +12, +16
            s32 vx = (s32)ReadU32(ch + 8);
            s32 vy = (s32)ReadU32(ch + 12);
            s32 vz = (s32)ReadU32(ch + 16);
            joint.rotationX = (s16)vx;
            joint.rotationY = (s16)vy;
            joint.rotationZ = (s16)vz;
        } else if (keyType == 5) {
            // tJoint3DOFangle: read first keyframe value (packed u32)
            u32 numKeys = ReadU32(ch + 8);
            if (numKeys == 0) {
                continue;
            }
            u32 valDwordOff = ReadU32(ch + 16);
            u32 valByteOff = valDwordOff * 4;
            if (valByteOff + 4 > rawAnimSize) {
                continue;
            }
            // PSX CHANNEL.CPP packing for tJoint3DOFangle.
            u32 packed = ReadU32(rawAnimData + valByteOff);
            joint.rotationX = (s16)((packed << 5) & 0xFFFF);
            joint.rotationY = (s16)((packed >> 6) & 0xFFE0);
            joint.rotationZ = (s16)((packed >> 16) & 0xFFC0);
        } else if (keyType == 3) {
            // tJoint1DOFangle: +8=numKeys, +12=keyTimesOff(u8), +16=dofIndex,
            // +20=keyValuesOff(s16). Evaluate at frame 0.
            u32 numKeys = ReadU32(ch + 8);
            if (numKeys == 0) {
                continue;
            }
            u32 keyTimesOff = ReadU32(ch + 12);
            u32 dofIndex = ReadU32(ch + 16);
            u32 keyValsOff = ReadU32(ch + 20);
            u32 keyTimesByteOff = keyTimesOff * 4;
            u32 keyValsByteOff = keyValsOff * 4;
            if (keyTimesByteOff + numKeys > rawAnimSize) {
                continue;
            }
            if (keyValsByteOff + numKeys * 2 > rawAnimSize) {
                continue;
            }

            const u8* times = rawAnimData + keyTimesByteOff;
            s32 bracket = 0;
            if (numKeys > 1) {
                for (u32 k = 0; k < numKeys - 1; k++) {
                    if (0 >= (s32)times[k] && 0 < (s32)times[k + 1]) {
                        bracket = (s32)k;
                        break;
                    }
                    if (0 >= (s32)times[k + 1]) {
                        bracket = (s32)(k + 1);
                    }
                }
            }

            s16 val = ReadS16(rawAnimData + keyValsByteOff + bracket * 2);
            if (dofIndex == 0) {
                joint.rotationX = val;
            } else if (dofIndex == 1) {
                joint.rotationY = val;
            } else {
                joint.rotationZ = val;
            }
        } else {
            LOG("[Anim] Rot channel %d: unknown type %u", i, keyType);
        }
    }

    // Process translation channels
    for (s32 i = 0; i < numTransCh; i++) {
        u32 chDwordOff = ReadU32(rawAnimData + transArrayByteOff + i * 4);
        u32 chByteOff = chDwordOff * 4;
        if (chByteOff + 20 > rawAnimSize) {
            continue;
        }

        const u8* ch = rawAnimData + chByteOff;
        u32 jointParam = ReadU32(ch + 0);
        u32 keyType = ReadU32(ch + 4);

        if (jointParam >= skeleton->numMapEntries) {
            continue;
        }
        u32 jointIdx = skeleton->jointOrderMap[jointParam];
        if (jointIdx >= skeleton->numJoints) {
            continue;
        }
        STreeJoint& joint = skeleton->joints[jointIdx];

        if (keyType == 12) {
            // tStatic3DOFKeyList: values at +8, +12, +16
            joint.translationX = (s32)ReadU32(ch + 8);
            joint.translationY = (s32)ReadU32(ch + 12);
            joint.translationZ = (s32)ReadU32(ch + 16);
        } else if (keyType == 8) {
            // tJoint3DOFlpPSX: read first keyframe (3 x s16)
            u32 numKeys = ReadU32(ch + 8);
            if (numKeys == 0) {
                continue;
            }
            u32 valDwordOff = ReadU32(ch + 16);
            u32 valByteOff = valDwordOff * 4;
            if (valByteOff + 6 > rawAnimSize) {
                continue;
            }
            joint.translationX = ReadS16(rawAnimData + valByteOff + 0);
            joint.translationY = ReadS16(rawAnimData + valByteOff + 2);
            joint.translationZ = ReadS16(rawAnimData + valByteOff + 4);
        } else {
            LOG("[Anim] Trans channel %d: unknown type %u", i, keyType);
        }
    }

    // Save a stable bind baseline used by runtime animation evaluation.
    for (u32 i = 0; i < skeleton->numJoints; i++) {
        STreeJoint& j = skeleton->joints[i];
        j.bindTranslationX = j.translationX;
        j.bindTranslationY = j.translationY;
        j.bindTranslationZ = j.translationZ;
        j.bindRotationX = j.rotationX;
        j.bindRotationY = j.rotationY;
        j.bindRotationZ = j.rotationZ;
    }

}

void BuildPerJointMeshes(OriginalSTree* original, const u8* primGeomData, u32 primGeomSize) {
    STreeData* skeleton = original->skeleton;
    if (!skeleton || !primGeomData || primGeomSize < 108) {
        return;
    }

    // Parse tPrimGeom header
    u32 vertListOff = ReadU32(primGeomData + 0x10) << 2;
    u16 numVerts    = ReadU16(primGeomData + 0x14);
    u16 numPolys    = ReadU16(primGeomData + 0x16);
    u32 primListOff = ReadU32(primGeomData + 0x40) << 2;
    u32 polyDataOff = ReadU32(primGeomData + 0x54) << 2;
    u32 loopCtOff   = ReadU32(primGeomData + 0x60) << 2;
    s16 numLoops    = ReadS16(primGeomData + 0x66);

    if (numVerts == 0 || numPolys == 0) {
        return;
    }
    if (numLoops < 1) numLoops = 1;

    if (vertListOff + numVerts * 8 > primGeomSize) {
        return;
    }
    if (polyDataOff + numPolys * 4 > primGeomSize) {
        return;
    }

    const u8* verts = primGeomData + vertListOff;
    const u8* polys = primGeomData + polyDataOff;

    // Build per-loop vertex bases
    struct LoopInfo { u16 vertCount; u16 polyCount; u16 vertBase; u16 polyBase; };
    std::vector<LoopInfo> loops;
    u16 vertAccum = 0;
    u16 polyAccum = 0;
    if (loopCtOff + numLoops * 4 <= primGeomSize) {
        for (int i = 0; i < numLoops; i++) {
            u32 val = ReadU32(primGeomData + loopCtOff + i * 4);
            LoopInfo li;
            li.vertCount = (u16)(val & 0xFFFF);
            li.polyCount = (u16)((val >> 16) & 0xFFFF);
            li.vertBase = vertAccum;
            li.polyBase = polyAccum;
            vertAccum += li.vertCount;
            polyAccum += li.polyCount;
            loops.push_back(li);
        }
    }
    if (loops.empty()) {
        loops.push_back({numVerts, numPolys, 0, 0});
    }

    // Build vertex-to-joint map: primGeomStartIdx/primGeomCount are vertex ranges
    std::vector<u32> vertJointMap(numVerts, 0);
    for (u32 j = 0; j < skeleton->numJoints; j++) {
        const STreeJoint& jt = skeleton->joints[j];
        if (!(jt.flags & STF_HAS_MESH)) continue;
        for (u32 v = 0; v < jt.primGeomCount; v++) {
            u32 vi = jt.primGeomStartIdx + v;
            if (vi < numVerts) {
                vertJointMap[vi] = j;
            }
        }
    }

    // Collect SkinVertex data (joint-local space) + indices
    std::vector<SkinVertex> skinVerts;
    std::vector<u16> allIndices;

    u32 primCursor = primListOff;
    u32 polyIdx = 0;

    for (int loop = 0; loop < (int)loops.size(); loop++) {
        u16 vertBase = loops[loop].vertBase;
        u16 loopPolyCount = loops[loop].polyCount;

        for (u16 lp = 0; lp < loopPolyCount && polyIdx < numPolys; lp++, polyIdx++) {
            if (primCursor + 8 > primGeomSize) break;
            u32 otTag = ReadU32(primGeomData + primCursor);
            u8 wordCount = (u8)((otTag >> 24) & 0xFF);
            u32 pktSize = (wordCount + 1) * 4;
            if (primCursor + pktSize > primGeomSize) break;

            const u8* pkt = primGeomData + primCursor;
            u8 cmd = pkt[7];
            u8 cmdBase = cmd & 0xFD;

            const u8* poly = polys + polyIdx * 4;
            u8 vi0 = poly[0], vi1 = poly[1], vi2 = poly[2], vi3 = poly[3];

            auto makeSkinVert = [&](u8 vi) -> SkinVertex {
                u32 idx = vertBase + vi;
                if (idx >= numVerts) idx = 0;
                const u8* vp = verts + idx * 8;
                SkinVertex sv;
                sv.lx = (f32)ReadS16(vp + 0);
                sv.ly = (f32)ReadS16(vp + 2);
                sv.lz = (f32)ReadS16(vp + 4);
                sv.r = 0.7f; sv.g = 0.7f; sv.b = 0.7f;
                sv.u = 0.0f; sv.v = 0.0f;
                sv.tpage = -1.0f; sv.cba = 0.0f;
                sv.jointIdx = vertJointMap[idx];
                return sv;
            };

            auto readRGB = [&](int off) {
                if (off + 3 > (int)pktSize) return std::make_tuple(0.5f, 0.5f, 0.5f);
                f32 r = std::min(1.0f, pkt[off]     / 128.0f);
                f32 g = std::min(1.0f, pkt[off + 1] / 128.0f);
                f32 b = std::min(1.0f, pkt[off + 2] / 128.0f);
                return std::make_tuple(r, g, b);
            };

            if (cmdBase == 0x3C || cmdBase == 0x2C) {
                if (pktSize < 52) { primCursor += pktSize; continue; }
                SkinVertex v0 = makeSkinVert(vi0), v1 = makeSkinVert(vi1);
                SkinVertex v2 = makeSkinVert(vi2), v3 = makeSkinVert(vi3);
                f32 tp = (f32)ReadU16(pkt + 26);
                f32 cb = (f32)ReadU16(pkt + 14);
                auto [r0,g0,b0] = readRGB(4);
                auto [r1,g1,b1] = readRGB(16);
                auto [r2,g2,b2] = readRGB(28);
                auto [r3,g3,b3] = readRGB(40);
                v0.r=r0; v0.g=g0; v0.b=b0; v1.r=r1; v1.g=g1; v1.b=b1;
                v2.r=r2; v2.g=g2; v2.b=b2; v3.r=r3; v3.g=g3; v3.b=b3;
                v0.u=pkt[12]; v0.v=pkt[13]; v0.tpage=tp; v0.cba=cb;
                v1.u=pkt[24]; v1.v=pkt[25]; v1.tpage=tp; v1.cba=cb;
                v2.u=pkt[36]; v2.v=pkt[37]; v2.tpage=tp; v2.cba=cb;
                v3.u=pkt[48]; v3.v=pkt[49]; v3.tpage=tp; v3.cba=cb;
                u16 base = (u16)skinVerts.size();
                skinVerts.push_back(v0); skinVerts.push_back(v1);
                skinVerts.push_back(v2); skinVerts.push_back(v3);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);
                allIndices.push_back(base+1); allIndices.push_back(base+3); allIndices.push_back(base+2);

            } else if (cmdBase == 0x34 || cmdBase == 0x24) {
                if (pktSize < 40) { primCursor += pktSize; continue; }
                SkinVertex v0 = makeSkinVert(vi0), v1 = makeSkinVert(vi1), v2 = makeSkinVert(vi2);
                f32 tp = (f32)ReadU16(pkt + 26);
                f32 cb = (f32)ReadU16(pkt + 14);
                auto [r0,g0,b0] = readRGB(4);
                auto [r1,g1,b1] = readRGB(16);
                auto [r2,g2,b2] = readRGB(28);
                v0.r=r0; v0.g=g0; v0.b=b0;
                v1.r=r1; v1.g=g1; v1.b=b1;
                v2.r=r2; v2.g=g2; v2.b=b2;
                v0.u=pkt[12]; v0.v=pkt[13]; v0.tpage=tp; v0.cba=cb;
                v1.u=pkt[24]; v1.v=pkt[25]; v1.tpage=tp; v1.cba=cb;
                v2.u=pkt[36]; v2.v=pkt[37]; v2.tpage=tp; v2.cba=cb;
                u16 base = (u16)skinVerts.size();
                skinVerts.push_back(v0); skinVerts.push_back(v1); skinVerts.push_back(v2);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);

            } else if (cmdBase == 0x38 || cmdBase == 0x28) {
                SkinVertex v0 = makeSkinVert(vi0), v1 = makeSkinVert(vi1);
                SkinVertex v2 = makeSkinVert(vi2), v3 = makeSkinVert(vi3);
                if (cmdBase == 0x38) {
                    auto [r0,g0,b0] = readRGB(4);
                    auto [r1,g1,b1] = readRGB(12);
                    auto [r2,g2,b2] = readRGB(20);
                    auto [r3,g3,b3] = readRGB(28);
                    v0.r=r0; v0.g=g0; v0.b=b0; v1.r=r1; v1.g=g1; v1.b=b1;
                    v2.r=r2; v2.g=g2; v2.b=b2; v3.r=r3; v3.g=g3; v3.b=b3;
                } else {
                    auto [r0,g0,b0] = readRGB(4);
                    v0.r=r0; v0.g=g0; v0.b=b0; v1.r=r0; v1.g=g0; v1.b=b0;
                    v2.r=r0; v2.g=g0; v2.b=b0; v3.r=r0; v3.g=g0; v3.b=b0;
                }
                u16 base = (u16)skinVerts.size();
                skinVerts.push_back(v0); skinVerts.push_back(v1);
                skinVerts.push_back(v2); skinVerts.push_back(v3);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);
                allIndices.push_back(base+1); allIndices.push_back(base+3); allIndices.push_back(base+2);

            } else if (cmdBase == 0x30 || cmdBase == 0x20) {
                SkinVertex v0 = makeSkinVert(vi0), v1 = makeSkinVert(vi1), v2 = makeSkinVert(vi2);
                if (cmdBase == 0x30) {
                    auto [r0,g0,b0] = readRGB(4);
                    auto [r1,g1,b1] = readRGB(12);
                    auto [r2,g2,b2] = readRGB(20);
                    v0.r=r0; v0.g=g0; v0.b=b0;
                    v1.r=r1; v1.g=g1; v1.b=b1;
                    v2.r=r2; v2.g=g2; v2.b=b2;
                } else {
                    auto [r0,g0,b0] = readRGB(4);
                    v0.r=r0; v0.g=g0; v0.b=b0;
                    v1.r=r0; v1.g=g0; v1.b=b0;
                    v2.r=r0; v2.g=g0; v2.b=b0;
                }
                u16 base = (u16)skinVerts.size();
                skinVerts.push_back(v0); skinVerts.push_back(v1); skinVerts.push_back(v2);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);
            }

            primCursor += pktSize;
        }
    }

    if (allIndices.empty()) {
        return;
    }

    // Store SkinData for per-frame CPU skinning
    SkinData* sd = new SkinData();
    sd->numVerts = (u32)skinVerts.size();
    sd->numIndices = (u32)allIndices.size();
    sd->verts = new SkinVertex[sd->numVerts];
    sd->indices = new u16[sd->numIndices];
    memcpy(sd->verts, skinVerts.data(), sd->numVerts * sizeof(SkinVertex));
    memcpy(sd->indices, allIndices.data(), sd->numIndices * sizeof(u16));
    original->skinData = sd;

    // Build initial mesh from current joint transforms
    Mat4* jointMatrices = new Mat4[skeleton->numJoints];
    skeleton->ComputeWorldMatrices(jointMatrices);

    u32 format = PDDI_V_POSITION | PDDI_V_COLOUR | PDDI_V_UV | PDDI_V_TEXINFO;
    pddiPrimBufferDesc desc(PDDI_PRIM_TRIANGLES, format,
                            sd->numVerts, sd->numIndices);

    pddiPrimBuffer* buffer = p3d::device->NewPrimBuffer(desc);

    std::vector<f32> vertData(sd->numVerts * 10);
    for (u32 i = 0; i < sd->numVerts; i++) {
        const SkinVertex& sv = sd->verts[i];
        const Mat4& m = jointMatrices[sv.jointIdx];
        f32 wx, wy, wz;
        Mat4TransformPoint(m, sv.lx, sv.ly, sv.lz, wx, wy, wz);
        vertData[i * 10 + 0] = wx;
        vertData[i * 10 + 1] = wy;
        vertData[i * 10 + 2] = wz;
        vertData[i * 10 + 3] = sv.r;
        vertData[i * 10 + 4] = sv.g;
        vertData[i * 10 + 5] = sv.b;
        vertData[i * 10 + 6] = sv.u;
        vertData[i * 10 + 7] = sv.v;
        vertData[i * 10 + 8] = sv.tpage;
        vertData[i * 10 + 9] = sv.cba;
    }

    buffer->SetVertexData(vertData.data(), sd->numVerts);
    buffer->SetIndices(sd->indices, sd->numIndices);
    skeleton->joints[0].meshBuffer = buffer;

    delete[] jointMatrices;

    LOG("[Skeleton] Built skinned mesh: %u verts, %u indices across %u joints",
        sd->numVerts, sd->numIndices, skeleton->numJoints);
}
