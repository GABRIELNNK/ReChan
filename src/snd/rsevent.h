#pragma once
#include "core.h"

// PSX: rsSoundEvent enum values (inferred from usage)
enum rsSoundEvent : s32 {
    RS_INITIALIZE = 1,   // jcsInitialize
    RS_TERMINATE = 2,   // jcsTerminate
    RS_UNLOAD_LEVEL = 3,   // jcsUnloadLevel - stop all sounds
    RS_SET_LOCATION = 4,   // jcsSetSoundLocation(param1) - set music track
    RS_LEVEL_BEGIN = 5,   // LevelBegin + jcsStartSound - start music
    RS_STOP_MUSIC = 6,   // jcsFadeOutEngine(14) - fade/stop music
    RS_FADE_OUT = 7,   // jcsFadeOutEngine(param1)
    RS_FADE_OUT_2 = 8,   // jcsFadeOutEngine(param1) alternate
    RS_PAUSE = 9,   // set pause flag
    RS_MUTE = 10,  // mute engine
    RS_UNMUTE = 11,  // unmute engine
    RS_CD_YIELD = 12,  // jcsCdYield(param1)
    RS_CD_ACCESS = 13,  // jcsCdAccess(param1)
    RS_SET_STEREO = 14,  // set stereo mode
    RS_SET_MONO = 15,  // set mono mode
    RS_SET_MUSIC_VOL = 16,  // SetMusicVolume__FP10hdMenuItem
    RS_SET_EFFECTS_VOL_AUX = 17,  // secondary effects/ambience volume path
    RS_SET_EFFECTS_VOL = 18,  // SetEffectsVolume__FP10hdMenuItem
    RS_SET_DIALOG_VOL = 19,  // SetDialogVolume__FP10hdMenuItem
    // Events 19-23: additional control events
    // Events 26-31: dialog events
    RS_LOAD_DIALOG = 26,
    RS_PLAY_DIALOG = 27,
    RS_STOP_DIALOG = 28,
    RS_KILL_DIALOG = 29,
    RS_LOAD_AND_PLAY_DIALOG = 30,
    RS_QUERY_DIALOG_PRIORITY = 31,
};

// Central sound event dispatcher
s32 rsEvent(s32 event, s32 param1, s32 param2, s32 param3);

// Handles control events 1-23
s32 jcsHandleControlEvent(s32 event, s32 param1, s32 param2, s32 param3);

// Fades in sound engine channels based on bitmask flags.
void jcsFadeInEngine(u32 flags);

// Fades out sound engine channels based on bitmask flags.
void jcsFadeOutEngine(u32 flags);

// Current sound location index (set by RS_SET_LOCATION event)
extern s32 g_currentSoundLocation;

extern bool g_deferLevelBeginMusic;

// PSX: CInteractiveMusicController::Think (MSCCTRLR.CPP:56)
// Runs per-frame during gameplay; switches FAG song section based on player block.
void InteractiveMusicControllerThink();

// Ticks the level ambience (rsdAmbiance) fades/crossfades. Must be called once
// per frame in every game state - from the main loop and the blocking
// menu-fade/loading sub-loops - so fades advance during menus and transitions.
void jcsUpdateAmbience();

// Returns non-zero if the dialog handle is still valid.
s32 jcsValidateHandle(s32 handle);

// Returns non-zero if the loaded dialog handle is currently playable.
s32 jcsIsPlayable(s32 handle);

// Returns non-zero if the current PSX dialog handle is playing.
s32 jcsIsPlaying();

// Returns non-zero if the loaded dialog handle is currently playing.
s32 jcsIsPlaying(s32 handle);

// Queries dialog priority for currently active dialog.
s32 jcsQueryDialogPriority();

// Queries dialog priority for a specific dialog handle.
s32 jcsQueryDialogPriority(s32 handle);

// Initializes dialog runtime state for a fresh load.
void jcsStartDialog();

// Menu/UI helper: stops any current dialog and plays one exact randomized
// variant. Returns the dialog handle, or 0 if it could not be loaded.
s32 jcsPlaySpecificDialog(s32 character, s32 dialogId, u32 variant, u32 timeout = 360);
