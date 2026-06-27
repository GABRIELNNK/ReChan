#pragma once
#include "core.h"

// 256 transient sound entries. Each maps to an rsd sample + volume/pitch params.
// The rsd sample (0-70) indexes the currently active WAX bank's descriptor table.

// Transient sound IDs - used with CSound::PlayTransient / PlayTransientStereo.
// IDs 0-216: gameplay sounds (hit, env, footstep, etc.)
// IDs 217-253: front-end UI sounds
// IDs 254-255: unused/padding
enum SoundId : u16 {
    // Gameplay - combat
    SND_HIT_0              = 0,   // rsd 38
    SND_HIT_1              = 1,   // rsd 37
    SND_HIT_2              = 2,   // rsd 67
    SND_HIT_3              = 3,   // rsd 36
    SND_SILENT_0           = 4,   // rsd 8, vol 0 (muted placeholder)
    SND_SILENT_1           = 5,   // rsd 8, vol 0
    SND_SILENT_2           = 6,   // rsd 8, vol 0
    SND_SILENT_3           = 7,   // rsd 8, vol 0
    SND_IMPACT_0           = 8,   // rsd 35
    SND_IMPACT_1           = 9,   // rsd 37
    SND_IMPACT_2           = 10,  // rsd 36
    SND_IMPACT_3           = 11,  // rsd 35
    SND_FOOTSTEP_0         = 12,  // rsd 24
    SND_FOOTSTEP_1         = 13,  // rsd 27
    SND_FOOTSTEP_2         = 14,  // rsd 28
    SND_WHOOSH_0           = 15,  // rsd 46
    SND_PUNCH_0            = 16,  // rsd 26
    SND_PUNCH_1            = 17,  // rsd 34
    SND_PUNCH_2            = 18,  // rsd 33
    SND_PUNCH_3            = 19,  // rsd 32
    SND_KICK_0             = 20,  // rsd 26
    SND_KICK_1             = 21,  // rsd 35
    SND_BLOCK              = 22,  // rsd 31
    SND_EXPLODE            = 23,  // rsd 68
    SND_BREAK_0            = 24,  // rsd 55
    SND_BREAK_1            = 25,  // rsd 62
    SND_THROW_0            = 26,  // rsd 35
    SND_THROW_1            = 27,  // rsd 35
    SND_THROW_2            = 28,  // rsd 35
    SND_THROW_3            = 29,  // rsd 35
    SND_FALL_0             = 30,  // rsd 23
    SND_FALL_1             = 31,  // rsd 22
    SND_FALL_2             = 32,  // rsd 21
    SND_FALL_3             = 33,  // rsd 47
    SND_LAND_0             = 34,  // rsd 19
    SND_LAND_1             = 35,  // rsd 20
    SND_LAND_2             = 36,  // rsd 17
    SND_LAND_3             = 37,  // rsd 18
    SND_LAND_4             = 38,  // rsd 0
    SND_ENV_0              = 39,  // rsd 60
    SND_ENV_1              = 40,  // rsd 56
    SND_ENV_FOOTSTEP_0     = 41,  // rsd 24
    SND_ENV_FOOTSTEP_1     = 42,  // rsd 27
    SND_ENV_FOOTSTEP_2     = 43,  // rsd 28
    SND_ENV_FOOTSTEP_3     = 44,  // rsd 27
    SND_ENV_STEP           = 45,  // rsd 8
    // 46-55: enemy/player combat variants
    // 56-65: additional sound variants
    // 66-215: level / event specific sounds
    // 216: more combat

