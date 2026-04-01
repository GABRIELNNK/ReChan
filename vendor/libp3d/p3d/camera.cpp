// camera.cpp — tCamera implementation
#include "p3d/camera.h"
#include "p3d/context.h"
#include "pddi/pddidev.h"

tCamera::tCamera()
    : fov(0.7f)
    , aspect(4.0f / 3.0f)
    , nearPlane(10.0f)
    , farPlane(100000.0f) {
}

tCamera::~tCamera() = default;

void tCamera::SetFOV(f32 fovY, f32 a) {
    fov = fovY;
    aspect = a;
}

void tCamera::SetCameraMatrix(const Mat4& wtc) {
    worldToCamera = wtc;
}

void tCamera::SetState() {
    // Use actual viewport aspect ratio so projection always matches the window
    f32 a = aspect;
    if (p3d::display) {
        int w = p3d::display->GetWidth();
        int h = p3d::display->GetHeight();
        if (h > 0) a = static_cast<f32>(w) / static_cast<f32>(h);
    }
    Mat4 proj = Perspective(fov, a, nearPlane, farPlane);
    // PSX is left-handed (+Z forward), GL is right-handed (+Z backward).
    // Negate X in projection to convert handedness without touching vertex data.
    proj.m[0] = -proj.m[0];
    p3d::context->SetProjectionMatrix(proj);
    p3d::context->SetViewMatrix(worldToCamera);
}
