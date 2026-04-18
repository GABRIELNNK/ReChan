#include "gen/common.h"
#include "snd/rsevent.h"
#include "snd/sound.h"
#include "snd/rsdworld.h"
#include "snd/adpcm.h"
#include "p3d/p3dmath.h"
#include "xclib/xcfile.h"

static constexpr u32 DIALOG_SECTOR_SIZE = 2048;
static constexpr u32 DIALOG_RATE = 11025;

static std::vector<u8> g_dialogFileData;
static std::vector<u8> g_dialogHeader;
static u32 g_dialogHeaderSize = 0;
static u32 g_dialogDataBaseOffset = 0;
static u16 g_lastDialogChoiceOffset = 0;

static u8 DialogHeaderU8(u32 offset) {
    if (offset >= g_dialogHeader.size()) {
        return 0;
    }
    return g_dialogHeader[offset];
}

static u16 DialogHeaderU16(u32 offset) {
    if (offset + 1 >= g_dialogHeader.size()) {
        return 0;
    }
    return (u16)((u16)g_dialogHeader[offset] | ((u16)g_dialogHeader[offset + 1] << 8));
}

static void ResetDialogHeaderState() {
    if (g_dialogHeaderSize == 0 || g_dialogFileData.size() < g_dialogHeaderSize) {
        g_dialogHeader.clear();
        return;
    }

    g_dialogHeader.assign(g_dialogFileData.begin(), g_dialogFileData.begin() + g_dialogHeaderSize);
    g_lastDialogChoiceOffset = 0;
}

static bool EnsureDialogDataLoaded() {
    if (!g_dialogFileData.empty() && !g_dialogHeader.empty()) {
        return true;
    }

    u8* raw = nullptr;
    u32 size = 0;
    if (!xcReadFileLow("SOUND/DIALOG/RSDIALOG.DLG", &raw, &size) || !raw || size < 6) {
        if (raw) {
            delete[] raw;
        }
        return false;
    }

    g_dialogFileData.assign(raw, raw + size);
    delete[] raw;

    g_dialogHeaderSize = (u32)((u16)g_dialogFileData[0] | ((u16)g_dialogFileData[1] << 8));
    if (g_dialogHeaderSize == 0 || g_dialogHeaderSize > g_dialogFileData.size()) {
        g_dialogFileData.clear();
        g_dialogHeader.clear();
        g_dialogHeaderSize = 0;
        return false;
    }

    g_dialogDataBaseOffset = (g_dialogHeaderSize + (DIALOG_SECTOR_SIZE - 1)) & ~(DIALOG_SECTOR_SIZE - 1);
    if (g_dialogDataBaseOffset >= g_dialogFileData.size()) {
        g_dialogFileData.clear();
        g_dialogHeader.clear();
        g_dialogHeaderSize = 0;
        g_dialogDataBaseOffset = 0;
        return false;
    }

    ResetDialogHeaderState();
    return !g_dialogHeader.empty();
}

