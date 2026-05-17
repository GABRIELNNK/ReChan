#pragma once
#include "core.h"
#include "p3d/keycode.h"

class PlatformInput;

// Game action IDs - bit positions in Humanoid::commandBits.
// These match the action IDs that PSX FindActionRequest returns.
// RequestAction(id) does: commandBits |= (1 << id)
enum GameAction : s32 {
    GA_NONE = 0,
    GA_GUARD_RELEASE = 1,
    GA_MOVE = 2,
    GA_JUMP = 3,
    GA_JUMP_DIRECTIONAL = 4,
    GA_DIVE_ROLL = 5,
    GA_STRAFE = 6,
    GA_GRAB = 7,
    GA_PUNCH = 8,
    GA_KICK = 9,
    GA_BACK_PUNCH = 10,
    GA_BACK_KICK = 11,
    GA_HEAVY_PUNCH = 12,
    GA_HEAVY_KICK = 13,
    GA_SPECIAL_GRAB = 14,
    GA_GRAB_FORWARD = 15,
    GA_GRAB_HELD = 17,
    GA_GRAB_FWD_HELD = 18,
    GA_AI_DIVE_ROLL = 21,
};

// Input actions - abstract game inputs that can be bound to keyboard or gamepad.
// The game queries these directly. No PSX pad bit emulation.
enum Action : s32 {
    ACTION_JUMP,
    ACTION_PUNCH,
    ACTION_KICK,
    ACTION_GRAB,
    ACTION_DIVE_ROLL,
    ACTION_STRAFE,
    ACTION_COUNTER,
    ACTION_STATUS_DISPLAY,
    ACTION_MOVE_UP,
    ACTION_MOVE_DOWN,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_OPEN_CLOSE_MENU,
    ACTION_MENU_UP,
    ACTION_MENU_DOWN,
    ACTION_MENU_LEFT,
    ACTION_MENU_RIGHT,
    ACTION_MENU_CONFIRM,
    ACTION_MENU_BACK,
    ACTION_MENU_CLEAR,
    ACTION_COUNT,
};

// Convert between action enum and inline prompt token text used by <ACT:...>.
const char* ActionToToken(Action action);
Action ActionFromToken(const char* token);

// Gamepad button IDs for bindings (matches GLFW gamepad layout)
namespace GpBtn {
    static constexpr s32 A = 0;
    static constexpr s32 B = 1;
    static constexpr s32 X = 2;
    static constexpr s32 Y = 3;
    static constexpr s32 LB = 4;
    static constexpr s32 RB = 5;
    static constexpr s32 Back = 6;
    static constexpr s32 Start = 7;
    static constexpr s32 Guide = 8;
    static constexpr s32 LStick = 9;
    static constexpr s32 RStick = 10;
    static constexpr s32 DpadUp = 11;
    static constexpr s32 DpadRight = 12;
    static constexpr s32 DpadDown = 13;
    static constexpr s32 DpadLeft = 14;
    static constexpr s32 NONE = -1;
};

namespace MouseBtn {
    static constexpr s32 Left = 0;
    static constexpr s32 Right = 1;
    static constexpr s32 Middle = 2;
    static constexpr s32 NONE = -1;
};

// Gamepad axis IDs for bindings
namespace GpAxis {
    static constexpr s32 LeftX = 0;
    static constexpr s32 LeftY = 1;
    static constexpr s32 RightX = 2;
    static constexpr s32 RightY = 3;
    static constexpr s32 LTrigger = 4;
    static constexpr s32 RTrigger = 5;
    static constexpr s32 NONE = -1;
};

struct ActionBinding {
    int keyboardKeys[2];   // per-slot keyboard key, or 0 for none
    s32 mouseButtons[2];   // per-slot MouseBtn value, or MouseBtn::NONE
    s32 gamepadButton;     // GpBtn value, or GpBtn::NONE
    s32 gamepadButton2;    // alternate GpBtn (e.g. D-pad for movement), or GpBtn::NONE
    s32 gamepadAxis;       // GpAxis value, or GpAxis::NONE
    float axisThreshold;   // axis value threshold (>0 for positive, <0 for negative direction)
};

struct InputState {
    bool down = false;
    bool prevDown = false;
    s16 duration = 0;
};

