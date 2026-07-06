#include "gen/path.h"
#include "p3d/p3dmath.h"
#include "p3d/hash.h"

static const s32 ATTRIB_NOT_FOUND = (s32)0xABCDABCD;
static const s32 VELOCITY_SENTINEL = (s32)0xABCDABCD;

// NodeAttribs

NodeAttribs::~NodeAttribs() {
    delete[] ids;
    delete[] values;
}

// PSX: __as__11NodeAttribsRC11NodeAttribs (PATH.CPP:107)
NodeAttribs& NodeAttribs::operator=(const NodeAttribs& other) {
    MARKFUNCTION(0x800A4530);
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
    }
    else {
        ids = nullptr;
        values = nullptr;
    }
    return *this;
}

// PSX: Init__11NodeAttribsP7DBPoint (PATH.CPP:127)
void NodeAttribs::Init(const DBPoint* point) {
    MARKFUNCTION(0x800A45D8);
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
            values[i] = (s32)attrib->value;
        }
        else {
            values[i] = (s32)p3dHash(attrib->strValue ? attrib->strValue : "");
        }
    }
}

// PSX: GetAttrib__11NodeAttribsi (PATH.CPP:166)
s32 NodeAttribs::GetAttrib(s32 id) const {
    MARKFUNCTION(0x800A46F4);
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
    MARKFUNCTION(0x800A44F4);
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

SubDivNode::~SubDivNode() {
}

// Path

Path::~Path() {
    delete[] positions;
    delete[] nodeAttribs;
}

// PSX: Flip__4Path (PATH.CPP:182)
void Path::Flip() {
    MARKFUNCTION(0x800A4760);
    s32 half = numPoints / 2;
    for (s32 i = 0; i < half; i++) {
        s32 j = numPoints - 1 - i;

        LVector tmp = positions[i];
        positions[i] = positions[j];
        positions[j] = tmp;

        Swap(nodeAttribs[i], nodeAttribs[j]);
    }
}

// PSX: Draw__4Path (PATH.CPP:198)
void Path::Draw() {
    MARKFUNCTION(0x800A4894);
}

// LinearPath

LinearPath::~LinearPath() {
}

// PSX: Subdivide__10LinearPathl (PATH.CPP:224)
s32 LinearPath::Subdivide(s32 threshold) {
    MARKFUNCTION(0x800A4958);
    ccMinList tempList;
    for (s32 i = 0; i < numPoints; i++) {
        SubDivNode* node = new SubDivNode();
        node->x = positions[i].x;
        node->y = positions[i].y;
        node->z = positions[i].z;
        node->attribs = nodeAttribs[i];
        nodeAttribs[i].count = 0;
        nodeAttribs[i].ids = nullptr;
        nodeAttribs[i].values = nullptr;
        tempList.AddNodeTail(node);
    }

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
                SubDivNode* mid = new SubDivNode();
                mid->x = (prev->x + cur->x) / 2;
                mid->y = (prev->y + cur->y) / 2;
                mid->z = (prev->z + cur->z) / 2;
                tempList.AddNode(prev, mid);
                cur = mid;
            }
            else {
                prev = cur;
                cur = static_cast<SubDivNode*>(cur->next);
            }
        }
    }

    delete[] positions;
    delete[] nodeAttribs;

    s32 count = 0;
    SubDivNode* n = static_cast<SubDivNode*>(tempList.GetFirst());
    while (n) {
        count++;
        n = static_cast<SubDivNode*>(n->next);
    }

    numPoints = count;
    positions = new LVector[count];
    nodeAttribs = new NodeAttribs[count]();

    s32 idx = 0;
    n = static_cast<SubDivNode*>(tempList.GetFirst());
    while (n) {
        positions[idx].x = n->x;
        positions[idx].y = n->y;
        positions[idx].z = n->z;
        nodeAttribs[idx] = n->attribs;
        n->attribs.count = 0;
        n->attribs.ids = nullptr;
        n->attribs.values = nullptr;
        idx++;
        n = static_cast<SubDivNode*>(n->next);
    }

    SubDivNode* del = static_cast<SubDivNode*>(tempList.GetFirst());
    while (del) {
        SubDivNode* next = static_cast<SubDivNode*>(del->next);
        delete del;
        del = next;
    }
    tempList.head = nullptr;
    tempList.tail = nullptr;

    return 0;
}

// PSX: EndOfPath__10LinearPath (PATH.HPP, inlined)
s32 LinearPath::EndOfPath() {
    MARKFUNCTION(0x800A6078);
    return (currentSegment >= numPoints - 1) ? 1 : 0;
}

