#pragma once
#include "core.h"
#include "p3d/lvector.h"
#include "gen/cclist.h"
#include "gen/database.h"

// NodeAttribs - per-point attribute storage extracted from DBPoint
// PSX: 12 bytes (count s32, ids u8*, values s32*)
struct NodeAttribs {
    s32 count = 0;
    u8* ids = nullptr;
    s32* values = nullptr;

    ~NodeAttribs();
    NodeAttribs& operator=(const NodeAttribs& other);
    void Init(const DBPoint* point);
    s32 GetAttrib(s32 id) const;
};

void Swap(NodeAttribs& a, NodeAttribs& b);

// SubDivNode - temporary node used during path subdivision
// PSX: 36 bytes (ccMinNode(12) + xyz(12) + NodeAttribs(12))
struct SubDivNode : public ccMinNode {
    s32 x = 0;
    s32 y = 0;
    s32 z = 0;
    NodeAttribs attribs;

    ~SubDivNode() override;
};

// Path - base class for interpolated paths
// Extends ccNode for linked list participation and naming.
// PSX layout (from offset 24 after ccNode):
//   positions, nodeAttribs, current(LVector), currentSegment, numPoints
struct Path : public ccNode {
    LVector* positions = nullptr;
    NodeAttribs* nodeAttribs = nullptr;
    LVector current = {};
    s32 currentSegment = 0;
    s32 numPoints = 0;

    ~Path() override;

    void Flip();
    void Draw();

    virtual s32 Subdivide(s32 threshold) = 0;
    virtual s32 EndOfPath() = 0;
    virtual s32 Reset() = 0;
};

// LinearPath - linear interpolation between path points
// PSX: 80 bytes. Extends Path with direction, velocity.
struct LinearPath : public Path {
    LVector direction = {};
    s32 field64 = 0;
    LVector velocity = {};

    ~LinearPath() override;

    void Init(const DBPath* path);
    void Init(const DBLine* line);
    s32 Move(s32 speed);
    s32 Subdivide(s32 threshold) override;
    s32 EndOfPath() override;
    s32 Reset() override;
};

// SplinePath - Catmull-Rom spline interpolation between path points
// PSX: ~128 bytes. Extends Path with direction, parametric t.
struct SplinePath : public Path {
    LVector direction = {};
    s32 field64 = 0;
    s32 t = 0; // parametric position in current segment (16.16 fixed, 0 to 0x10000)

    ~SplinePath() override;

    void Init(const DBPath* path);
    void Init(const DBLine* line);
    s32 Move(s32 speed);
    s32 Subdivide(s32 threshold) override;
    s32 EndOfPath() override;

    s32 Reset() override;

    // Catmull-Rom coefficient computation
    static void CalcCMRCoefficients(s32& a, s32& b, s32& c, s32& d,
                                    s32 p0, s32 p1, s32 p2, s32 p3);
};
