#pragma once
#include "snd/basesnd.h"

// CGenericPersistentSound is created by the factory for looping sound playback.
// It reads parameters from g_persistData and creates an rsdPersistent object.

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

    CGenericPersistentSound();
    ~CGenericPersistentSound() override;
    s32 Initialize(void* posPtr, u16 flags);
    s32 Initialize(void* posPtr);
    s32 Begin();
    s32 End();
    s32 SetVol(u8 vol);
    s32 Load(const void* data);
};
