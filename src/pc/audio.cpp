#include "gen/common.h"
#include "pc/audio.h"
#include "miniaudio.h"
#include <vector>
#include <mutex>
#include <cmath>

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
    f32 fadeStartVolume = 1.0f;
    f32 fadeTargetVolume = 1.0f;
    u32 fadeFramesTotal = 0;
    u32 fadeFramesRemaining = 0;
    f32 pan = 0.0f;
    f32 pitch = 1.0f;
    LVector worldPos = {};
    f32 minDistance = 0.0f;
    f32 maxDistance = 10000.0f;
    bool spatial = false;
    bool applyDistanceAttenuation = true;
    bool loop = false;
    bool active = false;
};

struct ListenerState {
    LVector pos = {};
    s32 yaw16 = 0;
    bool valid = false;
};

// Engine state
static constexpr u32 MAX_SAMPLES = 4096;
static constexpr u32 MAX_VOICES = 32;
static constexpr u32 ENGINE_SAMPLE_RATE = 44100; // fallback, actual rate from device
static constexpr u32 DEFAULT_MIX_CHANNELS = 2;
static constexpr u32 MUSIC_CHANNELS = 2;

static ma_device g_device;
static bool g_initialized = false;
static f32 g_masterVolume = 1.0f;
static u32 g_deviceSampleRate = ENGINE_SAMPLE_RATE; // actual device sample rate
static u32 g_outputChannels = DEFAULT_MIX_CHANNELS;
static bool g_outputMono = false;

static InternalSample g_samples[MAX_SAMPLES];
static u32 g_nextSampleId = 1;

static InternalVoice g_voices[MAX_VOICES];
static u32 g_nextVoiceId = 1;
static std::mutex g_voiceMutex;

static ListenerState g_listener;
static std::mutex g_listenerMutex;

// Music stream state
static ma_decoder g_musicDecoder;
static bool g_musicActive = false;
static bool g_musicLoop = false;
static f32 g_musicVolume = 1.0f;
static std::mutex g_musicMutex;

