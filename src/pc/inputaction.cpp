#include "pc/inputaction.h"
#include "p3d/input.h"
#include "pddi/pddidev.h"
#include <cmath>

static u8 AxisToPadByte(s32 axis) {
    s32 value = 128 + axis;
    if (value < 0) {
        value = 0;
    }
    else if (value > 255) {
        value = 255;
    }
    return (u8)value;
}

ActionInput* g_actionInput = nullptr;

ActionInput::ActionInput() {
    // keyboard, mouse, gpBtn, gpBtn2, gpAxis, threshold
    bindings[ACTION_JUMP] = { KEY_SPACE, MouseBtn::NONE, GpBtn::A, GpBtn::NONE, GpAxis::NONE, 0 };
    bindings[ACTION_PUNCH] = { KEY_J, MouseBtn::Left, GpBtn::X, GpBtn::NONE, GpAxis::NONE, 0 };
    bindings[ACTION_KICK] = { KEY_K, MouseBtn::Right, GpBtn::Y, GpBtn::NONE, GpAxis::NONE, 0 };
    bindings[ACTION_GRAB] = { KEY_L, MouseBtn::Middle, GpBtn::B, GpBtn::NONE, GpAxis::NONE, 0 };
    bindings[ACTION_DIVE_ROLL] = { KEY_LEFT_CONTROL, MouseBtn::NONE, GpBtn::RB, GpBtn::NONE, GpAxis::NONE, 0 };
    bindings[ACTION_STRAFE] = { KEY_LEFT_SHIFT, MouseBtn::NONE, GpBtn::NONE, GpBtn::NONE, GpAxis::RTrigger, 0.5f };
    bindings[ACTION_COUNTER] = { KEY_U, MouseBtn::NONE, GpBtn::LB, GpBtn::NONE, GpAxis::NONE, 0 };
    bindings[ACTION_STATUS_DISPLAY] = { KEY_TAB, MouseBtn::NONE, GpBtn::NONE, GpBtn::NONE, GpAxis::LTrigger, 0.5f };

    bindings[ACTION_MOVE_UP] = { KEY_W, MouseBtn::NONE, GpBtn::NONE, GpBtn::DpadUp, GpAxis::LeftY, -0.3f };
    bindings[ACTION_MOVE_DOWN] = { KEY_S, MouseBtn::NONE, GpBtn::NONE, GpBtn::DpadDown, GpAxis::LeftY, 0.3f };
    bindings[ACTION_MOVE_LEFT] = { KEY_A, MouseBtn::NONE, GpBtn::NONE, GpBtn::DpadLeft, GpAxis::LeftX, -0.3f };
    bindings[ACTION_MOVE_RIGHT] = { KEY_D, MouseBtn::NONE, GpBtn::NONE, GpBtn::DpadRight, GpAxis::LeftX, 0.3f };

    bindings[ACTION_LOOK_UP] = { 0, MouseBtn::NONE, GpBtn::NONE, GpBtn::NONE, GpAxis::RightY, -0.3f };
    bindings[ACTION_LOOK_DOWN] = { 0, MouseBtn::NONE, GpBtn::NONE, GpBtn::NONE, GpAxis::RightY, 0.3f };
    bindings[ACTION_LOOK_LEFT] = { 0, MouseBtn::NONE, GpBtn::NONE, GpBtn::NONE, GpAxis::RightX, -0.3f };
    bindings[ACTION_LOOK_RIGHT] = { 0, MouseBtn::NONE, GpBtn::NONE, GpBtn::NONE, GpAxis::RightX, 0.3f };

    bindings[ACTION_OPEN_CLOSE_MENU] = { KEY_ESCAPE, MouseBtn::NONE, GpBtn::Start, GpBtn::NONE, GpAxis::NONE, 0 };

    bindings[ACTION_MENU_UP] = { KEY_UP, MouseBtn::NONE, GpBtn::DpadUp, GpBtn::NONE, GpAxis::LeftY, -0.5f };
    bindings[ACTION_MENU_DOWN] = { KEY_DOWN, MouseBtn::NONE, GpBtn::DpadDown, GpBtn::NONE, GpAxis::LeftY, 0.5f };
    bindings[ACTION_MENU_LEFT] = { KEY_LEFT, MouseBtn::NONE, GpBtn::DpadLeft, GpBtn::NONE, GpAxis::LeftX, -0.5f };
    bindings[ACTION_MENU_RIGHT] = { KEY_RIGHT, MouseBtn::NONE, GpBtn::DpadRight, GpBtn::NONE, GpAxis::LeftX, 0.5f };
    bindings[ACTION_MENU_CONFIRM] = { KEY_ENTER, MouseBtn::NONE, GpBtn::A, GpBtn::NONE, GpAxis::NONE, 0 };
    bindings[ACTION_MENU_BACK] = { KEY_ESCAPE, MouseBtn::NONE, GpBtn::B, GpBtn::NONE, GpAxis::NONE, 0 };
}

