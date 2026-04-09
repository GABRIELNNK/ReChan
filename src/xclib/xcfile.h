#pragma once
#include "core.h"

extern bool xcReadFileLow(const char* path, u8** outData, u32* outSize);
extern bool xcReadFileHigh(const char* path, u8** outData, u32* outSize);
