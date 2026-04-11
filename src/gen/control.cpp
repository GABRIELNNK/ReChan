#include "gen/control.h"
#include "config.h"
#include "p3d/keycode.h"

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
// PSX: RawHandler__6Buttonl updates duration (frames held) and state.
void Button::Input(s32 bit) {
    prevInput = rawInput;
    rawInput = bit;

    if (rawInput) {
        duration++;
    }
    else {
        duration = 0;
    }

    if (mode == BUTTON_MODE_ONESHOT || mode == BUTTON_MODE_REPEAT) {
        state = (rawInput && !prevInput) ? 1 : 0;
    }
    else {
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

// PSX: GetMappedButton__C7Controlc (0x8002DF14)
// Maps PSX button bit index through reverseMap + controlMap to Button*.
Button* InputManager::GetButtonForBit(u16 padIndex, u8 bitIndex) {
    if (padIndex > 1) return &controls[0].buttons[0];
    if (bitIndex >= 16) return &controls[padIndex].buttons[0];
    Control& ctrl = controls[padIndex];
    u8 logical = reverseMap[bitIndex];
    u8 physical = ctrl.controlMap[logical & 0xF];
    return &ctrl.buttons[physical & 0xF];
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

u8 InputManager::GetPlayerConfig() const {
    return playerConfig;
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

// PSX: gp+144 - shock enabled flag (0 on PC, no DualShock)
static s32 g_shockEnabled = 0;

s32 GetShock() {
    return g_shockEnabled;
}

s32 IsDualShock() {
    return PadGetState(0) == 6 ? 1 : 0;
}

void SetShock(s32 enabled) {
    g_shockEnabled = enabled ? 1 : 0;
}

// PSX: PadGetState (Sony lib) - PC stub returns 0 (not state 6)
s32 PadGetState(s32 /*port*/) {
    return 0;
}

// PSX: SetActuator (CONTROL.CPP:250, 0x8002D540) - PC no-op
void SetActuator(u8 /*motor*/, u8 /*speed*/, u32 /*duration*/) {}

// PSX: ClearActuator (CONTROL.CPP:242, 0x8002D52C) - PC no-op
void ClearActuator() {}

// PSX: UpdateActuator (CONTROL.CPP:262, 0x8002D564) - PC no-op
void UpdateActuator(s32 /*param*/) {}

// PSX: Shock (CONTROL.CPP, 0x8002D6C0)
void Shock(ShockEnum type) {
    MARKFUNCTION(0x8002D6C0);
    if (!g_shockEnabled) {
        return;
    }
    if (PadGetState(0) != 6) {
        return;
    }
    u8 motor;
    u8 speed;
    u32 duration;
    switch (type) {
        case SHOCK_0:  motor = 0; speed = 128; duration = 15; break;
        case SHOCK_1:  motor = 0; speed = 96; duration = 15; break;
        case SHOCK_2:  motor = 0; speed = 96; duration = 15; break;
        case SHOCK_3:  motor = 0; speed = 128; duration = 15; break;
        case SHOCK_4:  motor = 0; speed = 128; duration = 15; break;
        case SHOCK_5:  motor = 0; speed = 128; duration = 15; break;
        case SHOCK_6:  motor = 0; speed = 188; duration = 15; break;
        case SHOCK_7:  motor = 0; speed = 128; duration = 15; break;
        case SHOCK_8:  motor = 0; speed = 255; duration = 15; break;
        case SHOCK_9:  motor = 0; speed = 255; duration = 15; break;
        case SHOCK_10: motor = 0; speed = 128; duration = 15; break;
        case SHOCK_11: motor = 0; speed = 128; duration = 15; break;
        case SHOCK_12: motor = 0; speed = 188; duration = 15; break;
        case SHOCK_13: motor = 0; speed = 128; duration = 10; break;
        case SHOCK_14: motor = 0; speed = 96; duration = 15; break;
        case SHOCK_15: motor = 0; speed = 255; duration = 15; break;
        case SHOCK_16: motor = 0; speed = 255; duration = 20; break;
        case SHOCK_17: motor = 0; speed = 255; duration = 20; break;
        case SHOCK_CLEAR:
            ClearActuator();
            UpdateActuator(0);
            return;
        default:
            motor = 1; speed = 0; duration = 15; break;
    }
    SetActuator(motor, speed, duration);
    UpdateActuator(0);
}
