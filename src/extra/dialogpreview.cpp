#include "extra/dialogpreview.h"

#include "snd/rsevent.h"
#include "snd/sound.h"
#include "pc/audio.h"
#include "gen/time.h"

AudioSample s_previewSample = AUDIO_SAMPLE_INVALID;
AudioVoice s_previewVoice = AUDIO_VOICE_INVALID;
f64 s_stopTimeSec = -1.0;

void DialogPreview::Stop() {
    if (s_previewVoice != AUDIO_VOICE_INVALID) {
        AudioEngine::StopVoice(s_previewVoice);
        s_previewVoice = AUDIO_VOICE_INVALID;
    }
    if (s_previewSample != AUDIO_SAMPLE_INVALID) {
        AudioEngine::UnloadSample(s_previewSample);
        s_previewSample = AUDIO_SAMPLE_INVALID;
    }
    s_stopTimeSec = -1.0;
}

void DialogPreview::Play(s32 character, s32 dialogId, s32 variant, f32 maxDurationSec) {
    Stop();

    if (!g_sound) {
        return;
    }

    s_previewSample = LoadDialogClipSample(character, dialogId, variant);
    if (s_previewSample == AUDIO_SAMPLE_INVALID) {
        return;
    }

    s_previewVoice = AudioEngine::PlaySample(s_previewSample, g_sound->dialogVolume, 0.0f, false);
    if (s_previewVoice != AUDIO_VOICE_INVALID && maxDurationSec > 0.0f) {
        s_stopTimeSec = Time::GetTimeInSeconds() + (f64)maxDurationSec;
    }
}

void DialogPreview::Update() {
    if (s_stopTimeSec >= 0.0 && Time::GetTimeInSeconds() >= s_stopTimeSec) {
        Stop();
    }
}
