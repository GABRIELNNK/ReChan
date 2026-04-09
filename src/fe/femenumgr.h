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

    feMenuMgr();
    ~feMenuMgr() override;

    void SelfInit() override;
    void GotoStartScreen() override;
    void Deactivate() override;
    void HandleInputChange() override;
    void QueryInput(bool processInput) override;
    void InputPadUp() override;
    void InputPadDown() override;
    void InputPadLeft() override;
    void InputPadRight() override;
    void InputItemPush() override;
    void PushMenu(hdMenu* menu) override;
    void PopMenu() override;
    void ShowNewGameMenu();
    void ShowLevel(FrontEndVolume* vol, Humanoid* hum);
    void InitLevelMenu();
    void OpenDoors();
    void PushLoadSaveMenu(s32 mode);

    s32 LevelValid(s32 levelID, s32 subLevel);
};

// Global feMenuMgr pointer
extern feMenuMgr* g_feMenuMgr;
