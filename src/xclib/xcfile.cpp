// xcfile.cpp 
// PSX source: XCFILE.CPP
#include "gen/common.h"

// PSX: xcReadFileLow__FPCcPPvPUl (XCFILE.CPP:76, 0x800911C0)
bool xcReadFileLow(const char* path, u8** outData, u32* outSize) {
    MARKFUNCTION(0x800911C0);
    FILE* f = std::fopen(path, "rb");
    if (!f) {
        LOG("[xcReadFileLow] Failed to open: %s", path);
        *outData = nullptr;
        *outSize = 0;
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    u32 size = (u32)std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    u8* data = new u8[size];
    std::fread(data, 1, size, f);
    std::fclose(f);
    *outData = data;
    *outSize = size;
    return true;
}

// PSX: xcReadFileHigh__FPCcPPvPUl (XCFILE.CPP:105, 0x80091268)
bool xcReadFileHigh(const char* path, u8** outData, u32* outSize) {
    MARKFUNCTION(0x80091268);
    // PSX uses different allocator for "high" memory; on PC same as Low
    return xcReadFileLow(path, outData, outSize);
}