static bool SelectDialogClipOffset(s32 character, s32 dialogID, u32* outStart, u32* outEnd, u16* outChoiceOffset) {
    if (!outStart || !outEnd || !outChoiceOffset) {
        return false;
    }

    if (!EnsureDialogDataLoaded()) {
        return false;
    }

    const u32 charTableOffset = 4;
    const u32 charEntryOffset = charTableOffset + ((u32)(u16)character * 2);
    if (charEntryOffset + 1 >= g_dialogHeaderSize) {
        return false;
    }

    const u16 charInfoOffset = DialogHeaderU16(charEntryOffset);
    if (charInfoOffset >= g_dialogHeaderSize) {
        return false;
    }

    const u8 dialogCount = DialogHeaderU8(charInfoOffset);
    if (dialogCount == 0) {
        return false;
    }

    const u32 dialogListOffset = (u32)charInfoOffset + 1;
    const u32 streamTableOffset = dialogListOffset + (u32)dialogCount;
    const u32 clipInfoTableOffset = streamTableOffset + ((u32)dialogCount * 2);
    if (clipInfoTableOffset + ((u32)dialogCount * 2) > g_dialogHeaderSize) {
        return false;
    }

    s32 dialogIndex = -1;
    for (u32 i = 0; i < (u32)dialogCount; ++i) {
        if ((s32)DialogHeaderU8(dialogListOffset + i) == dialogID) {
            dialogIndex = (s32)i;
            break;
        }
    }
    if (dialogIndex < 0) {
        return false;
    }

    const u32 streamSectors = (u32)DialogHeaderU16(streamTableOffset + ((u32)dialogIndex * 2));
    u32 streamStart = g_dialogDataBaseOffset + streamSectors * DIALOG_SECTOR_SIZE;

    const u16 clipInfoOffset = DialogHeaderU16(clipInfoTableOffset + ((u32)dialogIndex * 2));
    if (clipInfoOffset >= g_dialogHeaderSize) {
        return false;
    }

    const u8 clipCount = DialogHeaderU8(clipInfoOffset);
    if (clipCount == 0) {
        return false;
    }

    const u32 clipSizeTableOffset = (u32)clipInfoOffset + 1;
    if (clipSizeTableOffset + clipCount > g_dialogHeaderSize) {
        return false;
    }

    u8 available[256] = {};
    u32 availableCount = 0;
    for (u32 i = 0; i < (u32)clipCount; ++i) {
        const u8 flags = DialogHeaderU8(clipSizeTableOffset + i);
        if ((flags & 0xC0) == 0) {
            available[availableCount++] = (u8)i;
        }
    }

    if (availableCount == 0) {
        for (u32 i = 0; i < (u32)clipCount; ++i) {
            const u32 off = clipSizeTableOffset + i;
            const u8 flags = DialogHeaderU8(off);
            if ((flags & 0x80) != 0 && (flags & 0x40) == 0) {
                g_dialogHeader[off] = (u8)(flags & 0x3F);
            }
        }

        for (u32 i = 0; i < (u32)clipCount; ++i) {
            const u8 flags = DialogHeaderU8(clipSizeTableOffset + i);
            if ((flags & 0xC0) == 0) {
                available[availableCount++] = (u8)i;
            }
        }
    }

    if (availableCount == 0) {
        return false;
    }

    u32 selectedSlot = (u32)rmRangedRandom((s32)availableCount);
    u32 selected = (u32)available[selectedSlot];
    u16 choiceOffset = (u16)(clipSizeTableOffset + selected);

    // PSX avoids immediate repeats when multiple variants are available,
    // but it does not fail dialog playback if only one choice exists.
    if (availableCount > 1 && choiceOffset == g_lastDialogChoiceOffset) {
        for (u32 i = 1; i < availableCount; ++i) {
            const u32 trySlot = (selectedSlot + i) % availableCount;
            const u32 trySelected = (u32)available[trySlot];
            const u16 tryOffset = (u16)(clipSizeTableOffset + trySelected);
            if (tryOffset != g_lastDialogChoiceOffset) {
                selectedSlot = trySlot;
                selected = trySelected;
                choiceOffset = tryOffset;
                break;
            }
        }
    }

    for (u32 i = 0; i < selected; ++i) {
        const u32 clipSectors = (u32)(DialogHeaderU8(clipSizeTableOffset + i) & 0x3F);
        streamStart += clipSectors * DIALOG_SECTOR_SIZE;
    }

    const u32 selectedSectors = (u32)(DialogHeaderU8(choiceOffset) & 0x3F);
    const u32 clipSize = selectedSectors * DIALOG_SECTOR_SIZE;
    if (clipSize == 0) {
        return false;
    }

    const u32 clipEnd = streamStart + clipSize;
    if (streamStart >= g_dialogFileData.size() || clipEnd > g_dialogFileData.size()) {
        return false;
    }

    *outStart = streamStart;
    *outEnd = clipEnd;
    *outChoiceOffset = choiceOffset;
    return true;
}

