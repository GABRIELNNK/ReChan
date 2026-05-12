#include "p3d/input.h"
#include "pddi/pddidev.h"
#include <cmath>
#include <cstring>

void PlatformInput::TrackKey(int key) {
    if (prevKeys.find(key) == prevKeys.end()) {
        prevKeys[key] = false;
        currKeys[key] = display ? display->IsKeyDown(key) : false;
    }
}

float PlatformInput::ApplyDeadzone(float value, float deadzone) const {
    if (std::fabs(value) < deadzone) {
        return 0.0f;
    }
    float sign = (value > 0.0f) ? 1.0f : -1.0f;
    return sign * (std::fabs(value) - deadzone) / (1.0f - deadzone);
}

void PlatformInput::ServiceInput() {
    if (!display)
        return;

    // Save previous state
    prevMouseX = mouseX;
    prevMouseY = mouseY;
    for (auto& [key, val] : currKeys)
        prevKeys[key] = val;
    for (int i = 0; i < 3; i++)
        prevMouse[i] = currMouse[i];

    // Save previous gamepad state
    std::memcpy(gpButtonsPrev, gpButtonsCurr, sizeof(gpButtonsCurr));

    if (!enabled) {
        for (auto& [key, val] : currKeys)
            val = false;
        for (int i = 0; i < 3; i++)
            currMouse[i] = false;
        std::memset(gpButtonsCurr, 0, sizeof(gpButtonsCurr));
        std::memset(gpAxes, 0, sizeof(gpAxes));
        gamepadConnected = false;
        return;
    }

    // Poll current keyboard/mouse state
    display->GetMousePosition(mouseX, mouseY);
    for (auto& [key, val] : currKeys)
        val = display->IsKeyDown(key);
    for (int i = 0; i < 3; i++)
        currMouse[i] = display->IsMouseButtonDown(i);

    // Poll gamepad via abstract interface
    if (gamepad) {
        gamepad->Poll();
        gamepadConnected = gamepad->IsConnected();

        if (gamepadConnected) {
            for (int b = 0; b < 15; b++) {
                gpButtonsCurr[b] = gamepad->IsButtonDown(b);
            }
            for (int a = 0; a < 6; a++) {
                gpAxes[a] = gamepad->GetAxis(a);
            }
        } else {
            std::memset(gpButtonsCurr, 0, sizeof(gpButtonsCurr));
            std::memset(gpAxes, 0, sizeof(gpAxes));
        }
    } else {
        gamepadConnected = false;
    }
}

bool PlatformInput::HasAnyKeyboardInput() const {
    for (auto& [key, val] : currKeys) {
        if (val) {
            return true;
        }
    }
    return false;
}

bool PlatformInput::HasAnyMouseInput() const {
    for (int i = 0; i < 3; i++) {
        if (currMouse[i]) {
            return true;
        }
    }

    const double dx = std::fabs(mouseX - prevMouseX);
    const double dy = std::fabs(mouseY - prevMouseY);
    return dx > 0.5 || dy > 0.5;
}

bool PlatformInput::HasAnyGamepadInput() const {
    if (!gamepadConnected) {
        return false;
    }
    for (int b = 0; b < 15; b++) {
        if (gpButtonsCurr[b]) {
            return true;
        }
    }
    // Only check sticks (0-3), not triggers (4-5) which rest at -1.0 in GLFW
    for (int a = 0; a < 4; a++) {
        if (std::fabs(gpAxes[a]) > STICK_DEADZONE) {
            return true;
        }
    }
    return false;
}

bool PlatformInput::IsKeyDown(int key) const {
    if (!display)
        return false;

    // If key isn't tracked yet, query display directly
    auto it = currKeys.find(key);
    if (it != currKeys.end())
        return it->second;

    return display->IsKeyDown(key);
}

bool PlatformInput::IsKeyTriggered(int key) const {
    // Non-const: register key for tracking on first query
    auto* self = const_cast<PlatformInput*>(this);
    self->TrackKey(key);

    auto curr = currKeys.find(key);
    auto prev = prevKeys.find(key);
    if (curr == currKeys.end() || prev == prevKeys.end()) return false;
    return curr->second && !prev->second;
}

bool PlatformInput::IsMouseButtonDown(int button) const {
    if (button < 0 || button >= 3)
        return false;

    return currMouse[button];
}

bool PlatformInput::IsMouseButtonTriggered(int button) const {
    if (button < 0 || button >= 3)
        return false;

    return currMouse[button] && !prevMouse[button];
}

void PlatformInput::GetMousePosition(double& x, double& y) const {
    x = mouseX;
    y = mouseY;
}

void PlatformInput::GetMouseDelta(double& dx, double& dy) const {
    dx = mouseX - prevMouseX;
    dy = mouseY - prevMouseY;
}

bool PlatformInput::IsGamepadButtonDown(int button) const {
    if (button < 0 || button >= 15) {
        return false;
    }
    return gpButtonsCurr[button];
}

bool PlatformInput::IsGamepadButtonTriggered(int button) const {
    if (button < 0 || button >= 15) {
        return false;
    }
    return gpButtonsCurr[button] && !gpButtonsPrev[button];
}

float PlatformInput::GetGamepadAxis(int axis) const {
    if (axis < 0 || axis >= 6) {
        return 0.0f;
    }
    return gpAxes[axis];
}

bool PlatformInput::IsGamepadVibrationSupported() const {
    if (!gamepad || !gamepadConnected) {
        return false;
    }

    return gamepad->SupportsVibration();
}

bool PlatformInput::SetGamepadVibration(float lowFrequency, float highFrequency) {
    if (!gamepad) {
        return false;
    }

    float low = lowFrequency;
    float high = highFrequency;
    if (low < 0.0f) {
        low = 0.0f;
    }
    else if (low > 1.0f) {
        low = 1.0f;
    }

    if (high < 0.0f) {
        high = 0.0f;
    }
    else if (high > 1.0f) {
        high = 1.0f;
    }

    return gamepad->SetVibration(low, high);
}

float PlatformInput::GetLeftStickX() const {
    return ApplyDeadzone(gpAxes[GamepadAxis::LeftX], STICK_DEADZONE);
}

float PlatformInput::GetLeftStickY() const {
    return ApplyDeadzone(gpAxes[GamepadAxis::LeftY], STICK_DEADZONE);
}

float PlatformInput::GetRightStickX() const {
    return ApplyDeadzone(gpAxes[GamepadAxis::RightX], STICK_DEADZONE);
}

float PlatformInput::GetRightStickY() const {
    return ApplyDeadzone(gpAxes[GamepadAxis::RightY], STICK_DEADZONE);
}

float PlatformInput::GetLeftTrigger() const {
    // Triggers range from -1 (released) to +1 (fully pressed)
    float raw = gpAxes[GamepadAxis::LeftTrigger];
    return (raw + 1.0f) * 0.5f;
}

float PlatformInput::GetRightTrigger() const {
    float raw = gpAxes[GamepadAxis::RightTrigger];
    return (raw + 1.0f) * 0.5f;
}
