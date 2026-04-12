#pragma once
#include "fe/hdmenu.h"

class MenuMgr;
struct xcOverlayData;

// hdItemGoto (32 bytes PSX) - GOTO item, pushes a target menu when selected
// PSX: HDMENU.CPP:730, ctor 0x8005DD14
class hdItemGoto : public hdMenuItem {
public:
    u8* textObj = nullptr;      // +12 (a1[3]): xcTextObj prim
    union {
        u32 gotoHash;           // +28 (a1[7]): pre-PostFlight: xcHash of target menu name
        hdMenu* gotoMenu;       // +28 (a1[7]): post-PostFlight: resolved target menu ptr
    };

    hdItemGoto(u8* text, const char* target);
    ~hdItemGoto() override;

    void PostFlight(MenuMgr* mgr) override;
    void SelectItem(MenuMgr* mgr) override;
};

// hdItemButton (28 bytes PSX) - BUTTON item, calls callback when selected
// PSX: HDMENU.CPP:927, ctor 0x8005E118
class hdItemButton : public hdMenuItem {
public:
    u8* textObj = nullptr;      // +12 (a1[3]): xcTextObj prim

    hdItemButton(u8* text, const char* name);
    ~hdItemButton() override = default;

    void SelectItem(MenuMgr* mgr) override;
};

// hdItemSelection (36 bytes PSX) - SELECT item with multi-frame value display
// PSX: HDMENU.CPP:554, ctor 0x8005D778
class hdItemSelection : public hdMenuItem {
public:
    u8* labelTextObj = nullptr; // +12 (a1[3]): label xcTextObj
    u8* valueTextObj = nullptr; // +32 (a1[8]): value xcTextObj (multi-frame)
    s32 enabled = 1;            // +28 (a1[7]): enabled flag

    hdItemSelection(xcOverlayData* overlay, const char* name, u8* rawData);
    ~hdItemSelection() override = default;

    void SetColour(xcColour1555& col, bool flag) override;
    void IncItem() override;
    void DecItem() override;
    void SetValue(u32 val) override;
    u32 GetValue() override;
};

// hdShockSelection (36 bytes PSX) - SHKSELECT for DualShock vibration toggle
// PSX: HDMENU.CPP:957, ctor 0x8005E1CC
class hdShockSelection : public hdItemSelection {
public:
    // PSX: _16hdShockSelectionP9xcOverlayPc (0x8005E1CC)
    hdShockSelection(xcOverlayData* overlay, const char* name, u8* rawData);
    ~hdShockSelection() override = default;
};

// hdSndItemSelection (68 bytes PSX) - SNDSELECT for volume bars
// PSX: HDMENU.CPP:622, ctor 0x8005DA28
class hdSndItemSelection : public hdItemSelection {
public:
    s16 numSteps = 0;        // +36: number of bar segments
    s16 pad = 0;
    u32 maxValue = 0;        // +40: maximum raw value
    u32 stepSize = 0;        // +44: maxValue / numSteps
    s16 currentStep = 0;     // +48: current step index
    s16 pad2 = 0;
    u32 currentValue = 0;    // +52: current raw value
    char displayStr[12] = {}; // +56: bar display string (o/f chars)

    // PSX: _18hdSndItemSelectionP9xcOverlayPcii (0x8005DA28)
    hdSndItemSelection(xcOverlayData* overlay, const char* name, u8* rawData, s32 steps, u32 maxVal);
    ~hdSndItemSelection() override = default;

    void SetColour(xcColour1555& col, bool flag) override { hdMenuItem::SetColour(col, flag); }
    void IncItem() override;
    void DecItem() override;
    void SetValue(u32 val) override;
    u32 GetValue() override;
    void UpdateShown();
};

// hdControllerSelection (40 bytes PSX) - CTLSELECT for controller config
// PSX: FEMNUMGR.CPP:1539, ctor 0x80013148
class hdControllerSelection : public hdItemSelection {
public:
    xcOverlayData* descOverlay = nullptr; // +36 (a1[9]): overlay for control descriptions

    // PSX: _21hdControllerSelectionP9xcOverlayPcT1 (0x80013148)
    hdControllerSelection(xcOverlayData* overlay, const char* name, u8* rawData, xcOverlayData* descOvl);
    ~hdControllerSelection() override = default;

    void IncItem() override;
    void DecItem() override;
};

