#include "pc/settings.h"
#include "pc/audio.h"
#include "pc/log.h"
#include "gen/ccfile.h"
#include "gen/control.h"
#include "gen/display.h"
#include "gen/time.h"
#include "pc/inputaction.h"
#include "extra/customtext.h"
#include "p3d/context.h"
#include "snd/sound.h"
#include "snd/rsevent.h"
#include "extra/shadowcsm.h"
#include "extra/controllerpromptstyle.h"
#include "extra/fecustommenumgr.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <cstdio>
#include <cstring>
#include <cstdlib>

GameSettings g_settings;

static constexpr s32 kFallbackScreenWidth = 1280;
static constexpr s32 kFallbackScreenHeight = 720;

static s32 NormalizeFrameRateSetting(s32 fps) {
    if (fps <= 0) {
        return 0;
    }

    static const s32 kAllowed[] = { 30, 60, 120 };
    s32 closest = kAllowed[0];
    s32 closestDist = (fps > closest) ? (fps - closest) : (closest - fps);

    for (u32 i = 1; i < (u32)(sizeof(kAllowed) / sizeof(kAllowed[0])); i++) {
        const s32 candidate = kAllowed[i];
        const s32 dist = (fps > candidate) ? (fps - candidate) : (candidate - fps);
        if (dist < closestDist) {
            closest = candidate;
            closestDist = dist;
        }
    }

    return closest;
}

struct SettingDef {
    const char* section;
    const char* key;
    s32 defaultValue;
    s32 minValue;
    s32 maxValue;
    s32 (*GetValue)();
    void (*SetValue)(s32 value);
};

static void GetResolutionDimensions(s32& outWidth, s32& outHeight) {
    outWidth = kFallbackScreenWidth;
    outHeight = kFallbackScreenHeight;

    if (g_display) {
        outWidth = g_display->GetScreenWidth();
        outHeight = g_display->GetScreenHeight();
    }
}

static void ApplyResolutionDimensions(s32 width, s32 height) {
    if (!g_display || width <= 0 || height <= 0) {
        return;
    }

    const s32 count = g_display->GetResolutionCount();
    if (count <= 0) {
        return;
    }

    s32 bestIndex = 0;
    s64 bestScore = 0x7fffffffffffffffLL;

    for (s32 i = 0; i < count; i++) {
        pddiVideoMode mode;
        if (!g_display->GetResolutionMode(i, mode)) {
            continue;
        }

        const s64 dw = (s64)mode.width - (s64)width;
        const s64 dh = (s64)mode.height - (s64)height;
        const s64 score = dw * dw + dh * dh;
        if (score < bestScore) {
            bestScore = score;
            bestIndex = i;
            if (score == 0) {
                break;
            }
        }
    }

    pddiVideoMode bestMode = {};
    if (!g_display->GetResolutionMode(bestIndex, bestMode)) {
        return;
    }

    g_display->SetResolutionIndex(bestIndex);

    LOG("[Settings] Resolution apply: requested=%dx%d matched=%dx%d index=%d mode=%d",
        width,
        height,
        bestMode.width,
        bestMode.height,
        bestIndex,
        g_display->GetScreenMode());

    // In windowed/borderless modes, allow exact requested window size even when
    // monitor mode enumeration has no exact match (common on ultrawide setups).
    if ((bestMode.width != width || bestMode.height != height)
        && g_display->GetScreenMode() != ScreenMode_Fullscreen
        && p3d::display) {
        p3d::display->SetResolution(width, height);
        LOG("[Settings] Resolution apply: forced exact window size %dx%d", width, height);
    }
}

static char* TrimInPlace(char* text) {
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        text++;
    }

    size_t len = strlen(text);
    while (len > 0) {
        char ch = text[len - 1];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            break;
        }
        text[len - 1] = '\0';
        len--;
    }

    return text;
}

static bool EqualsIgnoreCase(const char* lhs, const char* rhs) {
    while (*lhs && *rhs) {
        char lc = *lhs;
        char rc = *rhs;
        if (lc >= 'A' && lc <= 'Z') {
            lc = (char)(lc + ('a' - 'A'));
        }
        if (rc >= 'A' && rc <= 'Z') {
            rc = (char)(rc + ('a' - 'A'));
        }
        if (lc != rc) {
            return false;
        }
        lhs++;
        rhs++;
    }
    return *lhs == '\0' && *rhs == '\0';
}

