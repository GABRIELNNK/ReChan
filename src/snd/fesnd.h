#pragma once
#include "snd/basesnd.h"

// CFrontEndSound extends CSound for front-end menu/title screen sounds.
// Dispatches FrontEndSoundEvents to appropriate playback methods.

class CGenericPersistentSound;

// PSX: FrontEndSoundEvent enum (from FESND.CPP usage)
// These are the event IDs passed to ProcessSoundEvent.
enum FrontEndSoundEvent : s32 {
    FE_SND_CURSOR_OPEN = 2,   // cursor enter -> jcsFadeInEngine(2)
    FE_SND_CURSOR_SELECT = 3,   // cursor select -> jcsFadeInEngine, set fadeState=1
    FE_SND_CURSOR_BACK = 4,   // cursor back -> jcsFadeOutEngine(2)
    FE_SND_CURSOR_CANCEL = 5,   // cursor cancel -> jcsFadeOutEngine(2)
    FE_SND_MENU_OPEN = 8,   // location-specific: menu open sound
    FE_SND_MENU_CLOSE = 9,   // location-specific
    FE_SND_MENU_MOVE = 10,  // location-specific
    FE_SND_MENU_ACCEPT = 11,  // location-specific
    FE_SND_MENU_SPECIAL_4 = 12,  // location-specific (calls HandleCursorEvent(5))
    FE_SND_MENU_5 = 13,  // location-specific
    FE_SND_MENU_6 = 14,  // location-specific
    FE_SND_MENU_7 = 15,  // location-specific
    FE_SND_MENU_8 = 16,  // location-specific
    FE_SND_MENU_9 = 17,  // location-specific: special (checks fadeState)
    // Events 19-31: jump table -> PlayTransient with specific sample IDs
    FE_SND_JT_0 = 19,  // PlayTransient(222, 8, 0)
    FE_SND_JT_1 = 20,  // PlayTransient(248, 8, 0)
    FE_SND_JT_2 = 21,  // PlayTransient(249, 8, 0)
    FE_SND_JT_3 = 22,  // PlayTransient(250, 8, 0)
    FE_SND_JT_4 = 23,  // PlayTransient(251, 8, 0)
    FE_SND_JT_5 = 24,  // PlayTransient(252, 8, 0)
    FE_SND_JT_6 = 25,  // PlayTransient(253, 8, 0)
    // 26 = BeginPersistent(0, &persistent1)
    // 27 = EndPersistent(&persistent1)
    // 28 = BeginPersistent(1, &persistent2)
    // 29 = EndPersistent(&persistent2)
    // 30 = BeginPersistent(2, &persistent3)
    // 31 = EndPersistent(&persistent3)
};

// PSX: CFrontEndSound (32 bytes, FESND.CPP) extends CSound (16 bytes)
// PSX layout:
//   +0:  CSound base (16 bytes)
//   +16: persistent1 (void*, init=0) - CGenericPersistentSound*
//   +20: persistent2 (void*, init=0)
//   +24: persistent3 (void*, init=0)
//   +28: fadeState (u32, init=0)
class CFrontEndSound : public CSound {
public:
    CGenericPersistentSound* persistent1;  // +16
    CGenericPersistentSound* persistent2;  // +20
    CGenericPersistentSound* persistent3;  // +24
    u32 fadeState;       // +28: 0 or 1 (set by HandleCursorEvent)

    CFrontEndSound();
    ~CFrontEndSound() override;

    // Main dispatch: routes event to HandleCursorEvent, ProcessLocationSpecificSound,
    // or the jump table for events 19-31.
    void ProcessSoundEvent(s32 event);

    // Handles events 8-17 with sound IDs that vary by current game location.
    // Mono (location < 21) uses PlayTransient; stereo uses PlayTransientStereo.
    void ProcessLocationSpecificSound(s32 event);

    // event==2: jcsFadeInEngine(2), if event==3 set fadeState=1
    // else: jcsFadeOutEngine(2), if event!=3 set fadeState=0
    void HandleCursorEvent(s32 event);
};

// Global front-end sound object (PSX: gp+72)
extern CFrontEndSound* g_frontEndSound;
