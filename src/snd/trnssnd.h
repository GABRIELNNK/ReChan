// trnssnd.h - CGenericTransientSound reversed from PSX TRNSSND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\TRNSSND.CPP
// CGenericTransientSound is created by the factory for one-shot sound playback.
// It reads parameters from g_transData and calls rsdWorld to play.
#pragma once

#include "snd/basesnd.h"

// PSX: g_transData (0x800DAACC) - transient sound parameter table
// Each entry is 2 ints (8 bytes), accessed as byte array:
//   byte 0: rsd sample ID (for rsdWorld)
//   byte 4: volume (0-100)
//   byte 5: volume variation (0-100, random reduction)
//   byte 6: pitch (0-200, 100=normal)
//   byte 7: pitch variation (random +/- range)
extern u32 g_transData[512];

// PSX: CGenericTransientSound (28 bytes) - inherits CSound (16 bytes)
// PSX layout:
//   +0:  CSound base (16 bytes)
//   +16: loadData (u32, set by Load - contains soundId at +18)
//   +20: leftVol (u8, init=100)
//   +21: rightVol (u8, init=100)
//   +22-27: padding
class CGenericTransientSound : public CSound {
public:
    union {
        u32 loadData;       // +16: raw load data (4 bytes)
        struct {
            u8 loadByte0;   // +16
            u8 loadByte1;   // +17
            u16 soundId;    // +18: index into g_transData
        };
    };
    u8 leftVol;             // +20: left volume scale (0-100)
    u8 rightVol;            // +21: right volume scale (0-100)

    // PSX: _22CGenericTransientSound (TRNSSND.CPP:18, 0x800AA804)
    CGenericTransientSound();

    // PSX: __22CGenericTransientSound (TRNSSND.CPP:27, 0x800AA84C)
    ~CGenericTransientSound() override;

    // PSX: Initialize__22CGenericTransientSoundPC10tagLVectorUs (TRNSSND.CPP:43, 0x800AA8A0)
    s32 Initialize(void* posPtr, u16 flags);

    // PSX: InitializeStereo__22CGenericTransientSoundUcUc (TRNSSND.CPP:54, 0x800AA8C0)
    s32 InitializeStereo(u8 left, u8 right);

    // PSX: Trigger__22CGenericTransientSoundUs (TRNSSND.CPP:62, 0x800AA8E8)
    s32 Trigger(u16 pan);

    // PSX: TriggerDialogWorld__22CGenericTransientSoundUs (TRNSSND.CPP:74, 0x800AA938)
    s32 TriggerDialogWorld(u16 pan);

    // PSX: TriggerPositional__22CGenericTransientSoundUs (TRNSSND.CPP:104, 0x800AAA68)
    s32 TriggerPositional(u16 pan);

    // PSX: TriggerNotPositional__22CGenericTransientSoundUs (TRNSSND.CPP:134, 0x800AAB9C)
    s32 TriggerNotPositional(u16 pan);

    // PSX: Load__22CGenericTransientSoundPCc (TRNSSND.CPP:169, 0x800AAD4C)
    s32 Load(const void* data);
};
