// control.h
// PSX InputManager / Control / Button classes
// Singleton on PSX at 0x800DD69C (gp+3408)
#pragma once

#include "core.h"
#include "config.h"

class PlatformInput; // PC platform input (keyboard/mouse via GLFW)


// PSX pad button bits (from Sony libpad, active-high after NOT by ReadSonyPads)
// These are the bit positions in Control::rawButtons and GetControlVal() output.

namespace PsxPad {
    static constexpr u32 Select   = 0x0001;
    static constexpr u32 L3       = 0x0002;
    static constexpr u32 R3       = 0x0004;
    static constexpr u32 Start    = 0x0008;
    static constexpr u32 Up       = 0x0010;
    static constexpr u32 Right    = 0x0020;
    static constexpr u32 Down     = 0x0040;
    static constexpr u32 Left     = 0x0080;
    static constexpr u32 L2       = 0x0100;
    static constexpr u32 R2       = 0x0200;
    static constexpr u32 L1       = 0x0400;
    static constexpr u32 R1       = 0x0800;
    static constexpr u32 Triangle = 0x1000;
    static constexpr u32 Circle   = 0x2000;
    static constexpr u32 Cross    = 0x4000;
    static constexpr u32 Square   = 0x8000;
}


// Button = individual button state tracker (PSX: 40 bytes per instance)
// 16 per Control, at Control+52, stride 40

enum ButtonMode : s32 {
    BUTTON_MODE_RAW     = 0, // GetState = true while held
    BUTTON_MODE_ONESHOT = 1, // GetState = true on first press frame only
};

struct Button {
    s32 rawInput  = 0;    // current raw input (0 or 1)
    s32 prevInput = 0;    // previous frame raw input (for oneshot)
    ButtonMode mode = BUTTON_MODE_RAW;

    void Input(s32 bit);                // 0x8002D8C4 = process raw bit
    s32  GetState() const;              // 0x8002D898 = return state by mode
    void SetMode(ButtonMode m);         // 0x8002D9E8 = set button mode
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
    u8 controlMap[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

    // +724: connected pad detected flag
    s32 hasConnectedPad = 0;

    void Input(u32 buttonBits);         // 0x8002DCF0 = dispatch bits to Buttons
    u32  GetMask() const;               // 0x8002DDEC = combined bitmask from Buttons
    void SetControlMapArray(const u8* map);
};


// InputManager = game-level input system (PSX: ~1492 bytes)
// Singleton on PSX at 0x800DD69C

class InputManager {
public:
    InputManager();                                         // 0x8002DF74
    ~InputManager();                                        // 0x8002E048

    // --- Per-frame pipeline (called from game loop) ---
    // PSX: ReadSonyPads() -> ServiceInput(buttons,0) -> ServiceInput(buttons>>16,1) -> Step()
    void ServiceInput(u32 buttons, u16 padIndex);           // 0x8002E0D4
    void Step();                                            // 0x8002E6E8

    // --- Button query ---
    // Returns processed button bitmask for pad port (via Control::GetMask)
    u32  GetControlVal(u16 padIndex);                       // 0x8002E5BC

    // Direct raw button access (DebugCam reads InputManager+60 = controls[0].rawButtons)
    u32  GetRawButtons(u16 padIndex) const;

    // Analog sticks
    void GetAnalog(u16 padIndex, u8& lx, u8& ly, u8& rx, u8& ry) const;

    // --- Configuration ---
    void SetControlMapArray(s16 padIndex, const u8* map);   // 0x8002E2F0
    void InternalReset();                                   // 0x8002E550

#if RC_FEATURE_PAD_KEYBOARD_EMULATION
    // PC: read keyboard state from PlatformInput => PSX button bits => ServiceInput
    void UpdateFromKeyboard(PlatformInput* platform, u16 padIndex = 0);
#endif

    // Direct access for game code that needs Control struct
    Control controls[2];

private:
    s32 controlValFlags[2] = {};   // per-frame query flags (PSX: at +1484)
};

// Global singleton (PSX: 0x800DD69C)
extern InputManager* g_inputManager;
