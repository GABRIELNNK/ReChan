// geometry.cpp — BLK level geometry parser implementation
#include "gen/geometry.h"
#include <cstring>

// BLK layout:
//   [0-23]   Block header (translation at +8: s16 x,y,z)
//   [24+]    tPrimGeom struct, also the base for word offsets
//
// tPrimGeom fields (all word offsets relative to pg = BLK+24):
//   +0x10  vertList    (u32 word offset -> SVECTOR array)
//   +0x14  numVerts    (u16)
//   +0x16  numPolys    (u16)
//   +0x40  primList    (u32 word offset -> GPU packets)
//   +0x54  polyData    (u32 word offset -> u8[4] vertex indices per face)
//   +0x60  loopCounts  (u32 word offset -> per-loop u32: lo16=vertCount hi16=polyCount)
//   +0x66  numLoops    (s16)
//
// Standard PSX GPU primitive packet layouts (byte offsets from packet start):
//
// POLY_GT4 (cmd 0x3C, wordCount=12, 52 bytes):
//   +0  OT tag          +4  [R0,G0,B0,CMD]     +8  [X0,Y0]
//   +12 [U0,V0,CBA]     +16 [R1,G1,B1,pad]     +20 [X1,Y1]
//   +24 [U1,V1,TPAGE]   +28 [R2,G2,B2,pad]     +32 [X2,Y2]
//   +36 [U2,V2,pad]     +40 [R3,G3,B3,pad]     +44 [X3,Y3]
//   +48 [U3,V3,pad]
//
// POLY_GT3 (cmd 0x34, wordCount=9, 40 bytes):
//   +0  OT tag          +4  [R0,G0,B0,CMD]     +8  [X0,Y0]
//   +12 [U0,V0,CBA]     +16 [R1,G1,B1,pad]     +20 [X1,Y1]
//   +24 [U1,V1,TPAGE]   +28 [R2,G2,B2,pad]     +32 [X2,Y2]
//   +36 [U2,V2,pad]
//
// POLY_G4 (cmd 0x38, wordCount=8, 36 bytes):
//   +0  OT tag     +4  [R0,G0,B0,CMD]  +8  [X0,Y0]
//   +12 [R1,G1,B1] +16 [X1,Y1]         +20 [R2,G2,B2]
//   +24 [X2,Y2]    +28 [R3,G3,B3]      +32 [X3,Y3]
//
// POLY_G3 (cmd 0x30, wordCount=6, 28 bytes):
//   +0  OT tag     +4  [R0,G0,B0,CMD]  +8  [X0,Y0]
//   +12 [R1,G1,B1] +16 [X1,Y1]         +20 [R2,G2,B2]
//   +24 [X2,Y2]

