#pragma once

#include "core.h"
#include "p3d/keycode.h"

class PlatformInput;

// Game action IDs - bit positions in Humanoid::commandBits.
// These match the action IDs that PSX FindActionRequest returns.
// RequestAction(id) does: commandBits |= (1 << id)
enum GameAction : s32 {
    GA_NONE             = 0,
    GA_GUARD_RELEASE    = 1,   // no button - idle/stand request
    GA_MOVE             = 2,   // analog stick or d-pad movement
    GA_JUMP             = 3,   // Cross tap (no direction)
    GA_JUMP_DIRECTIONAL = 4,   // Cross + direction
    GA_DIVE_ROLL        = 5,   // R1
    GA_STRAFE           = 6,   // R2
    GA_GRAB             = 7,   // Circle tap (grab/throw)
    GA_PUNCH            = 8,   // Square tap
    GA_KICK             = 9,   // Triangle tap
    GA_BACK_PUNCH       = 10,  // Square + backward
    GA_BACK_KICK        = 11,  // Triangle + backward
    GA_HEAVY_PUNCH      = 12,  // Square held / Triangle+Square
    GA_HEAVY_KICK       = 13,  // Triangle held
    GA_SPECIAL_GRAB     = 14,  // two-button grab combo
    GA_GRAB_FORWARD     = 15,  // Circle + forward
    GA_GRAB_HELD        = 17,  // Circle held
    GA_GRAB_FWD_HELD    = 18,  // Circle + forward + held
    GA_COUNTER          = 20,  // L1 / Triangle+Circle
    GA_AI_DIVE_ROLL     = 21,  // AI only
};

// Abstract input buttons for keyboard binding.
// Each maps to a semantic action group; the actual GameAction ID
// is resolved from button + direction + hold time.
enum class InputButton : s32 {
    Jump,
    Punch,
    Kick,
    Grab,
    DiveRoll,
    Strafe,
    Counter,
    Start,
    Select,
    COUNT,
};

static constexpr s32 INPUT_BUTTON_COUNT = static_cast<s32>(InputButton::COUNT);

struct KeyBinding {
    int key;
    InputButton button;
};

// ActionInput - PC input system that maps keyboard/mouse directly to game actions.
// For gamepad, the existing PSX pipeline (ServiceInput -> FindActionRequest) is used.
// For keyboard, this system resolves actions from key state + direction + hold time.
class ActionInput {
public:
    ActionInput();

    // Call once per frame before behaviour processing.
    // Polls keyboard for movement and action button states.
    void Update(PlatformInput* platform);

    // Resolve the highest-priority action ID from current keyboard state.
    // direction: camera-relative direction flags (DIR_FORWARD/BACKWARD/LEFT/RIGHT)
    // Returns a GameAction value matching what FindActionRequest would return.
    s32 ResolveAction(s32 direction) const;

    // Movement axes from WASD (same scale as PSX d-pad: -127..+127)
    s32 GetMoveX() const { return moveX; }
    s32 GetMoveY() const { return moveY; }
    bool HasMovement() const { return moveX != 0 || moveY != 0; }

    // Is an abstract button currently active (key held)?
    bool IsButtonActive(InputButton btn) const;

    // Hold duration in frames for an abstract button
    s16 GetButtonDuration(InputButton btn) const;

    // Returns true if a real gamepad is connected
    bool IsGamepadActive() const { return gamepadActive; }

    bool controlsEnabled = true;

private:
    struct ButtonState {
        bool active = false;
        bool prevActive = false;
        s16 duration = 0;
    };

    ButtonState buttonStates[INPUT_BUTTON_COUNT] = {};
    bool gamepadActive = false;
    s32 moveX = 0;
    s32 moveY = 0;

    // Hold thresholds from PSX command table
    static constexpr s16 GRAB_HOLD_THRESHOLD = 6;
    static constexpr s16 HEAVY_PUNCH_THRESHOLD = 8;
    static constexpr s16 HEAVY_KICK_THRESHOLD = 10;

    static const KeyBinding s_defaultBindings[];
    static const int s_numBindings;

    // Movement key codes
    static constexpr int MOVE_UP    = KEY_W;
    static constexpr int MOVE_DOWN  = KEY_S;
    static constexpr int MOVE_LEFT  = KEY_A;
    static constexpr int MOVE_RIGHT = KEY_D;
};

extern ActionInput* g_actionInput;
