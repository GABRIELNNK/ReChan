#pragma once
#include "core.h"
#include "p3d/skeleton.h"

// PSX CHANNEL.CPP key list types.
enum TransformKeyType : u32 {
    KEY_JOINT_1DOF_ANGLE = 3,
    KEY_JOINT_3DOF_ANGLE = 5,
    KEY_JOINT_3DOF_LP_PSX = 8,
    KEY_STATIC_3DOF_ANGLE = 11,
    KEY_STATIC_3DOF_POS = 12,
};

enum TransformDofAxis : u32 {
    DOF_AXIS_X = 0,
    DOF_AXIS_Y = 1,
    DOF_AXIS_Z = 2,
};

// Parsed animation data from raw tTransformAnim binary blob.
struct TransformAnim {
    u32 nameUID;
    s32 numFrames;
    s32 numRotChannels;
    s32 numTransChannels;

    // Per-channel info parsed from the raw binary
    struct Channel {
        u32 jointParam;     // animation parameter index
        u32 keyType;        // channel data type (3,5,8,11,12)
        const u8* chData;   // pointer to channel header in raw data
        const u8* rawBase;  // base of the whole raw animation blob
        u32 rawSize;        // total size of raw data
    };

    Channel* rotChannels;
    Channel* transChannels;
    u8* ownedRawData;

    TransformAnim();
    ~TransformAnim();

    // Parse a raw tTransformAnim binary blob into a usable structure.
    // The rawData pointer must remain valid for the lifetime of this object.
    static TransformAnim* Parse(const u8* rawData, u32 rawSize);
};

// TransformFlip - evaluates animation channels and writes to STree joints.
// Combines tFlipbook + tTransformFlip2 + tTreeFlip
struct TransformFlip {
    s32 frame;              // +0x14: current frame (integer)
    s32 frameReal;          // +0x24: current frame (16.16 fixed-point)
    s32 dirty;              // +0x28: needs re-evaluation flag
    bool additiveTranslation;// non-bind clips use translation offsets from bind pose
    TransformAnim* anim;    // +0x1C: animation data
    STreeData* tree;        // +0x34: target skeleton

    TransformFlip();
    ~TransformFlip();

    // PSX: Attach__9tTreeFlipP5tTreeP14tTransformAnim (CHANNEL.CPP:953)
    void Attach(STreeData* tree, TransformAnim* anim);

    // PSX: SetFrame__15tTransformFlip2i (CHANNEL.CPP:730)
    void SetFrame(s32 f);

    // PSX: SetFrameReal__15tTransformFlip2l (CHANNEL.CPP:737)
    void SetFrameReal(s32 f);

    // PSX: Reset__15tTransformFlip2 (CHANNEL.CPP:744)
    void Reset();

    // PSX: UpdateJoints__15tTransformFlip2P5tTree (CHANNEL.CPP:778)
    void UpdateJoints();

private:
    // Evaluate a rotation channel at current frame and write to joint
    void EvalRotChannel(const TransformAnim::Channel& ch, STreeJoint& joint);

    // Evaluate a translation channel at current frame and write to joint
    void EvalTransChannel(const TransformAnim::Channel& ch, STreeJoint& joint);

    // Find bracket keyframe index for the given 16.16 frame value.
    // Returns the index of the keyframe at or just before the frame.
    static s32 FindBracket(const u8* rawBase, u32 keyTimesOff, s32 numKeys, s32 frameReal);
};
