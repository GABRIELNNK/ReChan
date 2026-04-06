#include "p3d/flip.h"
#include "p3d/p3dmath.h"
#include <cstring>

static u16 ReadU16(const u8* p) { return p[0] | (p[1] << 8); }
static u32 ReadU32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static s16 ReadS16(const u8* p) { return (s16)(p[0] | (p[1] << 8)); }

// TransformAnim

TransformAnim::TransformAnim()
    : nameUID(0), numFrames(0), numRotChannels(0), numTransChannels(0),
      rotChannels(nullptr), transChannels(nullptr), ownedRawData(nullptr) {}

TransformAnim::~TransformAnim() {
    delete[] rotChannels;
    delete[] transChannels;
    if (ownedRawData) {
        std::free(ownedRawData);
    }
}

TransformAnim* TransformAnim::Parse(const u8* rawData, u32 rawSize) {
    if (!rawData || rawSize < 40) {
        return nullptr;
    }

    // tTransformAnim header layout (40 bytes):
    //   +0:  nameUID (u32)
    //   +4:  refCount (s32, ignored)
    //   +8:  vtable offset (u32, ignored)
    //   +12: numFrames (s32)
    //   +16: targetType (s32, ignored)
    //   +20: targetNameUID (u32, ignored)
    //   +24: numRotChannels (s32)
    //   +28: numTransChannels (s32)
    //   +32: rotChannelArrayOffset (u32 DWORD offset)
    //   +36: transChannelArrayOffset (u32 DWORD offset)

    s32 numRotCh = (s32)ReadU32(rawData + 24);
    s32 numTransCh = (s32)ReadU32(rawData + 28);
    u32 rotArrayByteOff = ReadU32(rawData + 32) * 4;
    u32 transArrayByteOff = ReadU32(rawData + 36) * 4;

    if (rotArrayByteOff + (u32)numRotCh * 4 > rawSize) {
        return nullptr;
    }
    if (transArrayByteOff + (u32)numTransCh * 4 > rawSize) {
        return nullptr;
    }

    TransformAnim* ta = new TransformAnim();
    ta->nameUID = ReadU32(rawData + 0);
    ta->numFrames = (s32)ReadU32(rawData + 12);
    ta->numRotChannels = numRotCh;
    ta->numTransChannels = numTransCh;

    // Parse rotation channels
    if (numRotCh > 0) {
        ta->rotChannels = new Channel[numRotCh];
        for (s32 i = 0; i < numRotCh; i++) {
            u32 chByteOff = ReadU32(rawData + rotArrayByteOff + i * 4) * 4;
            if (chByteOff + 8 > rawSize) {
                ta->rotChannels[i] = { 0, 0, nullptr, rawData, rawSize };
                continue;
            }
            const u8* ch = rawData + chByteOff;
            ta->rotChannels[i] = {
                ReadU32(ch + 0),    // jointParam
                ReadU32(ch + 4),    // keyType
                ch,                 // chData
                rawData,            // rawBase
                rawSize             // rawSize
            };
        }
    }

    // Parse translation channels
    if (numTransCh > 0) {
        ta->transChannels = new Channel[numTransCh];
        for (s32 i = 0; i < numTransCh; i++) {
            u32 chByteOff = ReadU32(rawData + transArrayByteOff + i * 4) * 4;
            if (chByteOff + 8 > rawSize) {
                ta->transChannels[i] = { 0, 0, nullptr, rawData, rawSize };
                continue;
            }
            const u8* ch = rawData + chByteOff;
            ta->transChannels[i] = {
                ReadU32(ch + 0),    // jointParam
                ReadU32(ch + 4),    // keyType
                ch,                 // chData
                rawData,            // rawBase
                rawSize             // rawSize
            };
        }
    }

    return ta;
}

// TransformFlip

TransformFlip::TransformFlip()
    : frame(0), frameReal(0), dirty(1), anim(nullptr), tree(nullptr) {}

TransformFlip::~TransformFlip() {
    // We don't own anim or tree - they are managed externally
}

// PSX: Attach__9tTreeFlipP5tTreeP14tTransformAnim (CHANNEL.CPP:953)
void TransformFlip::Attach(STreeData* t, TransformAnim* a) {
    dirty = 1;
    tree = t;
    anim = a;
}

