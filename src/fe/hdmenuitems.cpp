#include "fe/hdmenuitems.h"
#include "fe/menumgr.h"
#include "xclib/xclib.h"

// Text object field hashes used by selection items
static constexpr u32 HASH_LABEL_TEXT = 3132773;       // "label" textObj in overlay
static constexpr u32 HASH_VALUE_TEXT = 0xC8FCCAE0u;   // PSX value textObj used by selection items

// ---- hdItemGoto ----

// PSX: _10hdItemGotoP9xcTextObjPc (0x8005DD14)
hdItemGoto::hdItemGoto(u8* text, const char* target) {
    MARKFUNCTION(0x8005DD14);
    data = text;
    textObj = text;
    if (target)
        gotoHash = xcHash(target);
    else
        gotoHash = 0;
}

hdItemGoto::~hdItemGoto() {
    MARKFUNCTION(0x8005DD7C);
}

// PSX: PostFlight__10hdItemGotoP7MenuMgr (0x8005DDA4)
// Resolves gotoHash to actual hdMenu pointer
void hdItemGoto::PostFlight(MenuMgr* mgr) {
    MARKFUNCTION(0x8005DDA4);
    gotoMenu = mgr->FindMenu(gotoHash);
}

// PSX: SelectItem__10hdItemGotoP7MenuMgr (0x8005DDF8)
// Calls mgr->PushMenu(targetMenu)
void hdItemGoto::SelectItem(MenuMgr* mgr) {
    MARKFUNCTION(0x8005DDF8);
    mgr->PushMenu(gotoMenu);
}

// ---- hdItemButton ----

// PSX: _12hdItemButtonP9xcTextObjPc (0x8005E118)
hdItemButton::hdItemButton(u8* text, const char* name) {
    MARKFUNCTION(0x8005E118);
    data = text;
    textObj = text;
    itemID = xcHash(name);
}

// PSX: SelectItem__12hdItemButtonP7MenuMgr (0x8005E174)
// Calls callback, stores return value as menu state
void hdItemButton::SelectItem(MenuMgr* mgr) {
    MARKFUNCTION(0x8005E174);
    if (callback) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        s32 result = cb(this);
        mgr->state = result;
    }
}

// ---- hdItemSelection ----

// PSX: _15hdItemSelectionP9xcOverlayPc (0x8005D778)
hdItemSelection::hdItemSelection(xcOverlayData* overlay, const char* name, u8* rawData) {
    MARKFUNCTION(0x8005D778);
    if (overlay) {
        labelTextObj = overlay->GetTextObj(HASH_LABEL_TEXT, rawData);
        valueTextObj = overlay->GetTextObj(HASH_VALUE_TEXT, rawData);
    }
    data = labelTextObj;
    itemID = xcHash(name);
    enabled = 1;
}

// PSX: SetColour__15hdItemSelectionR12xcColour1555b (0x8005D990)
void hdItemSelection::SetColour(xcColour1555& col, bool flag) {
    MARKFUNCTION(0x8005D990);
    if (valueTextObj) {
        valueTextObj[40] = col.GetRed8();
        valueTextObj[41] = col.GetGreen8();
        valueTextObj[42] = col.GetBlue8();
        valueTextObj[43] = col.GetAlpha8();
    }
    hdMenuItem::SetColour(col, flag);
}

// PSX: IncItem__15hdItemSelection (0x8005D800)
void hdItemSelection::IncItem() {
    MARKFUNCTION(0x8005D800);
    if (!valueTextObj) return;
    // PSX: value textObj +45 = current frame, +44 = numFrames
    u8 curFrame = valueTextObj[45];
    u8 numFrames = valueTextObj[44];
    u8 newFrame = curFrame + 1;
    if (newFrame >= numFrames)
        newFrame = 0;
    valueTextObj[45] = newFrame;
    if (callback && enabled) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
}

