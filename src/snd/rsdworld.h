#pragma once
#include "core.h"

// Forward declarations
class CGenericPersistentSound;

// PSX: rsdWorld namespace - SPU transient sound playback
namespace rsdWorld {

    // PSX: PlayTransient__8rsdWorldlPC10tagLVectorUsUsUsUl (RSDUTIL.CPP:582, 0x80080234)
    // Positional variant: computes L/R volumes from position, plays SPU voice
    // sampleId: rsd sample index (from g_transData)
    // posPtr: 3D position for spatialization (can be null)
    // volume: PSX SPU volume (0-0xFFFF)
    // pitch: PSX SPU pitch (1024 = normal)
    // pan: pan value
    // flags: additional flags
    s32 PlayTransientPositional(u32 sampleId, void* posPtr, u16 volume, s16 pitch, u16 pan, u32 flags);
    void SetDialogTransient(bool isDialog);

    // PSX: GetObjectVolumes__8rsdWorld... - computes PSX-accurate stereo L/R
    // volume falloff for a sound at objPos relative to the listener. Shared
    // by PlayTransientPositional/rsdPersistent and dialog playback
    // (rsevent.cpp), which has its own sample/voice pipeline and can't reuse
    // PlayTransientPositional wholesale (that resolves samples through a WAX
    // sound bank; dialog samples are pre-decoded from RSDIALOG.DLG).
    void ComputeObjectVolumes(u16 baseVol, const LVector* objPos, u16& outVolL, u16& outVolR, u32 extraRange);

    // PSX: PlayTransient__8rsdWorldlUsUsUsUi (RSDUTIL.CPP:615, 0x80080334)
    // Non-positional variant: takes separate L/R volumes
    // volL: left channel PSX SPU volume
    // volR: right channel PSX SPU volume
    s32 PlayTransientNonPositional(u32 sampleId, u16 volL, u16 volR, s16 pitch, u16 pan);

    void UpdateSpatialAudioState();
    void StopAllPersistentSounds();
    // PSX: rsdPersistent::FadeOutAll(1500) -- mute all persistent sounds (keep them alive)
    void MuteAllPersistentSounds();
    // PSX: rsdPersistent::FadeInAll(1500) -- restore all persistent sounds from mute
    void UnmuteAllPersistentSounds();

} // namespace rsdWorld

// PSX: rsdPersistent - persistent (looping) SPU sound object
// On PSX this manages an SPU voice for continuous playback.
// On PC it wraps an AudioEngine looping voice.
class rsdPersistent {
public:
    u32 sampleId;       // rsd sample index
    void* voiceHandle;  // PC: AudioVoice cast to void*
    const LVector* posPtr; // source position pointer (world-space)
    u16 spatialFlags;   // original spatial attenuation flags
    u16 volume;         // current PSX volume

    // Params: sampleId, posPtr, reverb, volume, pitch, flags
    rsdPersistent(u32 sampleId, void* posPtr, u8 reverb, u16 volume, s16 pitch, u16 flags);
    ~rsdPersistent();

    // PSX: End/destroy the persistent sound
    void End();

    // PSX: ObjectExists__13rsdPersistentP13rsdPersistent (check if valid)
    static bool ObjectExists(rsdPersistent* obj);

    // PSX: SetVolume (update volume on active voice)
    void SetVolume(u16 psxVol);
    void SetMuted(bool muted);

    // PC: refresh spatial position/volume from live world state.
    void UpdateSpatial();
private:
    bool muted = false;
};
