// geometry.h — BLK level geometry parser
// Reads PSX tPrimGeom data from BLK stream entries and produces
// OpenGL vertex/index buffers with vertex positions + colors.
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "core.h"
#include <glad/gl.h>
#include <vector>

struct BlockMesh {
    u32 vao = 0;
    u32 vbo = 0;
    u32 ebo = 0;
    u32 indexCount = 0;
    u16 lod = 0; // LOD level from BLK header (higher = more detail)
    s32 tx = 0, ty = 0, tz = 0; // world translation from WDB

    void Destroy() {
        if (ebo) glDeleteBuffers(1, &ebo);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        vao = vbo = ebo = 0;
        indexCount = 0;
    }
};

// Parse a single BLK entry from a stream file into a renderable mesh.
// data points to the start of the BLK entry, size is its byte length.
// Returns a BlockMesh with GPU buffers ready for DrawPrimBuffer.
BlockMesh ParseBLK(const u8* data, u32 size);

#endif // GEOMETRY_H
