#pragma once

#include "core.h"

inline u16 p3dReadU16LE(const u8* p) {
    return static_cast<u16>(p[0] | (p[1] << 8));
}

inline u32 p3dReadU32LE(const u8* p) {
    return static_cast<u32>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

inline s16 p3dReadS16LE(const u8* p) {
    return static_cast<s16>(p3dReadU16LE(p));
}

inline s32 p3dReadS32LE(const u8* p) {
    return static_cast<s32>(p3dReadU32LE(p));
}