static inline f32 clampf(f32 v, f32 lo, f32 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void ComputeSpatialGains(
    const InternalVoice& voice,
    const ListenerState& listener,
    u32 outChannels,
    f32 outGains[8]) {
    for (u32 i = 0; i < 8; i++) {
        outGains[i] = 0.0f;
    }

    if (!listener.valid || outChannels == 0) {
        if (outChannels >= 2) {
            outGains[0] = 1.0f;
            outGains[1] = 1.0f;
        }
        else {
            outGains[0] = 1.0f;
        }
        return;
    }

    const f32 dx = (f32)(voice.worldPos.x - listener.pos.x);
    const f32 dy = (f32)(voice.worldPos.y - listener.pos.y);
    const f32 dz = (f32)(voice.worldPos.z - listener.pos.z);
    const f32 distSq = dx * dx + dy * dy + dz * dz;
    const f32 dist = std::sqrt(distSq);

    f32 distGain = 1.0f;
    if (voice.applyDistanceAttenuation && voice.maxDistance > voice.minDistance) {
        if (dist >= voice.maxDistance) {
            distGain = 0.0f;
        }
        else if (dist > voice.minDistance) {
            distGain = (voice.maxDistance - dist) / (voice.maxDistance - voice.minDistance);
        }
    }

    if (distGain <= 0.0f) {
        return;
    }

    f32 invLen = 1.0f;
    if (dist > 0.0001f) {
        invLen = 1.0f / dist;
    }

    const f32 dirX = dx * invLen;
    const f32 dirZ = dz * invLen;

    const f32 yawRad = ((f32)listener.yaw16) * (6.28318530717958647692f / 65536.0f);
    const f32 forwardX = std::sin(yawRad);
    const f32 forwardZ = std::cos(yawRad);
    const f32 rightX = forwardZ;
    const f32 rightZ = -forwardX;

    const f32 front = clampf(dirX * forwardX + dirZ * forwardZ, -1.0f, 1.0f);
    const f32 side = clampf(dirX * rightX + dirZ * rightZ, -1.0f, 1.0f);

    const f32 leftW = clampf((1.0f - side) * 0.5f, 0.0f, 1.0f);
    const f32 rightW = clampf((1.0f + side) * 0.5f, 0.0f, 1.0f);
    const f32 frontW = clampf((1.0f + front) * 0.5f, 0.0f, 1.0f);
    const f32 rearW = 1.0f - frontW;

    if (outChannels == 1) {
        outGains[0] = distGain;
        return;
    }

    if (outChannels == 2) {
        const f32 angle = (side + 1.0f) * (3.14159265358979323846f * 0.25f);
        outGains[0] = std::cos(angle) * distGain;
        outGains[1] = std::sin(angle) * distGain;
        return;
    }

    if (outChannels == 4) {
        outGains[0] = leftW * frontW * distGain;
        outGains[1] = rightW * frontW * distGain;
        outGains[2] = leftW * rearW * distGain;
        outGains[3] = rightW * rearW * distGain;
        return;
    }

    outGains[0] = leftW * frontW * distGain;
    outGains[1] = rightW * frontW * distGain;

    if (outChannels >= 3) {
        outGains[2] = (1.0f - std::fabs(side)) * frontW * distGain;
    }
    if (outChannels >= 4) {
        outGains[3] = 0.0f; // LFE not synthesized.
    }
    if (outChannels >= 6) {
        outGains[4] = leftW * rearW * distGain;
        outGains[5] = rightW * rearW * distGain;
    }
    if (outChannels >= 8) {
        outGains[6] = leftW * rearW * 0.75f * distGain;
        outGains[7] = rightW * rearW * 0.75f * distGain;
    }
}

// Audio callback - mixes all active voices + music into output
static void audioCallback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frameCount) {
    const u32 outChannels = (device && device->playback.channels > 0)
        ? (u32)device->playback.channels
        : g_outputChannels;

    f32* out = (f32*)output;
    memset(out, 0, frameCount * outChannels * sizeof(f32));

    ListenerState listener = {};
    {
        std::lock_guard<std::mutex> lock(g_listenerMutex);
        listener = g_listener;
    }

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

            f32 gains[8] = {};

            if (voice.spatial) {
                ComputeSpatialGains(voice, listener, outChannels, gains);
            }
            else {
                if (outChannels >= 2) {
                    const f32 pan = clampf(voice.pan, -1.0f, 1.0f);
                    const f32 angle = (pan + 1.0f) * (3.14159265358979323846f * 0.25f);
                    gains[0] = std::cos(angle);
                    gains[1] = std::sin(angle);
                }
                else {
                    gains[0] = 1.0f;
                }
            }

            // Rate ratio: how many source frames per output frame
            f64 rateRatio = (f64)smp.sampleRate / (f64)g_deviceSampleRate;
            f64 advance = rateRatio * (f64)voice.pitch;

            for (u32 i = 0; i < frameCount; i++) {
                if (voice.fadeFramesRemaining > 0) {
                    const u32 step = voice.fadeFramesTotal - voice.fadeFramesRemaining + 1;
                    const f32 t = (f32)step / (f32)voice.fadeFramesTotal;
                    voice.volume = voice.fadeStartVolume + (voice.fadeTargetVolume - voice.fadeStartVolume) * t;
                    voice.fadeFramesRemaining--;
                    if (voice.fadeFramesRemaining == 0) {
                        voice.volume = voice.fadeTargetVolume;
                    }
                }

                const f32 vol = voice.volume * g_masterVolume;

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

                const f32 sMono =
                    (smp.channels == 1)
                    ? (smp.data[pos] / 32768.0f)
                    : (((f32)smp.data[pos * 2] + (f32)smp.data[pos * 2 + 1]) * (0.5f / 32768.0f));

                if (voice.spatial) {
                    for (u32 ch = 0; ch < outChannels && ch < 8; ch++) {
                        out[i * outChannels + ch] += sMono * vol * gains[ch];
                    }
                }
                else {
                    if (outChannels == 1) {
                        out[i] += sMono * vol;
                    }
                    else {
                        f32 sampleL = sMono;
                        f32 sampleR = sMono;
                        if (smp.channels >= 2) {
                            sampleL = smp.data[pos * 2] / 32768.0f;
                            sampleR = smp.data[pos * 2 + 1] / 32768.0f;
                        }

                        out[i * outChannels] += sampleL * vol * gains[0];
                        out[i * outChannels + 1] += sampleR * vol * gains[1];
                    }
                }

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
                for (u32 i = 0; i < (u32)framesRead; i++) {
                    const f32 l = temp[i * MUSIC_CHANNELS] * mvol;
                    const f32 r = temp[i * MUSIC_CHANNELS + 1] * mvol;

                    if (outChannels == 1) {
                        out[offset + i] += 0.5f * (l + r);
                    }
                    else {
                        const u32 dst = offset + i * outChannels;
                        out[dst] += l;
                        out[dst + 1] += r;
                    }
                }
                offset += (u32)framesRead * outChannels;
                remaining -= (u32)framesRead;
            }
        }
    }

    if (g_outputMono && outChannels > 1) {
        for (u32 frame = 0; frame < frameCount; frame++) {
            f32 mono = 0.0f;
            for (u32 ch = 0; ch < outChannels; ch++) {
                mono += out[frame * outChannels + ch];
            }
            mono /= (f32)outChannels;
            for (u32 ch = 0; ch < outChannels; ch++) {
                out[frame * outChannels + ch] = mono;
            }
        }
    }

    // Clamp output
    for (u32 i = 0; i < frameCount * outChannels; i++) {
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
    g_outputMono = false;
    g_outputChannels = DEFAULT_MIX_CHANNELS;
    g_musicActive = false;
    g_listener = {};
    g_listener.valid = false;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 0; // Native device channel layout.
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
    g_outputChannels = (g_device.playback.channels > 0) ? (u32)g_device.playback.channels : DEFAULT_MIX_CHANNELS;

    if (ma_device_start(&g_device) != MA_SUCCESS) {
        LOG("AudioEngine: failed to start device");
        ma_device_uninit(&g_device);
        return false;
    }

    g_initialized = true;

    LOG("AudioEngine: initialized (device %u Hz, %u ch, period %u frames)",
        g_deviceSampleRate, g_outputChannels, config.periodSizeInFrames);
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
            g_voices[i].fadeStartVolume = volume;
            g_voices[i].fadeTargetVolume = volume;
            g_voices[i].fadeFramesTotal = 0;
            g_voices[i].fadeFramesRemaining = 0;
            g_voices[i].pan = pan;
            g_voices[i].pitch = 1.0f;
            g_voices[i].worldPos = {};
            g_voices[i].minDistance = 0.0f;
            g_voices[i].maxDistance = 10000.0f;
            g_voices[i].spatial = false;
            g_voices[i].applyDistanceAttenuation = false;
            g_voices[i].loop = loop;
            g_voices[i].active = true;
            return i + 1; // 1-based handle
        }
    }

    LOG("AudioEngine: no free voice slots");
    return AUDIO_VOICE_INVALID;
}