    // Front-end UI sounds - used by CFrontEndSound
    SND_FE_MENU_OPEN_L     = 217, // rsd 25, stereo left channel
    SND_FE_MENU_OPEN_R     = 218, // rsd 25, stereo right channel
    SND_FE_MENU_CLOSE_L    = 219, // rsd 25, stereo left channel
    SND_FE_MENU_CLOSE_R    = 220, // rsd 25, stereo right channel
    SND_FE_MENU_CLOSE_MONO = 221, // rsd 25, mono fallback
    SND_FE_CONFIRM         = 222, // rsd 66, "press start" / confirm jingle
    SND_FE_SELECT_MONO     = 223, // rsd 68, mono select
    SND_FE_SELECT_L        = 224, // rsd 68, stereo left
    SND_FE_SELECT_R        = 225, // rsd 68, stereo right
    SND_FE_BACK_MONO       = 226, // rsd 32, mono back
    SND_FE_BACK_L          = 227, // rsd 57, stereo left
    SND_FE_BACK_R          = 228, // rsd 45, stereo right
    SND_FE_CURSOR_MONO     = 229, // rsd 32, mono cursor
    SND_FE_CURSOR_L        = 230, // rsd 32, stereo left
    SND_FE_CURSOR_R        = 231, // rsd 32, stereo right
    SND_FE_SCROLL_MONO     = 232, // rsd 32, mono scroll
    SND_FE_SCROLL_L        = 233, // rsd 32, stereo left
    SND_FE_SCROLL_R        = 234, // rsd 32, stereo right
    SND_FE_TRANS_MONO      = 235, // rsd 32, mono transition
    SND_FE_TRANS_L         = 236, // rsd 32, stereo left
    SND_FE_TRANS_R         = 237, // rsd 32, stereo right
    SND_FE_ACCEPT_L        = 238, // rsd 32, "accept" stereo left
    SND_FE_ACCEPT_R        = 239, // rsd 32, "accept" stereo right
    SND_FE_240             = 240, // unused gap
    SND_FE_CANCEL_MONO     = 241, // rsd 34, cancel mono
    SND_FE_CANCEL_L        = 242, // rsd 34, stereo left
    SND_FE_CANCEL_R        = 243, // rsd 34, stereo right
    SND_FE_TOGGLE_MONO     = 244, // rsd 66, toggle mono
    SND_FE_TOGGLE_L        = 245, // rsd 66, stereo left
    SND_FE_TOGGLE_R        = 246, // rsd 66, stereo right
    SND_FE_FADE            = 247, // rsd 66, fade transition
    SND_FE_JINGLE_1        = 248, // rsd 12, jingle trigger 1
    SND_FE_JINGLE_2        = 249, // rsd 44, jingle trigger 2
    SND_FE_JINGLE_3        = 250, // rsd 12, jingle trigger 3
    SND_FE_JINGLE_4        = 251, // rsd 43, jingle trigger 4
    SND_FE_JINGLE_5        = 252, // rsd 28, jingle trigger 5
    SND_FE_JINGLE_6        = 253, // rsd 35, jingle trigger 6

    SND_INVALID            = 0xFFFF,
};

// Persistent sound IDs - used with CSound::BeginPersistent.
// Separate from transient IDs; indexed into g_persistData.
enum PersistentSoundId : u8 {
    PSND_LOOP_0 = 0,
    PSND_LOOP_1 = 1,
    PSND_LOOP_2 = 2,
};

// Sound locations - passed to RS_SET_LOCATION.
// Determines both the music track and the active WAX SFX bank.
enum SoundLocation : s32 {
    SND_LOC_CHINA_1     = 0,
    SND_LOC_CHINA_2     = 1,
    SND_LOC_CHINA_3     = 2,
    SND_LOC_CHINA_BOSS  = 3,
    SND_LOC_WATER_1     = 4,
    SND_LOC_WATER_2     = 5,
    SND_LOC_WATER_3     = 6,
    SND_LOC_WATER_BOSS  = 7,
    SND_LOC_SEWER_1     = 8,
    SND_LOC_SEWER_2     = 9,
    SND_LOC_SEWER_3     = 10,
    SND_LOC_SEWER_BOSS  = 11,
    SND_LOC_ROOF_1      = 12,
    SND_LOC_ROOF_2      = 13,
    SND_LOC_ROOF_3      = 14,
    SND_LOC_ROOF_BOSS   = 15,
    SND_LOC_FACTORY_1   = 16,
    SND_LOC_FACTORY_2   = 17,
    SND_LOC_FACTORY_3   = 18,
    SND_LOC_FACTORY_BOSS= 19,
    SND_LOC_TEMPLE      = 20,
    SND_LOC_DESTSEL     = 21,  // destination select
    SND_LOC_TITLE       = 22,  // front-end / title screen
    SND_LOC_GAMEOVER    = 23,
    SND_LOC_MOVIE       = 24,  // movie playback (no music FAG)
};
