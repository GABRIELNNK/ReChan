// camera.h - Pure3D v11.3 camera class hierarchy
// PSX: TCAMERA.CPP, TMATRIXCAM.CPP, T2POINTCAM.CPP
#pragma once

#include "p3d/entity.h"
#include "p3d/matrix.h"
#include "p3d/lvector.h"
#include "p3d/vector.h"
#include "core.h"

// tCamera - base camera (TCAMERA.CPP)
// PSX layout: +0 uid, +8 vtable, +12 near(u16), +14 far(u16), +16 fovA(s32), +20 fovB(s32)
// Size: 24 bytes on PSX
class tCamera : public tEntity {
public:
    tCamera();
    virtual ~tCamera();

    // PSX: SetFOV__7tCamerall (0x8008ECC0) - stores two s32 FOV values
    // fovA = horizontal half-angle tangent in Q16, fovB = vertical
    void SetFOV(s32 a, s32 b);

    // PSX: GetFOV__7tCameraPlT1 (0x8008ECCC)
    void GetFOV(s32* outA, s32* outB) const;

    // PSX: GetClipPlanes__7tCameraPUsT1 (0x8008ECE4)
    void GetClipPlanes(u16* outNear, u16* outFar) const;

    // PC bridge: clip planes as float for GL projection
    void SetNearPlane(f32 n) { nearPlaneF = n; }
    void SetFarPlane(f32 f) { farPlaneF = f; }
    f32 GetNearPlane() const { return nearPlaneF; }
    f32 GetFarPlane() const { return farPlaneF; }

    // PC bridge: the game must call this before each frame to provide the
    // projection matrix. SetState() pushes it to the render context.
    void SetProjectionMatrix(const Mat4& proj) { projection = proj; hasProjection = true; }
    bool HasProjectionMatrix() const { return hasProjection; }

    // PC bridge: push projection + view matrices to pddi context
    virtual void SetState();

protected:
    u16 nearClip;  // PSX default 1
    u16 farClip;   // PSX default 0xFFFF
    s32 fovA;      // PSX default 0x3333 (13107)
    s32 fovB;      // PSX default 0x2666 (9830)
    f32 nearPlaneF; // PC: GL near plane (default from nearClip)
    f32 farPlaneF;  // PC: GL far plane (default from farClip)
    Mat4 projection;    // PC: game-provided projection matrix
    bool hasProjection = false;
};

// tMatrixCamera - camera with explicit matrix (TMATRIXCAM.CPP)
// PSX layout: tCamera(24) + MATRIX(32) at +24
// Size: 56 bytes on PSX
class tMatrixCamera : public tCamera {
public:
    tMatrixCamera(const Mat4* initialMatrix = nullptr);
    virtual ~tMatrixCamera();

    // PSX: SetCameraMatrix__13tMatrixCameraP6MATRIX (0x8009D670)
    // Stores camera-to-world matrix. PC inverts for GL internally.
    void SetCameraMatrix(const Mat4& ctw);

    // PSX: GetCameraMatrix__13tMatrixCamera (0x8009D698)
    const Mat4& GetCameraMatrix() const { return cameraMatrix; }

    // PSX: UpdateMatrix__13tMatrixCamera (0x8009D6A0)
    // PSX copies matrix to global CVM. PC: pre-computes worldToCamera for GL.
    virtual void UpdateMatrix();

    // PSX: GetPosition__13tMatrixCameraP9_RMVECT16 (0x8009D6C8)
    // Extracts translation from the camera matrix.
    void GetPosition(LVector* out) const;

    // PC accessor for the pre-computed world-to-camera matrix (GL use)
    const Mat4& GetWorldToCameraMatrix() const { return worldToCamera; }

    // Override SetState to push both projection and view
    void SetState() override;

protected:
    Mat4 cameraMatrix;     // camera-to-world (PSX equivalent of MATRIX at +24)
    Mat4 worldToCamera;    // pre-computed inverse for GL
};

// t2PointMatrixCamera - 2-point camera (T2POINTCAM.CPP)
// PSX layout: tMatrixCamera(56) + position(12) + target(12) + twist(2)
// Size: ~84 bytes on PSX
class t2PointMatrixCamera : public tMatrixCamera {
public:
    t2PointMatrixCamera(const Mat4* initialMatrix = nullptr);
    virtual ~t2PointMatrixCamera();

    // PSX: SetPosition (0x8009D480)
    void SetPosition(const LVector* pos);

    // PSX: SetTarget (0x8009D460)
    void SetTarget(const LVector* tgt);

    // PSX: SetTwist (0x8009D4A0)
    void SetTwist(u16 t);

    // PSX: GetPosition (0x8009D4C8) - returns position field, not matrix
    void GetPosition(LVector* out) const;

    // PSX: GetTarget (0x8009D4A8)
    void GetTarget(LVector* out) const;

    // PSX: GetTwist (0x8009D4E8)
    u16 GetTwist() const;

    // PSX: UpdateMatrix (0x8009D4F4)
    // Builds heading matrix from position/target, applies twist, fills translation
    void UpdateMatrix() override;

private:
    LVector position;
    LVector target;
    u16 twist;
};

// Global 2-point camera instance (PSX: G_2ptcam at 0x800D3964)
extern t2PointMatrixCamera G_2ptcam;