AudioVoice AudioEngine::PlaySample3D(
    AudioSample sample,
    const LVector& position,
    f32 volume,
    bool loop,
    bool applyDistanceAttenuation,
    f32 minDistance,
    f32 maxDistance) {
    if (!g_initialized || sample == AUDIO_SAMPLE_INVALID) return AUDIO_VOICE_INVALID;

    std::lock_guard<std::mutex> lock(g_voiceMutex);

    for (u32 i = 0; i < MAX_VOICES; i++) {
        if (!g_voices[i].active) {
            g_voices[i].sample = sample;
            g_voices[i].positionF = 0.0;
            g_voices[i].volume = volume;
            g_voices[i].fadeStartVolume = volume;
            g_voices[i].fadeTargetVolume = volume;
            g_voices[i].fadeFramesTotal = 0;
            g_voices[i].fadeFramesRemaining = 0;
            g_voices[i].pan = 0.0f;
            g_voices[i].pitch = 1.0f;
            g_voices[i].worldPos = position;
            g_voices[i].minDistance = minDistance;
            g_voices[i].maxDistance = (maxDistance > minDistance) ? maxDistance : (minDistance + 1.0f);
            g_voices[i].spatial = true;
            g_voices[i].applyDistanceAttenuation = applyDistanceAttenuation;
            g_voices[i].loop = loop;
            g_voices[i].active = true;
            return i + 1;
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
    g_voices[voice - 1].fadeStartVolume = volume;
    g_voices[voice - 1].fadeTargetVolume = volume;
    g_voices[voice - 1].fadeFramesTotal = 0;
    g_voices[voice - 1].fadeFramesRemaining = 0;
}

void AudioEngine::FadeVoiceVolume(AudioVoice voice, f32 targetVolume, u32 fadeMs) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;

    std::lock_guard<std::mutex> lock(g_voiceMutex);
    InternalVoice& v = g_voices[voice - 1];
    if (!v.active) {
        return;
    }

    const f32 clampedTarget = clampf(targetVolume, 0.0f, 1.0f);
    if (fadeMs == 0) {
        v.volume = clampedTarget;
        v.fadeStartVolume = clampedTarget;
        v.fadeTargetVolume = clampedTarget;
        v.fadeFramesTotal = 0;
        v.fadeFramesRemaining = 0;
        return;
    }

    u32 fadeFrames = (g_deviceSampleRate * fadeMs) / 1000;
    if (fadeFrames == 0) {
        fadeFrames = 1;
    }

    v.fadeStartVolume = v.volume;
    v.fadeTargetVolume = clampedTarget;
    v.fadeFramesTotal = fadeFrames;
    v.fadeFramesRemaining = fadeFrames;
}

void AudioEngine::SetVoicePan(AudioVoice voice, f32 pan) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].pan = pan;
    g_voices[voice - 1].spatial = false;
}

