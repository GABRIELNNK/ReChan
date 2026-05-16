#include "pc/coldebug.h"
#include "gen/blockmgr.h"
#include "gen/colsect.h"
#include "gen/colwall.h"
#include "gen/colfloor.h"
#include "gen/colline.h"
#include "gen/game.h"
#include "gen/world.h"
#include "p3d/context.h"
#include "p3d/p3dmath.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include <vector>

namespace CollisionDebug {
    bool enabled = false;

    // Vertex: position(3) + color(3)
    struct DebugVertex {
        f32 x, y, z;
        f32 r, g, b;
    };

    static void PushLine(std::vector<DebugVertex>& verts, std::vector<u16>& indices,
                         f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1,
                         f32 r, f32 g, f32 b) {
        u16 base = static_cast<u16>(verts.size());
        verts.push_back({ x0, y0, z0, r, g, b });
        verts.push_back({ x1, y1, z1, r, g, b });
        indices.push_back(base);
        indices.push_back(base + 1);
    }

    static void BuildWallLines(const Wall* wall, std::vector<DebugVertex>& verts, std::vector<u16>& indices) {
        // Reconstruct the 4 corners of the wall quad
        // Wall is a plane (normalX*x + normalZ*z + distance = 0)
        // bounded by endpoints along its primary axis with top/bottom height lines
        f32 coords[2]; // the two endpoint coordinates along the wall's axis
        f32 topY[2], botY[2]; // heights at each endpoint
        f32 posX[2], posZ[2]; // world X,Z at each endpoint

        bool xAligned = (wall->flags & 0xFFFF0000) == 0xFFFF0000;
        if (xAligned) {
            coords[0] = (f32)wall->xBound1;
            coords[1] = (f32)wall->xBound2;
        }
        else {
            coords[0] = (f32)wall->zBound1;
            coords[1] = (f32)wall->zBound2;
        }

        for (int i = 0; i < 2; i++) {
            topY[i] = (f32)(fixmul16(wall->topSlope, (s32)coords[i]) + wall->topIntercept);
            botY[i] = (f32)(fixmul16(wall->bottomSlope, (s32)coords[i]) + wall->bottomIntercept);

            if (xAligned) {
                posX[i] = coords[i];
                // Solve for Z on the wall plane: normalX*x + normalZ*z + distance = 0
                // z = -(normalX*x + distance) / normalZ (all in fixed-point)
                if (wall->normalZ != 0) {
                    s32 num = fixmul16(wall->normalX, (s32)coords[i]) + wall->distance;
                    posZ[i] = (f32)(-rmDiv16i(num, wall->normalZ));
                }
                else {
                    posZ[i] = 0.0f;
                }
            }
            else {
                posZ[i] = coords[i];
                if (wall->normalX != 0) {
                    s32 num = fixmul16(wall->normalZ, (s32)coords[i]) + wall->distance;
                    posX[i] = (f32)(-rmDiv16i(num, wall->normalX));
                }
                else {
                    posX[i] = 0.0f;
                }
            }
        }

        // Draw 4 edges of the quad: top, bottom, left, right
        // Green = walls
        f32 r = 0.0f, g = 1.0f, b = 0.0f;
        // Top edge
        PushLine(verts, indices, posX[0], topY[0], posZ[0], posX[1], topY[1], posZ[1], r, g, b);
        // Bottom edge
        PushLine(verts, indices, posX[0], botY[0], posZ[0], posX[1], botY[1], posZ[1], r, g, b);
        // Left edge
        PushLine(verts, indices, posX[0], topY[0], posZ[0], posX[0], botY[0], posZ[0], r, g, b);
        // Right edge
        PushLine(verts, indices, posX[1], topY[1], posZ[1], posX[1], botY[1], posZ[1], r, g, b);
    }