// PSX: DecItem__15hdItemSelection (0x8005D8A8)
void hdItemSelection::DecItem() {
    MARKFUNCTION(0x8005D8A8);
    if (!valueTextObj) return;
    u8 curFrame = valueTextObj[45];
    u8 numFrames = valueTextObj[44];
    u8 newFrame;
    if (curFrame == 0)
        newFrame = numFrames - 1;
    else
        newFrame = curFrame - 1;
    valueTextObj[45] = newFrame;
    if (callback && enabled) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
}

// PSX: SetValue__15hdItemSelectionUl (0x8005D954)
void hdItemSelection::SetValue(u32 val) {
    MARKFUNCTION(0x8005D954);
    if (!valueTextObj) return;
    u8 numFrames = valueTextObj[44];
    if (val >= numFrames)
        val = 0;
    valueTextObj[45] = (u8)val;
}

// PSX: GetValue__15hdItemSelection (0x8005D97C)
u32 hdItemSelection::GetValue() {
    MARKFUNCTION(0x8005D97C);
    if (!valueTextObj) return 0;
    return valueTextObj[45];
}

// ---- hdShockSelection ----

// PSX: _16hdShockSelectionP9xcOverlayPc (0x8005E1CC)
hdShockSelection::hdShockSelection(xcOverlayData* overlay, const char* name, u8* rawData)
    : hdItemSelection(overlay, name, rawData) {
    MARKFUNCTION(0x8005E1CC);
}

// ---- hdSndItemSelection ----

// PSX: _18hdSndItemSelectionP9xcOverlayPcii (0x8005DA28)
hdSndItemSelection::hdSndItemSelection(xcOverlayData* overlay, const char* name, u8* rawData, s32 steps, u32 maxVal)
    : hdItemSelection(overlay, name, rawData) {
    MARKFUNCTION(0x8005DA28);
    numSteps = (s16)steps;
    maxValue = maxVal;
    if (numSteps > 0)
        stepSize = maxVal / numSteps;
    else
        stepSize = 1;
    currentStep = 0;
    currentValue = 0;

    if (valueTextObj && valueTextObj[44] > 0) {
        u8 numStrings = valueTextObj[44];
        u32* strings = reinterpret_cast<u32*>(valueTextObj + 56);
        for (u8 i = 0; i < numStrings; i++) {
            strings[i] = xcRegisterRuntimeString(displayStr);
        }
    }

    UpdateShown();
}

// PSX: IncItem__18hdSndItemSelection (0x8005DC14)
void hdSndItemSelection::IncItem() {
    MARKFUNCTION(0x8005DC14);
    u32 oldVal = currentValue;
    if (currentStep < numSteps)
        currentStep++;
    if (currentStep == numSteps)
        currentValue = maxValue;
    else
        currentValue = currentStep * stepSize;
    UpdateShown();
    if (callback) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
}

// PSX: DecItem__18hdSndItemSelection (0x8005DABC)
void hdSndItemSelection::DecItem() {
    MARKFUNCTION(0x8005DABC);
    u32 oldVal = currentValue;
    if (currentStep > 0)
        currentStep--;
    currentValue = currentStep * stepSize;
    UpdateShown();
    if (callback) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
}

// PSX: SetValue__18hdSndItemSelectionUl (0x8005DBD4)
void hdSndItemSelection::SetValue(u32 val) {
    MARKFUNCTION(0x8005DBD4);
    currentValue = val;
    currentStep = (s16)(val / stepSize);
    UpdateShown();
}

// PSX: GetValue__18hdSndItemSelection (0x8005DB54)
u32 hdSndItemSelection::GetValue() {
    MARKFUNCTION(0x8005DB54);
    return currentValue;
}

// PSX: UpdateShown__18hdSndItemSelection (0x8005DB60)
void hdSndItemSelection::UpdateShown() {
    MARKFUNCTION(0x8005DB60);
    s32 i = 0;
    while (i < currentStep && i < numSteps) {
        displayStr[i] = 'o';  // filled segment
        i++;
    }
    while (i < numSteps) {
        displayStr[i] = 'f';  // empty segment
        i++;
    }
    if (i < (s32)sizeof(displayStr))
        displayStr[i] = '\0';
}

