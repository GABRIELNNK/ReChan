// input.cpp — PlatformInput implementation
#include "p3d/input.h"
#include "pddi/pddidev.h"

void PlatformInput::TrackKey(int key) {
    if (prevKeys.find(key) == prevKeys.end()) {
        prevKeys[key] = false;
        currKeys[key] = false;
    }
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

    // Poll current state
    display->GetMousePosition(mouseX, mouseY);
    for (auto& [key, val] : currKeys)
        val = display->IsKeyDown(key);
    for (int i = 0; i < 3; i++)
        currMouse[i] = display->IsMouseButtonDown(i);
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
