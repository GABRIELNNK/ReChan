#pragma once
#include "snd/basesnd.h"

// CDirectorSound handles NIS (non-interactive sequence) sound events.
// It extends CSound with a persistent sound slot for looping NIS audio.

class CGenericPersistentSound;

// CDirectorSound (20 bytes on PSX) - inherits CSound (16 bytes)
// PSX layout:
//   +0:  CSound base (16 bytes)
//   +16: persistentSound (CGenericPersistentSound*, init=0)
class CDirectorSound : public CSound {
public:
    CGenericPersistentSound* persistentSound;  // +16

    CDirectorSound();
    ~CDirectorSound() override;
    void Initialize();

    // eventType: 1=PlayTransient, 2=BeginPersistent, 3=EndPersistent
    // soundId: global sample ID / persist ID
    s32 ProcessNISEvent(u32 eventType, u16 soundId);
};
