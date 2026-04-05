// basesnd.h - CSound base class reversed from PSX BASESND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\BASESND.CPP
// CSound is the base class for all sound objects.
#pragma once

#include "gen/common.h"

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

    // PSX: __6CSound (BASESND.CPP:31, 0x800A1E18)
    CSound();

    // PSX: _._6CSound
    virtual ~CSound();

    // PSX: Initialize__6CSoundPC10tagLVector (BASESND.CPP:152, 0x800A1F48)
    s32 Initialize(void* pos);

    // PSX: GetPosPtr__C6CSound (BASESND.CPP:202, 0x800A1FAC)
    void* GetPosPtr() const;

    // PSX: Release__6CSound (BASESND.CPP:174, 0x800A1F54)
    void Release();

    // PSX: PlayTransient__6CSoundUsUlUs (BASESND.CPP:309, 0x800A2088)
    // Creates a CGenericTransientSound via factory, Initialize, Trigger/TriggerDialogWorld, destroy.
    s32 PlayTransient(u16 soundId, u32 triggerFlags, u16 pan);

    // PSX: PlayTransientStereo__6CSoundUsUs (BASESND.CPP:360, 0x800A2164)
    // Creates two CGenericTransientSounds for left/right stereo playback.
    s32 PlayTransientStereo(u16 sndL, u16 sndR);

    // PSX: BeginPersistent__6CSoundUcPP23CGenericPersistentSound (BASESND.CPP:229, 0x800A1FB8)
    // Creates a CGenericPersistentSound, initializes and begins looping playback.
    s32 BeginPersistent(u8 soundId, CGenericPersistentSound** outObj);

    // PSX: EndPersistent__6CSoundPP23CGenericPersistentSound (BASESND.CPP:275, 0x800A2038)
    // Ends the persistent sound and sets the pointer to null.
    s32 EndPersistent(CGenericPersistentSound** obj);
};