static AudioSample LoadDialogSample(s32 character, s32 dialogID) {
    u32 start = 0;
    u32 end = 0;
    u16 choiceOffset = 0;
    if (!SelectDialogClipOffset(character, dialogID, &start, &end, &choiceOffset)) {
        return AUDIO_SAMPLE_INVALID;
    }

    const u8* clipData = &g_dialogFileData[start];
    const u32 clipSize = end - start;
    std::vector<s16> pcm = SpuAdpcm::Decode(clipData, clipSize, true);
    if (pcm.empty()) {
        return AUDIO_SAMPLE_INVALID;
    }

    AudioSample sample = AudioEngine::LoadSample(pcm.data(), (u32)pcm.size(), DIALOG_RATE, 1);
    if (sample == AUDIO_SAMPLE_INVALID) {
        return AUDIO_SAMPLE_INVALID;
    }

    g_dialogHeader[choiceOffset] = (u8)(g_dialogHeader[choiceOffset] | 0x80);
    g_lastDialogChoiceOffset = choiceOffset;
    return sample;
}

struct DialogEntry {
    s32 handle;
    s32 character;
    s32 dialogId;
    s32 priority;
    AudioSample sample;
    AudioVoice voice;
    bool valid;
};

static constexpr s32 DIALOG_ENTRY_COUNT = 64;
static DialogEntry g_dialogEntries[DIALOG_ENTRY_COUNT] = {};
static s32 g_nextDialogHandle = 1;

static DialogEntry* FindDialogEntry(s32 handle) {
    if (handle == 0) {
        return nullptr;
    }

    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        if (g_dialogEntries[i].valid && g_dialogEntries[i].handle == handle) {
            return &g_dialogEntries[i];
        }
    }

    return nullptr;
}

static DialogEntry* AllocateDialogEntry() {
    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        if (!g_dialogEntries[i].valid) {
            g_dialogEntries[i].handle = g_nextDialogHandle++;
            if (g_nextDialogHandle <= 0) {
                g_nextDialogHandle = 1;
            }
            g_dialogEntries[i].sample = AUDIO_SAMPLE_INVALID;
            g_dialogEntries[i].voice = AUDIO_VOICE_INVALID;
            g_dialogEntries[i].valid = true;
            return &g_dialogEntries[i];
        }
    }

    DialogEntry* entry = &g_dialogEntries[0];
    if (entry->voice != AUDIO_VOICE_INVALID) {
        AudioEngine::StopVoice(entry->voice);
    }
    if (entry->sample != AUDIO_SAMPLE_INVALID) {
        AudioEngine::UnloadSample(entry->sample);
    }
    entry->handle = g_nextDialogHandle++;
    if (g_nextDialogHandle <= 0) {
        g_nextDialogHandle = 1;
    }
    entry->sample = AUDIO_SAMPLE_INVALID;
    entry->voice = AUDIO_VOICE_INVALID;
    entry->valid = true;
    return entry;
}

static s32 PlayDialogHandle(DialogEntry* entry, u32 distanceHint) {
    if (!entry || !entry->valid || !g_sound) {
        return 0;
    }

    if (entry->sample == AUDIO_SAMPLE_INVALID) {
        return 0;
    }

    if (entry->voice != AUDIO_VOICE_INVALID) {
        AudioEngine::StopVoice(entry->voice);
    }

    entry->voice = AudioEngine::PlaySample(entry->sample, g_sound->dialogVolume, 0.0f, false);
    if (entry->voice == AUDIO_VOICE_INVALID) {
        return 0;
    }

    if (distanceHint != 0) {
        const f32 maxDistance = (f32)distanceHint * 100.0f;
        AudioEngine::SetVoiceDistanceRange(entry->voice, 0.0f, maxDistance);
    }

    return 1;
}

