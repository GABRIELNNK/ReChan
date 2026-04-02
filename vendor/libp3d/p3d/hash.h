// hash.h - Pure3D PJW hash function
// PSX: HASH.CPP:10
#pragma once

#include "core.h"

// p3dHash - PJW hash (hashpjw algorithm)
// PSX: used by CharacterManager, Path, entity name lookups
inline u32 p3dHash(const char* x) {
    u32 h = 0;
    u32 g;
    while (*x != 0) {
        h = (h << 4) + *x++;
        if ((g = h & 0xF0000000) != 0) {
            h = (h ^ (g >> 24)) ^ g;
        }
    }
    return h;
}
