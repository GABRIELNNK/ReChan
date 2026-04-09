#include "snd/fesnd.h"
#include "snd/rsevent.h"

// Global front-end sound object (PSX: gp+72, 0x800ED648)
CFrontEndSound* g_frontEndSound = nullptr;

// PSX: __14CFrontEndSound (FESND.CPP:121, 0x800535FC)
// Calls CSound base constructor, then sets factory table to 0x800CD040
// and zeroes the 4 extra fields.
CFrontEndSound::CFrontEndSound() {
    MARKFUNCTION(0x800535FC);
    // CSound base constructor called implicitly
    // PSX: this[12] = 0x800CD040 (CFrontEndSound vtable)
    persistent1 = nullptr;  // +16
    persistent2 = nullptr;  // +20
    persistent3 = nullptr;  // +24
    fadeState = 0;          // +28
}

CFrontEndSound::~CFrontEndSound() {}

// PSX: ProcessSoundEvent__14CFrontEndSound (FESND.CPP:18, 0x800534C8)
// Dispatch logic:
//   if (event-2 < 4) -> HandleCursorEvent        [events 2,3,4,5]
//   elif (event-8 < 10) -> ProcessLocationSpecificSound [events 8-17]
//   elif (event-19 < 13) -> jump table            [events 19-31]
//   else -> return -1000
void CFrontEndSound::ProcessSoundEvent(s32 event) {
    MARKFUNCTION(0x800534C8);

    // Events 2-5: cursor events
    if (event >= 2 && event <= 5) {
        HandleCursorEvent(event);
        return;
    }

    // Events 8-17: location-specific sounds
    if (event >= 8 && event <= 17) {
        ProcessLocationSpecificSound(event);
        return;
    }

    // Events 19-31: jump table -> PlayTransient with specific sample IDs
    if (event >= 19 && event <= 31) {
        s32 idx = event - 19;
        switch (idx) {
            case 0: // event 19: PlayTransient(222, 8, 0)
                PlayTransient(222, 8, 0);
                return;
            case 1: // event 20: PlayTransient(248, 8, 0)
                PlayTransient(248, 8, 0);
                return;
            case 2: // event 21: PlayTransient(249, 8, 0)
                PlayTransient(249, 8, 0);
                return;
            case 3: // event 22: PlayTransient(250, 8, 0)
                PlayTransient(250, 8, 0);
                return;
            case 4: // event 23: PlayTransient(251, 8, 0)
                PlayTransient(251, 8, 0);
                return;
            case 5: // event 24: PlayTransient(252, 8, 0)
                PlayTransient(252, 8, 0);
                return;
            case 6: // event 25: PlayTransient(253, 8, 0)
                PlayTransient(253, 8, 0);
                return;
            case 7: // event 26: BeginPersistent(0, &persistent1)
                BeginPersistent(0, &persistent1);
                return;
            case 8: // event 27: EndPersistent(&persistent1)
                EndPersistent(&persistent1);
                return;
            case 9: // event 28: BeginPersistent(1, &persistent2)
                BeginPersistent(1, &persistent2);
                return;
            case 10: // event 29: EndPersistent(&persistent2)
                EndPersistent(&persistent2);
                return;
            case 11: // event 30: BeginPersistent(2, &persistent3)
                BeginPersistent(2, &persistent3);
                return;
            case 12: // event 31: EndPersistent(&persistent3)
                EndPersistent(&persistent3);
                return;
        }
    }

    // PSX: returns -1000 for unhandled events
}

// PSX: ProcessLocationSpecificSound__14CFrontEndSound (FESND.CPP:152, 0x800536E0)
// Reads g_currentSoundLocation from 0x800ED6CC.
// If location >= 21: stereo mode (s1=1), else mono (s1=0).
// Jump table indexed by (event - 8) selects global sample ID.
// Stereo: PlayTransientStereo(id, id+1); Mono: PlayTransient(id, 8, 0).
void CFrontEndSound::ProcessLocationSpecificSound(s32 event) {
    MARKFUNCTION(0x800536E0);

    // PSX: s1 = (location < 21) ? 0 : 1
    s32 stereo = (g_currentSoundLocation >= 21) ? 1 : 0;

    s32 idx = event - 8;
    if (idx < 0 || idx >= 10) return;

    u16 sampleId = 0;

    switch (idx) {
        case 0: // event 8: menu open
            sampleId = 217;
            if (stereo) { PlayTransientStereo(217, 218); return; }
            break;
        case 1: // event 9: menu close
            if (stereo) { sampleId = 219; PlayTransientStereo(219, 220); return; }
            sampleId = 221;
            break;
        case 2: // event 10:
            if (stereo) { sampleId = 224; PlayTransientStereo(224, 225); return; }
            sampleId = 223;
            break;
        case 3: // event 11:
            if (stereo) { sampleId = 227; PlayTransientStereo(227, 228); return; }
            sampleId = 226;
            break;
        case 4: // event 12: special - calls HandleCursorEvent(5) first
            if (stereo) sampleId = 239; else sampleId = 238;
            HandleCursorEvent(5);
            if (stereo) { PlayTransientStereo(238, 239); return; }
            break;
        case 5: // event 13:
            if (stereo) { sampleId = 242; PlayTransientStereo(242, 243); return; }
            sampleId = 241;
            break;
        case 6: // event 14:
            if (stereo) { sampleId = 245; PlayTransientStereo(245, 246); return; }
            sampleId = 244;
            break;
        case 7: // event 15:
            if (stereo) { sampleId = 230; PlayTransientStereo(230, 231); return; }
            sampleId = 229;
            break;
        case 8: // event 16:
            if (stereo) { sampleId = 233; PlayTransientStereo(233, 234); return; }
            sampleId = 232;
            break;
        case 9: // event 17: special - checks fadeState
            if (fadeState != 0) {
                // PSX: PlayTransient(247, 1, 0) when fadeState is set
                PlayTransient(247, 1, 0);
                return;
            }
            if (stereo) { sampleId = 236; PlayTransientStereo(236, 237); return; }
            sampleId = 235;
            break;
    }

    // Mono path: PlayTransient(sampleId, 8, 0)
    PlayTransient(sampleId, 8, 0);
}

// PSX: HandleCursorEvent__14CFrontEndSound (FESND.CPP:250, 0x80053850)
// event==2: jcsFadeInEngine(2)
// else: jcsFadeOutEngine(2)
// Then: if event==3, fadeState=1; else fadeState=0
void CFrontEndSound::HandleCursorEvent(s32 event) {
    MARKFUNCTION(0x80053850);

    if (event == 2) {
        jcsFadeInEngine(2);
    }
    else {
        jcsFadeOutEngine(2);
    }

    if (event == 3) {
        fadeState = 1;
    }
    else {
        fadeState = 0;
    }
}
