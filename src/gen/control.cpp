// control.cpp — from C:\CHAN\GAME\SRC\GEN\CONTROL.CPP
// InputManager / Control / Button implementations
#include "gen/control.h"

#if RC_FEATURE_PAD_KEYBOARD_EMULATION
#include "p3d/input.h"    // PlatformInput (PC keyboard/mouse)
#include "pddi/pddidev.h"  // pddiInput key constants
#endif

// Global singleton (PSX: 0x800DD69C)
InputManager* g_inputManager = nullptr;

// ============================================================================
// Button
// ============================================================================

// Button::Input (0x8002D8C4) — process raw bit (0 or 1)
void Button::Input(s32 bit) {
    prevInput = rawInput;
    rawInput = bit;
}

// Button::GetState (0x8002D898) — return state based on mode
s32 Button::GetState() const {
    switch (mode) {
        case BUTTON_MODE_RAW:
            return rawInput;
        case BUTTON_MODE_ONESHOT:
            // True only on the frame the button is first pressed
            return (rawInput && !prevInput) ? 1 : 0;
        default:
            return rawInput;
    }
}

// Button::SetMode (0x8002D9E8)
void Button::SetMode(ButtonMode m) {
    mode = m;
    rawInput = 0;
    prevInput = 0;
}

// ============================================================================
// Control
// ============================================================================

// Control::Input (0x8002DCF0) — dispatch raw button bits to individual Buttons
// PSX iterates 16 buttons, looks up controlMap[i] for physical->logical mapping,
// then calls Button::Input with the corresponding bit.
void Control::Input(u32 buttonBits) {
    for (int i = 0; i < 16; ++i) {
        int mapped = controlMap[i];
        s32 bit = (buttonBits >> mapped) & 1;
        buttons[i].Input(bit);
    }
}

// Control::GetMask (0x8002DDEC) — combine 16 Button::GetState results into bitmask
u32 Control::GetMask() const {
    u32 mask = 0;
    for (int i = 0; i < 16; ++i) {
        if (buttons[i].GetState()) {
            mask |= (1u << i);
        }
    }
    return mask;
}

// Control::SetControlMapArray — set button remapping table
void Control::SetControlMapArray(const u8* map) {
    for (int i = 0; i < 16; ++i) {
        controlMap[i] = map[i];
    }
}

// ============================================================================
// InputManager
// ============================================================================

// InputManager::InputManager (0x8002DF74)
InputManager::InputManager() {
    InternalReset();
}

// InputManager::~InputManager (0x8002E048)
InputManager::~InputManager() {
    // PSX: destroys Control objects, calls ~Manager
}

// InputManager::ServiceInput (0x8002E0D4)
// Core pad processing: stores raw buttons, handles analog-to-digital,
// dispatches to Control::Input for per-button processing.
// PSX reads Sony pad buffer; on PC we pass pre-built button bits.
void InputManager::ServiceInput(u32 buttons, u16 padIndex) {
    if (padIndex > 1) return;

    Control& ctrl = controls[padIndex];

    // Store raw button bitmask (PSX: control+32)
    ctrl.rawButtons = buttons;

    // Mark as connected and updated (PSX: control+28, bits 0+1)
    ctrl.flags = 0x03;
    ctrl.hasConnectedPad = 1;

    // Dispatch to per-button processing
    ctrl.Input(buttons);
}

// InputManager::Step (0x8002E6E8)
// Per-frame cleanup: clears controlValFlags
void InputManager::Step() {
    controlValFlags[0] = 0;
    controlValFlags[1] = 0;
}

// InputManager::GetControlVal (0x8002E5BC)
// Returns processed button bitmask from Control::GetMask
u32 InputManager::GetControlVal(u16 padIndex) {
    if (padIndex > 1) return 0;

    Control& ctrl = controls[padIndex];

    // PSX checks flags: bit 1 (updated) and bit 0 (connected)
    if (!(ctrl.flags & 0x02)) return 0; // not updated this frame
    if (!(ctrl.flags & 0x01)) return 0; // not connected

    return ctrl.GetMask();
}

