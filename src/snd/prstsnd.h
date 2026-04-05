// prstsnd.h - CGenericPersistentSound reversed from PSX PRSTSND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\PRSTSND.CPP
// CGenericPersistentSound is created by the factory for looping sound playback.
// It reads parameters from g_persistData and creates an rsdPersistent object.
#pragma once

#include "snd/basesnd.h"

class rsdPersistent;

// PSX: g_persistData (0x800DB2EC) - persistent sound parameter table
// Each entry is 2 ints (8 bytes), accessed as byte array:
//   byte 0: rsd sample ID
//   byte 4: volume (0-100)
//   byte 5: pitch (0-200, 100=normal)
//   byte 6: reverb
extern u32 g_persistData[92];

// PSX: CGenericPersistentSound (24 bytes) - inherits CSound (16 bytes)
// PSX layout:
//   +0:  CSound base (16 bytes)
//   +16: loadByte0 (u8)
//   +17: persistId (u8, init=0xFF, index into g_persistData)
//   +18-19: padding
//   +20: rsdPersistentPtr (rsdPersistent*, init=0)
class CGenericPersistentSound : public CSound {
public:
    u8 loadByte0;           // +16
    u8 persistId;           // +17: index into g_persistData (init=0xFF)
    u16 pad18;              // +18
    rsdPersistent* persist; // +20: active rsdPersistent object

    // PSX: _23CGenericPersistentSound (PRSTSND.CPP:18, 0x800AC280)
    CGenericPersistentSound();

    // PSX: __23CGenericPersistentSound (PRSTSND.CPP:22, 0x800AC2CC)
    ~CGenericPersistentSound() override;

    // PSX: Initialize__23CGenericPersistentSoundPC10tagLVectorUs (PRSTSND.CPP:45, 0x800AC3AC)
    s32 Initialize(void* posPtr, u16 flags);

    // PSX: Initialize__23CGenericPersistentSoundPC10tagLVector (PRSTSND.CPP:117, 0x800AC564)
    s32 Initialize(void* posPtr);

    // PSX: Begin__23CGenericPersistentSound (PRSTSND.CPP:51, 0x800AC3CC)
    s32 Begin();

    // PSX: End__23CGenericPersistentSound (PRSTSND.CPP:77, 0x800AC498)
    s32 End();

    // PSX: SetVol__23CGenericPersistentSoundUc (PRSTSND.CPP:27, 0x800AC328)
    s32 SetVol(u8 vol);

    // PSX: Load__23CGenericPersistentSoundPCc (PRSTSND.CPP:93, 0x800AC4F8)
    s32 Load(const void* data);
};
