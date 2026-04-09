#pragma once
#include "gen/manager.h"
#include "gen/handler.h"
#include "p3d/view.h"

class Camera;

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

    tView& GetView() { return view; }
    Camera* GetCamera() { return theCamera; }

    s16 GetFrameCounter() const { return frameCounter; }

    // Handler callbacks registered at pri=62 and pri=-62
    static void dispBeginFrameHandler(Handler* h);
    static void dispEndFrameHandler(Handler* h);

    s32 GetScreenWidth() const { return screenWidth; }
    s32 GetScreenHeight() const { return screenHeight; }
    f32 GetAspectRatio() const { return aspectRatio; }

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
};

// PSX: theDisplay (gp+3556)
extern Display* g_display;