    static void BuildFloorLines(const Floor* floor, std::vector<DebugVertex>& verts, std::vector<u16>& indices) {
        // Get vertex count (3 for triangle, 4 for quad)
        s32 numBounds = Equal(floor->bound[2], floor->bound[3]) ? 3 : 4;

        // Compute vertices by intersecting adjacent boundary lines
        LVector corners[4];
        for (s32 i = 0; i < numBounds; i++) {
            s32 next = (i + 1) % numBounds;
            const Line& lineA = floor->bound[i];
            const Line& lineB = floor->bound[next];

            // Solve: a1*x + b1*z + c1 = 0, a2*x + b2*z + c2 = 0
            // All values are 16.16 fixed-point
            s64 det = (s64)lineA.a * lineB.b - (s64)lineB.a * lineA.b;
            if (det == 0) {
                corners[i] = { 0, 0, 0 };
                continue;
            }
            // x = (b1*c2 - b2*c1) / det, z = (a2*c1 - a1*c2) / det
            // Products are 32.32, det is 32.32, so quotient is plain integer (in 16.16 space)
            corners[i].x = (s32)(((s64)lineA.b * lineB.c - (s64)lineB.b * lineA.c) / det);
            corners[i].z = (s32)(((s64)lineB.a * lineA.c - (s64)lineA.a * lineB.c) / det);
            // Get Y from floor height equation
            corners[i].y = fixmul16(floor->normalX, corners[i].x) + fixmul16(floor->normalZ, corners[i].z) + floor->heightC;
        }

        // Cyan = floors, Magenta = ceilings
        f32 r, g, b;
        if (floor->flags & 0x0001) {
            r = 1.0f; g = 0.0f; b = 1.0f; // ceiling
        }
        else {
            r = 0.0f; g = 1.0f; b = 1.0f; // floor
        }

        // Draw edges
        for (s32 i = 0; i < numBounds; i++) {
            s32 next = (i + 1) % numBounds;
            PushLine(verts, indices,
                     (f32)corners[i].x, (f32)corners[i].y, (f32)corners[i].z,
                     (f32)corners[next].x, (f32)corners[next].y, (f32)corners[next].z,
                     r, g, b);
        }
    }

    void Draw(BlockManager* blockMgr) {
        if (!enabled || !blockMgr) return;

        std::vector<DebugVertex> verts;
        std::vector<u16> indices;
        verts.reserve(4096);
        indices.reserve(4096);

        u32 numBlocks = blockMgr->GetNumBlocks();
        for (u32 i = 0; i < numBlocks; i++) {
            Block* block = blockMgr->GetBlock(i);
            if (!block || !block->collision) continue;

            CollisionSector* sector = block->collision;
            if (sector->status == -1) continue;

            for (u32 w = 0; w < sector->wallCount; w++) {
                if (verts.size() + 8 > 65535) break; // u16 index limit
                BuildWallLines(&sector->walls[w], verts, indices);
            }

            for (u32 f = 0; f < sector->floorCount; f++) {
                if (verts.size() + 8 > 65535) break;
                BuildFloorLines(&sector->floors[f], verts, indices);
            }
        }

        if (indices.empty()) return;

        // Build and draw a temporary line prim buffer
        pddiPrimBufferDesc desc(PDDI_PRIM_LINES,
                                PDDI_V_POSITION | PDDI_V_COLOUR,
                                static_cast<u32>(verts.size()),
                                static_cast<u32>(indices.size()));
        pddiPrimBuffer* buf = p3d::device->NewPrimBuffer(desc);
        buf->SetVertexData(verts.data(), static_cast<u32>(verts.size()));
        buf->SetIndices(indices.data(), static_cast<u32>(indices.size()));

        // Draw at world origin (no per-block offset needed, collision data is in world space)
        const Mat4 savedWorld = p3d::context->GetWorldMatrix();
        Mat4 identity;
        p3d::context->SetWorldMatrix(identity);
        p3d::context->SetVRAMHandle(0);
        p3d::context->DrawPrimBuffer(buf);
        p3d::context->SetWorldMatrix(savedWorld);
        if (g_game && g_game->GetWorld()) {
            p3d::context->SetVRAMHandle(g_game->GetWorld()->GetVRAMHandle());
        }

        buf->Release();
    }

}