bool ActionInput::PollAction(Action action, PlatformInput* platform) const {
    const ActionBinding& b = bindings[action];

    // Check keyboard
    if (b.keyboardKey && platform->IsKeyDown(b.keyboardKey)) {
        return true;
    }

    if (b.mouseButton != MouseBtn::NONE && platform->IsMouseButtonDown(b.mouseButton)) {
        return true;
    }

    // Check gamepad button
    if (b.gamepadButton != GpBtn::NONE && platform->IsGamepadButtonDown(b.gamepadButton)) {
        return true;
    }

    // Check alternate gamepad button (e.g. D-pad)
    if (b.gamepadButton2 != GpBtn::NONE && platform->IsGamepadButtonDown(b.gamepadButton2)) {
        return true;
    }

    // Check gamepad axis
    if (b.gamepadAxis != GpAxis::NONE) {
        float val = platform->GetGamepadAxis(b.gamepadAxis);
        if (b.axisThreshold > 0 && val >= b.axisThreshold) {
            return true;
        }
        if (b.axisThreshold < 0 && val <= b.axisThreshold) {
            return true;
        }
    }

    return false;
}

void ActionInput::Update(PlatformInput* platform) {
    if (!platform) {
        return;
    }

    for (s32 i = 0; i < 3; i++) {
        mousePrev[i] = mouseDown[i];
        mouseDown[i] = platform->IsMouseButtonDown(i);
    }
    platform->GetMousePosition(mouseX, mouseY);

    // Ensure all bound keyboard keys are tracked for HasAnyKeyboardInput()
    if (!keysRegistered) {
        for (s32 i = 0; i < ACTION_COUNT; i++) {
            if (bindings[i].keyboardKey) {
                platform->TrackKey(bindings[i].keyboardKey);
            }
        }
        keysRegistered = true;
    }

    // Switch active device based on last input used
    bool gpConnected = platform->IsGamepadConnected();
    if (!gpConnected) {
        hadKeyboardInputThisFrame = platform->HasAnyKeyboardInput();
        hadGamepadInputThisFrame = false;
        gamepadActive = false;
    }
    else {
        bool hasKb = platform->HasAnyKeyboardInput();
        bool hasGp = platform->HasAnyGamepadInput();
        hadKeyboardInputThisFrame = hasKb;
        hadGamepadInputThisFrame = hasGp;
        if (hasGp) {
            gamepadActive = true;
        }
        if (hasKb) {
            gamepadActive = false;
        }
    }

    // Update all action states
    for (s32 i = 0; i < ACTION_COUNT; i++) {
        InputState& s = states[i];
        s.prevDown = s.down;
        s.down = PollAction(static_cast<Action>(i), platform);

        if (s.down) {
            s.duration++;
        }
        else {
            s.duration = 0;
        }
    }

    // Compute analog movement axes
    // Start with digital input from action states (works for both keyboard and D-pad)
    moveX = 0;
    moveY = 0;
    if (states[ACTION_MOVE_LEFT].down)  moveX -= 127;
    if (states[ACTION_MOVE_RIGHT].down) moveX += 127;
    if (states[ACTION_MOVE_UP].down)    moveY -= 127;
    if (states[ACTION_MOVE_DOWN].down)  moveY += 127;

    // Analog stick overrides digital when active
    if (gamepadActive) {
        float lx = platform->GetLeftStickX();
        float ly = platform->GetLeftStickY();
        s32 stickX = (s32)(lx * 127.0f);
        s32 stickY = (s32)(ly * 127.0f);
        if (stickX != 0 || stickY != 0) {
            moveX = stickX;
            moveY = stickY;
        }

        float rx = platform->GetRightStickX();
        float ry = platform->GetRightStickY();
        lookX = (s32)(rx * 127.0f);
        lookY = (s32)(ry * 127.0f);
    }
    else {
        lookX = 0;
        lookY = 0;
    }

    if (moveX > 127) moveX = 127;
    if (moveX < -127) moveX = -127;
    if (moveY > 127) moveY = 127;
    if (moveY < -127) moveY = -127;
}

bool ActionInput::JustPressed(Action action) const {
    if (action < 0 || action >= ACTION_COUNT) {
        return false;
    }
    const InputState& s = states[action];
    return s.down && !s.prevDown;
}

bool ActionInput::IsHeld(Action action) const {
    if (action < 0 || action >= ACTION_COUNT) {
        return false;
    }
    return states[action].down;
}

