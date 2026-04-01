// input.h — PlatformInput (PC platform-level keyboard/mouse wrapper around GLFW)
// The game-level InputManager is reversed in src/gen/control.h from CONTROL.CPP.
#pragma once

#include "core.h"
#include <unordered_map>

class pddiDisplay;

// PlatformInput — polls display for keyboard/mouse state each frame.
// This is the PC platform layer; the PSX had no equivalent (it used libpad directly).
class PlatformInput {
public:
    void SetDisplay(pddiDisplay* display) { this->display = display; }

    // Call once per frame before reading input (mirrors PSX Step/ServiceInput)
    void ServiceInput();

    // Key queries
    bool IsKeyDown(int key) const;
    bool IsKeyTriggered(int key) const; // oneshot: true on first frame of press

    // Mouse queries
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonTriggered(int button) const;
    void GetMousePosition(double& x, double& y) const;
    void GetMouseDelta(double& dx, double& dy) const;

private:
    pddiDisplay* display = nullptr;

    // Previous-frame state for oneshot detection (PSX Button::Oneshot mode)
    std::unordered_map<int, bool> prevKeys;
    std::unordered_map<int, bool> currKeys;
    bool prevMouse[3] = {};
    bool currMouse[3] = {};
    double mouseX = 0, mouseY = 0;
    double prevMouseX = 0, prevMouseY = 0;

    void TrackKey(int key);
};
