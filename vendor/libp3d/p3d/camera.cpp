// camera.cpp = tCamera implementation
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

// PSX: tMatrixCamera::SetCameraMatrix stores a camera-to-world matrix.
// The rendering pipeline inverts it for GTE use (world-to-camera).
// Same convention as SHAR's tCamera::SetCameraMatrix + Update.
void tCamera::SetCameraMatrix(const Mat4& ctw) {
    // Invert orthogonal camera-to-world to get world-to-camera for GL.
    // R_inv = R^T, T_inv = -R^T * T
    worldToCamera.m[0]  = ctw.m[0];
    worldToCamera.m[1]  = ctw.m[4];
    worldToCamera.m[2]  = ctw.m[8];
    worldToCamera.m[3]  = 0.0f;
    worldToCamera.m[4]  = ctw.m[1];
    worldToCamera.m[5]  = ctw.m[5];
    worldToCamera.m[6]  = ctw.m[9];
    worldToCamera.m[7]  = 0.0f;
    worldToCamera.m[8]  = ctw.m[2];
    worldToCamera.m[9]  = ctw.m[6];
    worldToCamera.m[10] = ctw.m[10];
    worldToCamera.m[11] = 0.0f;
    f32 tx = ctw.m[12], ty = ctw.m[13], tz = ctw.m[14];
    worldToCamera.m[12] = -(worldToCamera.m[0]*tx + worldToCamera.m[4]*ty + worldToCamera.m[8]*tz);
    worldToCamera.m[13] = -(worldToCamera.m[1]*tx + worldToCamera.m[5]*ty + worldToCamera.m[9]*tz);
    worldToCamera.m[14] = -(worldToCamera.m[2]*tx + worldToCamera.m[6]*ty + worldToCamera.m[10]*tz);
    worldToCamera.m[15] = 1.0f;
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
