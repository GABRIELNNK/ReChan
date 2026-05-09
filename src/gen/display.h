#pragma once
#include "gen/manager.h"
#include "gen/handler.h"
#include "p3d/view.h"
#include "pddi/pddidev.h"

class Camera;

enum ScreenMode : s32 {
    ScreenMode_Fullscreen = 0,
    ScreenMode_Borderless = 1,
    ScreenMode_Windowed = 2,
};

struct ChanProjectionState {
    s32 centerX = 160;
    s32 centerY = 120;
    s32 width = 320;
    s32 height = 240;
    f32 projectionDistanceX = 800.0f;
    f32 projectionDistanceY = 800.0f;
    u16 nearClip = 1;
    u16 farClip = 0xFFFF;
};

// PSX: Display (32 bytes) extends Manager(28) + no extra fields on PSX
// On PC we store the global tView and camera pointer here.
class Display : public Manager {
public:
    Display();
    ~Display() override;

    void InternalOpen() override;
    void InternalClose() override;
    void InternalReset() override;

    void BeginFrame();
    void EndFrame();

    static s32 BuildUniqueVideoModes(pddiDisplay* display, pddiVideoMode* outModes, s32 maxModes);

    tView& GetView() { return view; }
    Camera* GetCamera() { return theCamera; }

    s16 GetFrameCounter() const { return frameCounter; }

    // Handler callbacks registered at pri=62 and pri=-62
    static void dispBeginFrameHandler(Handler* h);
    static void dispEndFrameHandler(Handler* h);

    s32 GetScreenWidth() const { return screenWidth; }
    s32 GetScreenHeight() const { return screenHeight; }
    f32 GetAspectRatio() const { return aspectRatio; }
    ChanProjectionState GetChanProjectionState() const;

    s32 GetFullscreen() const { return screenMode == ScreenMode_Fullscreen ? 1 : 0; }
    void SetFullscreen(s32 enabled);
    s32 GetScreenMode() const { return screenMode; }
    void SetScreenMode(s32 mode);

    s32 GetVsync() const { return vsync; }
    void SetVsync(s32 enabled);

    s32 GetMSAA() const { return msaa; }
    void SetMSAA(s32 samples);

    s32 GetResolutionIndex() const { return resolutionIndex; }
    s32 GetResolutionCount() const;
    bool GetResolutionMode(s32 index, pddiVideoMode& mode) const;
    void SetResolutionIndex(s32 index);
    void SetCursorCaptured(bool captured);
    void SetCursorVisible(bool visible);

    static s32 GetDefaultFullscreen() { return s_defaultScreenMode == ScreenMode_Fullscreen ? 1 : 0; }
    static void SetDefaultFullscreen(s32 enabled) { s_defaultScreenMode = enabled ? ScreenMode_Fullscreen : ScreenMode_Windowed; }
    static s32 GetDefaultScreenMode() { return s_defaultScreenMode; }
    static void SetDefaultScreenMode(s32 mode) {
        if (mode < (s32)ScreenMode_Fullscreen) mode = (s32)ScreenMode_Fullscreen;
        if (mode > (s32)ScreenMode_Windowed) mode = (s32)ScreenMode_Windowed;
        s_defaultScreenMode = mode;
    }
    static s32 GetDefaultVsync() { return s_defaultVsync; }
    static void SetDefaultVsync(s32 enabled) { s_defaultVsync = enabled ? 1 : 0; }
    static s32 GetDefaultMSAA() { return s_defaultMsaa; }
    static void SetDefaultMSAA(s32 samples);

private:
    // PSX: view0 - global static tView with 7 layers
    tView view;

    // PSX: theCamera (0x800DD734) - created in platOpen, owned by Display
    Camera* theCamera = nullptr;

    // PSX: _MyFrameCounter (0x800DD664) - incremented every EndFrame
    s16 frameCounter = 0;

    // PC:
    s32 screenWidth = 1280;
    s32 screenHeight = 720;
    f32 aspectRatio = 16.0f / 9.0f;

    s32 screenMode = ScreenMode_Windowed;
    s32 vsync = 1;
    s32 msaa = 0;
    s32 resolutionIndex = 0;

    static s32 s_defaultScreenMode;
    static s32 s_defaultVsync;
    static s32 s_defaultMsaa;
};

// PSX: theDisplay (gp+3556)
extern Display* g_display;