static u32 ReadU32LE(const u8* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static u16 ReadU16LE(const u8* p) {
    return static_cast<u16>(p[0] | (p[1] << 8));
}

static s16 ReadS16LE(const u8* p) {
    return static_cast<s16>(p[0] | (p[1] << 8));
}

BlockMesh ParseBLK(const u8* data, u32 size) {
    BlockMesh mesh;

    if (size < 24 + 108) return mesh;

    // Block header: byte 0 = LOD level (u16)
    mesh.lod = ReadU16LE(data + 0);

    const u8* pg = data + 24; // tPrimGeom base
    u32 pgSize = size - 24;

    u32 vertListOff = ReadU32LE(pg + 0x10) << 2;
    u16 numVerts    = ReadU16LE(pg + 0x14);
    u16 numPolys    = ReadU16LE(pg + 0x16);
    u32 primListOff = ReadU32LE(pg + 0x40) << 2;
    u32 polyDataOff = ReadU32LE(pg + 0x54) << 2;
    u32 loopCtOff   = ReadU32LE(pg + 0x60) << 2;
    s16 numLoops    = ReadS16LE(pg + 0x66);

    if (numVerts == 0 || numPolys == 0) return mesh;
    if (numLoops < 1) numLoops = 1;

    // Validate offsets
    if (vertListOff + numVerts * 8 > pgSize) return mesh;
    if (polyDataOff + numPolys * 4 > pgSize) return mesh;

    const u8* verts = pg + vertListOff;
    const u8* polys = pg + polyDataOff;

    // Read per-loop vertex and poly counts from loopCounts
    // Each entry: u32 with lo16 = vertex count, hi16 = poly count
    struct LoopInfo { u16 vertCount; u16 polyCount; u16 vertBase; };
    std::vector<LoopInfo> loops;
    u16 vertAccum = 0;
    if (loopCtOff + numLoops * 4 <= pgSize) {
        for (int i = 0; i < numLoops; i++) {
            u32 val = ReadU32LE(pg + loopCtOff + i * 4);
            LoopInfo li;
            li.vertCount = static_cast<u16>(val & 0xFFFF);
            li.polyCount = static_cast<u16>((val >> 16) & 0xFFFF);
            li.vertBase = vertAccum;
            vertAccum += li.vertCount;
            loops.push_back(li);
        }
    }
    // Fallback: single loop with all verts/polys
    if (loops.empty()) {
        loops.push_back({numVerts, numPolys, 0});
    }

    // Vertex: pos3 + color3 + uv2 + tpage + cba = 10 floats (40 bytes)
    // For untextured faces: tpage = -1.0
    struct Vert { f32 x, y, z, r, g, b, u, v, tpage, cba; };
    std::vector<Vert> vertBuf;
    std::vector<u16> idxBuf;

    // Walk primList by OT tag sizes, processing per-loop
    u32 primCursor = primListOff;
    u32 polyIdx = 0;

    for (int loop = 0; loop < (int)loops.size(); loop++) {
        u16 vertBase = loops[loop].vertBase;
        u16 loopVertCount = loops[loop].vertCount;
        u16 loopPolyCount = loops[loop].polyCount;

        for (u16 lp = 0; lp < loopPolyCount && polyIdx < numPolys; lp++, polyIdx++) {
            if (primCursor + 8 > pgSize) break;
            u32 otTag = ReadU32LE(pg + primCursor);
            u8 wordCount = static_cast<u8>((otTag >> 24) & 0xFF);
            u32 pktSize = (wordCount + 1) * 4;
            if (primCursor + pktSize > pgSize) break;

            const u8* pkt = pg + primCursor;
            u8 cmd = pkt[7];

            const u8* poly = polys + polyIdx * 4;
            u8 vi0 = poly[0], vi1 = poly[1], vi2 = poly[2], vi3 = poly[3];

            auto readVert = [&](u8 vi) -> Vert {
                u32 idx = vertBase + vi;
                if (idx >= numVerts) idx = 0;
                const u8* v = verts + idx * 8;
                Vert vert;
                vert.x = ReadS16LE(v + 0) * 1.0f;
                vert.y = ReadS16LE(v + 2) * 1.0f;
                vert.z = ReadS16LE(v + 4) * 1.0f;
                vert.r = 0.7f; vert.g = 0.7f; vert.b = 0.7f;
                vert.u = 0.0f; vert.v = 0.0f;
                vert.tpage = -1.0f; vert.cba = 0.0f;
                return vert;
            };

            // Read RGB from byte offset, PSX 128-scale
            auto readRGB = [&](int byteOff) {
                if (byteOff + 3 > (int)pktSize) return std::make_tuple(0.5f, 0.5f, 0.5f);
                f32 r = std::min(1.0f, pkt[byteOff]     / 128.0f);
                f32 g = std::min(1.0f, pkt[byteOff + 1] / 128.0f);
                f32 b = std::min(1.0f, pkt[byteOff + 2] / 128.0f);
                return std::make_tuple(r, g, b);
            };

            if (cmd == 0x3C || cmd == 0x2C) {
                // POLY_GT4 / POLY_FT4: textured quad (52 bytes)
                // Colors at +4, +16, +28, +40; UVs at +12, +24, +36, +48
                // CBA at +14, TPAGE at +26
                if (pktSize < 52) { primCursor += pktSize; continue; }

                Vert v0 = readVert(vi0), v1 = readVert(vi1);
                Vert v2 = readVert(vi2), v3 = readVert(vi3);

                f32 tp = static_cast<f32>(ReadU16LE(pkt + 26));
                f32 cb = static_cast<f32>(ReadU16LE(pkt + 14));

                auto [r0,g0,b0] = readRGB(4);
                auto [r1,g1,b1] = readRGB(16);
                auto [r2,g2,b2] = readRGB(28);
                auto [r3,g3,b3] = readRGB(40);
                v0.r=r0; v0.g=g0; v0.b=b0;
                v1.r=r1; v1.g=g1; v1.b=b1;
                v2.r=r2; v2.g=g2; v2.b=b2;
                v3.r=r3; v3.g=g3; v3.b=b3;

                v0.u = pkt[12]; v0.v = pkt[13]; v0.tpage = tp; v0.cba = cb;
                v1.u = pkt[24]; v1.v = pkt[25]; v1.tpage = tp; v1.cba = cb;
                v2.u = pkt[36]; v2.v = pkt[37]; v2.tpage = tp; v2.cba = cb;
                v3.u = pkt[48]; v3.v = pkt[49]; v3.tpage = tp; v3.cba = cb;

                u16 base = static_cast<u16>(vertBuf.size());
                vertBuf.push_back(v0); vertBuf.push_back(v1);
                vertBuf.push_back(v2); vertBuf.push_back(v3);
                idxBuf.push_back(base); idxBuf.push_back(base+1); idxBuf.push_back(base+2);
                idxBuf.push_back(base+1); idxBuf.push_back(base+3); idxBuf.push_back(base+2);

            } else if (cmd == 0x38 || cmd == 0x28) {
                // POLY_G4 / POLY_F4: untextured quad (36 bytes)
                // Colors at +4, +12, +20, +28 (2-word groups: RGB + XY)
                Vert v0 = readVert(vi0), v1 = readVert(vi1);
                Vert v2 = readVert(vi2), v3 = readVert(vi3);

                if (cmd == 0x38) {
                    auto [r0,g0,b0] = readRGB(4);
                    auto [r1,g1,b1] = readRGB(12);
                    auto [r2,g2,b2] = readRGB(20);
                    auto [r3,g3,b3] = readRGB(28);
                    v0.r=r0; v0.g=g0; v0.b=b0;
                    v1.r=r1; v1.g=g1; v1.b=b1;
                    v2.r=r2; v2.g=g2; v2.b=b2;
                    v3.r=r3; v3.g=g3; v3.b=b3;
                } else {
                    auto [r0,g0,b0] = readRGB(4);
                    v0.r=r0; v0.g=g0; v0.b=b0;
                    v1.r=r0; v1.g=g0; v1.b=b0;
                    v2.r=r0; v2.g=g0; v2.b=b0;
                    v3.r=r0; v3.g=g0; v3.b=b0;
                }

                u16 base = static_cast<u16>(vertBuf.size());
                vertBuf.push_back(v0); vertBuf.push_back(v1);
                vertBuf.push_back(v2); vertBuf.push_back(v3);
                idxBuf.push_back(base); idxBuf.push_back(base+1); idxBuf.push_back(base+2);
                idxBuf.push_back(base+1); idxBuf.push_back(base+3); idxBuf.push_back(base+2);

            } else if (cmd == 0x34 || cmd == 0x24) {
                // POLY_GT3 / POLY_FT3: textured tri (40 bytes)
                // Colors at +4, +16, +28; UVs at +12, +24, +36
                // CBA at +14, TPAGE at +26
                if (pktSize < 40) { primCursor += pktSize; continue; }

                Vert v0 = readVert(vi0), v1 = readVert(vi1), v2 = readVert(vi2);

                f32 tp = static_cast<f32>(ReadU16LE(pkt + 26));
                f32 cb = static_cast<f32>(ReadU16LE(pkt + 14));

                auto [r0,g0,b0] = readRGB(4);
                auto [r1,g1,b1] = readRGB(16);
                auto [r2,g2,b2] = readRGB(28);
                v0.r=r0; v0.g=g0; v0.b=b0;
                v1.r=r1; v1.g=g1; v1.b=b1;
                v2.r=r2; v2.g=g2; v2.b=b2;

                v0.u = pkt[12]; v0.v = pkt[13]; v0.tpage = tp; v0.cba = cb;
                v1.u = pkt[24]; v1.v = pkt[25]; v1.tpage = tp; v1.cba = cb;
                v2.u = pkt[36]; v2.v = pkt[37]; v2.tpage = tp; v2.cba = cb;

                u16 base = static_cast<u16>(vertBuf.size());
                vertBuf.push_back(v0); vertBuf.push_back(v1); vertBuf.push_back(v2);
                idxBuf.push_back(base); idxBuf.push_back(base+1); idxBuf.push_back(base+2);

            } else if (cmd == 0x30 || cmd == 0x20) {
                // POLY_G3 / POLY_F3: untextured tri (28 bytes)
                // G3 colors at +4, +12, +20; F3 flat from +4
                Vert v0 = readVert(vi0), v1 = readVert(vi1), v2 = readVert(vi2);

                if (cmd == 0x30) {
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

                u16 base = static_cast<u16>(vertBuf.size());
                vertBuf.push_back(v0); vertBuf.push_back(v1); vertBuf.push_back(v2);
                idxBuf.push_back(base); idxBuf.push_back(base+1); idxBuf.push_back(base+2);
            }

            primCursor += pktSize;
        }
    }

    if (idxBuf.empty()) return mesh;

    mesh.indexCount = static_cast<u32>(idxBuf.size());

    // Upload to GPU: 10 floats per vertex (40 bytes stride)
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertBuf.size() * sizeof(Vert)),
                 vertBuf.data(), GL_STATIC_DRAW);

    // location 0: vec3 position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)0);
    glEnableVertexAttribArray(0);
    // location 1: vec3 color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)(3 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    // location 2: vec2 uv (raw 0-255 pixel coords)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)(6 * sizeof(f32)));
    glEnableVertexAttribArray(2);
    // location 3: vec2 texinfo (x=tpage, y=cba; -1 = untextured)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), (void*)(8 * sizeof(f32)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(idxBuf.size() * sizeof(u16)),
                 idxBuf.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return mesh;
}
