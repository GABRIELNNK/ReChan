#include "snd/rsdworld.h"
#include "snd/sound.h"
#include "snd/sndmath.h"
#include "gen/common.h"
#include "pc/audio.h"

// PC: resolve rsd sample ID to the active WAX bank.
// PSX loads one bank at a time; rsd sample IDs (0-70) index directly
// into the single active bank's 71-entry descriptor table.
static bool RsdSampleToWax(u32 rsdSampleId, u32& outBank, u32& outSample) {
    if (!g_sound || g_sound->activeSfxBank < 0) {
        return false;
    }

    u32 bank = (u32)g_sound->activeSfxBank;
    if (bank >= MAX_WAX_BANKS) {
        return false;
    }
    if (rsdSampleId >= g_sound->GetBankSampleCount(bank)) {
        return false;
    }

    outBank = bank;
    outSample = rsdSampleId;
    return true;
}

// PSX: PlayTransient__8rsdWorldlPC10tagLVectorUsUsUsUl (0x80080234)
// On PSX: GetObjectVolumes -> rsdGetVoice -> rsdSetVolume -> rsdSetPitch -> rsdVoiceOn
// On PC: map sample, convert params, play via AudioEngine
s32 rsdWorld::PlayTransientPositional(u32 sampleId, void* posPtr, u16 volume, s16 pitch, u16 pan, u32 flags) {
    MARKFUNCTION(0x80080234);

    if (volume == 0) {
        return 0;
    }

    u32 bank, sample;
    if (!RsdSampleToWax(sampleId, bank, sample)) {
        return 0;
    }

    f32 vol = PsxVolToFloat(volume);
    f32 pitchF = PsxPitchToFloat(pitch);

    AudioSample s = g_sound->GetBankSample(bank, sample);
    if (s == AUDIO_SAMPLE_INVALID) {
        return 0;
    }

    AudioVoice v = AudioEngine::PlaySample(s, vol, 0.0f, false);
    if (v != AUDIO_VOICE_INVALID && pitchF != 1.0f) {
        AudioEngine::SetVoicePitch(v, pitchF);
    }

    return 0;
}

// PSX: PlayTransient__8rsdWorldlUsUsUsUi (0x80080334)
// Non-positional: separate L/R volumes, no 3D spatialization
s32 rsdWorld::PlayTransientNonPositional(u32 sampleId, u16 volL, u16 volR, s16 pitch, u16 pan) {
    MARKFUNCTION(0x80080334);

    if (volL == 0 && volR == 0) {
        return 0;
    }

    u32 bank, sample;
    if (!RsdSampleToWax(sampleId, bank, sample)) {
        return 0;
    }

    // PC: approximate L/R by averaging for volume and computing pan
    f32 fVolL = PsxVolToFloat(volL);
    f32 fVolR = PsxVolToFloat(volR);
    f32 vol = (fVolL + fVolR) * 0.5f;
    f32 pcPan = 0.0f;
    if (fVolL + fVolR > 0.0f) {
        pcPan = (fVolR - fVolL) / (fVolL + fVolR);
    }
    f32 pitchF = PsxPitchToFloat(pitch);

    AudioSample s = g_sound->GetBankSample(bank, sample);
    if (s == AUDIO_SAMPLE_INVALID) {
        return 0;
    }

    AudioVoice v = AudioEngine::PlaySample(s, vol, pcPan, false);
    if (v != AUDIO_VOICE_INVALID && pitchF != 1.0f) {
        AudioEngine::SetVoicePitch(v, pitchF);
    }

    return 0;
}

// rsdPersistent - persistent looping sound

// PSX: _13rsdPersistentlPC10tagLVectorUlUsUsUl (0x80080508)
rsdPersistent::rsdPersistent(u32 sampleId_, void* posPtr, u8 reverb, u16 volume_, s16 pitch, u16 flags) {
    sampleId = sampleId_;
    volume = volume_;
    voiceHandle = nullptr;

    u32 bank, sample;
    if (!RsdSampleToWax(sampleId_, bank, sample)) {
        return;
    }

    AudioSample s = g_sound->GetBankSample(bank, sample);
    if (s == AUDIO_SAMPLE_INVALID) {
        return;
    }

    f32 vol = PsxVolToFloat(volume_);
    f32 pitchF = PsxPitchToFloat(pitch);

    AudioVoice v = AudioEngine::PlaySample(s, vol, 0.0f, true);
    if (v != AUDIO_VOICE_INVALID) {
        if (pitchF != 1.0f) {
            AudioEngine::SetVoicePitch(v, pitchF);
        }
        voiceHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(v));
    }
}

rsdPersistent::~rsdPersistent() {
    End();
}

void rsdPersistent::End() {
    if (voiceHandle) {
        AudioVoice v = static_cast<AudioVoice>(reinterpret_cast<uintptr_t>(voiceHandle));
        AudioEngine::StopVoice(v);
        voiceHandle = nullptr;
    }
}

// PSX: ObjectExists__13rsdPersistentP13rsdPersistent
bool rsdPersistent::ObjectExists(rsdPersistent* obj) {
    if (!obj) {
        return false;
    }
    if (!obj->voiceHandle) {
        return false;
    }
    AudioVoice v = static_cast<AudioVoice>(reinterpret_cast<uintptr_t>(obj->voiceHandle));
    return AudioEngine::IsVoicePlaying(v);
}

void rsdPersistent::SetVolume(u16 psxVol) {
    volume = psxVol;
    if (voiceHandle) {
        AudioVoice v = static_cast<AudioVoice>(reinterpret_cast<uintptr_t>(voiceHandle));
        AudioEngine::SetVoiceVolume(v, PsxVolToFloat(psxVol));
    }
}
