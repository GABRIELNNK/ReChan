// common.h - rechan project-wide definitions
#pragma once

#include "core.h"

// Portable fopen wrapper
inline FILE* FileOpen(const char* path, const char* mode) {
#ifdef PLATFORM_WINDOWS
    FILE* f = nullptr;
    fopen_s(&f, path, mode);
    return f;
#else
    return fopen(path, mode);
#endif
}

#include "pc/log.h"

// IDA address marker (no-op on PC)
#define MARKFUNCTION(addr) ((void)0)

// File I/O - read entire file into allocated buffer, caller owns the memory
inline bool FileReadAll(const char* path, u8** outData, u32* outSize) {
    FILE* f = FileOpen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    u32 size = (u32)ftell(f);
    fseek(f, 0, SEEK_SET);
    u8* data = new u8[size];
    fread(data, 1, size, f);
    fclose(f);
    *outData = data;
    *outSize = size;
    return true;
}

template <typename T>
static inline T Clamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// PSX native resolution (overlay coordinate space)
#define PSX_SCREEN_WIDTH  512
#define PSX_SCREEN_HEIGHT 240

// PSX font/text coordinate space
#define PSX_FONT_WIDTH    320
#define PSX_FONT_HEIGHT   240

// PC window resolution
#define DEFAULT_SCREEN_WIDTH  1280
#define DEFAULT_SCREEN_HEIGHT 720

// Normalize PSX overlay coordinates (512x240) to 0..1
#define PSX_NORM_X(x)  ((f32)(x) / (f32)PSX_SCREEN_WIDTH)
#define PSX_NORM_Y(y)  ((f32)(y) / (f32)PSX_SCREEN_HEIGHT)
#define PSX_NORM_W(w)  ((f32)(w) / (f32)PSX_SCREEN_WIDTH)
#define PSX_NORM_H(h)  ((f32)(h) / (f32)PSX_SCREEN_HEIGHT)

// Normalize PSX font coordinates (320x240) to 0..1
#define FONT_NORM_X(x) ((f32)(x) / (f32)PSX_FONT_WIDTH)
#define FONT_NORM_Y(y) ((f32)(y) / (f32)PSX_FONT_HEIGHT)
#define FONT_NORM_W(w) ((f32)(w) / (f32)PSX_FONT_WIDTH)
#define FONT_NORM_H(h) ((f32)(h) / (f32)PSX_FONT_HEIGHT)