// PSX: SetFrame__15tTransformFlip2i (CHANNEL.CPP:730)
void TransformFlip::SetFrame(s32 f) {
    frame = f;
    frameReal = f << 16;
}

// PSX: SetFrameReal__15tTransformFlip2l (CHANNEL.CPP:737)
void TransformFlip::SetFrameReal(s32 f) {
    frame = f >> 16;
    frameReal = f;
}

// PSX: Reset__15tTransformFlip2 (CHANNEL.CPP:744)
void TransformFlip::Reset() {
    dirty = 1;
    SetFrame(0);
    UpdateJoints();
}

// PSX: UpdateJoints__15tTransformFlip2P5tTree (CHANNEL.CPP:778)
void TransformFlip::UpdateJoints() {
    if (!anim || !tree || !tree->joints) {
        return;
    }

    // Process translation channels
    for (s32 i = 0; i < anim->numTransChannels; i++) {
        const TransformAnim::Channel& ch = anim->transChannels[i];
        if (!ch.chData) {
            continue;
        }

        // Map animation parameter to joint index
        u32 jointParam = ch.jointParam;
        if (jointParam >= tree->numMapEntries || !tree->jointOrderMap) {
            continue;
        }
        u32 jointIdx = tree->jointOrderMap[jointParam];
        if (jointIdx >= tree->numJoints) {
            continue;
        }

        EvalTransChannel(ch, tree->joints[jointIdx]);
    }

    // Process rotation channels
    for (s32 i = 0; i < anim->numRotChannels; i++) {
        const TransformAnim::Channel& ch = anim->rotChannels[i];
        if (!ch.chData) {
            continue;
        }

        u32 jointParam = ch.jointParam;
        if (jointParam >= tree->numMapEntries || !tree->jointOrderMap) {
            continue;
        }
        u32 jointIdx = tree->jointOrderMap[jointParam];
        if (jointIdx >= tree->numJoints) {
            continue;
        }

        EvalRotChannel(ch, tree->joints[jointIdx]);
    }

    dirty = 0;
}

// Find the bracket index: returns i such that keyTimes[i] <= intFrame < keyTimes[i+1].
// Returns -1 if frame is before all keys, numKeys-1 if at or after last key.
s32 TransformFlip::FindBracket(const u8* rawBase, u32 keyTimesOff, s32 numKeys, s32 intFrame) {
    // Key times are stored as s16 array at rawBase + keyTimesOff * 4
    u32 byteOff = keyTimesOff * 4;
    const u8* times = rawBase + byteOff;

    // Check bounds
    s16 firstTime = ReadS16(times);
    if (intFrame <= firstTime) {
        return 0;
    }

    s16 lastTime = ReadS16(times + (numKeys - 1) * 2);
    if (intFrame >= lastTime) {
        return numKeys - 1;
    }

    // Linear search for bracket (numKeys is typically small)
    for (s32 i = 0; i < numKeys - 1; i++) {
        s16 t0 = ReadS16(times + i * 2);
        s16 t1 = ReadS16(times + (i + 1) * 2);
        if (intFrame >= t0 && intFrame < t1) {
            return i;
        }
    }

    return numKeys - 1;
}