static bool ParseSettingValue(const char* text, s32& outValue) {
    char* end = nullptr;
    long parsed = strtol(text, &end, 10);
    if (end != text) {
        while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
            end++;
        }
        if (*end == '\0') {
            outValue = (s32)parsed;
            return true;
        }
    }

    if (EqualsIgnoreCase(text, "true") || EqualsIgnoreCase(text, "on") || EqualsIgnoreCase(text, "yes")) {
        outValue = 1;
        return true;
    }
    if (EqualsIgnoreCase(text, "false") || EqualsIgnoreCase(text, "off") || EqualsIgnoreCase(text, "no")) {
        outValue = 0;
        return true;
    }
    return false;
}

static s32 ClampForSetting(const SettingDef& def, s32 value) {
    if (value < def.minValue) {
        return def.minValue;
    }
    if (value > def.maxValue) {
        return def.maxValue;
    }
    return value;
}

static s32 GetMusicVolume() {
    return g_sound ? (s32)g_sound->flag0 : 100;
}

static void SetMusicVolume(s32 value) {
    if (!g_sound) {
        return;
    }
    g_sound->flag0 = (s16)value;
    rsEvent(RS_SET_MUSIC_VOL, value, 0, 0);
}

static s32 GetEffectsVolume() {
    return g_sound ? (s32)g_sound->flag2 : 100;
}

static void SetEffectsVolume(s32 value) {
    if (!g_sound) {
        return;
    }
    g_sound->flag2 = (s16)value;
    rsEvent(RS_SET_EFFECTS_VOL, value, 0, 0);
    rsEvent(RS_SET_EFFECTS_VOL_AUX, value, 0, 0);
}

static s32 GetDialogVolume() {
    return g_sound ? (s32)g_sound->flag1 : 100;
}

static void SetDialogVolume(s32 value) {
    if (!g_sound) {
        return;
    }
    g_sound->flag1 = (s16)value;
    rsEvent(RS_SET_DIALOG_VOL, value, 0, 0);
}

static s32 GetStereoEnabled() {
    return g_sound && g_sound->activeFlag ? 1 : 0;
}

static void SetStereoEnabled(s32 value) {
    if (!g_sound) {
        return;
    }

    s32 enabled = value ? 1 : 0;
    g_sound->activeFlag = (u32)enabled;
    rsEvent(enabled ? RS_SET_STEREO : RS_SET_MONO, 0, 0, 0);
}

static s32 GetSurroundEnabledSetting() {
    return AudioEngine::GetSurroundEnabled() ? 1 : 0;
}

static void SetSurroundEnabledSetting(s32 value) {
    AudioEngine::SetSurroundEnabled(value != 0);
}

static s32 GetPlayerConfigSetting() {
    if (!g_inputManager) {
        return 0;
    }
    return g_inputManager->GetPlayerConfig();
}

static void SetPlayerConfigSetting(s32 value) {
    if (!g_inputManager) {
        return;
    }
    g_inputManager->SetPlayerConfig((u8)value);
}

static s32 GetControllerPromptStyleSetting() {
    return ControllerPromptManager::GetStyle();
}

static void SetControllerPromptStyleSetting(s32 value) {
    if (ControllerPromptManager::SetStyle(value) && g_feCustomMenuMgr) {
        g_feCustomMenuMgr->ReloadControllerPromptTextures();
    }
}

static s32 GetShockEnabledSetting() {
    return GetShock() ? 1 : 0;
}

static void SetShockEnabledSetting(s32 value) {
    SetShock(value ? 1 : 0);
}

static s32 GetScreenModeSetting() {
    if (g_display) {
        return g_display->GetScreenMode();
    }
    return Display::GetDefaultScreenMode();
}

static void SetScreenModeSetting(s32 v) {
    if (g_display) {
        g_display->SetScreenMode(v);
    }
    else {
        Display::SetDefaultScreenMode(v);
    }
}

static s32 GetVsyncSetting() {
    if (g_display) {
        return g_display->GetVsync();
    }
    return Display::GetDefaultVsync();
}

static void SetVsyncSetting(s32 v) {
    if (g_display) {
        g_display->SetVsync(v);
    }
    else {
        Display::SetDefaultVsync(v);
    }
}

static s32 GetMsaaSetting() {
    if (g_display) {
        return g_display->GetMSAA();
    }
    return Display::GetDefaultMSAA();
}

static void SetMsaaSetting(s32 v) {
    if (g_display) {
        g_display->SetMSAA(v);
    }
    else {
        Display::SetDefaultMSAA(v);
    }
}

