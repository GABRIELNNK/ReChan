#pragma once

#include "snd/basesnd.h"

class CGenericPersistentSound;

// PSX: CParticleEffectSound (24 bytes) - particle persistent sound script.
class CParticleEffectSound : public CSound {
public:
    u8 loadByte0;            // +16
    s8 persistentId;         // +17
    u16 pad18;               // +18
    CGenericPersistentSound* persistent; // +20

    CParticleEffectSound();
    ~CParticleEffectSound() override;

    s32 Initialize(const LVector* pos);
    s32 Load(const void* data);
    s32 StartAnimating();
    s32 StopAnimating();
};

// PSX: CWorldEffectSound (32 bytes) - world effect ambient/persistent sound script.
class CWorldEffectSound : public CSound {
public:
    u8 loadByte0;            // +16
    s8 persistentId;         // +17
    u16 startFrame;          // +18
    u16 endFrame;            // +20
    u16 soundFlags;          // +22
    u16 updateFlags;         // +24 (bit 0 enables volume rise/fall)
    u16 pad26;               // +26
    CGenericPersistentSound* persistent; // +28

    CWorldEffectSound();
    ~CWorldEffectSound() override;

    s32 Initialize(const LVector* pos);
    s32 Load(const void* data);
    s32 Update(u32 frame);
    s32 StartAnimating();
    s32 StopAnimating();
    s32 VolRiseAndFall(u32 frame);
};
