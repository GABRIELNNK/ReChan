#pragma once

#include "core.h"
#include "pc/audio.h"
#include <vector>

// PSX: rsdAmbiance (RSDAMBCE.CPP) - per-location streamed ambient sound bed.
class Ambiance {
public:
    Ambiance() = default;
    ~Ambiance();

    // PSX: Open__11rsdAmbiancePCclll (0x80080E0C)
    // baseName e.g. "SNDAMB1"; loads SOUND/AMBIENCE/<baseName>.CLP + .AMS.
    s32 Open(const char* baseName);
    // PSX: Close__11rsdAmbiance (0x8008132C)
    void Close();

    // PSX: Start__11rsdAmbianceUl (0x80081464). space == -1 uses default space.
    s32 Start(s32 space = -1);
    // PSX: Stop__11rsdAmbiance (0x8008158C)
    s32 Stop();

    // PSX: SetSpace__11rsdAmbianceUl (0x800816C0)
    void SetSpace(s32 space);
    // PSX: GetSpace__11rsdAmbiance (0x80081780)
    s32 GetSpace() const { return curSpace; }
    // PSX: SetCrossFadeDuration__11rsdAmbiancel (0x80081828). Duration in frames.
    void SetCrossFadeDuration(s32 frames);

    // PSX: FadeIn__11rsdAmbianceUlbUl (0x800818C8)
    void FadeIn(u32 durationMs);
    // PSX: FadeOut__11rsdAmbianceUlb (0x800819C0)
    void FadeOut(u32 durationMs);

    // PSX: AmbianceTask__11rsdAmbiance (0x80081EB0) - per-frame service tick.
    void Update();

    bool IsOpen() const { return opened; }
    bool IsRunning() const { return running; }

private:
    struct Space {
        s32 reverbMode = 0;
        s32 reverbDepth = 0;
        s32 volPct = 100;
        std::vector<u32> bg; // background clip indices (1 word each)
        std::vector<u32> fg; // foreground clip indices (from 3-word entries)
    };

    struct Layer {
        AudioVoice voice = AUDIO_VOICE_INVALID;
        s32 lastClip = -1;
    };

    void StopLayer(Layer& layer);
    void StopVoices();
    // retrigger: true for the active bed (chains random clips), false for an
    // outgoing bed being crossfaded out (let its current clip finish and die).
    void ServiceLayer(Layer& layer, const std::vector<u32>& clipList, f32 gain, bool retrigger);
    f32 MasterGain() const;

    bool opened = false;
    bool running = false;

    std::vector<AudioSample> clips; // decoded clip index -> sample handle
    std::vector<Space> spaces;
    s32 defaultSpace = 0;

    s32 curSpace = 0;
    s32 prevSpace = -1;

    // Overlapping space crossfade: mix ramps 0 -> 1 (prev bed out, cur bed in).
    f32 mix = 1.0f;
    f32 crossFadeStep = 1.0f / 60.0f; // default ~1s @ 60fps until a switch overrides
    bool crossfading = false;

    // Global fade (level enter/exit, engine mute), 0..1.
    f32 globalFade = 1.0f;
    f32 globalFadeStep = 0.0f;
    s32 globalFadeDir = 0;

    Layer bgCur, fgCur;   // active bed for curSpace
    Layer bgPrev, fgPrev; // outgoing bed for prevSpace during a crossfade
};
