#include "pc/inputaction.h"
#include "p3d/input.h"
#include "pddi/pddidev.h"
#include <cmath>

ActionInput* g_actionInput = nullptr;

ActionInput::ActionInput() {
    //                            keyboard         gpBtn           gpBtn2              gpAxis          threshold
    bindings[ACTION_JUMP]       = { KEY_SPACE,       GpBtn::A,       GpBtn::NONE,        GpAxis::NONE,   0 };
    bindings[ACTION_PUNCH]      = { KEY_J,           GpBtn::X,       GpBtn::NONE,        GpAxis::NONE,   0 };
    bindings[ACTION_KICK]       = { KEY_K,           GpBtn::Y,       GpBtn::NONE,        GpAxis::NONE,   0 };
    bindings[ACTION_GRAB]       = { KEY_L,           GpBtn::B,       GpBtn::NONE,        GpAxis::NONE,   0 };
    bindings[ACTION_DIVE_ROLL]  = { KEY_LEFT_SHIFT,  GpBtn::RB,      GpBtn::NONE,        GpAxis::NONE,   0 };
    bindings[ACTION_STRAFE]     = { KEY_F,           GpBtn::NONE,    GpBtn::NONE,        GpAxis::RTrigger, 0.5f };
    bindings[ACTION_COUNTER]    = { KEY_Q,           GpBtn::LB,      GpBtn::NONE,        GpAxis::NONE,   0 };

    bindings[ACTION_MOVE_UP]    = { KEY_W,           GpBtn::NONE,    GpBtn::DpadUp,      GpAxis::LeftY, -0.3f };
    bindings[ACTION_MOVE_DOWN]  = { KEY_S,           GpBtn::NONE,    GpBtn::DpadDown,    GpAxis::LeftY,  0.3f };
    bindings[ACTION_MOVE_LEFT]  = { KEY_A,           GpBtn::NONE,    GpBtn::DpadLeft,    GpAxis::LeftX, -0.3f };
    bindings[ACTION_MOVE_RIGHT] = { KEY_D,           GpBtn::NONE,    GpBtn::DpadRight,   GpAxis::LeftX,  0.3f };

    bindings[ACTION_LOOK_UP]    = { 0,               GpBtn::NONE,    GpBtn::NONE,        GpAxis::RightY, -0.3f };
    bindings[ACTION_LOOK_DOWN]  = { 0,               GpBtn::NONE,    GpBtn::NONE,        GpAxis::RightY,  0.3f };
    bindings[ACTION_LOOK_LEFT]  = { 0,               GpBtn::NONE,    GpBtn::NONE,        GpAxis::RightX, -0.3f };
    bindings[ACTION_LOOK_RIGHT] = { 0,               GpBtn::NONE,    GpBtn::NONE,        GpAxis::RightX,  0.3f };

    bindings[ACTION_START]      = { KEY_ESCAPE,      GpBtn::Start,   GpBtn::NONE,        GpAxis::NONE,   0 };
    bindings[ACTION_SELECT]     = { KEY_TAB,         GpBtn::Back,    GpBtn::NONE,        GpAxis::NONE,   0 };

    bindings[ACTION_MENU_UP]      = { KEY_UP,         GpBtn::DpadUp,    GpBtn::NONE,      GpAxis::LeftY, -0.5f };
    bindings[ACTION_MENU_DOWN]    = { KEY_DOWN,       GpBtn::DpadDown,  GpBtn::NONE,      GpAxis::LeftY,  0.5f };
    bindings[ACTION_MENU_LEFT]    = { KEY_LEFT,       GpBtn::DpadLeft,  GpBtn::NONE,      GpAxis::LeftX, -0.5f };
    bindings[ACTION_MENU_RIGHT]   = { KEY_RIGHT,      GpBtn::DpadRight, GpBtn::NONE,      GpAxis::LeftX,  0.5f };
    bindings[ACTION_MENU_CONFIRM] = { KEY_ENTER,     GpBtn::A,       GpBtn::NONE,        GpAxis::NONE,   0 };
    bindings[ACTION_MENU_BACK]    = { KEY_ESCAPE,    GpBtn::B,       GpBtn::NONE,        GpAxis::NONE,   0 };
}

bool ActionInput::PollAction(Action action, PlatformInput* platform) const {
    const ActionBinding& b = bindings[action];

    // Check keyboard
    if (b.keyboardKey && platform->IsKeyDown(b.keyboardKey)) {
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
        gamepadActive = false;
    } else {
        bool hasKb = platform->HasAnyKeyboardInput();
        bool hasGp = platform->HasAnyGamepadInput();
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
        } else {
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
    } else {
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

s32 ActionInput::GetGamepadButtonBinding(Action action) const {
    if (action >= 0 && action < ACTION_COUNT) {
        return bindings[action].gamepadButton;
    }
    return GpBtn::NONE;
}

// Resolve combat action from input state + direction.
// Matches PSX FindActionRequest priority ordering.
s32 ActionInput::ResolveGameAction(s32 direction) const {
    if (!controlsEnabled) {
        return GA_GUARD_RELEASE;
    }

    // Priority 1: Dive roll
    if (IsHeld(ACTION_DIVE_ROLL)) {
        return GA_DIVE_ROLL;
    }

    // Priority 2: Counter
    if (IsHeld(ACTION_COUNTER)) {
        return GA_COUNTER;
    }

    // Priority 3: Punch variants
    if (IsHeld(ACTION_PUNCH)) {
        if (direction & 2) {
            return GA_BACK_PUNCH;
        }
        if (GetDuration(ACTION_PUNCH) >= HEAVY_PUNCH_THRESHOLD) {
            return GA_HEAVY_PUNCH;
        }
        return GA_PUNCH;
    }

    // Priority 4: Kick variants
    if (IsHeld(ACTION_KICK)) {
        if (direction & 2) {
            return GA_BACK_KICK;
        }
        if (GetDuration(ACTION_KICK) >= HEAVY_KICK_THRESHOLD) {
            return GA_HEAVY_KICK;
        }
        return GA_KICK;
    }

    // Priority 5: Jump (oneshot)
    if (JustPressed(ACTION_JUMP)) {
        if (direction != 0) {
            return GA_JUMP_DIRECTIONAL;
        }
        return GA_JUMP;
    }

    // Priority 6: Grab variants
    if (IsHeld(ACTION_GRAB)) {
        if ((direction & 1) && GetDuration(ACTION_GRAB) >= GRAB_HOLD_THRESHOLD) {
            return GA_GRAB_FWD_HELD;
        }
        if (GetDuration(ACTION_GRAB) >= GRAB_HOLD_THRESHOLD) {
            return GA_GRAB_HELD;
        }
        if (direction & 1) {
            return GA_GRAB_FORWARD;
        }
        return GA_GRAB;
    }

    // Priority 7: Strafe
    if (IsHeld(ACTION_STRAFE)) {
        return GA_STRAFE;
    }

    // Priority 8: Movement
    if (HasMovement()) {
        return GA_MOVE;
    }

    return GA_GUARD_RELEASE;
}
