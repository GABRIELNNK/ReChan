// cammgr.cpp - CameraManager
// Original: C:\CHAN\GAME\SRC\GEN\CAMMGR.CPP
#include "gen/cammgr.h"
#include "p3d/p3dmath.h"

// PSX global
CameraManager* g_cameraManager = nullptr;

// DBCameraPathNode (0x8004B480, 0x8004B4B4)
DBCameraPathNode::DBCameraPathNode() {
    MARKFUNCTION(0x8004B480);
}

DBCameraPathNode::~DBCameraPathNode() {
    MARKFUNCTION(0x8004B4B4);
}

// DBCameraPath (0x8004AB6C)
DBCameraPath::DBCameraPath() {
    MARKFUNCTION(0x8004AB6C);
}

DBCameraPath::~DBCameraPath() {
    MARKFUNCTION(0x8004ABE8);
}

// DBCameraPath::AddSourceNode (0x8004AC40)
// Creates a DBCameraPathNode from a DBPoint, reads attributes 7-16 into
// the node's camera parameters, then appends to the nodes list.
void DBCameraPath::AddSourceNode(DBPoint* /*point*/) {
    MARKFUNCTION(0x8004AC40);

    // TODO: Requires DBPoint (with position, attrib list, FindAttrib, GetAttribByIndex)
    // PSX flow:
    //   1. new DBCameraPathNode (64 bytes)
    //   2. Iterate DBPoint's attribs (count at point+56)
    //   3. Switch on attrib id (7..16):
    //        7 = node.fov, 8 = node.camAngleY, 9 = node.camAngleX
    //       10 = node.camAngleZ, 11 = node.zoom, 12 = node.speed
    //       13 = node.flags, 14 = node.param0, 15 = node.param1
    //       16 = node.param2
    //   4. node.pathIndex = (s16)this->pathID
    //   5. node.sourcePos = point->position (+28,+32,+36 in DBPoint)
    //   6. nodes.AddNodeTail(node)
    //   7. Update bboxMin/bboxMax from sourcePos
}

// DBCameraPath::AddTargetNode (0x8004ADD0)
// Walks existing source nodes and matching point nodes in parallel,
// setting target positions and updating the bounding box.
void DBCameraPath::AddTargetNode(DBPoint* /*point*/, s32 /*reverseOrder*/) {
    MARKFUNCTION(0x8004ADD0);

    // TODO: Requires DBPoint linked list traversal
    // PSX flow:
    //   1. node = this->nodes.head, pointNode = point
    //   2. While both non-null:
    //      a. node.targetPos = pointNode->position
    //      b. Update bboxMin/bboxMax from targetPos
    //      c. Advance: reverseOrder ? pointNode=pointNode->next : pointNode=pointNode->child
    //      d. node = node->next
}

// DBCameraPath::FinalizeBoundaries (0x8004AED0)
void DBCameraPath::FinalizeBoundaries(s32 margin) {
    MARKFUNCTION(0x8004AED0);

    bboxMin.x -= margin;
    bboxMin.y -= margin;
    bboxMin.z -= margin;
    bboxMax.x += margin;
    bboxMax.y += margin;
    bboxMax.z += margin;
}

// DBCameraPath::InRange (0x8004AF1C)
// Returns 1 if position is inside the bounding box, 0 otherwise.
s32 DBCameraPath::InRange(LVector pos) {
    MARKFUNCTION(0x8004AF1C);

    if (pos.x < bboxMin.x)
        return 0;
    if (pos.y < bboxMin.y)
        return 0;
    if (pos.z < bboxMin.z)
        return 0;
    if (pos.x > bboxMax.x)
        return 0;
    if (pos.y > bboxMax.y)
        return 0;
    if (pos.z > bboxMax.z)
        return 0;
    return 1;
}