// hdAlphaSelection (44 bytes PSX) - ALPHASELECT for letter selection (a-z)
// PSX: HDMENU.CPP:543, ctor 0x8005D728
// Inherits hdNumericSelection
class hdNumericSelection; // forward
class hdAlphaSelection : public hdMenuItem {
public:
    u8* labelTextObj = nullptr;  // +12
    s32 minValue = 0;            // +28
    s32 maxValue = 0;            // +32
    s32 currentValue = 0;        // +36
    char displayStr[8] = {};     // +40

    hdAlphaSelection(xcOverlayData* overlay, const char* name, u8* rawData, s32 minVal, s32 maxVal);
    ~hdAlphaSelection() override = default;

    void IncItem() override;
    void DecItem() override;
    void SetValue(u32 val) override;
    u32 GetValue() override;
};

// hdDynItemSelection (48 bytes PSX) - DYNSELECT for dynamic item lists
// PSX: HDMENU.CPP:714, ctor 0x8005DCD0
class hdDynItemSelection : public hdItemSelection {
public:
    xcOverlayData* dynOverlay = nullptr; // +36 (a1[9])

    hdDynItemSelection(xcOverlayData* overlay, const char* name, u8* rawData);
    ~hdDynItemSelection() override = default;
};

// hdNumericSelection (44 bytes PSX) - NUMSELECT for numeric value selection
// PSX: HDMENU.CPP:467, ctor 0x8005D49C
class hdNumericSelection : public hdMenuItem {
public:
    u8* labelTextObj = nullptr;  // +12 (a1[3])
    s32 minValue = 0;            // +28 (a1[7])
    s32 maxValue = 0;            // +32 (a1[8])
    s32 currentValue = 0;        // +36 (a1[9])
    char displayStr[8] = {};     // +40 (a1[10])

    // PSX: _18hdNumericSelectionP9xcOverlayPcii (0x8005D49C)
    hdNumericSelection(xcOverlayData* overlay, const char* name, u8* rawData, s32 minVal, s32 maxVal);
    ~hdNumericSelection() override = default;

    void IncItem() override;
    void DecItem() override;
    void SetValue(u32 val) override;
    u32 GetValue() override;
    virtual void ChangeValueText();
};

// hdDynMenu (60 bytes PSX) - DYNMENU with dynamic text slots
// PSX: HDMENU.CPP:826, ctor 0x8005DF40
class hdDynMenu : public hdMenu {
public:
    s32 field36 = 0;             // +36 (a1[9])
    s32 maxItems = 0;            // +40 (a1[10])
    const char* titleStr = nullptr; // +44 (a1[11])
    xcOverlayData* overlay = nullptr; // +48 (a1[12])
    MenuMgr* menuMgr = nullptr;  // +52 (a1[13])
    u8* titleTextObj = nullptr;  // +56 (a1[14])

    hdDynMenu(MenuMgr* mgr, xcOverlayData* ovl, s32 maxCount, u8* rawData);
    ~hdDynMenu() override = default;

    void DynSetup() override;
};

// hdMemCardMenu (80 bytes PSX) - MEMCARD menu for save/load
// PSX: FEMNUMGR.CPP:862, ctor 0x80011BE0
class hdMemCardMenu : public hdMenu {
public:
    u8* textObj1 = nullptr;      // +36
    u8* textObj2 = nullptr;      // +40
    xcOverlayData* overlay = nullptr; // +44
    void* field48 = nullptr;     // +48
    s32 field52 = 0;             // +52
    s32 field56 = 0;             // +56
    u8 field60 = 1;              // +60
    MenuMgr* menuMgr = nullptr;  // +64
    u8* field68 = nullptr;       // +68
    u8* field72 = nullptr;       // +72
    u8* field76 = nullptr;       // +76

    // PSX: _13hdMemCardMenuP7MenuMgrP9xcOverlayT2 (0x80011BE0)
    hdMemCardMenu(MenuMgr* mgr, xcOverlayData* ovl1, xcOverlayData* ovl2, u8* rawData);
    ~hdMemCardMenu() override;
};

// hdDynItemMenu (44 bytes PSX) - DYNITEMMENU with visibility-toggled items
// PSX: HDMENU.CPP:405, ctor 0x8005D2E0
class hdDynItemMenu : public hdMenu {
public:
    s32 field36 = 0;             // +36 (a1[9])
    s32 visibleCount = 0;        // +40 (a1[10])

    // PSX: _13hdDynItemMenui (0x8005D2E0)
    hdDynItemMenu(s32 count);
    ~hdDynItemMenu() override = default;

    void DynSetup() override;
    void InputNextItem() override;
    void InputPrevItem() override;
};