// PSX: Reset__10LinearPath (PATH.HPP:155)
s32 LinearPath::Reset() {
    MARKFUNCTION(0x800A6090);
    currentSegment = 0;
    velocity.x = VELOCITY_SENTINEL;
    current = positions[0];
    return VELOCITY_SENTINEL;
}

static void FreeNodeAttribsArray(NodeAttribs*& arr) {
    if (arr) {
        delete[] arr;
        arr = nullptr;
    }
}

// PSX: Init__10LinearPathPC6DBPath (PATH.CPP:276)
void LinearPath::Init(const DBPath* path) {
    MARKFUNCTION(0x800A4D9C);
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    numPoints = (s32)path->pointCount;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    s32 idx = 0;
    DBPoint* point = static_cast<DBPoint*>(path->points.GetFirst());
    while (idx < numPoints && point) {
        positions[idx] = point->pos;
        nodeAttribs[idx].Init(point);
        idx++;
        point = static_cast<DBPoint*>(point->next);
    }

    s32 flipFlag = nodeAttribs[0].GetAttrib(4);
    if (flipFlag == ATTRIB_NOT_FOUND) {
        Flip();
    }

    Subdivide(0x7FFF);
}

// PSX: Init__10LinearPathPC6DBLine (PATH.CPP:299)
void LinearPath::Init(const DBLine* line) {
    MARKFUNCTION(0x800A4F8C);
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    numPoints = (s32)line->vertexCount;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    s32 idx = 0;
    DBLineVertex* vert = static_cast<DBLineVertex*>(line->vertices.GetFirst());
    while (idx < numPoints && vert) {
        positions[idx].x = vert->x;
        positions[idx].y = vert->y;
        positions[idx].z = vert->z;
        vert = static_cast<DBLineVertex*>(vert->next);
        idx++;
    }

    Subdivide(0x7FFF);
}

// PSX: Move__10LinearPathl (PATH.CPP:319)
s32 LinearPath::Move(s32 speed) {
    MARKFUNCTION(0x800A513C);
    s32 crossed = 0;

    if (velocity.x == VELOCITY_SENTINEL) {
        s32 dir[3];
        dir[0] = (positions[1].x - positions[0].x) << 16;
        dir[1] = (positions[1].y - positions[0].y) << 16;
        dir[2] = (positions[1].z - positions[0].z) << 16;

        rmV3Normalize((LVector*)dir, (LVector*)dir);

        velocity.x = (s32)(((s64)speed * dir[0]) >> 16);
        velocity.y = (s32)(((s64)speed * dir[1]) >> 16);
        velocity.z = (s32)(((s64)speed * dir[2]) >> 16);
    }
    else {
        current.x += velocity.x;
        current.y += velocity.y;
        current.z += velocity.z;

        s32 seg = currentSegment;
        s32 segDx = positions[seg + 1].x - positions[seg].x;
        s32 segDy = positions[seg + 1].y - positions[seg].y;
        s32 segDz = positions[seg + 1].z - positions[seg].z;

        s32 toEndX = current.x - positions[seg + 1].x;
        s32 toEndY = current.y - positions[seg + 1].y;
        s32 toEndZ = current.z - positions[seg + 1].z;

        s32 dot = segDx * toEndX + segDy * toEndY + segDz * toEndZ;

        if (dot > 0) {
            crossed = 1;
            currentSegment++;

            if (EndOfPath()) {
                s32 lastSeg = currentSegment;
                current.x = positions[lastSeg].x;
                current.y = positions[lastSeg].y;
                current.z = positions[lastSeg].z;
                velocity.x = VELOCITY_SENTINEL;
            }
            else {
                s32 dir[3];
                dir[0] = (positions[currentSegment + 1].x - current.x) << 16;
                dir[1] = (positions[currentSegment + 1].y - current.y) << 16;
                dir[2] = (positions[currentSegment + 1].z - current.z) << 16;

                rmV3Normalize((LVector*)dir, (LVector*)dir);

                velocity.x = (s32)(((s64)speed * dir[0]) >> 16);
                velocity.y = (s32)(((s64)speed * dir[1]) >> 16);
                velocity.z = (s32)(((s64)speed * dir[2]) >> 16);
            }
        }
    }

    direction = velocity;

    return crossed;
}

// SplinePath

SplinePath::~SplinePath() {
}

// PSX: Subdivide__10SplinePathl (PATH.CPP:391)
s32 SplinePath::Subdivide(s32 /*threshold*/) {
    return 0;
}