// DBCameraPath::FindClosestNodes (0x8004AFB0)
// Finds the two closest nodes to 'pos'. Returns squared distance to closest,
// or -1 if no nodes exist. outNodeA = closest, outNodeB = second closest
// (or the adjacent node based on projection direction).
//
// PSX algorithm:
//   1. Walk all nodes, compute dist2 = (|dx|>>5)^2 + (|dy|>>5)^2 + (|dz|>>5)^2
//      (>>5 to prevent overflow in 32-bit multiply)
//   2. Track closest node and min distance
//   3. If two nodes are equally close, use dot product of segment direction
//      vs position offset to determine which pair to return
//   4. After finding closest, check block numbers for validity (skip if too far
//      apart, unless level 7 which relaxes the check)
s32 DBCameraPath::FindClosestNodes(LVector pos,
                                   DBCameraPathNode** outNodeA,
                                   DBCameraPathNode** outNodeB) {
    MARKFUNCTION(0x8004AFB0);

    DBCameraPathNode* closest = nullptr;
    s32 minDist = -1;

    // Find closest node by squared distance (>>5 scale to avoid overflow)
    DBCameraPathNode* node = static_cast<DBCameraPathNode*>(nodes.GetFirst());
    while (node) {
        s32 dx = pos.x - node->sourcePos.x;
        if (dx < 0) dx = -dx;
        dx >>= 5;

        s32 dy = pos.y - node->sourcePos.y;
        if (dy < 0) dy = -dy;
        dy >>= 5;

        s32 dz = pos.z - node->sourcePos.z;
        if (dz < 0) dz = -dz;
        dz >>= 5;

        s32 dist = dx * dx + dy * dy + dz * dz;

        if ((u32)dist < (u32)minDist) {
            minDist = dist;
            closest = node;
        } else if (dist == minDist && closest) {
            // Equal distance - use rmMag3 to pick closer of the two candidates
            s32 vecA_x = (pos.x - node->sourcePos.x) << 16;
            s32 vecA_y = (pos.y - node->sourcePos.y) << 16;
            s32 vecA_z = (pos.z - node->sourcePos.z) << 16;

            s32 vecB_x = (pos.x - closest->sourcePos.x) << 16;
            s32 vecB_y = (pos.y - closest->sourcePos.y) << 16;
            s32 vecB_z = (pos.z - closest->sourcePos.z) << 16;

            s32 magA = (s32)rmMag3((f32)vecA_x, (f32)vecA_y, (f32)vecA_z);
            s32 magB = (s32)rmMag3((f32)vecB_x, (f32)vecB_y, (f32)vecB_z);

            if (magA < magB) {
                closest = node;
            }
        }

        node = static_cast<DBCameraPathNode*>(node->next);
    }

    // TODO: Validate block proximity (PSX checks BlockManager)
    // blockA = BlockManager::GetBlockNumber(closest->sourcePos)
    // blockB = BlockManager::GetBlockNumber(pos)
    // If closest is null OR blockA is invalid OR |blockA - blockB| >= 3
    //   (unless level 7), mark as out-of-range.
    bool outOfRange = (closest == nullptr);

    if (outOfRange) {
        *outNodeA = nullptr;
        *outNodeB = nullptr;
        return -1;
    }

    // Determine the neighbor node (outNodeB)
    DBCameraPathNode* prevNode = static_cast<DBCameraPathNode*>(closest->prev);
    DBCameraPathNode* nextNode = static_cast<DBCameraPathNode*>(closest->next);

    if (!prevNode && !nextNode) {
        *outNodeA = closest;
        *outNodeB = nullptr;
        return minDist;
    }

    if (!prevNode) {
        *outNodeA = nextNode;
        *outNodeB = closest;
        return minDist;
    }

    if (!nextNode) {
        *outNodeA = closest;
        *outNodeB = prevNode;
        return minDist;
    }

    // Both neighbors exist - use dot product to determine direction
    // PSX builds two segment vectors, normalizes via rmDiv16i, then computes
    // a 64-bit dot product. If dot < 0, position is "behind" closest
    // relative to prev, so use (nextNode, closest); else (prevNode, closest).
    s32 segA_x = (prevNode->sourcePos.x - closest->sourcePos.x) << 16;
    s32 segA_y = (prevNode->sourcePos.y - closest->sourcePos.y) << 16;
    s32 segA_z = (prevNode->sourcePos.z - closest->sourcePos.z) << 16;

    s32 segB_x = (nextNode->sourcePos.x - closest->sourcePos.x) << 16;
    s32 segB_y = (nextNode->sourcePos.y - closest->sourcePos.y) << 16;
    s32 segB_z = (nextNode->sourcePos.z - closest->sourcePos.z) << 16;

    s32 toPos_x = (pos.x - closest->sourcePos.x) << 16;
    s32 toPos_y = (pos.y - closest->sourcePos.y) << 16;
    s32 toPos_z = (pos.z - closest->sourcePos.z) << 16;

    // Normalize segA
    s32 magA = (s32)rmMag3((f32)segA_x, (f32)segA_y, (f32)segA_z);
    if (magA > 0) {
        segA_x = (s32)(((s64)segA_x << 16) / magA);
        segA_y = (s32)(((s64)segA_y << 16) / magA);
        segA_z = (s32)(((s64)segA_z << 16) / magA);
    }

    // Normalize segB
    s32 magB = (s32)rmMag3((f32)segB_x, (f32)segB_y, (f32)segB_z);
    if (magB > 0) {
        segB_x = (s32)(((s64)segB_x << 16) / magB);
        segB_y = (s32)(((s64)segB_y << 16) / magB);
        segB_z = (s32)(((s64)segB_z << 16) / magB);
    }

    // (segA - segB) dot toPos
    s32 diff_x = segA_x - segB_x;
    s32 diff_y = segA_y - segB_y;
    s32 diff_z = segA_z - segB_z;

    s64 dot = (s64)diff_x * (s64)toPos_x
            + (s64)diff_y * (s64)toPos_y
            + (s64)diff_z * (s64)toPos_z;

    if (dot < 0) {
        *outNodeA = closest;
        *outNodeB = prevNode;
    } else {
        *outNodeA = nextNode;
        *outNodeB = closest;
    }

    return minDist;
}

