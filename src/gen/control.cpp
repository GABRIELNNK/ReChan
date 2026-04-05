// control.cpp = from C:\CHAN\GAME\SRC\GEN\CONTROL.CPP
// InputManager / Control / Button implementations
#include "gen/control.h"
#include "config.h"
#include "p3d/keycode.h"

#if PAD_KEYBOARD_EMULATION
#include "p3d/input.h"    // PlatformInput (PC keyboard/mouse)
#endif

// Global singleton (PSX: 0x800DD69C)
InputManager* g_inputManager = nullptr;

// PSX: 4Game_con_GameMode and 7MenuMgr_sControlMode expanded to 16 s16 entries.
static const s16 sGameControlMode[16] = {
    3, 2, 3, 3,
    3, 3, 3, 3,
    3, 1, 1, 3,
    2, 2, 2, 2,
};

static const s16 sTitleControlMode[16] = {
    3, 3, 3, 3,
    3, 3, 3, 3,
    3, 1, 1, 3,
    3, 3, 3, 3,
};

static const s16 sMenuControlMode[16] = {
    3, 3, 3, 3,
    3, 3, 3, 3,
    3, 3, 3, 3,
    3, 3, 3, 3,
};

const s16* GameControlModeArray() {
    return sGameControlMode;
}

const s16* TitleControlModeArray() {
    return sTitleControlMode;
}

const s16* MenuControlModeArray() {
    return sMenuControlMode;
}

// PSX: 12InputManager_sDefaultMapArray / 12InputManager_sPlayerMap
static const u8 sDefaultMapArray[16] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15
};

static const u8 sPlayerMap[3][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 0, 2, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 1, 2, 0, 3, 5, 6, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
};


// Button


// Button::Input (0x8002D8C4) = process raw bit (0 or 1)
void Button::Input(s32 bit) {
    prevInput = rawInput;
    rawInput = bit;

    if (mode == BUTTON_MODE_ONESHOT || mode == BUTTON_MODE_REPEAT) {
        state = (rawInput && !prevInput) ? 1 : 0;
    } else {
        state = rawInput ? 1 : 0;
    }
}

// Button::GetState (0x8002D898) = return state based on mode
s32 Button::GetState() const {
    s32 out = state;
    if (mode == BUTTON_MODE_ONESHOT || mode == BUTTON_MODE_REPEAT) {
        const_cast<Button*>(this)->state = 0;
    }
    return out;
}

// Button::SetMode (0x8002D9E8)
void Button::SetMode(s16 m) {
    mode = static_cast<ButtonMode>(m);
    rawInput = 0;
    prevInput = 0;
    state = 0;
}


// Control


// Control::Input (0x8002DCF0) = dispatch raw button bits to individual Buttons
// PSX iterates 16 buttons, looks up controlMap[i] for physical->logical mapping,
// then calls Button::Input with the corresponding bit.
void Control::Input(u32 buttonBits) {
    for (int i = 0; i < 16; ++i) {
        int mapped = controlMap[i];
        s32 bit = ((buttonBits & (1u << i)) != 0) ? 1 : 0;
        buttons[mapped].Input(bit);
    }
}

// Control::GetMask (0x8002DDEC) = combine 16 Button::GetState results into bitmask
u32 Control::GetMask() const {
    u32 mask = 0;
    for (int i = 0; i < 16; ++i) {
        if (buttons[i].GetState()) {
            mask |= (1u << i);
        }
    }
    return mask;
}

// Control::SetControlMapArray = set button remapping table
void Control::SetControlMapArray(const u8* map) {
    for (int i = 0; i < 16; ++i) {
        controlMap[i] = map[i];
    }
}

void Control::ApplyCurrentModeMap() {
    if (!modeMap) {
        return;
    }

    for (int i = 0; i < 16; ++i) {
        buttons[i].SetMode(modeMap[i]);
    }
}


// InputManager


