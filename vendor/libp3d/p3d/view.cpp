// view.cpp — tView implementation
#include "p3d/view.h"
#include "p3d/camera.h"
#include "p3d/context.h"
#include "pddi/pddidev.h"

tView::tView()
    : l(0.0f), t(0.0f), r(1.0f), b(1.0f)
    , camera(nullptr)
    , clearColour(30, 30, 35)
    , ambientLight(128, 128, 128)
    , clearMask(PDDI_BUFFER_ALL)
    , fogEnabled(false)
    , fogColour(128, 128, 128)
    , fogNear(0.0f), fogFar(1000.0f) {
}

tView::~tView() {
    if (camera)
        camera->Release();
}

void tView::SetWindow(f32 left, f32 top, f32 right, f32 bottom) {
    l = left;
    t = top;
    r = right;
    b = bottom;
}

void tView::SetCamera(tCamera* cam) {
    if (cam) cam->AddRef();
    if (camera) camera->Release();
    camera = cam;
}

void tView::BeginRender() {
    // Clear buffers
    if (clearMask) {
        p3d::context->SetClearColour(clearColour);
        p3d::context->Clear(clearMask);
    }

    // Set camera state (projection + view matrices)
    if (camera)
        camera->SetState();
}

void tView::EndRender() {
    // Currently nothing needed on the OpenGL path
}
