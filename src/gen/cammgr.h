#pragma once
#include "core.h"
#include "p3d/lvector.h"
#include "gen/manager.h"  // Manager -> ccNode, ccMinNode, ccMinList
#include "gen/database.h"    // DBPath, DBPoint, DBRoot

// Manages camera rail paths loaded from the WDB database.
// CameraManager loads paths, CameraAnchor owns them, Camera references the anchor.

// DBCameraPathNode (0x8004B480)
// PSX struct: 64 bytes. Each node is a point along a camera rail.
// +0:  ccMinNode base (next, prev)
// +12: source position (LVector - where the camera sits)
// +24: target position (LVector - where the camera looks)
// +36: pathIndex (s16 - ID copied from parent DBCameraPath)
// +38: fov (s16 - attrib 7)
// +40: camAngleY (s16 - attrib 8, yaw)
// +42: camAngleX (s16 - attrib 9, pitch)
// +44: camAngleZ (s16 - attrib 10, roll)
// +46: zoom (s16 - attrib 11)
// +48: speed (s16 - attrib 12)
// +50: flags (u8 - attrib 13)
// +52: param0 (s32 - attrib 20)
// +56: param1 (s32 - attrib 21)
// +60: param2 (s32 - attrib 22)
struct DBCameraPathNode : public ccMinNode {
    // PSX +12: source (eye) position
    LVector sourcePos = {};
    // PSX +24: target (look-at) position
    LVector targetPos = {};
    // PSX +36
    s16 pathIndex = 0;
    // PSX +38..+48: per-node camera parameters from DB attributes
    s16 fov = 0;        // attrib 7
    s16 camAngleY = 0;  // attrib 8 - yaw
    s16 camAngleX = 0;  // attrib 9 - pitch
    s16 camAngleZ = 0;  // attrib 10 - roll
    s16 zoom = 0;       // attrib 11
    s16 speed = 0;      // attrib 12
    // PSX +50
    u8 flags = 0;       // attrib 13
    // PSX +52, +56, +60
    s32 param0 = 0;     // attrib 20
    s32 param1 = 0;     // attrib 21
    s32 param2 = 0;     // attrib 22

    DBCameraPathNode();
    ~DBCameraPathNode() override;
};

// DBCameraPath (0x8004AB6C)
// PSX struct: 52 bytes. A camera rail with a list of nodes + bounding box.
// +0:  ccMinNode base (next, prev)
// +12: ccMinList of DBCameraPathNode (nodes)
// +24: pathID (s32 - from DB attrib 6)
// +28: bboxMin (LVector - shrinks as nodes added)
// +40: bboxMax (LVector - grows as nodes added)
struct DBCameraPath : public ccMinNode {
    // PSX +12: linked list of DBCameraPathNode
    ccMinList nodes;

    // PSX +24: path identifier (from DB attrib 6)
    s32 pathID = 0;

    // PSX +28,+32,+36: bounding box minimum
    LVector bboxMin = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };
    // PSX +40,+44,+48: bounding box maximum
    LVector bboxMax = { (s32)0x80000000, (s32)0x80000000, (s32)0x80000000 };

    DBCameraPath();
    ~DBCameraPath() override;

    // Add a source node (camera position) from DB point data
    void AddSourceNode(DBPoint* point);

    // Add target positions to existing nodes from DB point data
    void AddTargetNode(DBPoint* point, s32 reverseOrder);

    // Expand bounding box by 'margin' in all directions
    void FinalizeBoundaries(s32 margin);

    // Test if position is inside the bounding box
    s32 InRange(LVector pos);

    // Find the two closest nodes to a position
    // Returns squared distance to closest node (-1 if none found)
    s32 FindClosestNodes(LVector pos,
                         DBCameraPathNode** outNodeA,
                         DBCameraPathNode** outNodeB);
};

// CameraAnchor (0x8004A780)
// PSX struct: 48 bytes. Owns all camera paths for a level.
// +0:  ccNode base (next, prev, vtable, name - 24 bytes)
// +24: ccMinList (reserved / anchor chain)
// +36: ccMinList of DBCameraPath (source paths)
struct CameraAnchor : public ccNode {
    // PSX +24: anchor chain list (usage unclear, possibly for sub-anchors)
    ccMinList anchorList;

    // PSX +36: source camera paths
    ccMinList sourcePaths;

    CameraAnchor();
    ~CameraAnchor() override;

    // Add a source camera path from a DB path entry (type 155 = 0x9B)
    void AddCameraSourcePath(DBPath* path);

    // Add target nodes to an existing source path (type 156 = 0x9C)
    void AddCameraTargetPath(DBPath* path);

    // Find source path by its pathID
    // PSX: GetPathWithID (0x8004AA30)
    DBCameraPath* GetPathWithID(u32 id);

    // Find the two closest path nodes to a world position
    // Returns squared distance (-1 if none found)
    s32 FindClosestNodes(LVector pos,
                         DBCameraPathNode** outNodeA,
                         DBCameraPathNode** outNodeB);
};

// Loads camera paths from Database.
class CameraManager : public Manager {
public:
    CameraManager();
    ~CameraManager() override;

    // Load camera paths from the Database and set up the CameraAnchor.
    // Creates a callback node that calls SetupPaths.
    // On PC we call SetupPaths directly since we don't have the async callback system.
    void InternalOpen() override;

    // Iterate all DB paths, create DBCameraPath entries, assign to CameraAnchor.
    void SetupPaths();

    // The camera anchor for this level
    CameraAnchor* GetAnchor() const {
        return anchor;
    }

private:
    CameraAnchor* anchor = nullptr;
};

// PSX: gp+3548, defined in cammgr.cpp
extern CameraManager* g_cameraManager;