void TransformFlip::EvalRotChannel(const TransformAnim::Channel& ch, STreeJoint& joint) {
    const u8* data = ch.chData;
    const u8* raw = ch.rawBase;
    u32 rawSize = ch.rawSize;

    if (ch.keyType == 11) {
        // tStatic3DOFKeyList: constant values at +8, +12, +16
        joint.rotationX = (s16)(s32)ReadU32(data + 8);
        joint.rotationY = (s16)(s32)ReadU32(data + 12);
        joint.rotationZ = (s16)(s32)ReadU32(data + 16);
    } else if (ch.keyType == 5) {
        // tJoint3DOFangle: packed rotation values
        // +8: numKeys, +12: keyTimesOff, +16: keyValuesOff
        u32 numKeys = ReadU32(data + 8);
        if (numKeys == 0) {
            return;
        }
        u32 keyTimesOff = ReadU32(data + 12);
        u32 keyValsOff = ReadU32(data + 16);
        u32 keyValsByteOff = keyValsOff * 4;

        if (numKeys == 1 || dirty) {
            // Single key or first evaluation: just use the value at current frame
            s32 bracket = FindBracket(raw, keyTimesOff, (s32)numKeys, frame);
            u32 packed = ReadU32(raw + keyValsByteOff + bracket * 4);
            joint.rotationX = (s16)((packed >> 11) * 64);
            joint.rotationY = (s16)(((packed >> 5) & 0x3F) * 32);
            joint.rotationZ = (s16)((packed & 0x1F) * 32);
            return;
        }

        // Multi-key interpolation
        s32 bracket = FindBracket(raw, keyTimesOff, (s32)numKeys, frame);

        if (bracket >= (s32)numKeys - 1) {
            // At or past last key
            u32 packed = ReadU32(raw + keyValsByteOff + (numKeys - 1) * 4);
            joint.rotationX = (s16)((packed >> 11) * 64);
            joint.rotationY = (s16)(((packed >> 5) & 0x3F) * 32);
            joint.rotationZ = (s16)((packed & 0x1F) * 32);
            return;
        }

        // Unpack both bracket keyframes
        u32 packed0 = ReadU32(raw + keyValsByteOff + bracket * 4);
        u32 packed1 = ReadU32(raw + keyValsByteOff + (bracket + 1) * 4);

        s32 rx0 = (s32)((packed0 >> 11) * 64);
        s32 ry0 = (s32)(((packed0 >> 5) & 0x3F) * 32);
        s32 rz0 = (s32)((packed0 & 0x1F) * 32);
        s32 rx1 = (s32)((packed1 >> 11) * 64);
        s32 ry1 = (s32)(((packed1 >> 5) & 0x3F) * 32);
        s32 rz1 = (s32)((packed1 & 0x1F) * 32);

        // Get bracket key times for interpolation fraction
        u32 timesByteOff = keyTimesOff * 4;
        s16 t0 = ReadS16(raw + timesByteOff + bracket * 2);
        s16 t1 = ReadS16(raw + timesByteOff + (bracket + 1) * 2);
        s32 timeDelta = t1 - t0;
        if (timeDelta <= 0) {
            joint.rotationX = (s16)rx0;
            joint.rotationY = (s16)ry0;
            joint.rotationZ = (s16)rz0;
            return;
        }

        // Fraction in 16.16: (frameReal - t0<<16) / (timeDelta<<16)
        s32 frameDelta = frameReal - ((s32)t0 << 16);
        s32 frac16 = (s32)(((s64)frameDelta << 16) / ((s64)timeDelta << 16));

        // Lerp: v0 + (v1 - v0) * frac16 >> 16
        joint.rotationX = (s16)(rx0 + (((rx1 - rx0) * (s64)frac16) >> 16));
        joint.rotationY = (s16)(ry0 + (((ry1 - ry0) * (s64)frac16) >> 16));
        joint.rotationZ = (s16)(rz0 + (((rz1 - rz0) * (s64)frac16) >> 16));
    } else if (ch.keyType == 3) {
        // tJoint1DOFangle: single axis rotation
        // +8=numKeys, +12=keyTimesOff(u8 array), +16=dofIndex, +20=keyValuesOff(s16 array)
        u32 numKeys = ReadU32(data + 8);
        if (numKeys == 0) {
            return;
        }
        u32 keyTimesOff = ReadU32(data + 12);
        u32 dofIndex = ReadU32(data + 16);
        u32 keyValsOff = ReadU32(data + 20);
        u32 keyTimesByteOff = keyTimesOff * 4;
        u32 keyValsByteOff = keyValsOff * 4;

        // Key times are u8, find bracket by linear search
        const u8* times = raw + keyTimesByteOff;
        s32 bracket = 0;
        if (numKeys > 1) {
            for (u32 k = 0; k < numKeys - 1; k++) {
                if (frame >= (s32)times[k] && frame < (s32)times[k + 1]) {
                    bracket = (s32)k;
                    break;
                }
                if (frame >= (s32)times[k + 1]) {
                    bracket = (s32)(k + 1);
                }
            }
        }

        s16 val;
        if (numKeys == 1 || bracket >= (s32)numKeys - 1) {
            val = ReadS16(raw + keyValsByteOff + bracket * 2);
        } else {
            s32 t0 = (s32)times[bracket];
            s32 t1 = (s32)times[bracket + 1];
            s32 timeDelta = t1 - t0;
            if (timeDelta <= 0) {
                val = ReadS16(raw + keyValsByteOff + bracket * 2);
            } else {
                s16 v0 = ReadS16(raw + keyValsByteOff + bracket * 2);
                s16 v1 = ReadS16(raw + keyValsByteOff + (bracket + 1) * 2);
                s32 frameDelta = frameReal - (t0 << 16);
                s32 frac16 = (s32)(((s64)frameDelta << 16) / ((s64)timeDelta << 16));
                val = (s16)((s32)v0 + ((((s32)v1 - (s32)v0) * (s64)frac16) >> 16));
            }
        }

        if (dofIndex == 0) {
            joint.rotationX = val;
        } else if (dofIndex == 1) {
            joint.rotationY = val;
        } else {
            joint.rotationZ = val;
        }
    }
}

