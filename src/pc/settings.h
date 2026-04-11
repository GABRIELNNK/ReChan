#pragma once
#include "core.h"

struct GameSettings {
    bool Load(const char* path);
    bool Save(const char* path);
};

extern GameSettings g_settings;

static constexpr const char* SETTINGS_PATH = "userfiles/game.ini";
