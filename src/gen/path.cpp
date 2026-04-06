// path.cpp - Path interpolation system reversed from PSX PATH.CPP
// Original: C:\CHAN\GAME\SRC\GEN\PATH.CPP
#include "gen/path.h"
#include "p3d/p3dmath.h"
#include "p3d/hash.h"

// Sentinel value for "attribute not found"
static const s32 ATTRIB_NOT_FOUND = (s32)0xABCDABCD;

// Fixed-point 16.16 sentinel for uninitialized velocity
static const s32 VELOCITY_SENTINEL = (s32)0xABCDABCD;

// Helper: integer-based vector normalize (16.16 fixed point in/out)
static void IntV3Normalize(s32* v) {
    f32 fx = (f32)v[0];
    f32 fy = (f32)v[1];
    f32 fz = (f32)v[2];
    f32 mag = sqrt(fx * fx + fy * fy + fz * fz);
    if (mag > 0.0f) {
        v[0] = FLOAT_TO_FIX16(fx / mag);
        v[1] = FLOAT_TO_FIX16(fy / mag);
        v[2] = FLOAT_TO_FIX16(fz / mag);
    }
}

// Helper: integer 3D magnitude
static s32 IntMag3(s32 x, s32 y, s32 z) {
    f32 fx = (f32)x;
    f32 fy = (f32)y;
    f32 fz = (f32)z;
    return (s32)sqrt(fx * fx + fy * fy + fz * fz);
}

// Helper: fixed-point divide (a << 16) / b
static s32 IntDiv16(s32 a, s32 b) {
    if (b == 0)
        return 0;
    return (s32)(((s64)a << 16) / b);
}

// NodeAttribs

// PSX: ~NodeAttribs (inlined in destructors)
NodeAttribs::~NodeAttribs() {
    delete[] ids;
    delete[] values;
}

// PSX: __as__11NodeAttribsRC11NodeAttribs (PATH.CPP:107)
NodeAttribs& NodeAttribs::operator=(const NodeAttribs& other) {
    if (this == &other)
        return *this;

    delete[] ids;
    delete[] values;

    count = other.count;

    if (count > 0) {
        ids = new u8[count];
        memcpy(ids, other.ids, count);
        values = new s32[count];
        memcpy(values, other.values, count * sizeof(s32));
    } else {
        ids = nullptr;
        values = nullptr;
    }
    return *this;
}

// PSX: Init__11NodeAttribsP7DBPoint (PATH.CPP:127)
// Extracts per-point attributes from a DBPoint into parallel arrays.
void NodeAttribs::Init(const DBPoint* point) {
    delete[] ids;
    ids = nullptr;
    delete[] values;
    values = nullptr;

    if (point->attribCount == 0) {
        count = 0;
        return;
    }

    count = (s32)point->attribCount;
    ids = new u8[count];
    values = new s32[count];

    for (s32 i = 0; i < count; i++) {
        const DBAttrib* attrib = point->GetAttribByIndex(i);
        ids[i] = (u8)(attrib->id & 0xFF);
        if (attrib->type != 0) {
            // Numeric attribute
            values[i] = (s32)attrib->value;
        } else {
            // String attribute: hash to integer
            values[i] = (s32)p3dHash(attrib->strValue ? attrib->strValue : "");
        }
    }
}

// PSX: GetAttrib__11NodeAttribsi (PATH.CPP:166)
// Returns the value for a given attribute ID, or 0xABCDABCD if not found.
s32 NodeAttribs::GetAttrib(s32 id) const {
    if (count <= 0)
        return ATTRIB_NOT_FOUND;

    u8 target = (u8)(id & 0xFF);
    for (s32 i = 0; i < count; i++) {
        if (ids[i] == target) {
            return values[i];
        }
    }
    return ATTRIB_NOT_FOUND;
}

