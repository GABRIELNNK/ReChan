#pragma once
#include "fe/oxovl.h"
#include "xclib/xccolour.h"

class oxScreenManager;
struct xcTextPrim;
struct xcPolyG4Prim;
struct xcSection;

// hdTtlive (12 bytes on PSX: HDITEM.CPP:295)
// Timed-visibility overlay: counts down a timer, hides when reaching zero.
// Inherits oxOvl.
// PSX layout: +0 oxOvl(8), +8 timer(s32)
class hdTtlive : public oxOvl {
public:
    s32 timer = -1;

    hdTtlive();
    ~hdTtlive() override;
    void Update();
    void SetTtlive(s32 frames);
};

// hdHealth (36 bytes on PSX: HDITEM.CPP:205)
// Health bar display with damage flash effect.
// Inherits hdTtlive.
class hdHealth : public hdTtlive {
public:
    xcPolyG4Prim* healthBar = nullptr;  // +12
    s16 barWidth = 0;                   // +16
    s32 maxHealth = 1;                  // +20
    xcTextPrim* textObj = nullptr;      // +24
    s32 flashAlpha = 0;                 // +28
    s32 lastValue = 0;                  // +32
    u8* rawData = nullptr;              // PC: rawData for prim lookup

    hdHealth();
    ~hdHealth() override;
    void SelfInit() override;
    void SetMax(s32 max);
    void SetValue(s32 value);
    void SetText(const char* text);
    void Update();
};

// hdTextOvl (28 bytes on PSX: HDITEM.CPP:403)
// Text overlay with inline number buffer.
// Inherits hdTtlive.
class hdTextOvl : public hdTtlive {
public:
    xcTextPrim* textPrim = nullptr;     // +12
    char numberBuf[12] = {};            // +16
    u8* rawData = nullptr;              // PC: rawData for prim lookup

    hdTextOvl();
    ~hdTextOvl() override;
    void SelfInit() override;
    void SetNumber(s32 num);
};

// hdAnimTextOvl (128 bytes on PSX: HDITEM.CPP:431)
// Animated text overlay with slide-in/out animation.
// Inherits hdTextOvl.
class hdAnimTextOvl : public hdTextOvl {
public:
    s32 stepX = 0;          // +32
    s32 stepY = 0;          // +36
    s32 animDuration = 0;   // +40
    s32 pauseDuration = 0;  // +44
    s32 currentFrame = 0;   // +48
    s32 isPlaying = 0;      // +52
    s32 pauseFlag = 0;      // +56
    s16 origPosX[16] = {};  // +60
    s16 origPosY[16] = {};  // +92
    s16 numPrims = 0;       // +124

    hdAnimTextOvl();
    ~hdAnimTextOvl() override;
    void SelfInit() override;
    void SetAnimInfo(s32 offsetX, s32 offsetY, s32 dur, s32 pause);
    void SetPos(s32 deltaX, s32 deltaY);
    void GoToMinPos();
    void Update();
    void Play();
    void Reset();
    void SetPauseState(s32 state, s32 arg);
};

// hdDragon (140 bytes on PSX: HDITEM.CPP:588)
// Dragon counter overlay with gold dragon font swap.
// Inherits hdAnimTextOvl.
class hdDragon : public hdAnimTextOvl {
public:
    s16 dragonCount = 0;            // +128
    xcTextPrim* goldTextObj = nullptr; // +132
    s16 goldDragonFlag = 0;         // +136

    hdDragon();
    ~hdDragon() override;
    void SelfInit() override;
    void SetNum(s32 num);
    void SetGoldDragons(s16 flag);
};

// hdHits (64 bytes on PSX: HDITEM.CPP:620)
// Hit counter display with color flash and shake effects.
// Not an oxOvl subclass.
class hdHits {
public:
    xcOverlayData* overlay = nullptr;   // +0
    xcTextPrim* hitsTextObj = nullptr;  // +4
    xcTextPrim* hitsTextObj2 = nullptr; // +8
    u32 fontHashLo = 0;     // +12
    u32 fontHashMid = 0;    // +16
    u32 fontHashXl = 0;     // +20
    s32 hitCount = 0;   // +24
    char hitsBuf[16] = {};  // +28
    s16 origPosX = 0;   // +44
    s16 origPosY = 0;   // +48
    s16 colorR = 255;   // +52
    s16 colorG = 255;   // +54
    s16 colorB = 255;   // +56
    u8* rawData = nullptr;  // PC: rawData for prim lookup

    hdHits();
    void Init(oxScreenManager* scrmgr);
    void Update();
    void IncrementHits();
    void TriggerUpdate();
    void SetVisible(s32 vis);
};

// hdTally (128 bytes on PSX: HDITEM.CPP:759)
// End-of-level score tally screen with state machine.
class hdTally {
public:
    s32 state = 0;                  // +0
    oxOvl fightOvl;                 // +4
    oxOvl comboOvl;                 // +12
    char fightScoreBuf[7] = {};     // +20
    char comboScoreBuf[7] = {};     // +27
    char styleScoreBuf[7] = {};     // +34
    oxOvl gradeOvl;                 // +44
    oxOvl rdragonOvl;               // +52
    char rdragonBuf[12] = {};       // +60
    oxOvl gdragonOvl;               // +64
    char gdragonBuf[12] = {};       // +72
    oxOvl rdragonBonusOvl;          // +76
    oxOvl movieBonusOvl;            // +84
    oxOvl doneOvl;                  // +92
    hdAnimTextOvl* takesOvlPtr = nullptr; // +100
    s32 currentScore = 0;           // +104
    s32 targetScore = 0;            // +108
    s32 delayCounter = 0;           // +112
    s32 frameCounter = 0;           // +116
    xcTextPrim* doneTextPrim = nullptr; // +120
    xcColour1555 doneColor;         // +124
    u8* rawData = nullptr;          // PC: rawData for prim lookup

    hdTally();
    void Init(oxScreenManager* scrmgr);
    void Show(s32 vis);
    void Start(s32 tally);
    void Update();
    void DoDoneStuff();
};

// hdDestSelect (24 bytes on PSX: HDITEM.CPP:308)
// Destination selection HUD overlay shown when returning to hub.
class hdDestSelect {
public:
    xcOverlayData* titleOvl1 = nullptr;
    xcOverlayData* titleOvl2 = nullptr;
    hdTtlive ttlive;
    s32 currentLevel = 0;
    u8* rawData = nullptr;
    xcSection* section = nullptr;

    hdDestSelect();
    void Init(oxScreenManager* scrmgr);
    void Start(s32 totalGoldDragon);
    void Hide();
    void ShowLevel(s32 level);
    void Update();
};
