#pragma once

#include "core.h"
#include "fe/hdmenu.h"
#include "fe/fe_sound_slider_helpers.h"
#include "snd/rsevent.h"
#include "snd/sound.h"
#include "snd/fesnd.h"

inline s32 FeSoundMenuSetMusicVolume(hdMenuItem* item) {
    const u32 raw = item->GetValue();
    const u32 val = FeSoundSliderToFlag(raw);
    rsEvent(RS_SET_MUSIC_VOL, (s32)val, 0, 0);
    if (g_sound) {
        g_sound->flag0 = (s16)val;
    }
    return 8;
}

inline s32 FeSoundMenuSetEffectsVolume(hdMenuItem* item) {
    const u32 raw = item->GetValue();
    const u32 val = FeSoundSliderToFlag(raw);
    rsEvent(RS_SET_EFFECTS_VOL, (s32)val, 0, 0);
    rsEvent(RS_SET_EFFECTS_VOL_AUX, (s32)val, 0, 0);
    if (g_sound) {
        g_sound->flag2 = (s16)val;
    }
    return 8;
}

inline s32 FeSoundMenuSetDialogVolume(hdMenuItem* item) {
    const u32 raw = item->GetValue();
    const u32 val = FeSoundSliderToFlag(raw);
    rsEvent(RS_SET_DIALOG_VOL, (s32)val, 0, 0);
    if (g_sound) {
        g_sound->flag1 = (s16)val;
    }
    return 8;
}

inline s32 FeSoundMenuSetStereoOnOff(hdMenuItem* item) {
    if (item->GetValue()) {
        rsEvent(RS_SET_STEREO, 0, 0, 0);
        if (g_sound) {
            g_sound->activeFlag = 1;
        }
    }
    else {
        rsEvent(RS_SET_MONO, 0, 0, 0);
        if (g_sound) {
            g_sound->activeFlag = 0;
        }
    }
    return 8;
}

inline void FeInstallSoundMenuCommon(
    hdMenu* menu,
    u32 hashSoundEffect,
    u32 hashSoundMusic,
    u32 hashSoundVoice,
    u32 hashSoundStereo,
    hdMenuItemCallback effectsCallback,
    hdMenuItemCallback musicCallback,
    hdMenuItemCallback dialogCallback,
    hdMenuItemCallback stereoCallback) {
    if (!menu || !g_sound) {
        return;
    }

    menu->SetCallback(hashSoundEffect, effectsCallback);
    hdMenuItem* effectsItem = menu->FindItem(hashSoundEffect);
    if (effectsItem) {
        effectsItem->SetValue(FeSoundFlagToSlider((u32)(u16)g_sound->flag2));
    }

    menu->SetCallback(hashSoundMusic, musicCallback);
    hdMenuItem* musicItem = menu->FindItem(hashSoundMusic);
    if (musicItem) {
        musicItem->SetValue(FeSoundFlagToSlider((u32)(u16)g_sound->flag0));
    }

    menu->SetCallback(hashSoundVoice, dialogCallback);
    hdMenuItem* voiceItem = menu->FindItem(hashSoundVoice);
    if (voiceItem) {
        voiceItem->SetValue(FeSoundFlagToSlider((u32)(u16)g_sound->flag1));
    }

    menu->SetCallback(hashSoundStereo, stereoCallback);
    hdMenuItem* stereoItem = menu->FindItem(hashSoundStereo);
    if (stereoItem) {
        stereoItem->SetValue(g_sound->activeFlag != 0);
    }

    if (g_frontEndSound) {
        g_frontEndSound->ProcessSoundEvent(FE_SND_CURSOR_BACK);
    }
}