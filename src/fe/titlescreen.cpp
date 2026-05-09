#include "fe/titlescreen.h"
#include "gen/common.h"
#include "xclib/xclib.h"

// PSX: __11TitleScreen (Overlay4 0x800118F0, FEMNUMGR.CPP:726)
TitleScreen::TitleScreen() {
    MARKFUNCTION(0x800118F0);
    MenuColorStart(menuColor);
}

TitleScreen::~TitleScreen() {
    // base class handles cleanup
}

// PSX: SelfInit__11TitleScreen (Overlay4 0x800119A8, FEMNUMGR.CPP:738)
// Finds the "PRESS START" text overlay and a sprite, adjusts sprite Y position.
void TitleScreen::SelfInit() {
    MARKFUNCTION(0x800119A8);
    if (!section) return;

    xcOverlayData* ovl = FindOverlay((u32)102709646);
    if (!ovl) return;

    // Get the text prim for "PRESS START" color cycling
    u8* textRaw = ovl->GetTextObj((u32)140659118, section->rawData);
    if (textRaw) {
        pressStartText = reinterpret_cast<xcTextPrim*>(textRaw);
    }

    // Get a sprite and adjust its Y position up by 8 pixels
    u8* spriteRaw = ovl->GetSprite((u32)1547109930, section->rawData);
    if (spriteRaw) {
        auto* sprite = reinterpret_cast<xcSpritePrim*>(spriteRaw);
        sprite->mtx.SetY(sprite->mtx.GetY() - 8);
    }
}

// PSX: SelfUpdate__11TitleScreen (Overlay4 0x8001192C, FEMNUMGR.CPP:731)
// Cycles menu color and updates RGBA bytes on the "PRESS START" text prim.
void TitleScreen::SelfUpdate() {
    MARKFUNCTION(0x8001192C);

    MenuColorNext(menuColor);

    if (pressStartText) {
        pressStartText->colorR = menuColor.GetRed8();
        pressStartText->colorG = menuColor.GetGreen8();
        pressStartText->colorB = menuColor.GetBlue8();
        pressStartText->colorA = menuColor.GetAlpha8();
    }
}

// PSX: GetScreenNames__11TitleScreen (Overlay4 0x80011A1C, FEMNUMGR.CPP:750)
// Returns screen names array (stored in PSX GP-relative data).
// PSX decompiled: return gp + 3044 (pointer to static string array)
const char** TitleScreen::GetScreenNames() {
    MARKFUNCTION(0x80011A1C);
    // PSX: returns a static array of screen names for the title section
    // For TITLE.1, there's typically just one screen
    return nullptr;
}