// PSX: rsDialogEvent (RSEVENT.CPP:82, 0x8003470C)
static s32 rsDialogEvent(s32 event, s32 param1, s32 param2, s32 param3) {
    switch (event) {
        case RS_LOAD_DIALOG:
        {
            DialogEntry* entry = AllocateDialogEntry();
            if (!entry) {
                return 0;
            }
            entry->character = param1;
            entry->dialogId = param2;
            entry->priority = param3;
            entry->sample = LoadDialogSample(param1, param2);
            if (entry->sample == AUDIO_SAMPLE_INVALID) {
                entry->valid = false;
                return 0;
            }
            return entry->handle;
        }

        case RS_PLAY_DIALOG:
        {
            DialogEntry* entry = FindDialogEntry(param1);
            return PlayDialogHandle(entry, (u32)param3);
        }

        case RS_STOP_DIALOG:
        {
            for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
                DialogEntry& entry = g_dialogEntries[i];
                if (!entry.valid) {
                    continue;
                }
                if (entry.voice != AUDIO_VOICE_INVALID) {
                    AudioEngine::StopVoice(entry.voice);
                    entry.voice = AUDIO_VOICE_INVALID;
                }
                if (entry.sample != AUDIO_SAMPLE_INVALID) {
                    AudioEngine::UnloadSample(entry.sample);
                    entry.sample = AUDIO_SAMPLE_INVALID;
                }
            }
            return 0;
        }

        case RS_KILL_DIALOG:
        {
            DialogEntry* entry = FindDialogEntry(param1);
            if (!entry) {
                return 0;
            }
            if (entry->voice != AUDIO_VOICE_INVALID) {
                AudioEngine::StopVoice(entry->voice);
                entry->voice = AUDIO_VOICE_INVALID;
            }
            if (entry->sample != AUDIO_SAMPLE_INVALID) {
                AudioEngine::UnloadSample(entry->sample);
                entry->sample = AUDIO_SAMPLE_INVALID;
            }
            entry->valid = false;
            return 1;
        }

        case RS_LOAD_AND_PLAY_DIALOG:
        {
            const s32 handle = rsDialogEvent(RS_LOAD_DIALOG, param1, param2, 255);
            if (!handle) {
                return 0;
            }
            return rsDialogEvent(RS_PLAY_DIALOG, handle, param3, 360);
        }

        case RS_QUERY_DIALOG_PRIORITY:
            return jcsQueryDialogPriority();

        default:
            break;
    }

    return 0;
}

// PSX: gp+544 - global sound enabled flag (0 = enabled, nonzero = disabled)
static s32 g_soundDisabled = 0;

// PSX: gp+572 - pause flag
static s32 g_pauseFlag = 0;

// PSX: gp+732 - mute flag
static s32 g_muteFlag = 0;

// PSX: Sound location to music file mapping (gp-relative at 0x800ED6CC)
// Event 4 (RS_SET_LOCATION) with param1 sets the "current sound location"
// which determines which music track plays when RS_LEVEL_BEGIN fires.
s32 g_currentSoundLocation = 0;

// PSX music file table indexed by sound location (LocInfo at 0x800D6588)
// Matches PSX LocInfo[n][1] ordering exactly
static const char* s_musicTable[] = {
    "SOUND/MUSIC/CHINA1.FAG",    //  0: Chinatown petal 1
    "SOUND/MUSIC/CHINA2.FAG",    //  1: Chinatown petal 2
    "SOUND/MUSIC/CHINA3.FAG",    //  2: Chinatown petal 3
    "SOUND/MUSIC/CHINAB.FAG",    //  3: Chinatown boss
    "SOUND/MUSIC/WATER1.FAG",    //  4: Waterfront petal 1
    "SOUND/MUSIC/WATER2.FAG",    //  5: Waterfront petal 2
    "SOUND/MUSIC/WATER3.FAG",    //  6: Waterfront petal 3
    "SOUND/MUSIC/WATERB.FAG",    //  7: Waterfront boss
    "SOUND/MUSIC/SEWER1.FAG",    //  8: Sewer petal 1
    "SOUND/MUSIC/SEWER2.FAG",    //  9: Sewer petal 2
    "SOUND/MUSIC/SEWER3.FAG",    // 10: Sewer petal 3
    "SOUND/MUSIC/SEWERB.FAG",    // 11: Sewer boss
    "SOUND/MUSIC/ROOF1.FAG",     // 12: Rooftop petal 1
    "SOUND/MUSIC/ROOF2.FAG",     // 13: Rooftop petal 2
    "SOUND/MUSIC/ROOF3.FAG",     // 14: Rooftop petal 3
    "SOUND/MUSIC/ROOFB.FAG",     // 15: Rooftop boss
    "SOUND/MUSIC/FACTORY1.FAG",  // 16: Factory petal 1
    "SOUND/MUSIC/FACTORY2.FAG",  // 17: Factory petal 2
    "SOUND/MUSIC/FACTORY3.FAG",  // 18: Factory petal 3
    "SOUND/MUSIC/FACTORYB.FAG",  // 19: Factory boss
    "SOUND/MUSIC/TEMPLE.FAG",    // 20: Temple
    "SOUND/MUSIC/DESTSEL.FAG",   // 21: Destination select
    "SOUND/MUSIC/TITLE.FAG",     // 22: Title screen
    "SOUND/MUSIC/GAMEOVER.FAG",  // 23: Game over
    nullptr,                      // 24: movie (no FAG)
};
static constexpr s32 MUSIC_TABLE_COUNT = 25;