// InputManager::GetRawButtons
u32 InputManager::GetRawButtons(u16 padIndex) const {
    if (padIndex > 1) return 0;
    return controls[padIndex].rawButtons;
}

// InputManager::GetAnalog
void InputManager::GetAnalog(u16 padIndex, u8& lx, u8& ly, u8& rx, u8& ry) const {
    if (padIndex > 1) { lx = ly = rx = ry = 128; return; }
    const Control& ctrl = controls[padIndex];
    lx = ctrl.analogLX;
    ly = ctrl.analogLY;
    rx = ctrl.analogRX;
    ry = ctrl.analogRY;
}

// InputManager::SetControlMapArray (0x8002E2F0)
void InputManager::SetControlMapArray(s16 padIndex, const u8* map) {
    if (padIndex < 0 || padIndex > 1) return;
    controls[padIndex].SetControlMapArray(map);
}

// InputManager::InternalReset (0x8002E550)
void InputManager::InternalReset() {
    // PSX: sets button modes from config bytes at this+744 and this+747
    // Default: all buttons in RAW mode
    for (int p = 0; p < 2; ++p) {
        for (int i = 0; i < 16; ++i) {
            controls[p].buttons[i].SetMode(BUTTON_MODE_RAW);
        }
        controls[p].flags = 0;
        controls[p].rawButtons = 0;
        controls[p].hasConnectedPad = 0;
        controls[p].analogLX = controls[p].analogLY = 128;
        controls[p].analogRX = controls[p].analogRY = 128;
    }
    controlValFlags[0] = controlValFlags[1] = 0;
}

// ============================================================================
// PC keyboard -> PSX pad emulation (behind config macro)
// ============================================================================
#if RC_FEATURE_PAD_KEYBOARD_EMULATION

// Keyboard binding: maps a PC key code to a PSX pad button bit
struct KeyPadBinding {
    int keyCode;
    u32 padBit;
};

// Default keyboard -> PSX pad bindings
// Movement:     WASD -> D-pad Up/Left/Down/Right
// Face buttons: I/J/K/L -> Triangle/Square/Cross/Circle
// Shoulders:    U/O -> L1/R1,  Y/P -> L2/R2
// Start/Select: Enter/Backspace
// Stick clicks: N/M -> L3/R3
static const KeyPadBinding s_keyBindings[] = {
    // D-pad
    { 'W',  PsxPad::Up       },
    { 'A',  PsxPad::Left     },
    { 'S',  PsxPad::Down     },
    { 'D',  PsxPad::Right    },
    // Face buttons
    { 'I',  PsxPad::Triangle },
    { 'K',  PsxPad::Cross    },
    { 'J',  PsxPad::Square   },
    { 'L',  PsxPad::Circle   },
    // Shoulders
    { 'U',  PsxPad::L1       },
    { 'O',  PsxPad::R1       },
    { 'Y',  PsxPad::L2       },
    { 'P',  PsxPad::R2       },
    // Start / Select
    { 257,  PsxPad::Start    },  // GLFW_KEY_ENTER
    { 259,  PsxPad::Select   },  // GLFW_KEY_BACKSPACE
    // Stick clicks
    { 'N',  PsxPad::L3       },
    { 'M',  PsxPad::R3       },
};

static constexpr int s_numKeyBindings = sizeof(s_keyBindings) / sizeof(s_keyBindings[0]);

void InputManager::UpdateFromKeyboard(PlatformInput* platform, u16 padIndex) {
    if (!platform) return;

    u32 buttons = 0;
    for (int i = 0; i < s_numKeyBindings; ++i) {
        if (platform->IsKeyDown(s_keyBindings[i].keyCode)) {
            buttons |= s_keyBindings[i].padBit;
        }
    }

    ServiceInput(buttons, padIndex);
}

#endif // RC_FEATURE_PAD_KEYBOARD_EMULATION
