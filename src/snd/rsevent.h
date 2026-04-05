// rsevent.h - rsEvent sound dispatch reversed from PSX RSEVENT.CPP
// PSX source: C:\CHAN\GAME\SRC\SND\RSEVENT.CPP
// rsEvent is the central sound event dispatcher used throughout the game.
// Events 1-23 route to jcsHandleControlEvent, events 26-31 to rsDialogEvent.
#pragma once

#include "gen/common.h"

// PSX: rsSoundEvent enum values (inferred from usage)
enum rsSoundEvent : s32 {
    RS_INITIALIZE      = 1,   // jcsInitialize
    RS_TERMINATE       = 2,   // jcsTerminate
    RS_UNLOAD_LEVEL    = 3,   // jcsUnloadLevel - stop all sounds
    RS_SET_LOCATION    = 4,   // jcsSetSoundLocation(param1) - set music track
    RS_LEVEL_BEGIN     = 5,   // LevelBegin + jcsStartSound - start music
    RS_STOP_MUSIC      = 6,   // jcsFadeOutEngine(14) - fade/stop music
    RS_FADE_OUT        = 7,   // jcsFadeOutEngine(param1)
    RS_FADE_OUT_2      = 8,   // jcsFadeOutEngine(param1) alternate
    RS_PAUSE           = 9,   // set pause flag
    RS_MUTE            = 10,  // mute engine
    RS_UNMUTE          = 11,  // unmute engine
    RS_CD_YIELD        = 12,  // jcsCdYield(param1)
    RS_CD_ACCESS       = 13,  // jcsCdAccess(param1)
    RS_SET_STEREO      = 14,  // set stereo mode
    RS_SET_MONO        = 15,  // set mono mode
    RS_SET_SFX_VOL     = 16,  // set SFX volume
    RS_NOP             = 17,  // no-op
    RS_SET_MUSIC_VOL   = 18,  // set music volume
    // Events 19-23: additional control events
    // Events 26-31: dialog events
    RS_LOAD_DIALOG     = 26,
    RS_PLAY_DIALOG     = 27,
    RS_STOP_DIALOG     = 28,
    RS_KILL_DIALOG     = 29,
};

// PSX: rsEvent (RSEVENT.CPP:48, 0x800346B0)
// Central sound event dispatcher
s32 rsEvent(s32 event, s32 param1, s32 param2, s32 param3);

// PSX: jcsHandleControlEvent (JCSOUND.CPP:947, 0x80035B00)
// Handles control events 1-23
s32 jcsHandleControlEvent(s32 event, s32 param1, s32 param2, s32 param3);

// PSX: jcsFadeInEngine (JCSOUND.CPP:764, 0x8003582C)
// Fades in sound engine channels based on bitmask flags.
void jcsFadeInEngine(u32 flags);

// PSX: jcsFadeOutEngine (JCSOUND.CPP:721, 0x80035760)
// Fades out sound engine channels based on bitmask flags.
void jcsFadeOutEngine(u32 flags);

// PSX: g_currentSoundLocation (gp-relative at 0x800ED6CC)
// Current sound location index (set by RS_SET_LOCATION event)
extern s32 g_currentSoundLocation;
