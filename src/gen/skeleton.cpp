// skeleton.cpp - game-side skeleton loading utilities
// ParseP3DStreamFull: extracts textures + skeleton from P3D stream
// BuildPerJointMeshes: builds pddiPrimBuffer from tPrimGeom + skeleton
#include "common.h"
#include "gen/skeleton.h"
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
            // Packed u32: bits[0:4]*32 = rotZ, bits[5:10]*32 = rotY, bits[11:15]*64 = rotX
            u32 packed = ReadU32(rawAnimData + valByteOff);
            joint.rotationX = (s16)((packed >> 11) * 64);
            joint.rotationY = (s16)(((packed >> 5) & 0x3F) * 32);
            joint.rotationZ = (s16)((packed & 0x1F) * 32);
        } else if (keyType == 3) {
            // tJoint1DOFangle: single s16 value, DOF is the channel's axis
            // For frame-0, just skip - 1DOF channels are rare in idle anims
            LOG("[Anim] Rot channel %d: unhandled type 3 (1DOFangle)", i);
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

    LOG("[Anim] Applied frame-0: %d rot channels, %d trans channels",
        numRotCh, numTransCh);

    // Debug: dump joint values after animation
    for (u32 i = 0; i < skeleton->numJoints && i < 10; i++) {
        const STreeJoint& j = skeleton->joints[i];
        LOG("[Anim] Joint %u: flags=0x%02X trans=(%d,%d,%d) rot=(%d,%d,%d)",
            i, j.flags, j.translationX, j.translationY, j.translationZ,
            (int)j.rotationX, (int)j.rotationY, (int)j.rotationZ);
    }
}

