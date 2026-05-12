#pragma once

#include "core.h"
#include <unordered_map>

class pddiDisplay;
class pddiGamepad;

// PlatformInput - polls display for keyboard/mouse state and pddiGamepad for controller each frame.
// This is the PC platform layer; the PSX had no equivalent (it used libpad directly).
class PlatformInput {
public:
    void SetDisplay(pddiDisplay* display) { this->display = display; }
    void SetGamepad(pddiGamepad* gp) { this->gamepad = gp; }

    void SetEnabled(bool e) { enabled = e; }
    bool IsEnabled() const { return enabled; }

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

    // Gamepad queries
    bool IsGamepadConnected() const { return gamepadConnected; }
    bool IsGamepadButtonDown(int button) const;
    bool IsGamepadButtonTriggered(int button) const;
    float GetGamepadAxis(int axis) const;
    bool IsGamepadVibrationSupported() const;
    bool SetGamepadVibration(float lowFrequency, float highFrequency);

    // Device activity detection - true if any input on that device this frame
    bool HasAnyKeyboardInput() const;
    bool HasAnyMouseInput() const;
    bool HasAnyGamepadInput() const;

    // Left stick with deadzone applied (-1..+1)
    float GetLeftStickX() const;
    float GetLeftStickY() const;

    // Right stick with deadzone applied (-1..+1)
    float GetRightStickX() const;
    float GetRightStickY() const;

    // L2/R2 triggers (0..+1)
    float GetLeftTrigger() const;
    float GetRightTrigger() const;

    static constexpr float STICK_DEADZONE = 0.15f;
    static constexpr float TRIGGER_THRESHOLD = 0.5f;

    // Register a keyboard key for tracking (so HasAnyKeyboardInput detects it)
    void TrackKey(int key);

private:
    pddiDisplay* display = nullptr;
    pddiGamepad* gamepad = nullptr;
    bool enabled = true;

    // Previous-frame state for oneshot detection (PSX Button::Oneshot mode)
    std::unordered_map<int, bool> prevKeys;
    std::unordered_map<int, bool> currKeys;
    bool prevMouse[3] = {};
    bool currMouse[3] = {};
    double mouseX = 0, mouseY = 0;
    double prevMouseX = 0, prevMouseY = 0;

    // Gamepad state (previous frame for oneshot, current via pddiGamepad)
    bool gamepadConnected = false;
    bool gpButtonsPrev[15] = {};
    bool gpButtonsCurr[15] = {};
    float gpAxes[6] = {};

    float ApplyDeadzone(float value, float deadzone) const;
};
