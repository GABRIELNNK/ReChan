#pragma once
#include "core.h"

// PSX: Decibel100 lookup table (0x800DA768)
// Extracted from GAME_REL.CPE binary - maps 0-100 linear volume to decibel-scaled value
static const u8 g_decibelTable[101] = {
     0,  3,  7, 10, 13, 16, 18, 21, 23, 25,
    27, 29, 31, 33, 35, 37, 38, 40, 41, 43,
    44, 46, 47, 48, 49, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
    66, 67, 67, 68, 69, 70, 71, 71, 72, 73,
    74, 74, 75, 76, 76, 77, 78, 78, 79, 80,
    80, 81, 81, 82, 82, 83, 84, 84, 85, 85,
    86, 86, 87, 87, 88, 88, 89, 89, 90, 90,
    91, 91, 92, 92, 93, 93, 94, 94, 95, 95,
    95, 96, 96, 97, 97, 98, 98, 98, 99, 99,
    100
};

// PSX: Decibel100__FUl (SNDMATH.CPP:18, 0x800BA0E0)
static inline u32 Decibel100(u32 val) {
    if (val >= 101) {
        return 0;
    }
    return g_decibelTable[val];
}

// PSX: PsxVol100__FUl (SNDMATH.CPP:47, 0x800BA118)
// Converts 0-100 linear volume to PSX SPU volume (0-0xFFFF)
static inline u16 PsxVol100(u32 val) {
    return (u16)(0xFFFF * Decibel100(val) / 100);
}

// PSX: PsxPitch200__FUl (SNDMATH.CPP:52, 0x800BA150)
// Converts 0-200 pitch scale to PSX SPU pitch register value
// 100 = normal pitch (returns 0x0400 = 1024)
static inline s16 PsxPitch200(s32 val) {
    return (s16)(10 * (val - 100) + 1024);
}

// PC: convert PSX SPU volume (0-0xFFFF) to float (0.0-1.0)
static inline f32 PsxVolToFloat(u16 psxVol) {
    return (f32)psxVol / 65535.0f;
}

// PC: convert PSX SPU pitch (1024 = normal) to float ratio (1.0 = normal)
static inline f32 PsxPitchToFloat(s16 psxPitch) {
    return (f32)psxPitch / 1024.0f;
}
