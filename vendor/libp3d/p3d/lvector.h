// lvector.h - PSX integer vector types
// LVector = PSX tagLVector (s32 x,y,z) - 12 bytes, world-space positions
// SVector = PSX _RMVECT16 (s16 x,y,z,pad) - 8 bytes, direction/offset vectors
#pragma once

#include <cstdint>

using s16 = int16_t;
using s32 = int32_t;

// PSX 3D integer vector (tagLVector)
struct LVector {
    s32 x, y, z;
};

// PSX short vector (_RMVECT16)
struct SVector {
    s16 x, y, z, pad;
};