static s32 GetFrameRateSetting() {
    if (g_time) {
        return NormalizeFrameRateSetting(g_time->targetFPS);
    }

#if HIGH_FPS_PLAY_PRESENTATION
    return 60;
#else
    return 30;
#endif
}

static s32 GetLanguageSetting() {
    return (s32)g_customText.GetLanguage();
}

static void SetLanguageSetting(s32 v) {
    if (v < 0 || v >= (s32)NumLanguages) {
        v = (s32)LangEnglish;
    }

    g_customText.SetLanguage((GameLanguage)v);
}

static void SetFrameRateSetting(s32 v) {
#if HIGH_FPS_PLAY_PRESENTATION
    if (g_time) {
        g_time->targetFPS = NormalizeFrameRateSetting(v);
    }
#endif
}

static s32 GetShadowQualitySetting() {
#if MODERN_GRAPHICS
    return (s32)ShadowCSM::GetQuality();
#else
    return 0;
#endif
}

static void SetShadowQualitySetting(s32 v) {
#if MODERN_GRAPHICS
    ShadowCSM::SetQuality((ShadowQuality)v);
#endif
}

static const SettingDef kSettingDefs[] = {
    { "general", "language",      0, 0, (s32)NumLanguages - 1, GetLanguageSetting,    SetLanguageSetting },
    { "audio", "music_volume",   100, 0, 100, GetMusicVolume,   SetMusicVolume },
    { "audio", "effects_volume", 100, 0, 100, GetEffectsVolume, SetEffectsVolume },
    { "audio", "dialog_volume",  100, 0, 100, GetDialogVolume,  SetDialogVolume },
    { "audio", "stereo",           1, 0,   1, GetStereoEnabled, SetStereoEnabled },
    { "audio", "surround",         1, 0,   1, GetSurroundEnabledSetting, SetSurroundEnabledSetting },
    { "controls", "player_config", 0, 0,   2, GetPlayerConfigSetting, SetPlayerConfigSetting },
    { "controls", "shock",         0, 0,   1, GetShockEnabledSetting, SetShockEnabledSetting },
    { "controls", "prompt_style", (s32)kDefaultControllerPromptStyle, 0, (s32)ControllerPromptStyle_Count - 1,
      GetControllerPromptStyleSetting, SetControllerPromptStyleSetting },
    { "display", "screen_mode",     2, 0,   2, GetScreenModeSetting,  SetScreenModeSetting },
    { "display", "vsync",          1, 0,   1, GetVsyncSetting,       SetVsyncSetting },
    { "display", "frame_rate",    30, 0, 120, GetFrameRateSetting,   SetFrameRateSetting },
    { "display", "msaa",           0, 0,  16, GetMsaaSetting,        SetMsaaSetting },
    { "display", "shadow_quality", 0, 0,   4, GetShadowQualitySetting, SetShadowQualitySetting },
};

static constexpr u32 kSettingDefCount = (u32)(sizeof(kSettingDefs) / sizeof(kSettingDefs[0]));

static s32 FindSettingDefIndex(const char* section, const char* key) {
    for (u32 i = 0; i < kSettingDefCount; i++) {
        const SettingDef& def = kSettingDefs[i];
        if (strcmp(def.section, section) == 0 && strcmp(def.key, key) == 0) {
            return (s32)i;
        }
    }
    return -1;
}

static bool IsPersistedKeybindAction(Action action) {
    return action >= ACTION_JUMP && action < ACTION_OPEN_CLOSE_MENU;
}