// PSX: CalcCMRCoefficiants__10SplinePathRlN31llll (PATH.CPP:398)
void SplinePath::CalcCMRCoefficients(s32& a, s32& b, s32& c, s32& d,
                                     s32 p0, s32 p1, s32 p2, s32 p3) {
    MARKFUNCTION(0x800A54C4);
    d = p1;
    c = -(p0 / 2) + (p2 / 2);
    b = p0 - (5 * p1) / 2 + 2 * p2 - p3 / 2;
    a = -(p0 / 2) + (3 * p1) / 2 - (3 * p2) / 2 + p3 / 2;
}

// PSX: EndOfPath__10SplinePath (PATH.HPP:210)
s32 SplinePath::EndOfPath() {
    MARKFUNCTION(0x800A5F4C);
    return (currentSegment >= numPoints - 2) ? 1 : 0;
}

// PSX: Reset__10SplinePath (PATH.HPP:204)
s32 SplinePath::Reset() {
    MARKFUNCTION(0x800A5F64);
    currentSegment = 1;
    t = 0;
    current = positions[0];
    return 1;
}

// PSX: Init__10SplinePathPC6DBPath (PATH.CPP:417)
void SplinePath::Init(const DBPath* path) {
    MARKFUNCTION(0x800A556C);
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    numPoints = (s32)path->pointCount + 2;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    s32 idx = 1;
    DBPoint* point = static_cast<DBPoint*>(path->points.GetFirst());
    while (idx < numPoints - 1 && point) {
        positions[idx] = point->pos;
        nodeAttribs[idx].Init(point);
        idx++;
        point = static_cast<DBPoint*>(point->next);
    }

    positions[0] = positions[1];
    positions[numPoints - 1] = positions[numPoints - 2];

    s32 flipFlag = nodeAttribs[1].GetAttrib(4);
    if (flipFlag == ATTRIB_NOT_FOUND) {
        Flip();
    }

    Subdivide(0x7FFF);
}

// PSX: Init__10SplinePathPC6DBLine (PATH.CPP:442)
void SplinePath::Init(const DBLine* line) {
    MARKFUNCTION(0x800A57BC);
    delete[] positions;
    positions = nullptr;
    FreeNodeAttribsArray(nodeAttribs);

    numPoints = (s32)line->vertexCount;
    positions = new LVector[numPoints];
    nodeAttribs = new NodeAttribs[numPoints]();

    s32 idx = 1;
    DBLineVertex* vert = static_cast<DBLineVertex*>(line->vertices.GetFirst());
    while (idx < numPoints - 1 && vert) {
        positions[idx].x = vert->x;
        positions[idx].y = vert->y;
        positions[idx].z = vert->z;
        vert = static_cast<DBLineVertex*>(vert->next);
        idx++;
    }

    positions[0] = positions[1];
    positions[numPoints - 1] = positions[numPoints - 2];

    Subdivide(0x7FFF);
}

// PSX: Move__10SplinePathl (PATH.CPP:464)
s32 SplinePath::Move(s32 speed) {
    MARKFUNCTION(0x800A59CC);
    s32 crossed = 0;

    s64 t64 = (s64)t;
    s64 tSq = (t64 * t64) >> 16;
    s64 tCu = (tSq * t64) >> 16;

    s32 seg = currentSegment;
    s32 i0 = seg - 1;
    s32 i1 = seg;
    s32 i2 = seg + 1;
    s32 i3 = seg + 2;

    if (i0 < 0) i0 = 0;
    if (i3 >= numPoints) i3 = numPoints - 1;

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

    direction.x = (s32)((3 * (s64)ax * tSq + 2 * (s64)bx * t64 + ((s64)cx << 16)) >> 16);
    direction.y = (s32)((3 * (s64)ay * tSq + 2 * (s64)by * t64 + ((s64)cy << 16)) >> 16);
    direction.z = (s32)((3 * (s64)az * tSq + 2 * (s64)bz * t64 + ((s64)cz << 16)) >> 16);

    s32 mag = (s32)rmMag3((f32)direction.x, (f32)direction.y, (f32)direction.z);
    s32 dt = 0;
    if (mag > 0) {
        dt = rmDiv16i(speed, mag);
    }

    current.x = (s32)(((s64)ax * tCu + (s64)bx * tSq + (s64)cx * t64) >> 16) + dx;
    current.y = (s32)(((s64)ay * tCu + (s64)by * tSq + (s64)cy * t64) >> 16) + dy;
    current.z = (s32)(((s64)az * tCu + (s64)bz * tSq + (s64)cz * t64) >> 16) + dz;

    t += dt;

    while (t > FIX16_ONE) {
        t -= FIX16_ONE;
        currentSegment++;
        crossed = 1;
    }

    return crossed;
}
