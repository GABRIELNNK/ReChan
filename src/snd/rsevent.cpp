// rsevent.cpp - rsEvent sound dispatch reversed from PSX RSEVENT.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\RSEVENT.CPP
// Dispatches sound events to the Sound manager.
#include "snd/rsevent.h"
#include "snd/sound.h"

// PSX: gp+544 - global sound enabled flag (0 = enabled, nonzero = disabled)
static s32 g_soundDisabled = 0;

// PSX: Sound location to music file mapping (gp-relative at 0x800ED6CC)
// Event 4 (RS_SET_LOCATION) with param1 sets the "current sound location"
// which determines which music track plays when RS_LEVEL_BEGIN fires.
s32 g_currentSoundLocation = 0;

// PSX music file table indexed by sound location
// Location 22 = title screen music
static const char* s_musicTable[] = {
    "SOUND/MUSIC/CHINA1.FAG",    //  0
    "SOUND/MUSIC/CHINA2.FAG",    //  1
    "SOUND/MUSIC/CHINA3.FAG",    //  2
    "SOUND/MUSIC/CHINAB.FAG",    //  3
    "SOUND/MUSIC/FACTORY1.FAG",  //  4
    "SOUND/MUSIC/FACTORY2.FAG",  //  5
    "SOUND/MUSIC/FACTORY3.FAG",  //  6
    "SOUND/MUSIC/FACTORYB.FAG",  //  7
    "SOUND/MUSIC/ROOF1.FAG",     //  8
    "SOUND/MUSIC/ROOF2.FAG",     //  9
    "SOUND/MUSIC/ROOF3.FAG",     // 10
    "SOUND/MUSIC/ROOFB.FAG",     // 11
    "SOUND/MUSIC/SEWER1.FAG",    // 12
    "SOUND/MUSIC/SEWER2.FAG",    // 13
    "SOUND/MUSIC/SEWER3.FAG",    // 14
    "SOUND/MUSIC/SEWERB.FAG",    // 15
    "SOUND/MUSIC/WATER1.FAG",    // 16
    "SOUND/MUSIC/WATER2.FAG",    // 17
    "SOUND/MUSIC/WATER3.FAG",    // 18
    "SOUND/MUSIC/WATERB.FAG",    // 19
    "SOUND/MUSIC/TEMPLE.FAG",    // 20
    "SOUND/MUSIC/GAMEOVER.FAG",  // 21
    "SOUND/MUSIC/TITLE.FAG",     // 22
    "SOUND/MUSIC/DESTSEL.FAG",   // 23
    nullptr,                      // 24 (movie music - no FAG)
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
        // rsDialogEvent - dialog playback (not implemented yet)
        LOG("[rsEvent] Dialog event %d (param1=%d)", event, param1);
        return 0;
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
            break;

        case RS_LEVEL_BEGIN: // 5 - start music for current location
        {
            LOG("[rsEvent] LevelBegin - start music (location=%d)", g_currentSoundLocation);
            if (g_currentSoundLocation >= 0 && g_currentSoundLocation < MUSIC_TABLE_COUNT) {
                const char* path = s_musicTable[g_currentSoundLocation];
                if (path) {
                    g_sound->PlayMusicTrack(path, 0.7f);
                }
            }
            break;
        }

        case RS_STOP_MUSIC: // 6 - fade/stop music
            LOG("[rsEvent] StopMusic");
            g_sound->StopMusic();
            break;

        case RS_FADE_OUT: // 7 - fade out
        case RS_FADE_OUT_2: // 8 - fade out alternate
            LOG("[rsEvent] FadeOut(%d)", param1);
            g_sound->StopMusic();
            break;

        case RS_PAUSE: // 9
            LOG("[rsEvent] Pause");
            break;

        case RS_MUTE: // 10
            LOG("[rsEvent] Mute");
            break;

        case RS_UNMUTE: // 11
            LOG("[rsEvent] Unmute");
            break;

        case RS_CD_YIELD: // 12
            break;

        case RS_CD_ACCESS: // 13
            break;

        case RS_SET_SFX_VOL: // 16
        {
            f32 vol = (f32)param1 * 0.8f / 100.0f;
            LOG("[rsEvent] SetSFXVol(%d -> %.2f)", param1, vol);
            break;
        }

        case RS_SET_MUSIC_VOL: // 18
        {
            f32 vol = (f32)param1 * 0.8f / 100.0f;
            LOG("[rsEvent] SetMusicVol(%d -> %.2f)", param1, vol);
            g_sound->SetMusicVolume(vol);
            break;
        }

        case 20: // jcsSetAmbienceSpace + optional crossfade
            LOG("[rsEvent] SetAmbienceSpace(%d, crossfade=%d)", param1, param2);
            break;

        case 21: // jcsSetListener(playerPos, cameraData) - 3D audio listener
            LOG("[rsEvent] SetListener (stub)");
            break;

        case 22: // jcsFadeOutEngine(-1) - fade out all
            LOG("[rsEvent] FadeOutAll");
            g_sound->StopMusic();
            break;

        case 23: // jcsFadeInEngine(-1) - fade in all
            LOG("[rsEvent] FadeInAll");
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
        // Music is already playing; just ensure volume is restored
        // PSX: calls FadeIn on the music player object at gp+736
        LOG("[jcsFadeInEngine] FadeIn music (flags=0x%X)", flags);
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
        g_sound->StopMusic();
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