bool ActionInput::JustReleased(Action action) const {
    if (action < 0 || action >= ACTION_COUNT) {
        return false;
    }
    const InputState& s = states[action];
    return !s.down && s.prevDown;
}

s16 ActionInput::GetDuration(Action action) const {
    if (action < 0 || action >= ACTION_COUNT) {
        return 0;
    }
    return states[action].duration;
}

bool ActionInput::AnyJustPressed() const {
    for (s32 i = 0; i < ACTION_COUNT; i++) {
        if (states[i].down && !states[i].prevDown) {
            return true;
        }
    }
    return false;
}

void ActionInput::SetKeyBinding(Action action, int key) {
    if (action >= 0 && action < ACTION_COUNT) {
        bindings[action].keyboardKey = key;
    }
}

void ActionInput::SetMouseButtonBinding(Action action, s32 mouseButton) {
    if (action >= 0 && action < ACTION_COUNT) {
        bindings[action].mouseButton = mouseButton;
    }
}

void ActionInput::SetGamepadButtonBinding(Action action, s32 gpButton) {
    if (action >= 0 && action < ACTION_COUNT) {
        bindings[action].gamepadButton = gpButton;
    }
}

int ActionInput::GetKeyBinding(Action action) const {
    if (action >= 0 && action < ACTION_COUNT) {
        return bindings[action].keyboardKey;
    }
    return 0;
}

s32 ActionInput::GetMouseButtonBinding(Action action) const {
    if (action >= 0 && action < ACTION_COUNT) {
        return bindings[action].mouseButton;
    }
    return MouseBtn::NONE;
}

s32 ActionInput::GetGamepadButtonBinding(Action action) const {
    if (action >= 0 && action < ACTION_COUNT) {
        return bindings[action].gamepadButton;
    }
    return GpBtn::NONE;
}

u32 ActionInput::GetPadButtons(bool menuActionsEnabled) const {
    u32 buttons = 0;

    if (IsHeld(ACTION_MOVE_UP) || (menuActionsEnabled && IsHeld(ACTION_MENU_UP))) {
        buttons |= 0x1000;
    }
    if (IsHeld(ACTION_MOVE_RIGHT) || (menuActionsEnabled && IsHeld(ACTION_MENU_RIGHT))) {
        buttons |= 0x2000;
    }
    if (IsHeld(ACTION_MOVE_DOWN) || (menuActionsEnabled && IsHeld(ACTION_MENU_DOWN))) {
        buttons |= 0x4000;
    }
    if (IsHeld(ACTION_MOVE_LEFT) || (menuActionsEnabled && IsHeld(ACTION_MENU_LEFT))) {
        buttons |= 0x8000;
    }

    if (IsHeld(ACTION_STATUS_DISPLAY)) {
        buttons |= 0x0001;
    }
    if (IsHeld(ACTION_STRAFE)) {
        buttons |= 0x0002;
    }
    if (IsHeld(ACTION_DIVE_ROLL)) {
        buttons |= 0x0008;
    }
    if (IsHeld(ACTION_COUNTER)) {
        buttons |= 0x0004;
    }
    if (IsHeld(ACTION_KICK)) {
        buttons |= 0x0010;
    }
    if (IsHeld(ACTION_GRAB)) {
        buttons |= 0x0020;
    }
    if (menuActionsEnabled && IsHeld(ACTION_MENU_BACK)) {
        buttons |= 0x0010;
    }
    if (IsHeld(ACTION_JUMP) || (menuActionsEnabled && IsHeld(ACTION_MENU_CONFIRM))) {
        buttons |= 0x0040;
    }
    if (IsHeld(ACTION_PUNCH)) {
        buttons |= 0x0080;
    }
    if (IsHeld(ACTION_OPEN_CLOSE_MENU)) {
        buttons |= 0x0800;
    }

    return buttons;
}

void ActionInput::GetPadAnalog(u8& lx, u8& ly, u8& rx, u8& ry) const {
    lx = AxisToPadByte(moveX);
    ly = AxisToPadByte(moveY);
    rx = AxisToPadByte(lookX);
    ry = AxisToPadByte(lookY);
}

void ActionInput::GetMousePosition(double& x, double& y) const {
    x = mouseX;
    y = mouseY;
}

bool ActionInput::IsMouseButtonDown(s32 button) const {
    if (button < 0 || button >= 3) {
        return false;
    }
    return mouseDown[button];
}

bool ActionInput::IsMouseButtonTriggered(s32 button) const {
    if (button < 0 || button >= 3) {
        return false;
    }
    return mouseDown[button] && !mousePrev[button];
}

s32 ActionInput::ConsumeScrollDelta() {
    const s32 delta = scrollDelta;
    scrollDelta = 0;
    return delta;
}
