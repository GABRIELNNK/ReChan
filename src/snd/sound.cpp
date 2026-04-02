// sound.cpp - Sound manager reversed from PSX SOUND.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\SOUND.CPP
#include "snd/sound.h"
#include "snd/rsdformat.h"
#include <cstdio>
#include <cstring>

// PSX: gp-relative global
Sound* g_sound = nullptr;

// PSX sample rate constants
// SPU pitch 0x0400 = 11025 Hz (SFX), SPU pitch 0x0800 = 22050 Hz (music)
static constexpr u32 PSX_SFX_RATE = 11025;
static constexpr u32 PSX_MUSIC_RATE = 22050;

// Helper: read entire file into memory
static u8* ReadFileBytes(const char* path, u32& outSize) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return nullptr;
    std::fseek(f, 0, SEEK_END);
    outSize = (u32)std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    u8* data = new u8[outSize];
    std::fread(data, 1, outSize, f);
    std::fclose(f);
    return data;
}

// PSX: __5Sound (SOUND.CPP, 0x80059794)
Sound::Sound() {
    MARKFUNCTION(0x80059794);
    std::memset(banks, 0, sizeof(banks));
}

// PSX: _._5Sound (SOUND.CPP, 0x800597E8)
Sound::~Sound() {
    MARKFUNCTION(0x800597E8);
}

// PSX: InternalOpen__5Sound (0x80059818) - allocates callback nodes for load/unload
void Sound::InternalOpen() {
    MARKFUNCTION(0x80059818);
    // PC: initialize audio engine and load sound data
    // PSX: soundLoadFunc callback calls SetupSound() later; PC: call directly
    AudioEngine::Init();
    SetupSound();
}

// PSX: InternalClose__5Sound
void Sound::InternalClose() {
    // PC: shutdown audio engine
    CleanupSound();
    AudioEngine::Shutdown();
}

// PSX: SetupSound__5Sound (0x800598D8) - loads sound data
void Sound::SetupSound() {
    MARKFUNCTION(0x800598D8);

    // Load WAX sound effect banks
    numWaxBanks = 0;
    u32 totalSamples = 0;
    char path[256];
    for (u32 i = 0; i < 16; i++) {
        std::snprintf(path, sizeof(path), "SOUND/FX/RS%04u.WAX", i);
        u32 fileSize = 0;
        u8* fileData = ReadFileBytes(path, fileSize);
        if (!fileData) continue;

        RsdFormat::WaxBank bank = RsdFormat::LoadWax(fileData, fileSize);
        delete[] fileData;

        u32 count = (u32)bank.pcmSamples.size();
        if (count > MAX_SAMPLES_PER_BANK) count = MAX_SAMPLES_PER_BANK;
        banks[i].numSamples = count;

        for (u32 j = 0; j < count; j++) {
            if (bank.pcmSamples[j].empty()) continue;
            u32 numFrames = (u32)bank.pcmSamples[j].size();
            banks[i].samples[j] = AudioEngine::LoadSample(
                bank.pcmSamples[j].data(), numFrames, PSX_SFX_RATE, 1);
            if (banks[i].samples[j] != AUDIO_SAMPLE_INVALID) {
                totalSamples++;
            }
        }

        if (count > 0) {
            numWaxBanks++;
            RC_LOG("Sound: loaded WAX bank %u (%u samples)", i, count);
        }
    }

    RC_LOG("Sound: loaded %u WAX banks (%u total samples)", numWaxBanks, totalSamples);
}

// PSX: CleanupSound__5Sound (0x800599B0) - unloads sound data
void Sound::CleanupSound() {
    MARKFUNCTION(0x800599B0);

    StopMusic();
    AudioEngine::StopAllVoices();
    AudioEngine::UnloadAllSamples();

    std::memset(banks, 0, sizeof(banks));
    musicSample = AUDIO_SAMPLE_INVALID;
    musicVoice = AUDIO_VOICE_INVALID;
    numWaxBanks = 0;
}

// PC: play a specific sample from a WAX bank
AudioVoice Sound::PlayWaxSample(u32 bankIndex, u32 sampleIndex, f32 volume, f32 pan) {
    if (bankIndex >= MAX_WAX_BANKS) return AUDIO_VOICE_INVALID;
    if (sampleIndex >= banks[bankIndex].numSamples) return AUDIO_VOICE_INVALID;
    AudioSample s = banks[bankIndex].samples[sampleIndex];
    if (s == AUDIO_SAMPLE_INVALID) return AUDIO_VOICE_INVALID;
    return AudioEngine::PlaySample(s, volume, pan, false);
}

u32 Sound::GetBankSampleCount(u32 bankIndex) const {
    if (bankIndex >= MAX_WAX_BANKS) return 0;
    return banks[bankIndex].numSamples;
}

// PC: music - decode FAG to mono PCM, load as AudioSample, play via voice mixer
bool Sound::PlayMusicTrack(const char* fagPath, f32 volume) {
    u32 fileSize = 0;
    u8* fileData = ReadFileBytes(fagPath, fileSize);
    if (!fileData) {
        RC_ERR("Sound: failed to load music '%s'", fagPath);
        return false;
    }

    RsdFormat::FagTrack track = RsdFormat::LoadFag(fileData, fileSize);
    delete[] fileData;

    if (track.pcmData.empty()) {
        RC_ERR("Sound: failed to decode music '%s'", fagPath);
        return false;
    }

    // Stop any existing music
    StopMusic();

    // Load decoded PCM as an AudioSample (stereo for FAG)
    musicSample = AudioEngine::LoadSample(
        track.pcmData.data(), track.numFrames, PSX_MUSIC_RATE, track.channels);
    if (musicSample == AUDIO_SAMPLE_INVALID) {
        RC_ERR("Sound: failed to load music sample");
        return false;
    }

    // Play looped through the voice mixer
    musicVoice = AudioEngine::PlaySample(musicSample, volume, 0.0f, true);
    musicPlaying = (musicVoice != AUDIO_VOICE_INVALID);

    RC_LOG("Sound: playing music '%s' (%u frames, %u ch @ %u Hz, sample=%u, voice=%u)",
        fagPath, track.numFrames, track.channels, PSX_MUSIC_RATE, musicSample, musicVoice);
    return musicPlaying;
}

void Sound::StopMusic() {
    if (musicVoice != AUDIO_VOICE_INVALID) {
        AudioEngine::StopVoice(musicVoice);
        musicVoice = AUDIO_VOICE_INVALID;
    }
    if (musicSample != AUDIO_SAMPLE_INVALID) {
        AudioEngine::UnloadSample(musicSample);
        musicSample = AUDIO_SAMPLE_INVALID;
    }
    musicPlaying = false;
}

void Sound::SetMusicVolume(f32 volume) {
    if (musicVoice != AUDIO_VOICE_INVALID) {
        AudioEngine::SetVoiceVolume(musicVoice, volume);
    }
}