// ---- hdControllerSelection ----

// PSX: _21hdControllerSelectionP9xcOverlayPcT1 (0x80013148)
hdControllerSelection::hdControllerSelection(xcOverlayData* overlay, const char* name, u8* rawData, xcOverlayData* descOvl)
    : hdItemSelection(overlay, name, rawData) {
    MARKFUNCTION(0x80013148);
    descOverlay = descOvl;
}

// PSX: IncItem__21hdControllerSelection (0x80013178)
void hdControllerSelection::IncItem() {
    MARKFUNCTION(0x80013178);
    hdItemSelection::IncItem();
    // PSX: SetControlDescription - updates control layout display
}

// PSX: DecItem__21hdControllerSelection (0x800131A4)
void hdControllerSelection::DecItem() {
    MARKFUNCTION(0x800131A4);
    hdItemSelection::DecItem();
    // PSX: SetControlDescription
}

// ---- hdAlphaSelection ----

hdAlphaSelection::hdAlphaSelection(xcOverlayData* overlay, const char* name, u8* rawData, s32 minVal, s32 maxVal) {
    MARKFUNCTION(0x8005D728);
    if (overlay)
        labelTextObj = overlay->GetTextObj(HASH_LABEL_TEXT, rawData);
    itemID = xcHash(name);
    minValue = minVal;
    maxValue = maxVal;
    currentValue = minVal;
    displayStr[0] = (char)('a' + currentValue);
    displayStr[1] = '\0';
}

void hdAlphaSelection::IncItem() {
    s32 oldVal = currentValue;
    currentValue++;
    if (currentValue > maxValue)
        currentValue = minValue;
    displayStr[0] = (char)('a' + currentValue);
    displayStr[1] = '\0';
    if (callback) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
}

void hdAlphaSelection::DecItem() {
    s32 oldVal = currentValue;
    currentValue--;
    if (currentValue < minValue)
        currentValue = maxValue;
    displayStr[0] = (char)('a' + currentValue);
    displayStr[1] = '\0';
    if (callback) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
}

void hdAlphaSelection::SetValue(u32 val) {
    if ((s32)val >= minValue && (s32)val <= maxValue) {
        currentValue = (s32)val;
        displayStr[0] = (char)('a' + currentValue);
        displayStr[1] = '\0';
    }
}

u32 hdAlphaSelection::GetValue() {
    return (u32)currentValue;
}

// ---- hdDynItemSelection ----

// PSX: _18hdDynItemSelectionP9xcOverlayPc (0x8005DCD0)
hdDynItemSelection::hdDynItemSelection(xcOverlayData* overlay, const char* name, u8* rawData)
    : hdItemSelection(overlay, name, rawData) {
    MARKFUNCTION(0x8005DCD0);
    dynOverlay = overlay;
}

// ---- hdNumericSelection ----

// PSX: _18hdNumericSelectionP9xcOverlayPcii (0x8005D49C)
hdNumericSelection::hdNumericSelection(xcOverlayData* overlay, const char* name, u8* rawData, s32 minVal, s32 maxVal) {
    MARKFUNCTION(0x8005D49C);
    if (overlay)
        labelTextObj = overlay->GetTextObj(HASH_LABEL_TEXT, rawData);
    itemID = xcHash(name);
    minValue = minVal;
    maxValue = maxVal;
    currentValue = minVal;
    ChangeValueText();
}

// PSX: IncItem__18hdNumericSelection (0x8005D598)
void hdNumericSelection::IncItem() {
    MARKFUNCTION(0x8005D598);
    s32 oldVal = currentValue;
    s32 newVal = currentValue + 1;
    if (currentValue >= maxValue)
        newVal = minValue;
    currentValue = newVal;
    if (callback) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
    ChangeValueText();
}