// InputManager::InputManager (0x8002DF74)
InputManager::InputManager() {
    UpdateReverseMap();
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

void InputManager::SetControlModeArray(s16 padIndex, const s16* modeMap) {
    if (padIndex < 0 || padIndex > 1 || !modeMap) {
        return;
    }

    Control& ctrl = controls[padIndex];
    ctrl.modeMap = modeMap;
    ctrl.ApplyCurrentModeMap();
}

const u8* InputManager::PlayerMapArray() const {
    return sPlayerMap[playerConfig % 3];
}

const u8* InputManager::DefaultMapArray() const {
    return sDefaultMapArray;
}

void InputManager::SetPlayerConfig(u8 config) {
    playerConfig = config % 3;
    UpdateReverseMap();
}

void InputManager::UpdateReverseMap() {
    const u8* map = PlayerMapArray();
    for (u8 i = 0; i < 16; ++i) {
        reverseMap[map[i]] = i;
    }
}

// InputManager::InternalReset (0x8002E550)
void InputManager::InternalReset() {
    // PSX default resets map and button state, then applies mode map later.
    for (int p = 0; p < 2; ++p) {
        for (int i = 0; i < 16; ++i) {
            controls[p].buttons[i].SetMode(BUTTON_MODE_DEFAULT);
        }
        controls[p].SetControlMapArray(sDefaultMapArray);
        controls[p].modeMap = nullptr;
        controls[p].flags = 0;
        controls[p].rawButtons = 0;
        controls[p].hasConnectedPad = 0;
        controls[p].analogLX = controls[p].analogLY = 128;
        controls[p].analogRX = controls[p].analogRY = 128;
    }
    controlValFlags[0] = controlValFlags[1] = 0;
}


// PC keyboard -> PSX pad emulation (behind config macro)

#if PAD_KEYBOARD_EMULATION

// Keyboard binding: maps a PC key code to a PSX pad button bit
struct KeyPadBinding {
    int keyCode;
    u32 padBit;
};

// Default keyboard -> PSX pad bindings
// Movement:     WASD + Arrow Keys -> D-pad Up/Left/Down/Right
// Face buttons: I/J/K/L + V/Z/X/C + Space/Ctrl -> Triangle/Square/Cross/Circle
// Shoulders:    U/O -> L1/R1,  Y/P -> L2/R2
// Start/Select: Enter/Escape and Backspace/Tab
// Stick clicks: N/M -> L3/R3
static const KeyPadBinding s_keyBindings[] = {
    // D-pad
    { KEY_W,  PsxPad::Up       },
    { KEY_A,  PsxPad::Left     },
    { KEY_S,  PsxPad::Down     },
    { KEY_D,  PsxPad::Right    },
    { KEY_UP,    PsxPad::Up    },
    { KEY_LEFT,  PsxPad::Left  },
    { KEY_DOWN,  PsxPad::Down  },
    { KEY_RIGHT, PsxPad::Right },
    // Face buttons
    { KEY_I,  PsxPad::Triangle },
    { KEY_K,  PsxPad::Cross    },
    { KEY_J,  PsxPad::Square   },
    { KEY_L,  PsxPad::Circle   },
    { KEY_V,  PsxPad::Triangle },
    { KEY_X,  PsxPad::Cross    },
    { KEY_Z,  PsxPad::Square   },
    { KEY_C,  PsxPad::Circle   },
    { KEY_SPACE,        PsxPad::Cross },
    // Shoulders
    { KEY_U,  PsxPad::L1       },
    { KEY_O,  PsxPad::R1       },
    { KEY_Y,  PsxPad::L2       },
    { KEY_P,  PsxPad::R2       },
    // Start / Select
    { KEY_ENTER,     PsxPad::Start  },
    { KEY_ESCAPE,    PsxPad::Start  },
    { KEY_BACKSPACE, PsxPad::Select },
    { KEY_TAB,       PsxPad::Select },
    // Stick clicks
    { KEY_N,  PsxPad::L3       },
    { KEY_M,  PsxPad::R3       },
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

#endif // PAD_KEYBOARD_EMULATION
