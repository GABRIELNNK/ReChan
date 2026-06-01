#pragma once

#include "snd/basesnd.h"

class CGenericPersistentSound;

class CPushableSound : public CSound {
public:
    u16 pad16 = 0;                            // +16
    u16 sfxID = 0xFFFF;                       // +18 transient kick/hit sound effect ID
    u8  loopSoundId = 0xFF;                   // +20 persistent push loop sound ID
    u8  pad21 = 0;                            // +21
    u16 pad22 = 0;                            // +22
    u32 material = 0;                         // +24 floor material (returned by GetMaterial)
    CGenericPersistentSound* pushPersistent = nullptr; // +28 active looping push sound
    u16 kickCooldown = 0;                     // +32 frames until next kick sound is allowed
    u16 pad34 = 0;                            // +34

    CPushableSound();
    ~CPushableSound() override;

    s32 Load(const void* data);
    s32 Initialize(const LVector* pos);
    void Think();
    void BeginPush();
    void EndPush();
    void Kick();
    void HitHumanoid();
    void GetMaterial(s32* outMaterial) const;
};