// PSX: rsEvent (RSEVENT.CPP:48, 0x800346B0)
s32 rsEvent(s32 event, s32 param1, s32 param2, s32 param3) {
    MARKFUNCTION(0x800346B0);

    // PSX: reads gp+544 flag, returns 0 if sound disabled
    if (g_soundDisabled != 0) return 0;

    // PSX dispatch:
    // Events 26-31 -> rsDialogEvent
    // Events 1-23  -> jcsHandleControlEvent
    if (event >= 26 && event <= 31) {
        return rsDialogEvent(event, param1, param2, param3);
    }

    if (event >= 1 && event <= 23) {
        return jcsHandleControlEvent(event, param1, param2, param3);
    }

    return 0;
}

// PSX: jcsHandleControlEvent (JCSOUND.CPP:947, 0x80035B00)
s32 jcsHandleControlEvent(s32 event, s32 param1, s32 param2, s32 param3) {
    MARKFUNCTION(0x80035B00);

    if (!g_sound) return 0;

    switch (event) {
        case RS_INITIALIZE: // 1
            LOG("[rsEvent] Initialize");
            break;

        case RS_TERMINATE: // 2
            LOG("[rsEvent] Terminate");
            g_sound->StopMusic();
            break;

        case RS_UNLOAD_LEVEL: // 3 - stop all sounds
            LOG("[rsEvent] UnloadLevel - stop all");
            g_sound->StopMusic();
            break;

        case RS_SET_LOCATION: // 4 - set sound location (music + SFX bank)
            LOG("[rsEvent] SetSoundLocation(%d)", param1);
            g_currentSoundLocation = param1;
            // PSX: LocInfo[loc][0] -> GAME.WAP location table -> bank index.
            // Mapping: location 0-20 -> bank 0-20, location 21+ -> bank 21.
            if (g_sound) {
                g_sound->activeSfxBank = (param1 < 21) ? param1 : 21;
            }
            // PSX resets dialog header selection flags when switching level/location.
            if (EnsureDialogDataLoaded()) {
                ResetDialogHeaderState();
            }
            break;

        case RS_LEVEL_BEGIN: // 5 - start music for current location
        {
            LOG("[rsEvent] LevelBegin - start music (location=%d, musicVol=%.2f)", g_currentSoundLocation, g_sound->musicVolume);
            g_pauseFlag = 0;
            g_muteFlag = 0;
            if (g_currentSoundLocation >= 0 && g_currentSoundLocation < MUSIC_TABLE_COUNT) {
                const char* path = s_musicTable[g_currentSoundLocation];
                if (path) {
                    g_sound->PlayMusicTrack(path, g_sound->musicVolume);
                }
            }
            break;
        }

        case RS_STOP_MUSIC: // 6 - fade/stop music
            LOG("[rsEvent] StopMusic");
            g_pauseFlag = 0;
            jcsFadeOutEngine(14);
            break;

        case RS_FADE_OUT: // 7 - fade out
            LOG("[rsEvent] FadeOut(%d)", param1);
            jcsFadeOutEngine((u32)param1);
            break;

        case RS_FADE_OUT_2: // 8 - fade in
            LOG("[rsEvent] FadeIn(%d)", param1);
            if (!g_muteFlag) {
                jcsFadeInEngine((u32)param1);
            }
            break;

        case RS_PAUSE: // 9
            LOG("[rsEvent] Pause");
            g_pauseFlag = 1;
            break;

        case RS_MUTE: // 10
            LOG("[rsEvent] Mute");
            if (!g_muteFlag) {
                jcsFadeOutEngine(14);
                g_muteFlag = 1;
            }
            break;

        case RS_UNMUTE: // 11
            LOG("[rsEvent] Unmute");
            if (g_muteFlag) {
                if (!g_pauseFlag) {
                    jcsFadeInEngine(14);
                }
                g_muteFlag = 0;
            }
            break;

        case RS_CD_YIELD: // 12
            break;

        case RS_CD_ACCESS: // 13
            break;

        case RS_SET_MUSIC_VOL: // 16
        {
            // PSX: 80 * param / 100 -> SPU vol (0-100 range out of 127)
            // PC: scale 0-125 slider to 0.0-0.80 float
            f32 vol = (f32)param1 * 0.8f / 125.0f;
            LOG("[rsEvent] SetMusicVol(%d -> %.2f)", param1, vol);
            g_sound->SetMusicVolume(vol);
            break;
        }

        case RS_SET_EFFECTS_VOL_AUX: // 17
        {
            // PSX: no case handler - falls through to default (no-op)
            LOG("[rsEvent] SetEffectsVolAux(%d)", param1);
            break;
        }

        case RS_SET_EFFECTS_VOL: // 18
        {
            // PSX: stores raw value as effects vol
            // PC: scale 0-125 slider to 0.0-1.0 float
            f32 vol = (f32)param1 / 125.0f;
            LOG("[rsEvent] SetEffectsVol(%d -> %.2f)", param1, vol);
            g_sound->SetEffectsVolume(vol);
            break;
        }

        case RS_SET_DIALOG_VOL: // 19
        {
            // PSX: 83 * param / 100 -> SPU vol
            // PC: scale 0-125 slider to 0.0-0.83 float
            f32 vol = (f32)param1 * 0.83f / 125.0f;
            LOG("[rsEvent] SetDialogVol(%d -> %.2f)", param1, vol);
            g_sound->SetDialogVolume(vol);
            break;
        }

        case RS_SET_STEREO: // 14
            LOG("[rsEvent] SetStereo");
            AudioEngine::SetOutputMono(false);
            break;

        case RS_SET_MONO: // 15
            LOG("[rsEvent] SetMono");
            AudioEngine::SetOutputMono(true);
            break;

        case 20: // jcsSetAmbienceSpace + optional crossfade
            LOG("[rsEvent] SetAmbienceSpace(%d, crossfade=%d)", param1, param2);
            break;

        case 21: // jcsSetListener(playerPos, cameraData) - 3D audio listener
            rsdWorld::UpdateSpatialAudioState();
            break;

        case 22: // jcsFadeOutEngine(-1) - fade out all
            LOG("[rsEvent] FadeOutAll");
            jcsFadeOutEngine(0xFFFFFFFF);
            break;

        case 23: // jcsFadeInEngine(-1) - fade in all
            LOG("[rsEvent] FadeInAll");
            if (!g_muteFlag) {
                jcsFadeInEngine(0xFFFFFFFF);
            }
            break;

        default:
            LOG("[rsEvent] Unhandled control event %d (p1=%d, p2=%d, p3=%d)",
                event, param1, param2, param3);
            break;
    }

    return 1;
}

