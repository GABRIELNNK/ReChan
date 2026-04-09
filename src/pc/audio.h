#pragma once
#include "core.h"

// Opaque handle types
using AudioSample = u32;  // handle to a loaded sample in the engine
using AudioVoice = u32;   // handle to a playing voice
static constexpr AudioSample AUDIO_SAMPLE_INVALID = 0;
static constexpr AudioVoice AUDIO_VOICE_INVALID = 0;

// AudioEngine - PC audio abstraction over miniaudio
// All game code accesses audio through this interface only
class AudioEngine {
public:
    // Init/shutdown
    static bool Init();
    static void Shutdown();
    static bool IsInitialized();

    // Sample management (preloaded PCM in memory)
    // data: interleaved s16 PCM, takes ownership via copy
    static AudioSample LoadSample(const s16* data, u32 numFrames, u32 sampleRate, u32 channels);
    static void UnloadSample(AudioSample handle);
    static void UnloadAllSamples();

    // Voice playback
    static AudioVoice PlaySample(AudioSample sample, f32 volume = 1.0f, f32 pan = 0.0f, bool loop = false);
    static void StopVoice(AudioVoice voice);
    static void StopAllVoices();
    static bool IsVoicePlaying(AudioVoice voice);
    static void SetVoiceVolume(AudioVoice voice, f32 volume);
    static void SetVoicePan(AudioVoice voice, f32 pan);
    static void SetVoicePitch(AudioVoice voice, f32 pitch);

    // Streaming music (reads from file, decoded on the fly)
    static bool PlayMusic(const char* path, f32 volume = 1.0f, bool loop = true);
    static void StopMusic();
    static void SetMusicVolume(f32 volume);
    static bool IsMusicPlaying();

    // Master volume
    static void SetMasterVolume(f32 volume);
    static f32 GetMasterVolume();
};