void BuildPerJointMeshes(STreeData* skeleton, const u8* primGeomData, u32 primGeomSize) {
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

    // Compute per-joint world matrices from current rotation/translation
    Mat4* jointMatrices = new Mat4[skeleton->numJoints];
    skeleton->ComputeWorldMatrices(jointMatrices);

    // Debug: dump first few joint matrices
    for (u32 j = 0; j < skeleton->numJoints && j < 6; j++) {
        const Mat4& m = jointMatrices[j];
        LOG("[Skel] Joint %u matrix: [%.1f %.1f %.1f %.1f] [%.1f %.1f %.1f %.1f] [%.1f %.1f %.1f %.1f] t=(%.1f,%.1f,%.1f)",
            j, m.m[0], m.m[4], m.m[8], m.m[12],
            m.m[1], m.m[5], m.m[9], m.m[13],
            m.m[2], m.m[6], m.m[10], m.m[14],
            m.m[12], m.m[13], m.m[14]);
    }

    // Build vertex-to-joint map: for each vertex, which joint owns it
    std::vector<u32> vertJointMap(numVerts, 0);
    for (u32 j = 0; j < skeleton->numJoints; j++) {
        const STreeJoint& jt = skeleton->joints[j];
        for (u32 v = 0; v < jt.primGeomCount; v++) {
            u32 vi = jt.primGeomStartIdx + v;
            if (vi < numVerts) {
                vertJointMap[vi] = j;
            }
        }
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

    // Collect triangles with per-joint transforms applied.
    // Each vertex is transformed from joint-local to model space using its owning joint's matrix.
    struct TriVert { f32 x, y, z, r, g, b, u, v, tpage, cba; };
    std::vector<TriVert> allVerts;
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

            auto makeVert = [&](u8 vi) -> TriVert {
                u32 idx = vertBase + vi;
                if (idx >= numVerts) idx = 0;
                const u8* vp = verts + idx * 8;
                f32 lx = (f32)ReadS16(vp + 0);
                f32 ly = (f32)ReadS16(vp + 2);
                f32 lz = (f32)ReadS16(vp + 4);

                // Transform from joint-local to model space
                const Mat4& m = jointMatrices[vertJointMap[idx]];
                TriVert tv;
                Mat4TransformPoint(m, lx, ly, lz, tv.x, tv.y, tv.z);
                tv.r = 0.7f; tv.g = 0.7f; tv.b = 0.7f;
                tv.u = 0.0f; tv.v = 0.0f;
                tv.tpage = -1.0f; tv.cba = 0.0f;
                return tv;
            };

            auto readRGB = [&](int off) {
                if (off + 3 > (int)pktSize) return std::make_tuple(0.5f, 0.5f, 0.5f);
                f32 r = std::min(1.0f, pkt[off]     / 128.0f);
                f32 g = std::min(1.0f, pkt[off + 1] / 128.0f);
                f32 b = std::min(1.0f, pkt[off + 2] / 128.0f);
                return std::make_tuple(r, g, b);
            };

            if (cmdBase == 0x3C || cmdBase == 0x2C) {
                // POLY_GT4 / POLY_FT4: textured quad
                if (pktSize < 52) { primCursor += pktSize; continue; }

                TriVert v0 = makeVert(vi0), v1 = makeVert(vi1);
                TriVert v2 = makeVert(vi2), v3 = makeVert(vi3);

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

                u16 base = (u16)allVerts.size();
                allVerts.push_back(v0); allVerts.push_back(v1);
                allVerts.push_back(v2); allVerts.push_back(v3);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);
                allIndices.push_back(base+1); allIndices.push_back(base+3); allIndices.push_back(base+2);

            } else if (cmdBase == 0x34 || cmdBase == 0x24) {
                // POLY_GT3 / POLY_FT3: textured tri
                if (pktSize < 40) { primCursor += pktSize; continue; }

                TriVert v0 = makeVert(vi0), v1 = makeVert(vi1), v2 = makeVert(vi2);

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

                u16 base = (u16)allVerts.size();
                allVerts.push_back(v0); allVerts.push_back(v1); allVerts.push_back(v2);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);

            } else if (cmdBase == 0x38 || cmdBase == 0x28) {
                // POLY_G4 / POLY_F4: untextured quad
                TriVert v0 = makeVert(vi0), v1 = makeVert(vi1);
                TriVert v2 = makeVert(vi2), v3 = makeVert(vi3);

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

                u16 base = (u16)allVerts.size();
                allVerts.push_back(v0); allVerts.push_back(v1);
                allVerts.push_back(v2); allVerts.push_back(v3);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);
                allIndices.push_back(base+1); allIndices.push_back(base+3); allIndices.push_back(base+2);

            } else if (cmdBase == 0x30 || cmdBase == 0x20) {
                // POLY_G3 / POLY_F3: untextured tri
                TriVert v0 = makeVert(vi0), v1 = makeVert(vi1), v2 = makeVert(vi2);

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

                u16 base = (u16)allVerts.size();
                allVerts.push_back(v0); allVerts.push_back(v1); allVerts.push_back(v2);
                allIndices.push_back(base); allIndices.push_back(base+1); allIndices.push_back(base+2);
            }

            primCursor += pktSize;
        }
    }

    if (allIndices.empty()) {
        delete[] jointMatrices;
        return;
    }

    // Create combined pddiPrimBuffer
    u32 format = PDDI_V_POSITION | PDDI_V_COLOUR | PDDI_V_UV | PDDI_V_TEXINFO;
    pddiPrimBufferDesc desc(PDDI_PRIM_TRIANGLES, format,
                            (u32)allVerts.size(), (u32)allIndices.size());

    pddiPrimBuffer* buffer = p3d::device->NewPrimBuffer(desc);

    std::vector<f32> vertData(allVerts.size() * 10);
    for (size_t i = 0; i < allVerts.size(); i++) {
        vertData[i * 10 + 0] = allVerts[i].x;
        vertData[i * 10 + 1] = allVerts[i].y;
        vertData[i * 10 + 2] = allVerts[i].z;
        vertData[i * 10 + 3] = allVerts[i].r;
        vertData[i * 10 + 4] = allVerts[i].g;
        vertData[i * 10 + 5] = allVerts[i].b;
        vertData[i * 10 + 6] = allVerts[i].u;
        vertData[i * 10 + 7] = allVerts[i].v;
        vertData[i * 10 + 8] = allVerts[i].tpage;
        vertData[i * 10 + 9] = allVerts[i].cba;
    }

    buffer->SetVertexData(vertData.data(), (u32)allVerts.size());
    buffer->SetIndices(allIndices.data(), (u32)allIndices.size());

    skeleton->joints[0].meshBuffer = buffer;

    delete[] jointMatrices;

    LOG("[Skeleton] Built combined mesh: %u verts, %u indices across %u joints",
        (u32)allVerts.size(), (u32)allIndices.size(), skeleton->numJoints);
}
