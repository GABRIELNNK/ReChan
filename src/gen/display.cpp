#include "gen/display.h"
#include "gen/camera.h"
#include "p3d/context.h"
#include "p3d/camera.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"

Display* g_display = nullptr;

// PSX: __7Display (0x800C9A30) / _7Display (overlay, 0x8012BAD0)
Display::Display() {
    MARKFUNCTION(0x800C9A30);
    g_display = this;
    pri = 60; // PSX: byte at offset 18 = 60
}

// PSX: __7Display (DISPLAY.CPP:67, 0x800DB408)
Display::~Display() {
    MARKFUNCTION(0x800DB408);
    InternalClose();
    g_display = nullptr;
}

// PSX: InternalOpen__7Display (overlay, 0x8012BAE4)
// Calls platOpen which creates the camera, sets view layers, viewport, lighting.
void Display::InternalOpen() {
    MARKFUNCTION(0x8012BAE4);

    // PSX: platOpen__7Display (overlay, 0x8012BCC0)
    // PSX: _builtin_new(492) -> _6CameraPC10tagLVector({0, 800, 6553})
    // PSX: theCamera->vtable+12() (Reset)
    // PSX: SetCamera(view0, theCamera+404)   [+404 = embedded tMatrixCamera]
    theCamera = new Camera();
    theCamera->SetPosition(0, 800, 6553); // PSX init position {0, 0x320, 0x1999}
    theCamera->Reset();

    tMatrixCamera* p3dCam = theCamera->GetP3DCamera();
    p3dCam->SetNearPlane(100.0f);
    p3dCam->SetFarPlane(500000.0f);
    view.SetCamera(p3dCam);
    view.SetBackgroundColour(pddiColour(30, 30, 35));
    view.SetClearMask(PDDI_BUFFER_ALL);

    // PSX: SetupLayer calls for 7 view layers, SetViewPort, SetLighting
    // PC: tView doesn't have layer system, viewport is full-screen by default.

    frameCounter = 0;
}

// PSX: InternalClose__7Display (DISPLAY.CPP:76, 0x800DB418)
void Display::InternalClose() {
    MARKFUNCTION(0x800DB418);
    // PSX: platClose -> theCamera = 0
    // PSX doesn't free the camera here (memory allocator handles it).
    // PC: we own it, so delete it.
    view.SetCamera(nullptr);
    if (theCamera) {
        delete theCamera;
        theCamera = nullptr;
    }
}

// PSX: InternalReset__7Display (DISPLAY.CPP:83, 0x800DB428)
void Display::InternalReset() {
    MARKFUNCTION(0x800DB428);
    // PSX: platReset - empty
}

// PSX: BeginFrame__7Display (PSXDISP.CPP:75, 0x80026CA0)
void Display::BeginFrame() {
    MARKFUNCTION(0x80026CA0);
    // PSX: P3D::BeginFrame() + save prim ptr + tView::BeginRender(view0)
    p3d::context->BeginFrame();
    p3d::context->Clear(PDDI_BUFFER_ALL);
    view.BeginRender();
}

// PSX: EndFrame__7Display (PSXDISP.CPP:86, 0x80026CB4)
void Display::EndFrame() {
    MARKFUNCTION(0x80026CB4);
    // PSX: tView::EndRender + ++_MyFrameCounter + P3D::EndFrame(1) -> SwapBuffers
    view.EndRender();
    ++frameCounter;
    p3d::context->EndFrame();
    p3d::display->RenderOverlay();
    p3d::display->SwapBuffers();
}

// PSX: dispBeginFrameHandler (DISPLAY.CPP:105, 0x800DB43C)
void Display::dispBeginFrameHandler(Handler*) {
    if (g_display) {
        g_display->BeginFrame();
    }
}

// PSX: dispEndFrameHandler (DISPLAY.CPP:112, 0x800DB448)
void Display::dispEndFrameHandler(Handler*) {
    if (g_display) {
        g_display->EndFrame();
    }
}
