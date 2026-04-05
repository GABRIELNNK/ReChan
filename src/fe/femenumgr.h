// femenumgr.h - feMenuMgr reversed from PSX FEMENUMGR.CPP (Overlay 4)
// PSX source: C:\CHAN\GAME\SRC\FE\FEMNUMGR.CPP
// feMenuMgr is the main frontend menu system - title, options, level select,
// save/load, credits. Lives in PSX Overlay 4 (front-end overlay).
#pragma once

#include "fe/menumgr.h"

class Game;
struct xcSectionMan;

// Forward declarations for systems feMenuMgr interacts with
class FrontEndVolume;
class Humanoid;

// feMenuMgr (100 bytes on PSX) - inherits MenuMgr (80)
// PSX overlay: Overlay 4 (OL2_REL.BIN)
// PSX layout:
//   +0:  MenuMgr base (80 bytes)
//   +80: startScreenHashes[2] (u32[2]) - [0]=main, [1]=level, indexed by feMode
//   +88: feMode (s16) - 0=main menu, 1=level select
//   +90: padding (s16)
//   +92: frontEndVolume (FrontEndVolume*)
//   +96: humanoid (Humanoid*)
class feMenuMgr : public MenuMgr {
public:
    u32 startScreenHashes[2] = {};  // +80: screen hashes indexed by feMode
    s16 feMode = 0;                 // +88: 0=normal menu, 1=level select
    s16 fePad = 0;                  // +90: padding
    FrontEndVolume* frontEndVolume = nullptr;  // +92
    Humanoid* humanoid = nullptr;   // +96

    // PSX: __9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010EB4)
    feMenuMgr();

    // PSX: _._9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010F04)
    ~feMenuMgr() override;

    // --- oxScreenManager overrides ---

    // PSX: SelfInit__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x8001103C)
    void SelfInit() override;

    // PSX: GotoStartScreen__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x800115BC)
    void GotoStartScreen() override;

    // --- MenuMgr overrides ---

    // PSX: Deactivate__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80011540)
    void Deactivate() override;

    // PSX: HandleInputChange__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010F2C)
    void HandleInputChange() override;

    // PSX: QueryInput__9feMenuMgrb (FEMNUMGR.CPP, Overlay4 0x80011774)
    void QueryInput(bool processInput) override;

    // PSX: InputPadUp__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010B40)
    void InputPadUp() override;

    // PSX: InputPadDown__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010BC0)
    void InputPadDown() override;

    // PSX: InputPadLeft__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010C40)
    void InputPadLeft() override;

    // PSX: InputPadRight__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010CA4)
    void InputPadRight() override;

    // PSX: InputItemPush__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010A7C)
    void InputItemPush() override;

    // PSX: PushMenu__9feMenuMgrP6hdMenu (FEMNUMGR.CPP, Overlay4 0x80010D08)
    void PushMenu(hdMenu* menu) override;

    // PSX: PopMenu__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80010E30)
    void PopMenu() override;

    // --- feMenuMgr-specific functions ---

    // PSX: ShowNewGameMenu__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x800115F0)
    void ShowNewGameMenu();

    // PSX: ShowLevel__9feMenuMgrP14FrontEndVolumeP8Humanoid (Overlay4 0x80011218)
    void ShowLevel(FrontEndVolume* vol, Humanoid* hum);

    // PSX: InitLevelMenu__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80011260)
    void InitLevelMenu();

    // PSX: OpenDoors__9feMenuMgr (FEMNUMGR.CPP, Overlay4 0x80011680)
    void OpenDoors();

    // PSX: PushLoadSaveMenu__9feMenuMgri (FEMNUMGR.CPP, Overlay4 0x80011618)
    void PushLoadSaveMenu(s32 mode);

    // PSX: LevelValid__9feMenuMgril (FEMNUMGR.CPP, Overlay4 0x80011188)
    s32 LevelValid(s32 levelID, s32 subLevel);
};

// Global feMenuMgr pointer
extern feMenuMgr* g_feMenuMgr;