// CameraAnchor (0x8004A780)
CameraAnchor::CameraAnchor() {
    MARKFUNCTION(0x8004A780);
}

CameraAnchor::~CameraAnchor() {
    MARKFUNCTION(0x8004A7F8);
}

// CameraAnchor::AddCameraSourcePath (0x8004A870)
// Creates a DBCameraPath from a DBPath (type 0x9B = 155).
// Reads source nodes, checks for reverse-order flag (attrib 4),
// reads the pathID (attrib 6), and appends to sourcePaths.
void CameraAnchor::AddCameraSourcePath(DBPath* /*path*/) {
    MARKFUNCTION(0x8004A870);

    // TODO: Requires DBPath (with firstPoint, attribs, FindAttrib)
    // PSX flow:
    //   1. new DBCameraPath (52 bytes)
    //   2. reverseOrder = 0
    //   3. point = path->firstPoint (at path+60)
    //   4. If point has attrib 4 with nonzero value: reverseOrder = 1
    //   5. pathID = GetAttribValue(FindAttrib(point, 6))
    //   6. camPath->pathID = pathID
    //   7. While point != null:
    //        camPath->AddSourceNode(point)
    //        point = reverseOrder ? point->prev : point->next
    //   8. sourcePaths.AddNodeTail(camPath)
}

