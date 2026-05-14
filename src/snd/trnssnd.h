#pragma once
#include "snd/basesnd.h"

// PSX: g_transData (0x800DAACC) - transient sound parameter table
// Each entry is 2 ints (8 bytes), accessed as byte array:
//   byte 0: rsd sample ID (for rsdWorld)
//   byte 4: volume (0-100)
//   byte 5: volume variation (0-100, random reduction)
//   byte 6: pitch (0-200, 100=normal)
//   byte 7: pitch variation (random +/- range)
extern u32 g_transData[520];

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

    CGenericTransientSound();
    ~CGenericTransientSound() override;
    s32 Initialize(void* posPtr, u16 flags);
    s32 InitializeStereo(u8 left, u8 right);
    s32 Trigger(u16 pan);
    s32 TriggerDialogWorld(u16 pan);
    s32 TriggerPositional(u16 pan);
    s32 TriggerNotPositional(u16 pan);
    s32 Load(const void* data);
};