// PSX: jcsFadeInEngine__FUl (JCSOUND.CPP:764, 0x8003582C)
// Flags bitmask:
//   bit 1 (0x02): fade in music player
//   bit 2 (0x04): fade in ambiance
//   bit 0 (0x01): resume dialog
//   bit 3 (0x08): fade in persistent sounds + set reverb
// PC: music is the primary concern; other subsystems are stubs.
void jcsFadeInEngine(u32 flags) {
    MARKFUNCTION(0x8003582C);

    if (!g_sound) return;

    // PSX: bit 1 -> rsdMusicPlayer::FadeIn
    if (flags & 0x02) {
        LOG("[jcsFadeInEngine] FadeIn music (flags=0x%X)", flags);
        g_sound->UnmuteMusic();
    }

    // PSX: bit 2 -> rsdAmbiance::FadeIn (1500ms)
    if (flags & 0x04) {
        LOG("[jcsFadeInEngine] FadeIn ambiance (flags=0x%X)", flags);
    }

    // PSX: bit 0 -> jcsResumeDialog
    if (flags & 0x01) {
        LOG("[jcsFadeInEngine] ResumeDialog (flags=0x%X)", flags);
    }

    // PSX: bit 3 -> rsdPersistent::FadeInAll(1500) + rsdSetReverb
    if (flags & 0x08) {
        LOG("[jcsFadeInEngine] FadeIn persistent + reverb (flags=0x%X)", flags);
    }
}

