#include "pc/settings.h"
#include "pc/log.h"
#include "gen/ccfile.h"
#include "gen/control.h"
#include "gen/display.h"
#include "snd/sound.h"
#include "snd/rsevent.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <cstring>
#include <cstdlib>

GameSettings g_settings;

static constexpr s32 kFallbackScreenWidth = 1280;
static constexpr s32 kFallbackScreenHeight = 720;

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

    g_display->SetResolutionIndex(bestIndex);
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

static const SettingDef kSettingDefs[] = {
    { "audio", "music_volume",   100, 0, 100, GetMusicVolume,   SetMusicVolume },
    { "audio", "effects_volume", 100, 0, 100, GetEffectsVolume, SetEffectsVolume },
    { "audio", "dialog_volume",  100, 0, 100, GetDialogVolume,  SetDialogVolume },
    { "audio", "stereo",           1, 0,   1, GetStereoEnabled, SetStereoEnabled },
    { "controls", "player_config", 0, 0,   2, GetPlayerConfigSetting, SetPlayerConfigSetting },
    { "controls", "shock",         0, 0,   1, GetShockEnabledSetting, SetShockEnabledSetting },
    { "display", "screen_mode",     2, 0,   2, GetScreenModeSetting,  SetScreenModeSetting },
    { "display", "vsync",          1, 0,   1, GetVsyncSetting,       SetVsyncSetting },
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
                strncpy_s(currentSection, sizeof(currentSection), section, _TRUNCATE);
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
