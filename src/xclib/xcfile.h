#pragma once

extern bool xcReadFileLow(const char* path, u8** outData, u32* outSize);
extern bool xcReadFileHigh(const char* path, u8** outData, u32* outSize);
extern FILE* xcOpenFile(const char* path, const char* mode);
