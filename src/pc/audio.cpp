#include "gen/common.h"
#include "pc/audio.h"
#include "miniaudio.h"
#include <vector>
#include <mutex>

// Internal sample storage
struct InternalSample {
    s16* data = nullptr;
    u32 numFrames = 0;
    u32 sampleRate = 0;
    u32 channels = 0;
};

// Internal voice
struct InternalVoice {
    AudioSample sample = AUDIO_SAMPLE_INVALID;
    f64 positionF = 0.0; // fractional frame position (for sample rate conversion)
    f32 volume = 1.0f;
    f32 pan = 0.0f;
    f32 pitch = 1.0f;
    bool loop = false;
    bool active = false;
};

// Engine state
static constexpr u32 MAX_SAMPLES = 4096;
static constexpr u32 MAX_VOICES = 32;
static constexpr u32 ENGINE_SAMPLE_RATE = 44100; // fallback, actual rate from device
static constexpr u32 ENGINE_CHANNELS = 2;

static ma_device g_device;
static bool g_initialized = false;
static f32 g_masterVolume = 1.0f;
static u32 g_deviceSampleRate = ENGINE_SAMPLE_RATE; // actual device sample rate

static InternalSample g_samples[MAX_SAMPLES];
static u32 g_nextSampleId = 1;

static InternalVoice g_voices[MAX_VOICES];
static u32 g_nextVoiceId = 1;
static std::mutex g_voiceMutex;

// Music stream state
static ma_decoder g_musicDecoder;
static bool g_musicActive = false;
static bool g_musicLoop = false;
static f32 g_musicVolume = 1.0f;
static std::mutex g_musicMutex;

// Audio callback - mixes all active voices + music into output
static void audioCallback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount) {
    f32* out = (f32*)output;
    memset(out, 0, frameCount * ENGINE_CHANNELS * sizeof(f32));

    // Mix voices
    {
        std::lock_guard<std::mutex> lock(g_voiceMutex);
        for (u32 v = 0; v < MAX_VOICES; v++) {
            InternalVoice& voice = g_voices[v];
            if (!voice.active) continue;

            AudioSample sid = voice.sample;
            if (sid == AUDIO_SAMPLE_INVALID || sid > MAX_SAMPLES) {
                voice.active = false;
                continue;
            }
            InternalSample& smp = g_samples[sid - 1];
            if (!smp.data) {
                voice.active = false;
                continue;
            }

            f32 vol = voice.volume * g_masterVolume;
            f32 panL = (voice.pan <= 0.0f) ? 1.0f : (1.0f - voice.pan);
            f32 panR = (voice.pan >= 0.0f) ? 1.0f : (1.0f + voice.pan);

            // Rate ratio: how many source frames per output frame
            f64 rateRatio = (f64)smp.sampleRate / (f64)g_deviceSampleRate;
            f64 advance = rateRatio * (f64)voice.pitch;

            for (u32 i = 0; i < frameCount; i++) {
                u32 pos = (u32)voice.positionF;
                if (pos >= smp.numFrames) {
                    if (voice.loop) {
                        voice.positionF = 0.0;
                        pos = 0;
                    }
                    else {
                        voice.active = false;
                        break;
                    }
                }

                f32 sampleL, sampleR;
                if (smp.channels == 1) {
                    f32 s = smp.data[pos] / 32768.0f;
                    sampleL = s;
                    sampleR = s;
                }
                else {
                    sampleL = smp.data[pos * 2] / 32768.0f;
                    sampleR = smp.data[pos * 2 + 1] / 32768.0f;
                }

                out[i * 2] += sampleL * vol * panL;
                out[i * 2 + 1] += sampleR * vol * panR;

                voice.positionF += advance;
            }
        }
    }

    // Mix music
    {
        std::lock_guard<std::mutex> lock(g_musicMutex);
        if (g_musicActive) {
            // Read into temp buffer as f32
            f32 temp[4096];
            u32 remaining = frameCount;
            u32 offset = 0;
            while (remaining > 0) {
                u32 toRead = remaining;
                if (toRead > 2048) toRead = 2048; // 2048 frames * 2 channels = 4096 floats
                ma_uint64 framesRead = 0;
                ma_decoder_read_pcm_frames(&g_musicDecoder, temp, toRead, &framesRead);
                if (framesRead == 0) {
                    if (g_musicLoop) {
                        ma_decoder_seek_to_pcm_frame(&g_musicDecoder, 0);
                        continue;
                    }
                    else {
                        g_musicActive = false;
                        break;
                    }
                }
                f32 mvol = g_musicVolume * g_masterVolume;
                for (u32 i = 0; i < (u32)framesRead * ENGINE_CHANNELS; i++) {
                    out[offset + i] += temp[i] * mvol;
                }
                offset += (u32)framesRead * ENGINE_CHANNELS;
                remaining -= (u32)framesRead;
            }
        }
    }

    // Clamp output
    for (u32 i = 0; i < frameCount * ENGINE_CHANNELS; i++) {
        if (out[i] > 1.0f) out[i] = 1.0f;
        if (out[i] < -1.0f) out[i] = -1.0f;
    }
}

// AudioEngine implementation

