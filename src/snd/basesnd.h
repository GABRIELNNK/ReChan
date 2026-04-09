#pragma once
#include "core.h"

// CSound is the base class for all sound objects.

class CGenericPersistentSound;

// PSX: CSound (16 bytes)
// PSX layout:
//   +0:  refCount (u32, init=1)
//   +4:  posPtr (void*, position vector, init=0)
//   +8:  flags (u16, init=0)
//   +12: factoryTable (void*, vtable ptr)
class CSound {
public:
    u32 refCount;   // +0
    void* posPtr;   // +4: LVector* position
    u16 flags;      // +8
    u32 pad12;      // +12: PSX factory table ptr (not used on PC)

    CSound();
    virtual ~CSound();
    s32 Initialize(void* pos);
    void* GetPosPtr() const;
    void Release();

    // Creates a CGenericTransientSound via factory, Initialize, Trigger/TriggerDialogWorld, destroy.
    s32 PlayTransient(u16 soundId, u32 triggerFlags, u16 pan);

    // Creates two CGenericTransientSounds for left/right stereo playback.
    s32 PlayTransientStereo(u16 sndL, u16 sndR);

    // Creates a CGenericPersistentSound, initializes and begins looping playback.
    s32 BeginPersistent(u8 soundId, CGenericPersistentSound** outObj);

    // Ends the persistent sound and sets the pointer to null.
    s32 EndPersistent(CGenericPersistentSound** obj);
};