// PSX: Swap__FR11NodeAttribsT0 (PATH.CPP:71)
void Swap(NodeAttribs& a, NodeAttribs& b) {
    s32 tc = a.count;
    a.count = b.count;
    b.count = tc;

    u8* ti = a.ids;
    a.ids = b.ids;
    b.ids = ti;

    s32* tv = a.values;
    a.values = b.values;
    b.values = tv;
}

// SubDivNode

// PSX: _._10SubDivNode (PATH.CPP:67)
SubDivNode::~SubDivNode() {
    // NodeAttribs destructor handles cleanup
}

// Path

Path::~Path() {
    delete[] positions;
    delete[] nodeAttribs;
}

// PSX: Flip__4Path (PATH.CPP:182)
// Reverses the order of points and their attributes.
void Path::Flip() {
    s32 half = (numPoints - 1) / 2;
    for (s32 i = 0; i < half; i++) {
        s32 j = numPoints - 1 - i;

        // Swap positions
        LVector tmp = positions[i];
        positions[i] = positions[j];
        positions[j] = tmp;

        // Swap attributes
        Swap(nodeAttribs[i], nodeAttribs[j]);
    }
}

// PSX: Draw__4Path (PATH.CPP:198)
// Debug draw of the path. Needs line drawing functions.
void Path::Draw() {
    // TODO: Requires MyDrawLine rendering function
}

// LinearPath

LinearPath::~LinearPath() {
    // Base Path dtor handles positions/nodeAttribs
}

// PSX: Subdivide__10LinearPathl (PATH.CPP:224)
// Adaptive subdivision: inserts midpoints until all segments are shorter
// than 'threshold' on every axis.
s32 LinearPath::Subdivide(s32 threshold) {
    // Build a temp linked list of SubDivNodes from current positions
    ccMinList tempList;
    for (s32 i = 0; i < numPoints; i++) {
        SubDivNode* node = new SubDivNode();
        node->x = positions[i].x;
        node->y = positions[i].y;
        node->z = positions[i].z;
        node->attribs = nodeAttribs[i];
        // Clear source so it won't double-free
        nodeAttribs[i].count = 0;
        nodeAttribs[i].ids = nullptr;
        nodeAttribs[i].values = nullptr;
        tempList.AddNodeTail(node);
    }

    // Walk the list, subdividing long segments
    SubDivNode* prev = static_cast<SubDivNode*>(tempList.GetFirst());
    if (prev) {
        SubDivNode* cur = static_cast<SubDivNode*>(prev->next);
        while (cur) {
            s32 dx = cur->x - prev->x;
            if (dx < 0) dx = -dx;
            s32 dy = cur->y - prev->y;
            if (dy < 0) dy = -dy;
            s32 dz = cur->z - prev->z;
            if (dz < 0) dz = -dz;

            if (dx > threshold || dy > threshold || dz > threshold) {
                // Insert midpoint
                SubDivNode* mid = new SubDivNode();
                mid->x = (prev->x + cur->x) / 2;
                mid->y = (prev->y + cur->y) / 2;
                mid->z = (prev->z + cur->z) / 2;
                tempList.AddNode(prev, mid);
                // Re-check from prev to mid next iteration
                cur = mid;
            } else {
                prev = cur;
                cur = static_cast<SubDivNode*>(cur->next);
            }
        }
    }

    // Free old arrays
    delete[] positions;
    delete[] nodeAttribs;

    // Count subdivided nodes
    s32 count = 0;
    SubDivNode* n = static_cast<SubDivNode*>(tempList.GetFirst());
    while (n) {
        count++;
        n = static_cast<SubDivNode*>(n->next);
    }

    numPoints = count;
    positions = new LVector[count];
    nodeAttribs = new NodeAttribs[count]();

    // Copy back from list
    s32 idx = 0;
    n = static_cast<SubDivNode*>(tempList.GetFirst());
    while (n) {
        positions[idx].x = n->x;
        positions[idx].y = n->y;
        positions[idx].z = n->z;
        nodeAttribs[idx] = n->attribs;
        // Clear to prevent double-free
        n->attribs.count = 0;
        n->attribs.ids = nullptr;
        n->attribs.values = nullptr;
        idx++;
        n = static_cast<SubDivNode*>(n->next);
    }

    // Clean up temp list
    SubDivNode* del = static_cast<SubDivNode*>(tempList.GetFirst());
    while (del) {
        SubDivNode* next = static_cast<SubDivNode*>(del->next);
        delete del;
        del = next;
    }

    return 0;
}