// ActionInput - modern action-based input system.
// All game input flows through named actions with per-device bindings.
// Both keyboard and gamepad are treated as first-class input sources.
class ActionInput {
public:
    ActionInput();

    // Call once per frame before anything reads input.
    void Update(PlatformInput* platform);

    // Action state queries
    bool JustPressed(Action action) const;
    bool IsHeld(Action action) const;
    bool JustReleased(Action action) const;
    s16 GetDuration(Action action) const;

    // Analog movement axes (-127..+127), from WASD or left stick
    s32 GetMoveX() const { return moveX; }
    s32 GetMoveY() const { return moveY; }
    bool HasMovement() const { return moveX != 0 || moveY != 0; }

    // Right stick axes (-127..+127)
    s32 GetLookX() const { return lookX; }
    s32 GetLookY() const { return lookY; }

    // True if gamepad is the active input device (for UI glyph display, etc.)
    bool IsGamepadActive() const { return gamepadActive; }
    bool HadKeyboardInputThisFrame() const { return hadKeyboardInputThisFrame; }
    bool HadMouseInputThisFrame() const { return hadMouseInputThisFrame; }
    bool HadGamepadInputThisFrame() const { return hadGamepadInputThisFrame; }

    // True if any action was just pressed this frame (for "press any button" screens)
    bool AnyJustPressed() const;
    bool AnyJustPressed2() const;

    // Mouse state cached during Update()
    void GetMousePosition(double& x, double& y) const;
    bool IsMouseButtonDown(s32 button) const;
    bool IsMouseButtonTriggered(s32 button) const;

    // Mouse wheel accumulation (fed by window proc)
    void AddScrollDelta(s32 delta) { scrollDelta += delta; }
    s32 ConsumeScrollDelta();

    // Rebinding
    void SetKeyBinding(Action action, int key);
    void SetMouseButtonBinding(Action action, s32 mouseButton);
    void SetKeyBindingSlot(Action action, s32 slot, int key);
    void SetMouseButtonBindingSlot(Action action, s32 slot, s32 mouseButton);
    void SetGamepadButtonBinding(Action action, s32 gpButton);
    int GetKeyBinding(Action action) const;
    s32 GetMouseButtonBinding(Action action) const;
    int GetKeyBindingSlot(Action action, s32 slot) const;
    s32 GetMouseButtonBindingSlot(Action action, s32 slot) const;
    s32 GetGamepadButtonBinding(Action action) const;

    // Persisted desktop binding code format:
    //  0 = unbound, >0 = keyboard key code, <0 = mouse button code (-1-left, -2-right, -3-middle)
    static s32 EncodeKeyboardBindingCode(s32 key);
    static s32 EncodeMouseBindingCode(s32 mouseButton);
    void SetDesktopBindingCode(Action action, s32 slot, s32 code);
    s32 GetDesktopBindingCode(Action action, s32 slot) const;
    void GetDesktopBindingLabel(Action action, s32 slot, char* outLabel, s32 outLabelLen) const;

    // Last desktop input edge seen during Update(), for bind capture UIs.
    s32 GetTriggeredKeyThisFrame() const { return triggeredKeyThisFrame; }
    s32 GetTriggeredMouseButtonThisFrame() const { return triggeredMouseButtonThisFrame; }

    u32 GetPadButtons(bool menuActionsEnabled) const;
    bool UsesAnalogPad() const { return gamepadActive; }
    void GetPadAnalog(u8& lx, u8& ly, u8& rx, u8& ry) const;

private:
    InputState states[ACTION_COUNT] = {};
    ActionBinding bindings[ACTION_COUNT] = {};

    bool gamepadActive = false;
    bool keysRegistered = false;
    bool hadKeyboardInputThisFrame = false;
    bool hadMouseInputThisFrame = false;
    bool hadGamepadInputThisFrame = false;
    s32 moveX = 0;
    s32 moveY = 0;
    s32 lookX = 0;
    s32 lookY = 0;
    double mouseX = 0.0;
    double mouseY = 0.0;
    bool mouseDown[3] = {};
    bool mousePrev[3] = {};
    s32 triggeredKeyThisFrame = 0;
    s32 triggeredMouseButtonThisFrame = MouseBtn::NONE;
    s32 scrollDelta = 0;

    bool PollAction(Action action, PlatformInput* platform) const;
};

extern ActionInput* g_actionInput;
