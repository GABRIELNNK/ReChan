#pragma once
#include "core.h"
#include "config.h"
#include "gen/manager.h"

class ActionInput;

// PSX game-side pad bits after ReadSonyPads byte-swap + NOT processing.
// These are the bit positions in Control::rawButtons and GetControlVal() output.

namespace PsxPad {
    static constexpr u32 Select = 0x0100;
    static constexpr u32 L3 = 0x0200;
    static constexpr u32 R3 = 0x0400;
    static constexpr u32 Start = 0x0800;
    static constexpr u32 Up = 0x1000;
    static constexpr u32 Right = 0x2000;
    static constexpr u32 Down = 0x4000;
    static constexpr u32 Left = 0x8000;
    static constexpr u32 L2 = 0x0001;
    static constexpr u32 R2 = 0x0002;
    static constexpr u32 L1 = 0x0004;
    static constexpr u32 R1 = 0x0008;
    static constexpr u32 Triangle = 0x0010;
    static constexpr u32 Circle = 0x0020;
    static constexpr u32 Cross = 0x0040;
    static constexpr u32 Square = 0x0080;

#if MENU_BACK_USES_CIRCLE
    static constexpr u32 MenuBack = Circle;
#else
    static constexpr u32 MenuBack = Triangle;
#endif
}

// PSX control mode tables used by gameplay and menu systems.
const s16* GameControlModeArray();
const s16* TitleControlModeArray();
const s16* MenuControlModeArray();


// Button = individual button state tracker (PSX: 40 bytes per instance)
// 16 per Control, at Control+52, stride 40

enum ButtonMode : s16 {
    BUTTON_MODE_RAW = 0,
    BUTTON_MODE_ANALOG = 1,
    BUTTON_MODE_DEFAULT = 2,
    BUTTON_MODE_ONESHOT = 3,
    BUTTON_MODE_REPEAT = 5,
};

struct Button {
    s32 rawInput = 0;    // current raw input (0 or 1)
    s32 prevInput = 0;    // previous frame raw input
    s32 state = 0;        // current reported state
    s16 duration = 0;     // PSX +12: frames held (set by RawHandler, read by FindActionRequest)
    ButtonMode mode = BUTTON_MODE_DEFAULT;

    void Input(s32 bit);
    s32  GetState() const;
    void SetMode(s16 m);
};


// Control = one controller port (PSX: 728 bytes, 2 per InputManager)
// PSX offsets relative to Control base (InputManager+28 for controls[0])
struct Control {
    // +28: flags (bit 0 = connected, bit 1 = updated this frame)
    s32 flags = 0;

    // +32: raw button bitmask from ServiceInput (active-high)
    u32 rawButtons = 0;

    // +36: pad type byte from Sony pad data header
    u16 padType = 0;

    // +44: analog stick axes (128=center, 0=full left/up, 255=full right/down)
    // Order: RX, RY, LX, LY (matches Sony DualShock pad data bytes 4-7)
    u8 analogRX = 128;
    u8 analogRY = 128;
    u8 analogLX = 128;
    u8 analogLY = 128;

    // +52: 16 Button objects (PSX: 40 bytes each, total 640 bytes)
    Button buttons[16] = {};

    // +708: logical->physical button map (16 entries)
    u8 controlMap[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

    // +48: pointer to 16-entry button mode table (s16 each)
    const s16* modeMap = nullptr;

    // +724: connected pad detected flag
    s32 hasConnectedPad = 0;

    void Input(u32 buttonBits);         // 0x8002DCF0 = dispatch bits to Buttons
    u32  GetMask() const;               // 0x8002DDEC = combined bitmask from Buttons
    void SetControlMapArray(const u8* map);
    void ApplyCurrentModeMap();         // 0x8002DE88
};


// InputManager = game-level input system (PSX: ~1492 bytes)
// Singleton on PSX at 0x800DD69C
class InputManager : public Manager {
public:
    InputManager();
    ~InputManager() override;

    void ServiceInput(u32 buttons, u16 padIndex);
    void ServiceHostPads(const ActionInput* actionInput, bool commitNow = true);
    void CommitHostPads();
    void Step();

    // Returns processed button bitmask for pad port (via Control::GetMask)
    u32  GetControlVal(u16 padIndex);
    // Direct raw button access (DebugCam reads InputManager+60 = controls[0].rawButtons)
    u32  GetRawButtons(u16 padIndex) const;
    // Analog sticks
    void GetAnalog(u16 padIndex, u8& lx, u8& ly, u8& rx, u8& ry) const;

    void SetControlMapArray(s16 padIndex, const u8* map);
    void SetControlModeArray(s16 padIndex, const s16* modeMap);
    const u8* PlayerMapArray() const;
    const u8* DefaultMapArray() const;
    u8 GetPlayerConfig() const;
    void SetPlayerConfig(u8 config);
    void InternalReset() override;

    // Direct access for game code that needs Control struct
    Control controls[2];

    // PSX: GetMappedButton__C7Controlc (0x8002DF14)
    // Maps PSX button bit index through reverseMap + controlMap to Button*
    Button* GetButtonForBit(u16 padIndex, u8 bitIndex);

private:
    s32 controlValFlags[2] = {};   // per-frame query flags (PSX: at +1484)
    u8 playerConfig = 0;
    u8 reverseMap[16] = {};

    u32 sampledButtons[2] = {};
    u32 sampledPressLatch[2] = {};
    u16 sampledPadType[2] = {};
    u8 sampledAnalogLX[2] = { 128, 128 };
    u8 sampledAnalogLY[2] = { 128, 128 };
    u8 sampledAnalogRX[2] = { 128, 128 };
    u8 sampledAnalogRY[2] = { 128, 128 };

    void UpdateReverseMap();
};

// PSX: 0x800DD69C, defined in control.cpp
extern InputManager* g_inputManager;

// PSX: ShockEnum values for controller rumble (CONTROL.CPP)
enum ShockEnum : s32 {
    SHOCK_0 = 0,
    SHOCK_1 = 1,
    SHOCK_2 = 2,
    SHOCK_3 = 3,
    SHOCK_4 = 4,
    SHOCK_5 = 5,
    SHOCK_6 = 6,
    SHOCK_7 = 7,
    SHOCK_8 = 8,
    SHOCK_9 = 9,
    SHOCK_10 = 10,
    SHOCK_11 = 11,
    SHOCK_12 = 12,
    SHOCK_13 = 13,
    SHOCK_14 = 14,
    SHOCK_15 = 15,
    SHOCK_16 = 16,
    SHOCK_17 = 17,
    SHOCK_CLEAR = 18,
};

void Shock(ShockEnum type);
void SetShock(s32 enabled);
s32 GetShock();
s32 IsDualShock();
void SetActuator(u8 motor, u8 speed, u32 duration);
void ClearActuator();
void UpdateActuator(s32 param);

// PSX: PadGetState (Sony lib) - PC stub
s32 PadGetState(s32 port);
