#include "gen/common.h"
#include "xclib/xcfile.h"
#include "gen/ccfile.h"

static bool xcReadFileInternal(const char* path, u8** outData, u32* outSize) {
    ccFile file;
    if (!file.Open(path, ccFile::OPEN_READ)) {
        LOG("[xcReadFile] Failed to open: %s", path);
        *outData = nullptr;
        *outSize = 0;
        return false;
    }

    u32 size = (u32)file.GetLength();
    u8* data = new u8[size];
    if (size && file.Read(data, size) != (s32)size) {
        delete[] data;
        file.Close();
        *outData = nullptr;
        *outSize = 0;
        return false;
    }

    file.Close();
    *outData = data;
    *outSize = size;
    return true;
}

// PSX: xcReadFileLow__FPCcPPvPUl (XCFILE.CPP:76, 0x800911C0)
bool xcReadFileLow(const char* path, u8** outData, u32* outSize) {
    MARKFUNCTION(0x800911C0);
    return xcReadFileInternal(path, outData, outSize);
}

// PSX: xcReadFileHigh__FPCcPPvPUl (XCFILE.CPP:105, 0x80091268)
bool xcReadFileHigh(const char* path, u8** outData, u32* outSize) {
    MARKFUNCTION(0x80091268);
    return xcReadFileInternal(path, outData, outSize);
}
