#include "pc/inputaction.h"
#include "p3d/input.h"
#include "pddi/pddidev.h"

ActionInput* g_actionInput = nullptr;

// Default keyboard bindings - maps keys to abstract input buttons.
// The actual GameAction ID is resolved by ResolveAction based on
// button + direction + hold time, matching PSX FindActionRequest behavior.
const KeyBinding ActionInput::s_defaultBindings[] = {
    // Jump
    { KEY_SPACE,       InputButton::Jump },
    // Punch
    { KEY_J,           InputButton::Punch },
    // Kick
    { KEY_K,           InputButton::Kick },
    // Grab / throw
    { KEY_L,           InputButton::Grab },
    // Dive roll
    { KEY_LEFT_SHIFT,  InputButton::DiveRoll },
    // Strafe
    { KEY_F,           InputButton::Strafe },
    // Counter
    { KEY_Q,           InputButton::Counter },
    // Start
    { KEY_ENTER,       InputButton::Start },
    { KEY_ESCAPE,      InputButton::Start },
    // Select
    { KEY_TAB,         InputButton::Select },
    { KEY_BACKSPACE,   InputButton::Select },
};

const int ActionInput::s_numBindings = sizeof(s_defaultBindings) / sizeof(s_defaultBindings[0]);

ActionInput::ActionInput() {
}

void ActionInput::Update(PlatformInput* platform) {
    if (!platform) {
        return;
    }

    gamepadActive = platform->IsGamepadConnected();

    if (gamepadActive) {
        // Gamepad path: movement from left stick
        float sx = platform->GetLeftStickX();
        float sy = platform->GetLeftStickY();
        moveX = (s32)(sx * 127.0f);
        moveY = (s32)(sy * 127.0f);

        // D-pad contributes to movement when stick is idle
        if (moveX == 0 && moveY == 0) {
            if (platform->IsGamepadButtonDown(GamepadButton::DpadLeft))  moveX = -127;
            if (platform->IsGamepadButtonDown(GamepadButton::DpadRight)) moveX = 127;
            if (platform->IsGamepadButtonDown(GamepadButton::DpadUp))    moveY = -127;
            if (platform->IsGamepadButtonDown(GamepadButton::DpadDown))  moveY = 127;
        }

        return;
    }

    // Keyboard path: WASD movement
    moveX = 0;
    moveY = 0;
    if (platform->IsKeyDown(MOVE_LEFT)) {
        moveX = -127;
    } else if (platform->IsKeyDown(MOVE_RIGHT)) {
        moveX = 127;
    }
    if (platform->IsKeyDown(MOVE_UP)) {
        moveY = -127;
    } else if (platform->IsKeyDown(MOVE_DOWN)) {
        moveY = 127;
    }

    // Update abstract button states from key bindings
    // First, mark all buttons as inactive for this frame
    bool anyActive[INPUT_BUTTON_COUNT] = {};

    for (int i = 0; i < s_numBindings; i++) {
        if (platform->IsKeyDown(s_defaultBindings[i].key)) {
            s32 idx = static_cast<s32>(s_defaultBindings[i].button);
            anyActive[idx] = true;
        }
    }

    for (int i = 0; i < INPUT_BUTTON_COUNT; i++) {
        ButtonState& bs = buttonStates[i];
        bs.prevActive = bs.active;
        bs.active = anyActive[i];

        if (bs.active) {
            bs.duration++;
        } else {
            bs.duration = 0;
        }
    }
}

bool ActionInput::IsButtonActive(InputButton btn) const {
    s32 idx = static_cast<s32>(btn);
    if (idx < 0 || idx >= INPUT_BUTTON_COUNT) {
        return false;
    }
    return buttonStates[idx].active;
}

s16 ActionInput::GetButtonDuration(InputButton btn) const {
    s32 idx = static_cast<s32>(btn);
    if (idx < 0 || idx >= INPUT_BUTTON_COUNT) {
        return 0;
    }
    return buttonStates[idx].duration;
}

// Resolve keyboard input to a game action ID, matching PSX FindActionRequest priority.
// Returns GA_GUARD_RELEASE (1) if no action button is pressed.
s32 ActionInput::ResolveAction(s32 direction) const {
    if (!controlsEnabled) {
        return GA_GUARD_RELEASE;
    }

    // Priority order matches PSX command table (highest priority first)

    // 1. Dive roll (R1 equivalent - oneshot semantics handled by caller)
    if (IsButtonActive(InputButton::DiveRoll)) {
        return GA_DIVE_ROLL;
    }

    // 2. Counter (L1 equivalent)
    if (IsButtonActive(InputButton::Counter)) {
        return GA_COUNTER;
    }

    // 3. Punch variants (direction + hold time)
    if (IsButtonActive(InputButton::Punch)) {
        if (direction & 2) { // DIR_BACKWARD
            return GA_BACK_PUNCH;
        }
        if (GetButtonDuration(InputButton::Punch) >= HEAVY_PUNCH_THRESHOLD) {
            return GA_HEAVY_PUNCH;
        }
        return GA_PUNCH;
    }

    // 4. Kick variants (direction + hold time)
    if (IsButtonActive(InputButton::Kick)) {
        if (direction & 2) { // DIR_BACKWARD
            return GA_BACK_KICK;
        }
        if (GetButtonDuration(InputButton::Kick) >= HEAVY_KICK_THRESHOLD) {
            return GA_HEAVY_KICK;
        }
        return GA_KICK;
    }

    // 5. Jump variants (direction)
    if (IsButtonActive(InputButton::Jump)) {
        if (direction != 0) {
            return GA_JUMP_DIRECTIONAL;
        }
        return GA_JUMP;
    }

    // 6. Grab variants (direction + hold time)
    if (IsButtonActive(InputButton::Grab)) {
        if ((direction & 1) && GetButtonDuration(InputButton::Grab) >= GRAB_HOLD_THRESHOLD) {
            return GA_GRAB_FWD_HELD;
        }
        if (GetButtonDuration(InputButton::Grab) >= GRAB_HOLD_THRESHOLD) {
            return GA_GRAB_HELD;
        }
        if (direction & 1) { // DIR_FORWARD
            return GA_GRAB_FORWARD;
        }
        return GA_GRAB;
    }

    // 7. Strafe (R2 equivalent)
    if (IsButtonActive(InputButton::Strafe)) {
        return GA_STRAFE;
    }

    // 8. Movement (from WASD)
    if (HasMovement()) {
        return GA_MOVE;
    }

    return GA_GUARD_RELEASE;
}