static void BuildKeybindSettingKey(Action action, s32 slot, char* outKey, s32 outKeyLen) {
    if (!outKey || outKeyLen <= 0) {
        return;
    }

    outKey[0] = '\0';

    if (slot < 0 || slot >= 2) {
        return;
    }

    if (!IsPersistedKeybindAction(action)) {
        return;
    }

    const char* token = ActionToToken(action);
    if (!token) {
        return;
    }

    char lowerToken[48] = {};
    s32 write = 0;
    for (s32 i = 0; token[i] != '\0' && write + 1 < (s32)sizeof(lowerToken); i++) {
        char c = token[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        lowerToken[write++] = c;
    }
    lowerToken[write] = '\0';

    snprintf(outKey, outKeyLen, "%s_%d", lowerToken, slot + 1);
}

static bool ParseKeybindSettingKey(const char* key, Action& outAction, s32& outSlot) {
    outAction = ACTION_COUNT;
    outSlot = -1;

    if (!key || key[0] == '\0') {
        return false;
    }

    const char* slotSep = strrchr(key, '_');
    if (!slotSep || slotSep == key) {
        return false;
    }

    s32 slot = -1;
    if (!ParseSettingValue(slotSep + 1, slot)) {
        return false;
    }
    slot -= 1;
    if (slot < 0 || slot >= 2) {
        return false;
    }

    char tokenUpper[48] = {};
    const s32 tokenLen = (s32)(slotSep - key);
    if (tokenLen <= 0 || tokenLen >= (s32)sizeof(tokenUpper)) {
        return false;
    }

    for (s32 i = 0; i < tokenLen; i++) {
        char c = key[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        tokenUpper[i] = c;
    }
    tokenUpper[tokenLen] = '\0';

    Action action = ActionFromToken(tokenUpper);
    if (action < 0 || action >= ACTION_COUNT) {
        return false;
    }

    outAction = action;
    outSlot = slot;
    return true;
}

static std::string BuildIniText() {
    std::string text;
    const char* currentSection = nullptr;

    for (const SettingDef& def : kSettingDefs) {
        if (!currentSection || strcmp(currentSection, def.section) != 0) {
            if (!text.empty()) {
                text += "\n";
            }
            text += "[";
            text += def.section;
            text += "]\n";
            currentSection = def.section;
        }

        char line[128];
        snprintf(line, sizeof(line), "%s = %d\n", def.key, def.GetValue());
        text += line;
    }

    s32 screenWidth = kFallbackScreenWidth;
    s32 screenHeight = kFallbackScreenHeight;
    GetResolutionDimensions(screenWidth, screenHeight);

    if (!currentSection || strcmp(currentSection, "display") != 0) {
        if (!text.empty()) {
            text += "\n";
        }
        text += "[display]\n";
    }
    char displayLine[128];
    snprintf(displayLine, sizeof(displayLine), "screen_width = %d\n", screenWidth);
    text += displayLine;
    snprintf(displayLine, sizeof(displayLine), "screen_height = %d\n", screenHeight);
    text += displayLine;

    if (g_actionInput) {
        text += "\n[keybinds]\n";
        for (s32 i = 0; i < ACTION_COUNT; i++) {
            const Action action = (Action)i;
            if (!IsPersistedKeybindAction(action)) {
                continue;
            }

            for (s32 slot = 0; slot < 2; slot++) {
                char keyName[64] = {};
                BuildKeybindSettingKey(action, slot, keyName, (s32)sizeof(keyName));
                if (keyName[0] == '\0') {
                    continue;
                }

                const s32 code = g_actionInput->GetDesktopBindingCode(action, slot);
                char line[128] = {};
                snprintf(line, sizeof(line), "%s = %d\n", keyName, code);
                text += line;
            }
        }
    }

    return text;
}

static bool ReadWholeFile(const char* path, std::string& outText) {
    ccFile file;
    if (!file.Open(path, ccFile::OPEN_READ)) {
        return false;
    }

    s32 fileLen = file.GetLength();
    if (fileLen < 0) {
        file.Close();
        return false;
    }

    outText.clear();
    if (fileLen > 0) {
        outText.resize((size_t)fileLen);
        s32 bytesRead = file.Read((void*)outText.data(), (u32)fileLen);
        if (bytesRead < 0) {
            bytesRead = 0;
        }
        if (bytesRead < fileLen) {
            outText.resize((size_t)bytesRead);
        }
    }

    file.Close();
    return true;
}

static bool WriteWholeFile(const char* path, const std::string& text) {
    std::filesystem::path iniPath(path);
    std::filesystem::path parent = iniPath.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }

    ccFile file;
    if (!file.Open(path, ccFile::OPEN_WRITE)) {
        return false;
    }

    if (!text.empty()) {
        file.Write((void*)text.data(), (u32)text.size());
    }

    file.Close();
    return true;
}

bool GameSettings::Load(const char* path) {
    if (!g_sound) {
        LOG("[Settings] Load: g_sound is null, skipping");
        return false;
    }

    s32 values[kSettingDefCount] = {};
    for (u32 i = 0; i < kSettingDefCount; i++) {
        values[i] = kSettingDefs[i].defaultValue;
    }

    ccFile file;
    bool loadedFromFile = file.Open(path, ccFile::OPEN_READ) != 0;

    if (loadedFromFile) {
        LOG("[Settings] Load: opened '%s' (len=%d)", path, file.GetLength());
        char currentSection[64] = {};
        s32 pendingWidth = -1;
        s32 pendingHeight = -1;
        bool sawWidth = false;
        bool sawHeight = false;

        char line[256];
        while (file.ReadString(line, sizeof(line)) > 0) {
            char* trimmed = TrimInPlace(line);
            if (*trimmed == '\0' || *trimmed == '#' || *trimmed == ';') {
                continue;
            }

            if (*trimmed == '[') {
                char* close = strchr(trimmed, ']');
                if (!close) {
                    continue;
                }
                *close = '\0';
                char* section = TrimInPlace(trimmed + 1);
                std::snprintf(currentSection, sizeof(currentSection), "%s", section);
                continue;
            }

            char* eq = strchr(trimmed, '=');
            if (!eq) {
                continue;
            }

            *eq = '\0';
            char* key = TrimInPlace(trimmed);
            char* valueText = TrimInPlace(eq + 1);

            if (strcmp(currentSection, "display") == 0) {
                s32 parsedValue = 0;
                if (strcmp(key, "screen_width") == 0) {
                    if (ParseSettingValue(valueText, parsedValue) && parsedValue > 0) {
                        pendingWidth = parsedValue;
                        sawWidth = true;
                    }
                    else {
                        LOG("[Settings] Load: bad value for [display] screen_width = '%s'", valueText);
                    }
                    continue;
                }
                if (strcmp(key, "screen_height") == 0) {
                    if (ParseSettingValue(valueText, parsedValue) && parsedValue > 0) {
                        pendingHeight = parsedValue;
                        sawHeight = true;
                    }
                    else {
                        LOG("[Settings] Load: bad value for [display] screen_height = '%s'", valueText);
                    }
                    continue;
                }
            }

            if (strcmp(currentSection, "keybinds") == 0) {
                Action action = ACTION_COUNT;
                s32 slot = -1;
                if (!ParseKeybindSettingKey(key, action, slot)) {
                    LOG("[Settings] Load: unknown key [%s] %s", currentSection, key);
                    continue;
                }

                if (!IsPersistedKeybindAction(action)) {
                    continue;
                }

                s32 parsedValue = 0;
                if (!ParseSettingValue(valueText, parsedValue)) {
                    LOG("[Settings] Load: bad value for [%s] %s = '%s'", currentSection, key, valueText);
                    continue;
                }

                if (g_actionInput) {
                    g_actionInput->SetDesktopBindingCode(action, slot, parsedValue);
                }
                continue;
            }

            s32 defIndex = FindSettingDefIndex(currentSection, key);
            if (defIndex < 0) {
                LOG("[Settings] Load: unknown key [%s] %s", currentSection, key);
                continue;
            }

            const SettingDef& def = kSettingDefs[defIndex];

            s32 parsedValue = 0;
            if (!ParseSettingValue(valueText, parsedValue)) {
                LOG("[Settings] Load: bad value for [%s] %s = '%s'", currentSection, key, valueText);
                continue;
            }

            values[defIndex] = ClampForSetting(def, parsedValue);
        }

        file.Close();

        if (sawWidth && sawHeight) {
            ApplyResolutionDimensions(pendingWidth, pendingHeight);
            LOG("[Settings] Load: [display] screen_width = %d", pendingWidth);
            LOG("[Settings] Load: [display] screen_height = %d", pendingHeight);
        }
        else {
            ApplyResolutionDimensions(kFallbackScreenWidth, kFallbackScreenHeight);
            LOG("[Settings] Load: [display] screen_width/screen_height missing, fallback = %dx%d", kFallbackScreenWidth, kFallbackScreenHeight);
        }
    }
    else {
        LOG("[Settings] Load: file '%s' not found, using defaults", path);
        ApplyResolutionDimensions(kFallbackScreenWidth, kFallbackScreenHeight);
        LOG("[Settings] Load: [display] fallback = %dx%d", kFallbackScreenWidth, kFallbackScreenHeight);
    }

    for (u32 i = 0; i < kSettingDefCount; i++) {
        LOG("[Settings] Load: [%s] %s = %d", kSettingDefs[i].section, kSettingDefs[i].key, values[i]);
        kSettingDefs[i].SetValue(values[i]);
    }

    return loadedFromFile;
}

bool GameSettings::Save(const char* path) {
    if (!g_sound) {
        return false;
    }

    std::string text = BuildIniText();
    return WriteWholeFile(path, text);
}