void AudioEngine::SetVoicePitch(AudioVoice voice, f32 pitch) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].pitch = pitch;
}

void AudioEngine::SetVoicePosition(AudioVoice voice, const LVector& position) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].worldPos = position;
    g_voices[voice - 1].spatial = true;
}

void AudioEngine::SetVoiceDistanceRange(AudioVoice voice, f32 minDistance, f32 maxDistance) {
    if (voice == AUDIO_VOICE_INVALID || voice > MAX_VOICES) return;
    std::lock_guard<std::mutex> lock(g_voiceMutex);
    g_voices[voice - 1].minDistance = minDistance;
    g_voices[voice - 1].maxDistance = (maxDistance > minDistance) ? maxDistance : (minDistance + 1.0f);
}

void AudioEngine::SetListener(const LVector& position, s32 yaw16) {
    std::lock_guard<std::mutex> lock(g_listenerMutex);
    g_listener.pos = position;
    g_listener.yaw16 = yaw16;
    g_listener.valid = true;
}

bool AudioEngine::PlayMusic(const char* path, f32 volume, bool loop) {
    if (!g_initialized || !path) return false;

    StopMusic();

    std::lock_guard<std::mutex> lock(g_musicMutex);

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, MUSIC_CHANNELS, g_deviceSampleRate);
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

void AudioEngine::SetOutputMono(bool mono) {
    g_outputMono = mono;
}

bool AudioEngine::GetOutputMono() {
    return g_outputMono;
}

u32 AudioEngine::GetOutputChannels() {
    return g_outputChannels;
}
