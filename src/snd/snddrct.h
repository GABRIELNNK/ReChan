#pragma once
#include "core.h"

class CGenericPersistentSound;

// PSX: CSoundDirect (SNDDRCT.CPP)
// All methods are static - no instance state.
class CSoundDirect {
public:
    // Creates a transient sound via factory, triggers it, destroys it.
    // soundId: global sample ID (index into g_transData)
    // posPtr: 3D position (can be null)
    // pan: pan value
    // flags: Initialize flags
    static s32 PlayTransient(u16 soundId, void* posPtr, u16 pan, u32 flags);

    // Creates a persistent sound via factory, initializes and begins playback.
    static s32 BeginPersistent(u8 persistId, CGenericPersistentSound** outObj, void* posPtr);

    // Ends and destroys the persistent sound, sets pointer to null.
    static s32 EndPersistent(CGenericPersistentSound** obj);
};
