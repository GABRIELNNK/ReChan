#include "fe/gameoverscreen.h"
#include "xclib/xclib.h"

GameOverScreen::GameOverScreen() {
    MARKFUNCTION(0x80011A28);
    MenuColorStart(menuColor);
}

GameOverScreen::~GameOverScreen() {
    // base class handles cleanup
}

// PSX: SelfInit__14GameOverScreen (Overlay4 0x80011AE0, FEMNUMGR.CPP:770)
// Finds overlay, stores text object ptr, adjusts sprite Y.
void GameOverScreen::SelfInit() {
    MARKFUNCTION(0x80011AE0);
    if (!section) return;

    xcOverlayData* ovl = FindOverlay((u32)1052841317);
    if (!ovl) return;

    // Get text prim for game over text
    u8* textRaw = ovl->GetTextObj((u32)(-956583547 & 0xFFFFFFFF), section->rawData);
    if (textRaw) {
        continueText = reinterpret_cast<xcTextPrim*>(textRaw);
    }

    // Adjust sprite Y position up by 8 pixels
    u8* spriteRaw = ovl->GetSprite((u32)(-1875786850 & 0xFFFFFFFF), section->rawData);
    if (spriteRaw) {
        auto* sprite = reinterpret_cast<xcSpritePrim*>(spriteRaw);
        sprite->mtx.SetY(sprite->mtx.GetY() - 8);
    }
}

// PSX: SelfUpdate__14GameOverScreen (Overlay4 0x80011A64, FEMNUMGR.CPP:763)
// Same as TitleScreen::SelfUpdate - cycles color on text prim RGBA.
void GameOverScreen::SelfUpdate() {
    MARKFUNCTION(0x80011A64);

    MenuColorNext(menuColor);

    if (continueText) {
        continueText->colorR = menuColor.GetRed8();
        continueText->colorG = menuColor.GetGreen8();
        continueText->colorB = menuColor.GetBlue8();
        continueText->colorA = menuColor.GetAlpha8();
    }
}

// PSX: GetScreenNames__14GameOverScreen (Overlay4 0x80011B54, FEMNUMGR.CPP:782)
const char** GameOverScreen::GetScreenNames() {
    MARKFUNCTION(0x80011B54);
    return nullptr;
}