bool AudioEngine::Init() {
    if (g_initialized) return true;

    // Clear state before starting device (callback may fire immediately)
    memset(g_samples, 0, sizeof(g_samples));
    memset(g_voices, 0, sizeof(g_voices));
    g_nextSampleId = 1;
    g_nextVoiceId = 1;
    g_masterVolume = 1.0f;
    g_musicActive = false;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = ENGINE_CHANNELS;
    config.sampleRate = 0; // use native device rate to avoid resampling
    config.dataCallback = audioCallback;
    config.periodSizeInFrames = 512;
    config.periods = 3;

    if (ma_device_init(nullptr, &config, &g_device) != MA_SUCCESS) {
        LOG("AudioEngine: failed to init device");
        return false;
    }

    // Store actual device sample rate (may differ from ENGINE_SAMPLE_RATE)
    g_deviceSampleRate = g_device.sampleRate;

    if (ma_device_start(&g_device) != MA_SUCCESS) {
        LOG("AudioEngine: failed to start device");
        ma_device_uninit(&g_device);
        return false;
    }

    g_initialized = true;

    LOG("AudioEngine: initialized (device %u Hz, %u ch, period %u frames)",
        g_deviceSampleRate, ENGINE_CHANNELS, config.periodSizeInFrames);
    return true;
}

void AudioEngine::Shutdown() {
    if (!g_initialized) return;

    StopAllVoices();
    StopMusic();
    UnloadAllSamples();

    ma_device_uninit(&g_device);
    g_initialized = false;

    LOG("AudioEngine: shutdown");
}

bool AudioEngine::IsInitialized() {
    return g_initialized;
}

AudioSample AudioEngine::LoadSample(const s16* data, u32 numFrames, u32 sampleRate, u32 channels) {
    if (!g_initialized || !data || numFrames == 0) return AUDIO_SAMPLE_INVALID;

    // Find free slot
    for (u32 i = 0; i < MAX_SAMPLES; i++) {
        if (g_samples[i].data == nullptr) {
            u32 totalSamples = numFrames * channels;
            g_samples[i].data = new s16[totalSamples];
            memcpy(g_samples[i].data, data, totalSamples * sizeof(s16));
            g_samples[i].numFrames = numFrames;
            g_samples[i].sampleRate = sampleRate;
            g_samples[i].channels = channels;
            return i + 1; // 1-based handle
        }
    }

    LOG("AudioEngine: no free sample slots");
    return AUDIO_SAMPLE_INVALID;
}

void AudioEngine::UnloadSample(AudioSample handle) {
    if (handle == AUDIO_SAMPLE_INVALID || handle > MAX_SAMPLES) return;
    InternalSample& smp = g_samples[handle - 1];
    delete[] smp.data;
    smp.data = nullptr;
    smp.numFrames = 0;
}

void AudioEngine::UnloadAllSamples() {
    for (u32 i = 0; i < MAX_SAMPLES; i++) {
        delete[] g_samples[i].data;
        g_samples[i].data = nullptr;
        g_samples[i].numFrames = 0;
    }
}

AudioVoice AudioEngine::PlaySample(AudioSample sample, f32 volume, f32 pan, bool loop) {
    if (!g_initialized || sample == AUDIO_SAMPLE_INVALID) return AUDIO_VOICE_INVALID;

    std::lock_guard<std::mutex> lock(g_voiceMutex);

    // Find free voice slot
    for (u32 i = 0; i < MAX_VOICES; i++) {
        if (!g_voices[i].active) {
            g_voices[i].sample = sample;
            g_voices[i].positionF = 0.0;
            g_voices[i].volume = volume;
            g_voices[i].pan = pan;
            g_voices[i].pitch = 1.0f;
            g_voices[i].loop = loop;
            g_voices[i].active = true;
            return i + 1; // 1-based handle
        }
    }

    LOG("AudioEngine: no free voice slots");
    return AUDIO_VOICE_INVALID;
}

void AudioEngine::StopVoice(AudioVoice voice) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].active = false;
}

void AudioEngine::StopAllVoices() {
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    for (u32 i = 0; i < MAX_VOICES; i++) {
        g_voices[i].active = false;
    }
}

bool AudioEngine::IsVoicePlaying(AudioVoice voice) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return false;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    return g_voices[voice - 1].active;
}

void AudioEngine::SetVoiceVolume(AudioVoice voice, f32 volume) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].volume = volume;
}

void AudioEngine::SetVoicePan(AudioVoice voice, f32 pan) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].pan = pan;
}

void AudioEngine::SetVoicePitch(AudioVoice voice, f32 pitch) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].pitch = pitch;
}

bool AudioEngine::PlayMusic(const char* path, f32 volume, bool loop) {
    if (!g_initialized || !path) return false;

    StopMusic();

    std::lock_guard<std::mutex> lock(g_musicMutex);

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, ENGINE_CHANNELS, g_deviceSampleRate);
    if (ma_decoder_init_file(path, &decoderConfig, &g_musicDecoder) != MA_SUCCESS) {
        LOG("AudioEngine: failed to load music '%s'", path);
        return false;
    }

    g_musicVolume = volume;
    g_musicLoop = loop;
    g_musicActive = true;

    LOG("AudioEngine: playing music '%s'", path);
    return true;
}

void AudioEngine::StopMusic() {
    std::lock_guard<std::mutex> lock(g_musicMutex);
    if (g_musicActive) {
        g_musicActive = false;
        ma_decoder_uninit(&g_musicDecoder);
    }
}

void AudioEngine::SetMusicVolume(f32 volume) {
    std::lock_guard<std::mutex> lock(g_musicMutex);
    g_musicVolume = volume;
}

bool AudioEngine::IsMusicPlaying() {
    std::lock_guard<std::mutex> lock(g_musicMutex);
    return g_musicActive;
}

void AudioEngine::SetMasterVolume(f32 volume) {
    g_masterVolume = volume;
}

f32 AudioEngine::GetMasterVolume() {
    return g_masterVolume;
}