// PSX: jcsFadeOutEngine__FUl (JCSOUND.CPP:721, 0x80035760)
// Flags bitmask:
//   bit 1 (0x02): fade out music player
//   bit 2 (0x04): fade out ambiance
//   bit 0 (0x01): pause dialog
//   bit 3 (0x08): fade out persistent sounds + save/clear reverb
// PC: music fade-out stops the music track.
void jcsFadeOutEngine(u32 flags) {
    MARKFUNCTION(0x80035760);

    if (!g_sound) return;

    // PSX: bit 1 -> rsdMusicPlayer::FadeOut(0)
    if (flags & 0x02) {
        LOG("[jcsFadeOutEngine] FadeOut music (flags=0x%X)", flags);
        g_sound->MuteMusic();
    }

    // PSX: bit 2 -> rsdAmbiance::FadeOut(1500)
    if (flags & 0x04) {
        LOG("[jcsFadeOutEngine] FadeOut ambiance (flags=0x%X)", flags);
    }

    // PSX: bit 0 -> jcsPauseDialog
    if (flags & 0x01) {
        LOG("[jcsFadeOutEngine] PauseDialog (flags=0x%X)", flags);
    }

    // PSX: bit 3 -> rsdPersistent::FadeOutAll(1500) + save reverb + set reverb(0)
    if (flags & 0x08) {
        LOG("[jcsFadeOutEngine] FadeOut persistent + reverb (flags=0x%X)", flags);
    }
}

// PSX: jcsValidateHandle (JCSDLG.CPP:1639, 0x800434D8)
s32 jcsValidateHandle(s32 handle) {
    return FindDialogEntry(handle) != nullptr;
}

// PSX: jcsIsPlayable (JCSDLG.CPP:428, 0x80042908)
s32 jcsIsPlayable(s32 handle) {
    return jcsValidateHandle(handle);
}

// PSX: jcsIsPlaying(handle) (JCSDLG.CPP:506, 0x800429CC)
s32 jcsIsPlaying(s32 handle) {
    DialogEntry* entry = FindDialogEntry(handle);
    if (!entry || entry->voice == AUDIO_VOICE_INVALID) {
        return 0;
    }

    return AudioEngine::IsVoicePlaying(entry->voice) ? 1 : 0;
}

// PSX: jcsQueryDialogPriority() (JCSDLG.CPP:1285, 0x80043218)
s32 jcsQueryDialogPriority() {
    s32 bestPriority = 0;

    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        DialogEntry& entry = g_dialogEntries[i];
        if (!entry.valid) {
            continue;
        }
        if (entry.voice != AUDIO_VOICE_INVALID && AudioEngine::IsVoicePlaying(entry.voice)) {
            if (entry.priority > bestPriority) {
                bestPriority = entry.priority;
            }
        }
    }

    return bestPriority;
}

// PSX: jcsQueryDialogPriority(handle) (JCSDLG.CPP:1312, 0x80043254)
s32 jcsQueryDialogPriority(s32 handle) {
    DialogEntry* entry = FindDialogEntry(handle);
    if (!entry) {
        return 0;
    }
    return entry->priority;
}

// PSX: jcsStartDialog__Fv (JCSDLG.CPP:1648, 0x800434F0)
void jcsStartDialog() {
    for (s32 i = 0; i < DIALOG_ENTRY_COUNT; ++i) {
        DialogEntry& entry = g_dialogEntries[i];
        if (entry.voice != AUDIO_VOICE_INVALID) {
            AudioEngine::StopVoice(entry.voice);
        }
        if (entry.sample != AUDIO_SAMPLE_INVALID) {
            AudioEngine::UnloadSample(entry.sample);
        }
        entry = {};
    }

    g_nextDialogHandle = 1;
}