// PSX: EndOfPath__10LinearPath (PATH.HPP, inlined)
s32 LinearPath::EndOfPath() {
    return (currentSegment >= numPoints - 1) ? 1 : 0;
}

// Helper: free nodeAttribs array properly
static void FreeNodeAttribsArray(NodeAttribs*& arr) {
    if (arr) {
        delete[] arr;
        arr = nullptr;
    }
}

// PSX: Init__10LinearPathPC6DBPath (PATH.CPP:276)
void LinearPath::Init(const DBPath* path) {
    // Free old data
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    // Allocate from DBPath
    numPoints = (s32)path->pointCount;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    // Copy points from linked list
    s32 idx = 0;
    DBPoint* point = static_cast<DBPoint*>(path->points.GetFirst());
    while (idx < numPoints && point) {
        positions[idx] = point->pos;
        nodeAttribs[idx].Init(point);
        idx++;
        point = static_cast<DBPoint*>(point->next);
    }

    // If attrib 4 is not found on first point, flip the path
    s32 flipFlag = nodeAttribs[0].GetAttrib(4);
    if (flipFlag == ATTRIB_NOT_FOUND) {
        Flip();
    }

    // Subdivide to threshold
    Subdivide(0x7FFF);
}

// PSX: Init__10LinearPathPC6DBLine (PATH.CPP:299)
void LinearPath::Init(const DBLine* line) {
    // Free old data
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    // Allocate from DBLine
    numPoints = (s32)line->vertexCount;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    // Copy vertices from linked list
    s32 idx = 0;
    DBLineVertex* vert = static_cast<DBLineVertex*>(line->vertices.GetFirst());
    while (idx < numPoints && vert) {
        positions[idx].x = vert->x;
        positions[idx].y = vert->y;
        positions[idx].z = vert->z;
        vert = static_cast<DBLineVertex*>(vert->next);
        idx++;
    }

    // Subdivide to threshold
    Subdivide(0x7FFF);
}

// PSX: Move__10LinearPathl (PATH.CPP:319)
// Advances along the path by 'speed' units.
// Returns 1 if a node boundary was crossed, 0 otherwise.
s32 LinearPath::Move(s32 speed) {
    s32 crossed = 0;

    if (velocity.x == VELOCITY_SENTINEL) {
        // First call: compute initial direction from first two points
        s32 dir[3];
        dir[0] = (positions[1].x - positions[0].x) << 16;
        dir[1] = (positions[1].y - positions[0].y) << 16;
        dir[2] = (positions[1].z - positions[0].z) << 16;

        IntV3Normalize(dir);

        velocity.x = (s32)(((s64)speed * dir[0]) >> 16);
        velocity.y = (s32)(((s64)speed * dir[1]) >> 16);
        velocity.z = (s32)(((s64)speed * dir[2]) >> 16);
    } else {
        // Update accumulated position
        current.x += velocity.x;
        current.y += velocity.y;
        current.z += velocity.z;

        // Compute segment direction
        s32 seg = currentSegment;
        s32 segDx = positions[seg + 1].x - positions[seg].x;
        s32 segDy = positions[seg + 1].y - positions[seg].y;
        s32 segDz = positions[seg + 1].z - positions[seg].z;

        // Distance from current pos to next endpoint
        s32 toEndX = current.x - positions[seg + 1].x;
        s32 toEndY = current.y - positions[seg + 1].y;
        s32 toEndZ = current.z - positions[seg + 1].z;

        // Dot product: segment direction dot velocity
        s64 dot = (s64)segDx * (s64)velocity.x
                + (s64)segDy * (s64)velocity.y
                + (s64)segDz * (s64)velocity.z;

        if (dot <= 0) {
            // Passed the endpoint, advance segment
            crossed = 1;
            currentSegment++;

            if (EndOfPath()) {
                // Reached the end: snap to last point
                s32 lastSeg = currentSegment;
                current.x = positions[lastSeg].x;
                current.y = positions[lastSeg].y;
                current.z = positions[lastSeg].z;
                velocity.x = VELOCITY_SENTINEL;
            } else {
                // Compute new direction for next segment
                s32 dir[3];
                dir[0] = (positions[currentSegment + 1].x - current.x) << 16;
                dir[1] = (positions[currentSegment + 1].y - current.y) << 16;
                dir[2] = (positions[currentSegment + 1].z - current.z) << 16;

                IntV3Normalize(dir);

                velocity.x = (s32)(((s64)speed * dir[0]) >> 16);
                velocity.y = (s32)(((s64)speed * dir[1]) >> 16);
                velocity.z = (s32)(((s64)speed * dir[2]) >> 16);
            }
        }
    }

    // Copy velocity to direction output
    direction = velocity;

    return crossed;
}