// PSX: DecItem__18hdNumericSelection (0x8005D634)
void hdNumericSelection::DecItem() {
    MARKFUNCTION(0x8005D634);
    s32 oldVal = currentValue;
    s32 newVal = currentValue - 1;
    if (minValue >= currentValue)
        newVal = maxValue;
    currentValue = newVal;
    if (callback) {
        hdMenuItemCallback cb = (hdMenuItemCallback)callback;
        cb(this);
    }
    ChangeValueText();
}

// PSX: SetValue__18hdNumericSelectionUl (0x8005D6D0)
void hdNumericSelection::SetValue(u32 val) {
    MARKFUNCTION(0x8005D6D0);
    if ((s32)val >= minValue && maxValue >= (s32)val) {
        currentValue = (s32)val;
        ChangeValueText();
    }
}

// PSX: GetValue__18hdNumericSelection (0x8005D55C)
u32 hdNumericSelection::GetValue() {
    MARKFUNCTION(0x8005D55C);
    return (u32)currentValue;
}

// PSX: ChangeValueText__18hdNumericSelection (0x8005D568)
void hdNumericSelection::ChangeValueText() {
    MARKFUNCTION(0x8005D568);
    snprintf(displayStr, sizeof(displayStr), "%d", currentValue);
}

// ---- hdDynMenu ----

// PSX: _9hdDynMenuP7MenuMgrP9xcOverlayi (0x8005DF40)
hdDynMenu::hdDynMenu(MenuMgr* mgr, xcOverlayData* ovl, s32 maxCount, u8* rawData) {
    MARKFUNCTION(0x8005DF40);
    overlay = ovl;
    menuMgr = mgr;
    field36 = 0;
    maxItems = maxCount;
    titleStr = nullptr;
    if (ovl) {
        titleTextObj = ovl->GetTextObj(2912417, rawData);
    }
}

// PSX: DynSetup__9hdDynMenu (0x8005DFC0)
void hdDynMenu::DynSetup() {
    MARKFUNCTION(0x8005DFC0);
    hdMenu::DynSetup();
}

// ---- hdMemCardMenu ----

// PSX: _13hdMemCardMenuP7MenuMgrP9xcOverlayT2 (0x80011BE0)
hdMemCardMenu::hdMemCardMenu(MenuMgr* mgr, xcOverlayData* ovl1, xcOverlayData* ovl2, u8* rawData) {
    MARKFUNCTION(0x80011BE0);
    menuMgr = mgr;
    overlay = ovl1;
    if (ovl2) {
        textObj2 = ovl2->GetTextObj(HASH_LABEL_TEXT, rawData);
        textObj1 = ovl2->GetTextObj(98451852, rawData);
    }
    field60 = 1;
}

hdMemCardMenu::~hdMemCardMenu() {
    MARKFUNCTION(0x80011C38);
}

// ---- hdDynItemMenu ----

// PSX: _13hdDynItemMenui (0x8005D2E0)
hdDynItemMenu::hdDynItemMenu(s32 count) {
    MARKFUNCTION(0x8005D2E0);
    field36 = 0;
    visibleCount = count;
}

// PSX: DynSetup__13hdDynItemMenu (0x8005D340)
void hdDynItemMenu::DynSetup() {
    MARKFUNCTION(0x8005D340);
    hdMenu::DynSetup();
}

// PSX: InputNextItem__13hdDynItemMenu (0x8005D3B4)
void hdDynItemMenu::InputNextItem() {
    MARKFUNCTION(0x8005D3B4);
    // PSX: walks item list forward, skipping invisible items
    hdMenu::InputNextItem();
}

// PSX: InputPrevItem__13hdDynItemMenu (0x8005D428)
void hdDynItemMenu::InputPrevItem() {
    MARKFUNCTION(0x8005D428);
    hdMenu::InputPrevItem();
}
