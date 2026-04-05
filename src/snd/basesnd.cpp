// basesnd.cpp - CSound base class reversed from PSX BASESND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\BASESND.CPP
#include "snd/basesnd.h"
#include "snd/sound.h"

// PSX: __6CSound (BASESND.CPP:31, 0x800A1E18)
CSound::CSound() {
    MARKFUNCTION(0x800A1E18);
    // PSX: refCount=1, posPtr=0, flags=0, factoryTable=0x800D3BA8
    refCount = 1;
    posPtr = nullptr;
    flags = 0;
    pad12 = 0;
    // PSX: calls ObjectCreated__13CSoundFactory (bookkeeping, no-op on PC)
}

CSound::~CSound() {
}

// PSX: Release__6CSound (BASESND.CPP:174, 0x800A1F54)
// PSX: refCount -= 2; if (refCount <= 0 && this != null) vtable[12]->destroy(3)
void CSound::Release() {
    MARKFUNCTION(0x800A1F54);
    if (refCount >= 2) {
        refCount -= 2;
    } else {
        refCount = 0;
    }
    // PSX: destroy via vtable if refCount <= 0
    // PC: caller manages lifetime
}

// PC helper: convert PSX global sample ID to WAX bank/sample pair
// PSX loads banks RS0000..RS0015 sequentially. Each bank has a variable
// number of samples. The global sample ID is a flat index across all banks.
bool CSound::GlobalSampleToWax(u16 globalId, u32& outBank, u32& outSample) {
    if (!g_sound) return false;

    u32 remaining = globalId;
    for (u32 bank = 0; bank < MAX_WAX_BANKS; bank++) {
        u32 count = g_sound->GetBankSampleCount(bank);
        if (count == 0) continue;
        if (remaining < count) {
            outBank = bank;
            outSample = remaining;
            return true;
        }
        remaining -= count;
    }
    return false;
}

// PSX: PlayTransient__6CSoundUsUlUs (BASESND.CPP:309, 0x800A2088)
// PSX flow:
//   CreateObject(10070, &tmpObj, soundId)
//   Initialize(tmpObj, this->posPtr, this->flags)
//   if (triggerFlags & 8):
//     if (triggerFlags & 1): Trigger(tmpObj, pan)
//     else: TriggerDialogWorld(tmpObj, pan)
//   Release(tmpObj)
// PC: map global sample ID to WAX bank/sample, play via Sound
s32 CSound::PlayTransient(u16 soundId, u32 triggerFlags, u16 pan) {
    MARKFUNCTION(0x800A2088);

    if (!g_sound) return -1000;

    u32 bank, sample;
    if (!GlobalSampleToWax(soundId, bank, sample)) {
        return -1000; // PSX: returns error if CreateObject fails
    }

    // PSX: Initialize sets position and flags, then Trigger plays the sound
    // Only play if trigger flag bit 3 is set (PSX: flags & 0x08)
    if (triggerFlags & 0x08) {
        // PSX pan is 0-127 center, normalized to -1..1 for PC
        f32 pcPan = 0.0f;
        if (pan > 0) {
            pcPan = ((f32)pan - 64.0f) / 64.0f;
        }
        g_sound->PlayWaxSample(bank, sample, 1.0f, pcPan);
    }

    return 0;
}

// PSX: PlayTransientStereo__6CSoundUsUs (BASESND.CPP:360, 0x800A2164)
// PSX flow:
//   CreateObject(10070, &objL, sndL)
//   CreateObject(10070, &objR, sndR)
//   InitializeStereo(objL, 100, 0)  // left channel
//   InitializeStereo(objR, 0, 100)  // right channel
//   Trigger(objL, 0)
//   Trigger(objR, 0)
//   Release(objL), Release(objR)
// PC: play both samples with stereo panning
s32 CSound::PlayTransientStereo(u16 sndL, u16 sndR) {
    MARKFUNCTION(0x800A2164);

    if (!g_sound) return -1000;

    u32 bankL, sampleL;
    u32 bankR, sampleR;

    if (GlobalSampleToWax(sndL, bankL, sampleL)) {
        // PSX: InitializeStereo(100, 0) = full left
        g_sound->PlayWaxSample(bankL, sampleL, 1.0f, -1.0f);
    }

    if (GlobalSampleToWax(sndR, bankR, sampleR)) {
        // PSX: InitializeStereo(0, 100) = full right
        g_sound->PlayWaxSample(bankR, sampleR, 1.0f, 1.0f);
    }

    return 0;
}
