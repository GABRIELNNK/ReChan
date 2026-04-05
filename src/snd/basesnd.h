// basesnd.h - CSound base class reversed from PSX BASESND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\BASESND.CPP
// CSound is the base class for all sound objects.
// On PSX it uses CSoundFactory + SPU ADPCM; on PC it maps to Sound (WAX/miniaudio).
#pragma once

#include "gen/common.h"

// PSX: CSound (16 bytes)
// PSX layout:
//   +0:  refCount (u32, init=1)
//   +4:  posPtr (void*, position vector, init=0)
//   +8:  flags (u16, init=0)
//   +12: factoryTable (void*, vtable ptr)
class CSound {
public:
    u32 refCount;   // +0
    void* posPtr;   // +4: LVector* position (unused on PC frontend sounds)
    u16 flags;      // +8
    u32 pad12;      // +12: PSX factory table ptr (not used on PC)

    // PSX: __6CSound (BASESND.CPP:31, 0x800A1E18)
    CSound();

    // PSX: _._6CSound
    virtual ~CSound();

    // PSX: Release__6CSound (BASESND.CPP:174, 0x800A1F54)
    // Decrements refCount by 2, destroys if <= 0.
    void Release();

    // PSX: PlayTransient__6CSoundUsUlUs (BASESND.CPP:309, 0x800A2088)
    // Creates a transient sound object with the given global sample ID.
    // flags: bit 3 (0x08) = trigger mode
    // pan: stereo pan (0 = center on PSX)
    // On PC: maps global sample ID to WAX bank/sample and plays via Sound.
    s32 PlayTransient(u16 soundId, u32 triggerFlags, u16 pan);

    // PSX: PlayTransientStereo__6CSoundUsUs (BASESND.CPP:360, 0x800A2164)
    // Creates two transient sounds for left/right stereo.
    // sndL goes to left channel (pan 100,0), sndR to right (pan 0,100).
    // On PC: plays both samples with appropriate panning.
    s32 PlayTransientStereo(u16 sndL, u16 sndR);

    // PC helper: convert PSX global sample ID to WAX (bank, sample) pair.
    // PSX WAX banks are loaded sequentially (RS0000-RS0015), samples are
    // numbered globally starting from 0. Returns false if ID is out of range.
    static bool GlobalSampleToWax(u16 globalId, u32& outBank, u32& outSample);
};