// CameraAnchor::AddCameraTargetPath (0x8004A968)
// Associates target positions with an existing source path.
// Finds the matching source path by pathID, then adds target nodes.
void CameraAnchor::AddCameraTargetPath(DBPath* /*path*/) {
    MARKFUNCTION(0x8004A968);

    // TODO: Requires DBPath
    // PSX flow:
    //   1. reverseOrder = 0
    //   2. point = path->firstPoint
    //   3. If point has attrib 4 with nonzero value: reverseOrder = 1; point = path->firstPoint->child
    //      else: point = path->firstPoint->next
    //   4. pathID = GetAttribValue(FindAttrib(point, 6))
    //   5. camPath = GetPathWithID(pathID)
    //   6. If camPath: AddTargetNode(point, reverseOrder)
    //   7. FinalizeBoundaries(10240)
}

// CameraAnchor::GetPathWithID (0x8004AA30)
DBCameraPath* CameraAnchor::GetPathWithID(u32 id) {
    MARKFUNCTION(0x8004AA30);

    DBCameraPath* path = static_cast<DBCameraPath*>(sourcePaths.GetFirst());
    while (path) {
        if ((u32)path->pathID == id) {
            return path;
        }
        path = static_cast<DBCameraPath*>(path->next);
    }
    return nullptr;
}

// CameraAnchor::FindClosestNodes (0x8004AA6C)
// Searches all source paths for the two closest nodes to 'pos'.
// Only considers paths where pos is InRange of the bounding box.
s32 CameraAnchor::FindClosestNodes(LVector pos,
                                    DBCameraPathNode** outNodeA,
                                    DBCameraPathNode** outNodeB) {
    MARKFUNCTION(0x8004AA6C);

    s32 bestDist = -1;
    *outNodeA = nullptr;
    *outNodeB = nullptr;

    DBCameraPath* path = static_cast<DBCameraPath*>(sourcePaths.GetFirst());
    while (path) {
        if (path->InRange(pos)) {
            DBCameraPathNode* nodeA = nullptr;
            DBCameraPathNode* nodeB = nullptr;
            s32 dist = path->FindClosestNodes(pos, &nodeA, &nodeB);

            if ((u32)dist < (u32)bestDist) {
                bestDist = dist;
                *outNodeA = nodeA;
                *outNodeB = nodeB;
            }
        }
        path = static_cast<DBCameraPath*>(path->next);
    }

    return bestDist;
}

// CameraManager (0x8004A548)
CameraManager::CameraManager() {
    MARKFUNCTION(0x8004A548);
    g_cameraManager = this;
}

CameraManager::~CameraManager() {
    MARKFUNCTION(0x8004A580);
    g_cameraManager = nullptr;
}

// CameraManager::InternalOpen (0x8004A5F4)
// PSX creates a Callback node with cameraLoadFunc as the callback,
// stores 'this' at node+32, and registers it with the load system.
// On PC we call SetupPaths directly when Database integration is ready.
void CameraManager::InternalOpen() {
    MARKFUNCTION(0x8004A5F4);

    // TODO: Once Database loading is implemented, call SetupPaths() here.
}

// CameraManager::SetupPaths (0x8004A668)
// Creates a CameraAnchor, iterates all Database paths:
//   type 155 (0x9B) = camera source paths -> AddCameraSourcePath
//   type 156 (0x9C) = camera target paths -> AddCameraTargetPath
// Finally stores the anchor in the global Camera object.
void CameraManager::SetupPaths() {
    MARKFUNCTION(0x8004A668);

    anchor = new CameraAnchor();
    anchor->SetName("CamAnchor", 0);

    // TODO: Requires Database iteration (GetFirstPath, path->type, etc.)
    // PSX flow:
    //   1. Add anchor to World's anchor list (at g_world + 28)
    //   2. Iterate g_database->GetFirstPath():
    //      while (path != null):
    //        if (path->firstPoint->attrib[26] == 155)
    //          anchor->AddCameraSourcePath(path)
    //        path = path->next
    //   3. Iterate again for type 156:
    //      while (path != null):
    //        if (path->firstPoint->attrib[26] == 156)
    //          anchor->AddCameraTargetPath(path)
    //        path = path->next
    //   4. g_camera->cameraAnchor = anchor (at camera + 392)

    this->anchor = anchor;
}

