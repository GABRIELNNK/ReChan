#include "snd/drctrsnd.h"
#include "snd/prstsnd.h"

// PSX: __14CDirectorSound (DRCTRSND.CPP:40, 0x8008EBAC)
CDirectorSound::CDirectorSound() {
    MARKFUNCTION(0x8008EBAC);
    persistentSound = nullptr;
}

// PSX: _._14CDirectorSound (DRCTRSND.CPP:47, 0x8008EBE4)
CDirectorSound::~CDirectorSound() {
    MARKFUNCTION(0x8008EBE4);
    // PSX: EndPersistent(&persistentSound), then ~CSound
    EndPersistent(&persistentSound);
}

// PSX: Initialize__14CDirectorSound (DRCTRSND.CPP:12, 0x8008EB24)
void CDirectorSound::Initialize() {
    MARKFUNCTION(0x8008EB24);
    // PSX: Initialize__6CSoundPC10tagLVector(this, 0)
    CSound::Initialize(nullptr);
}

// PSX: ProcessNISEvent__14CDirectorSoundUlUl (DRCTRSND.CPP:21, 0x8008EB44)
s32 CDirectorSound::ProcessNISEvent(u32 eventType, u16 soundId) {
    MARKFUNCTION(0x8008EB44);

    if (eventType == 1) {
        // PSX: PlayTransient__6CSoundUsUlUs(this, soundId, 8, 0)
        return PlayTransient(soundId, 8, 0);
    }

    if (eventType == 2) {
        // PSX: BeginPersistent__6CSoundUcPP23CGenericPersistentSound(this, soundId, &persistentSound)
        return BeginPersistent((u8)soundId, &persistentSound);
    }

    if (eventType == 3) {
        // PSX: EndPersistent__6CSoundPP23CGenericPersistentSound(this, &persistentSound)
        return EndPersistent(&persistentSound);
    }

    return -1000;
}