// SplinePath

SplinePath::~SplinePath() {
    // Base Path dtor handles positions/nodeAttribs
}

// PSX: Subdivide__10SplinePathl (PATH.CPP:391)
// No-op for spline paths.
s32 SplinePath::Subdivide(s32 /*threshold*/) {
    return 0;
}

// PSX: CalcCMRCoefficiants__10SplinePathRlN31llll (PATH.CPP:398)
// Catmull-Rom coefficient computation.
// Given 4 control points p0,p1,p2,p3, computes cubic coefficients:
//   pos(t) = a*t^3 + b*t^2 + c*t + d  (for t in [0,1])
//   a = (-p0 + 3*p1 - 3*p2 + p3) / 2
//   b = (2*p0 - 5*p1 + 4*p2 - p3) / 2
//   c = (-p0 + p2) / 2
//   d = p1
void SplinePath::CalcCMRCoefficients(s32& a, s32& b, s32& c, s32& d,
                                      s32 p0, s32 p1, s32 p2, s32 p3) {
    // PSX uses integer division by 2 (rounds toward zero)
    d = p1;
    c = -(p0 / 2) + (p2 / 2);
    b = p0 - (5 * p1) / 2 + 2 * p2 - p3 / 2;
    a = -(p0 / 2) + (3 * p1) / 2 - (3 * p2) / 2 + p3 / 2;
}

// PSX: EndOfPath__10SplinePath (PATH.HPP:210)
s32 SplinePath::EndOfPath() {
    return (currentSegment >= numPoints - 2) ? 1 : 0;
}

// PSX: Reset__10SplinePath (PATH.HPP:204)
void SplinePath::Reset() {
    currentSegment = 1;
    t = 0;
    current = positions[0];
}

// PSX: Init__10SplinePathPC6DBPath (PATH.CPP:417)
void SplinePath::Init(const DBPath* path) {
    // Free old data
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    // SplinePath adds 2 extra points for wrap-around
    numPoints = (s32)path->pointCount + 2;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    // Copy points from DBPath into positions[1..pointCount]
    s32 idx = 1;
    s32 attribOffset = 12; // byte offset; on PC we use array index
    DBPoint* point = static_cast<DBPoint*>(path->points.GetFirst());
    while (idx < numPoints - 1 && point) {
        positions[idx] = point->pos;
        nodeAttribs[idx].Init(point);
        idx++;
        point = static_cast<DBPoint*>(point->next);
    }

    // Wrap: first = copy of second
    positions[0] = positions[1];

    // Wrap: last = copy of second-to-last
    positions[numPoints - 1] = positions[numPoints - 2];

    // Check flip flag on second node (index 1)
    s32 flipFlag = nodeAttribs[1].GetAttrib(4);
    if (flipFlag == ATTRIB_NOT_FOUND) {
        Flip();
    }

    // Subdivide (no-op for spline)
    Subdivide(0x7FFF);
}

