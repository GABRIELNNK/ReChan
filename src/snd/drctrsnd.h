// drctrsnd.h - CDirectorSound class reversed from PSX DRCTRSND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\DRCTRSND.CPP
// CDirectorSound handles NIS (non-interactive sequence) sound events.
// It extends CSound with a persistent sound slot for looping NIS audio.
#pragma once

#include "snd/basesnd.h"

class CGenericPersistentSound;

// CDirectorSound (20 bytes on PSX) - inherits CSound (16 bytes)
// PSX layout:
//   +0:  CSound base (16 bytes)
//   +16: persistentSound (CGenericPersistentSound*, init=0)
class CDirectorSound : public CSound {
public:
    CGenericPersistentSound* persistentSound;  // +16

    // PSX: __14CDirectorSound (DRCTRSND.CPP:40, 0x8008EBAC)
    CDirectorSound();

    // PSX: _._14CDirectorSound (DRCTRSND.CPP:47, 0x8008EBE4)
    ~CDirectorSound() override;

    // PSX: Initialize__14CDirectorSound (DRCTRSND.CPP:12, 0x8008EB24)
    void Initialize();

    // PSX: ProcessNISEvent__14CDirectorSoundUlUl (DRCTRSND.CPP:21, 0x8008EB44)
    // eventType: 1=PlayTransient, 2=BeginPersistent, 3=EndPersistent
    // soundId: global sample ID / persist ID
    s32 ProcessNISEvent(u32 eventType, u16 soundId);
};