void TransformFlip::EvalTransChannel(const TransformAnim::Channel& ch, STreeJoint& joint) {
    const u8* data = ch.chData;
    const u8* raw = ch.rawBase;
    u32 rawSize = ch.rawSize;

    if (ch.keyType == 12) {
        // tStatic3DOFKeyList: constant values at +8, +12, +16
        joint.translationX = (s32)ReadU32(data + 8);
        joint.translationY = (s32)ReadU32(data + 12);
        joint.translationZ = (s32)ReadU32(data + 16);
    } else if (ch.keyType == 8) {
        // tJoint3DOFlpPSX: 3DOF linear position
        // +8: numKeys, +12: keyTimesOff, +16: keyValuesOff (3 x s16 per key)
        u32 numKeys = ReadU32(data + 8);
        if (numKeys == 0) {
            return;
        }
        u32 keyTimesOff = ReadU32(data + 12);
        u32 keyValsOff = ReadU32(data + 16);
        u32 keyValsByteOff = keyValsOff * 4;

        if (numKeys == 1 || dirty) {
            s32 bracket = FindBracket(raw, keyTimesOff, (s32)numKeys, frame);
            u32 vOff = keyValsByteOff + bracket * 6;
            if (vOff + 6 > rawSize) {
                return;
            }
            joint.translationX = (s32)ReadS16(raw + vOff + 0);
            joint.translationY = (s32)ReadS16(raw + vOff + 2);
            joint.translationZ = (s32)ReadS16(raw + vOff + 4);
            return;
        }

        // Multi-key interpolation
        s32 bracket = FindBracket(raw, keyTimesOff, (s32)numKeys, frame);

        if (bracket >= (s32)numKeys - 1) {
            u32 vOff = keyValsByteOff + (numKeys - 1) * 6;
            if (vOff + 6 > rawSize) {
                return;
            }
            joint.translationX = (s32)ReadS16(raw + vOff + 0);
            joint.translationY = (s32)ReadS16(raw + vOff + 2);
            joint.translationZ = (s32)ReadS16(raw + vOff + 4);
            return;
        }

        u32 vOff0 = keyValsByteOff + bracket * 6;
        u32 vOff1 = keyValsByteOff + (bracket + 1) * 6;
        if (vOff1 + 6 > rawSize) {
            return;
        }

        s32 x0 = (s32)ReadS16(raw + vOff0 + 0);
        s32 y0 = (s32)ReadS16(raw + vOff0 + 2);
        s32 z0 = (s32)ReadS16(raw + vOff0 + 4);
        s32 x1 = (s32)ReadS16(raw + vOff1 + 0);
        s32 y1 = (s32)ReadS16(raw + vOff1 + 2);
        s32 z1 = (s32)ReadS16(raw + vOff1 + 4);

        u32 timesByteOff = keyTimesOff * 4;
        s16 t0 = ReadS16(raw + timesByteOff + bracket * 2);
        s16 t1 = ReadS16(raw + timesByteOff + (bracket + 1) * 2);
        s32 timeDelta = t1 - t0;
        if (timeDelta <= 0) {
            joint.translationX = x0;
            joint.translationY = y0;
            joint.translationZ = z0;
            return;
        }

        s32 frameDelta = frameReal - ((s32)t0 << 16);
        s32 frac16 = (s32)(((s64)frameDelta << 16) / ((s64)timeDelta << 16));

        joint.translationX = (s32)(x0 + (((x1 - x0) * (s64)frac16) >> 16));
        joint.translationY = (s32)(y0 + (((y1 - y0) * (s64)frac16) >> 16));
        joint.translationZ = (s32)(z0 + (((z1 - z0) * (s64)frac16) >> 16));
    }
}
