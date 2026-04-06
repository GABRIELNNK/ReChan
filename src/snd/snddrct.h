// snddrct.h - CSoundDirect reversed from PSX SNDDRCT.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\SNDDRCT.CPP
// CSoundDirect provides static helper functions for playing sounds
// without an owning CSound object (e.g., from Director scripts).
#pragma once

#include "core.h"

class CGenericPersistentSound;

// PSX: CSoundDirect (SNDDRCT.CPP)
// All methods are static - no instance state.
class CSoundDirect {
public:
    // PSX: PlayTransient__12CSoundDirectUsPC10tagLVectorUsUl (SNDDRCT.CPP:18, 0x8008D5B0)
    // Creates a transient sound via factory, triggers it, destroys it.
    // soundId: global sample ID (index into g_transData)
    // posPtr: 3D position (can be null)
    // pan: pan value
    // flags: Initialize flags
    static s32 PlayTransient(u16 soundId, void* posPtr, u16 pan, u32 flags);

    // PSX: BeginPersistent__12CSoundDirectUcPP23CGenericPersistentSoundPC10tagLVector (SNDDRCT.CPP:45, 0x8008D650)
    // Creates a persistent sound via factory, initializes and begins playback.
    static s32 BeginPersistent(u8 persistId, CGenericPersistentSound** outObj, void* posPtr);

    // PSX: EndPersistent__12CSoundDirectPP23CGenericPersistentSound (SNDDRCT.CPP:71, 0x8008D6C4)
    // Ends and destroys the persistent sound, sets pointer to null.
    static s32 EndPersistent(CGenericPersistentSound** obj);
};
