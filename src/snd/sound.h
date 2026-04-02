// sound.h - Sound manager reversed from PSX SOUND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\SOUND.CPP
// Manages sound/music playback. On PSX this interfaces with SPU hardware.
// PC: uses AudioEngine (miniaudio wrapper) for actual playback.
#pragma once

#include "gen/manager.h"
#include "pc/audio.h"

// Max WAX banks (RS0000..RS0015 + extras)
static constexpr u32 MAX_WAX_BANKS = 24;
static constexpr u32 MAX_SAMPLES_PER_BANK = 128;

// Sound (44 bytes on PSX) - audio manager
// PSX layout:
//   +0:  Manager base (28 bytes: ccNode(24) + isOpen(s16))
//   +28: callback ref (u32)
//   +32: flags (3 x s16, init 0xFFFF)
//   +38: padding
//   +40: activeFlag (u32, init 1)
class Sound : public Manager {
public:
    s16 flag0 = -1;     // +32: init 0xFFFF
    s16 flag1 = -1;     // +34: init 0xFFFF
    s16 flag2 = -1;     // +36: init 0xFFFF
    u32 activeFlag = 1; // +40: 1 = active

    // PC: per-bank sample handles
    struct BankInfo {
        AudioSample samples[MAX_SAMPLES_PER_BANK] = {};
        u32 numSamples = 0;
    };
    BankInfo banks[MAX_WAX_BANKS] = {};
    u32 numWaxBanks = 0;

    // PC: music sample (decoded FAG loaded as one big sample)
    AudioSample musicSample = AUDIO_SAMPLE_INVALID;
    AudioVoice musicVoice = AUDIO_VOICE_INVALID;
    bool musicPlaying = false;

    // PSX: __5Sound (SOUND.CPP, 0x80059794)
    Sound();

    // PSX: _._5Sound (SOUND.CPP, 0x800597E8)
    ~Sound() override;

    // PSX: InternalOpen__5Sound (0x80059818)
    void InternalOpen() override;

    // PSX: InternalClose__5Sound
    void InternalClose() override;

    // PSX: SetupSound__5Sound (0x800598D8)
    void SetupSound();

    // PSX: CleanupSound__5Sound (0x800599B0)
    void CleanupSound();

    // PC: play a specific sample from a WAX bank
    AudioVoice PlayWaxSample(u32 bankIndex, u32 sampleIndex, f32 volume = 1.0f, f32 pan = 0.0f);
    u32 GetBankSampleCount(u32 bankIndex) const;

    // PC: music control
    bool PlayMusicTrack(const char* fagPath, f32 volume = 1.0f);
    void StopMusic();
    void SetMusicVolume(f32 volume);
};

// PSX: gp-relative global, defined in sound.cpp
extern Sound* g_sound;
