// camera.h = tCamera base class (Pure3D v11.3)
#pragma once

#include "p3d/entity.h"
#include "core.h"

class tCamera : public tEntity {
public:
    tCamera();
    virtual ~tCamera();

    // FOV (vertical, radians) and aspect ratio (width/height)
    void SetFOV(f32 fovY, f32 a);
    f32 GetFOV() const { return fov; }
    f32 GetAspect() const { return aspect; }

    // Clip planes
    void SetNearPlane(f32 n) { nearPlane = n; }
    void SetFarPlane(f32 f) { farPlane = f; }
    f32 GetNearPlane() const { return nearPlane; }
    f32 GetFarPlane() const { return farPlane; }

    // Set the world-to-camera transform directly
    virtual void SetCameraMatrix(const Mat4& wtc);
    const Mat4& GetWorldToCameraMatrix() const { return worldToCamera; }

    // Push projection + view matrices to the pddi context
    virtual void SetState();

protected:
    f32 fov;        // vertical FOV in radians
    f32 aspect;     // width / height
    f32 nearPlane;
    f32 farPlane;
    Mat4 worldToCamera;
};