// PSX: Init__10SplinePathPC6DBLine (PATH.CPP:442)
void SplinePath::Init(const DBLine* line) {
    // Free old data
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    // Allocate from DBLine (no extra wrap-around points for DBLine init)
    numPoints = (s32)line->vertexCount;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    // Copy vertices from linked list into positions[1..count-2]
    s32 idx = 1;
    DBLineVertex* vert = static_cast<DBLineVertex*>(line->vertices.GetFirst());
    while (idx < numPoints - 1 && vert) {
        positions[idx].x = vert->x;
        positions[idx].y = vert->y;
        positions[idx].z = vert->z;
        vert = static_cast<DBLineVertex*>(vert->next);
        idx++;
    }

    // Wrap: first = copy of second
    positions[0] = positions[1];

    // Wrap: last = copy of second-to-last
    positions[numPoints - 1] = positions[numPoints - 2];

    // Subdivide (no-op for spline)
    Subdivide(0x7FFF);
}

// PSX: Move__10SplinePathl (PATH.CPP:464)
// Advances along the spline by 'speed' units.
// Returns 1 if a segment boundary was crossed, 0 otherwise.
s32 SplinePath::Move(s32 speed) {
    s32 crossed = 0;

    // Compute t^2 and t^3 in 16.16 fixed point
    s64 t64 = (s64)t;
    s64 tSq = (t64 * t64) >> 16;          // t^2 in 16.16
    s64 tCu = (tSq * t64) >> 16;          // t^3 in 16.16

    // Get 4 control points around current segment
    s32 seg = currentSegment;
    s32 i0 = seg - 1;
    s32 i1 = seg;
    s32 i2 = seg + 1;
    s32 i3 = seg + 2;

    // Clamp indices
    if (i0 < 0) i0 = 0;
    if (i3 >= numPoints) i3 = numPoints - 1;

    // Compute CMR coefficients for each axis
    s32 ax, bx, cx, dx;
    s32 ay, by, cy, dy;
    s32 az, bz, cz, dz;

    CalcCMRCoefficients(ax, bx, cx, dx,
                         positions[i0].x, positions[i1].x,
                         positions[i2].x, positions[i3].x);
    CalcCMRCoefficients(ay, by, cy, dy,
                         positions[i0].y, positions[i1].y,
                         positions[i2].y, positions[i3].y);
    CalcCMRCoefficients(az, bz, cz, dz,
                         positions[i0].z, positions[i1].z,
                         positions[i2].z, positions[i3].z);

    // Evaluate derivative: direction = 3*a*t^2 + 2*b*t + c
    direction.x = (s32)((3 * (s64)ax * tSq + 2 * (s64)bx * t64 + ((s64)cx << 16)) >> 16);
    direction.y = (s32)((3 * (s64)ay * tSq + 2 * (s64)by * t64 + ((s64)cy << 16)) >> 16);
    direction.z = (s32)((3 * (s64)az * tSq + 2 * (s64)bz * t64 + ((s64)cz << 16)) >> 16);

    // Compute speed factor
    s32 mag = IntMag3(direction.x, direction.y, direction.z);
    s32 dt = 0;
    if (mag > 0) {
        dt = IntDiv16(speed, mag);
    }

    // Evaluate position: pos = a*t^3 + b*t^2 + c*t + d
    current.x = (s32)(((s64)ax * tCu + (s64)bx * tSq + (s64)cx * t64) >> 16) + dx;
    current.y = (s32)(((s64)ay * tCu + (s64)by * tSq + (s64)cy * t64) >> 16) + dy;
    current.z = (s32)(((s64)az * tCu + (s64)bz * tSq + (s64)cz * t64) >> 16) + dz;

    // Advance parametric position
    t += dt;

    // Check for segment crossing (t >= 1.0 in 16.16)
    while (t >= FIX16_ONE) {
        t -= FIX16_ONE;
        currentSegment++;
        crossed = 1;
    }

    return crossed;
}
