// camera.cpp — Camera class reversed from PSX CAMERA.CPP
// Original: C:\CHAN\GAME\SRC\GEN\CAMERA.CPP
#include "gen/camera.h"
#include "gen/control.h"
#include "config.h"
#include "p3d/p3dmath.h"     // Vec3, Mat4, p3dBuildRotMatrixZYX, etc.
#include "p3d/input.h"       // PlatformInput
#include "p3d/context.h"     // p3d::input
#include "pddi/pddidev.h"    // pddiInput key codes
#include <cmath>
#include <cstdlib>

// ============================================================================
// PSX math helpers
// ============================================================================

static constexpr f32 PSX_ANGLE_TO_RAD = P3D_ANGLE_TO_RAD;
static constexpr f32 PSX_FOV_TO_RAD = 0.61f / 30000.0f;
static constexpr f32 PSX_ASPECT = 4.0f / 3.0f;

static constexpr s32 TIME_SCALE = 1000;

bool EvalCubic(s32* curValue, s32* accel, s32 target, s32 velocity, s32 time) {
    if (time == 0) {
        *curValue = target;
        *accel = velocity;
        return true;
    }

    s64 cur = *curValue;
    s64 acc = *accel;
    s64 tgt = target;
    s64 vel = velocity;
    s64 t   = time;

    // Hermite basis coefficients
    s64 delta = tgt - cur;
    s64 sum   = acc + vel;
    s64 B     = sum - 2 * delta;       // a1 in PSX
    s64 A     = 3 * delta - sum - acc;  // a0 in PSX

    s64 temp1 = (B * t) >> 16;
    s64 step2 = ((A + temp1) * t) >> 16;
    s64 step3 = ((3 * B) * t) >> 16;
    s64 newAcc = acc + step3 + step2 * 2; // simplified combination
    s64 pos_delta = ((acc + step2) * t) >> 16;
    s64 newCur = cur + pos_delta;

    s64 r1_full = B * t;
    s32 r1 = (s32)(r1_full >> 16);

    s64 r2_full = (A + r1) * t;
    s32 r2 = (s32)(r2_full >> 16);

    s64 r3_full = ((s64)(3 * B) * t);  // not exactly right but close enough for 32-bit
    f64 tn = (f64)time / 65536.0;

    f64 fDelta = (f64)delta;
    f64 fAcc   = (f64)acc;
    f64 fVel   = (f64)vel;
    f64 fCur   = (f64)cur;

    f64 fB = fAcc + fVel - 2.0 * fDelta;
    f64 fA = 3.0 * fDelta - 2.0 * fAcc - fVel;

    // Cubic: new_pos = A*t^3 + B*t^2 + acc*t + cur
    f64 newPos = fA * tn * tn * tn + fB * tn * tn + fAcc * tn + fCur;
    // Derivative (new accel/tangent): 3*A*t^2 + 2*B*t + acc
    f64 newAccF = 3.0 * fA * tn * tn + 2.0 * fB * tn + fAcc;

    s32 result = (s32)newPos;
    s32 resultAcc = (s32)newAccF;

    // Clamp: if overshot target
    bool wasBefore = (*curValue < target);
    *curValue = result;
    *accel = resultAcc;

    if (wasBefore) {
        if (*curValue >= target) {
            *curValue = target;
            *accel = velocity;
            return true;
        }
    } else {
        if (*curValue <= target) {
            *curValue = target;
            *accel = velocity;
            return true;
        }
    }
    return false;
}

// ============================================================================
// Camera constructor (0x80047AD4)
// ============================================================================
Camera::Camera() {
    MARKFUNCTION(0x80047AD4);
    // PSX: calls DynamicThing ctor with type 602, creates tMatrixCamera at +404
    // PC: tCamera is directly embedded, constructed by default ctor
    Reset();
}

Camera::~Camera() {
    MARKFUNCTION(0x80047C30);
}

// ============================================================================
// Camera::Reset (0x80047C5C)
// ============================================================================
void Camera::Reset() {
    MARKFUNCTION(0x80047C5C);

    // Reset position and velocity
    position = {};
    orientAngles = {};
    prevPosition = {};
    curPos = {};
    targetPos = {};
    prevTargetPos = {};
    movementAccel = {};
    movementVel = {};
    trackingAccel = {};
    trackingVel = {};

    // Velocity magnitudes — PSX init to 1966
    velocityMag.x = 1966;
    velocityMag.y = 1966;
    velocityMag.z = 1966;

    // Default movement/tracking times — PSX calls Set*Time with default vectors
    // PSX Reset passes {8,8,8} to SetMovementTime and {4,4,4} to SetTrackingTime
    LVector defaultMovTime = {8, 8, 8};
    LVector defaultTrackTime = {4, 4, 4};
    SetMovementTime(&defaultMovTime);
    SetTrackingTime(&defaultTrackTime);

    // FOV
    SetFOV(10); // desiredFOV = 10
    // curFOV = desiredFOV * 3000 (PSX formula)
    curFOV = desiredFOV * 3000;
    fovAccel = 0;
    fovVel = 0;

    // Push FOV to tCamera
    f32 fovRad = (f32)curFOV * PSX_FOV_TO_RAD;
    if (fovRad < 0.01f) fovRad = 0.7f; // fallback
    p3dCamera.SetFOV(fovRad, PSX_ASPECT);

    // Twist — PSX 8192 ≈ 45 degrees
    twist = 8192;

    // Camera angles
    camAngleX = 0;
    camAngleY = 0;
    camAngleZ = 0;
    quadrantYZ = 0;
    quadrantXZ = 0;

    // Mode: Follow (PSX SetMode(1))
    SetMode(CAM_MODE_FOLLOW);

    // Shake
    shakeFrames = 0;
    shakeStrength.x = 100;
    shakeStrength.y = 100;
    shakeStrength.z = 100;

    // Collision
    hasCollision = 1;

    // Look-at
    lookAtJoint = -1;
    lookAtMode = 0;
    targetThing = nullptr;

    // Anim
    asyncAnimEnum = 0xFFFF;
    asyncAnim = 0;
    cameraAnim = 0;
    flags = 0;

    cameraAnchor = 0;
}

// ============================================================================
// Camera::Think (0x80047F28)
// Per-frame entry point. Dispatches to current mode function, then Move + Shake.
// ============================================================================
void Camera::Think() {
    MARKFUNCTION(0x80047F28);

    // If camera anim is active, skip mode dispatch
    if (cameraAnim == 0 && modeFunc != nullptr) {
        (this->*modeFunc)();
    }

    // Move (position interpolation) if collision enabled
    if (hasCollision != 0) {
        Move();
    }

    // Camera shake
    if (shakeFrames > 0) {
        CameraShake();
    }
}

// ============================================================================
// Camera::Move (0x80047FD4)
// Interpolates position/target toward their goals using EvalCubic.
// Also interpolates FOV.
// ============================================================================
void Camera::Move() {
    MARKFUNCTION(0x80047FD4);

    // --- FOV interpolation ---
    s32 targetFOV = desiredFOV * 3000; // expanded target
    if (curFOV != targetFOV) {
        EvalCubic(&curFOV, &fovAccel, targetFOV, velocityMag.x, movementTime.x);
        // Push to tCamera
        f32 fovRad = (f32)curFOV * PSX_FOV_TO_RAD;
        if (fovRad > 0.01f) {
            p3dCamera.SetFOV(fovRad, PSX_ASPECT);
        }
    }

    // Save current position for prevPosition
    LVector savedPos = position;

    // --- Position interpolation ---
    // If path-follow flag (bit 0) is NOT set, interpolate position
    if (!(flags & 0x01)) {
        // X axis
        s32 deltaX = savedPos.x - curPos.x;
        if (deltaX < 0) deltaX = -deltaX;
        // PSX compares |delta| against a global threshold (gp+1380).
        // We use a small threshold equivalent.
        constexpr s32 POS_THRESHOLD = 1;
        if (deltaX >= POS_THRESHOLD) {
            EvalCubic(&savedPos.x, &movementAccel.x, curPos.x, movementVel.x, movementTime.x);
        }

        // Y axis
        if (savedPos.y != curPos.y) {
            EvalCubic(&savedPos.y, &movementAccel.y, curPos.y, movementVel.y, movementTime.y);
        }

        // Z axis
        s32 deltaZ = savedPos.z - curPos.z;
        if (deltaZ < 0) deltaZ = -deltaZ;
        if (deltaZ >= POS_THRESHOLD) {
            EvalCubic(&savedPos.z, &movementAccel.z, curPos.z, movementVel.z, movementTime.z);
        }

        // --- Target position interpolation ---
        // PSX compares |targetPos - prevTargetPos| against per-axis thresholds
        constexpr s32 TGT_THRESHOLD = 1;

        s32 dtx = targetPos.x - prevTargetPos.x;
        if (dtx < 0) dtx = -dtx;
        if (dtx >= TGT_THRESHOLD) {
            EvalCubic(&targetPos.x, &trackingAccel.x, prevTargetPos.x, trackingVel.x, trackingTime.x);
        }

        s32 dty = targetPos.y - prevTargetPos.y;
        if (dty < 0) dty = -dty;
        if (dty >= TGT_THRESHOLD) {
            EvalCubic(&targetPos.y, &trackingAccel.y, prevTargetPos.y, trackingVel.y, trackingTime.y);
        }

        s32 dtz = targetPos.z - prevTargetPos.z;
        if (dtz < 0) dtz = -dtz;
        if (dtz >= TGT_THRESHOLD) {
            EvalCubic(&targetPos.z, &trackingAccel.z, prevTargetPos.z, trackingVel.z, trackingTime.z);
        }
    }

    // Write back
    prevPosition = position;
    position = savedPos;

    // --- LookAtTarget ---
    // If flags bit 1 is set, compute look-at angles from target position
    if (flags & 0x02) {
        LookAtTarget(&targetPos);
    }
}

// ============================================================================
// Camera::Update (0x800482DC)
// Builds the world-to-camera matrix and pushes it to tCamera.
// Two paths: angle-based (cameraAnim==0) or point-based (cameraAnim!=0).
// ============================================================================
void Camera::Update() {
    MARKFUNCTION(0x800482DC);

    // PC improved cam already set the matrix directly — skip
    if (directMatrix) {
        directMatrix = false;
        return;
    }

    if (cameraAnim == 0) {
        // PSX: p3dBuildRotMatrixZYX(camAngleZ, camAngleY, camAngleX, &matrix)
        Mat4 rot;
        p3dBuildRotMatrixZYX(camAngleZ, camAngleY, camAngleX, rot);

        // Translation from position
        rot.m[12] = (f32)position.x;
        rot.m[13] = (f32)position.y;
        rot.m[14] = (f32)position.z;

        p3dCamera.SetCameraMatrix(rot);
        return;
    }

    Vec3 eye((f32)curPos.x, (f32)curPos.y, (f32)curPos.z);
    Vec3 tgt((f32)targetPos.x, (f32)targetPos.y, (f32)targetPos.z);

    // Direction vector (target - eye)
    Vec3 fwd = (tgt - eye).Normalized();

    // Up hint — PSX uses p3dFillHeadingMatrix with twist-rotated up
    // If forward is nearly vertical, use X as up hint
    Vec3 up(0.0f, 1.0f, 0.0f);
    if (std::fabs(fwd.x) < 0.001f && std::fabs(fwd.z) < 0.001f)
        up.Set(1.0f, 0.0f, 0.0f);

    // Build view matrix: right/up/forward basis
    Vec3 right = fwd.Cross(up);
    right.Normalize();
    Vec3 realUp = right.Cross(fwd);

    Mat4 view;
    view.m[0] = right.x;  view.m[4] = right.y;  view.m[8]  = right.z;
    view.m[1] = realUp.x;  view.m[5] = realUp.y;  view.m[9]  = realUp.z;
    view.m[2] = -fwd.x;    view.m[6] = -fwd.y;    view.m[10] = -fwd.z;
    view.m[3] = 0;          view.m[7] = 0;          view.m[11] = 0;

    view.m[12] = -right.Dot(eye);
    view.m[13] = -realUp.Dot(eye);
    view.m[14] = fwd.Dot(eye);
    view.m[15] = 1.0f;

    p3dCamera.SetCameraMatrix(view);

    // Update all position fields (PSX copies back from tMatrixCamera GetPosition)
    position = curPos;
    prevPosition = curPos;

    // Copy target position chain
    prevTargetPos = targetPos;
}

// ============================================================================
// Camera::LookAtTarget (0x8004850C)
// Computes camera Euler angles from position toward target.
// ============================================================================
void Camera::LookAtTarget(const LVector* target) {
    MARKFUNCTION(0x8004850C);

    s32 dx = target->x - position.x;
    s32 dy = target->y - position.y;
    s32 dz = target->z - position.z;

    // PSX: rmMag2(dx, dz)
    f32 hMag = rmMag2((f32)dx, (f32)dz);

    // Pitch: PSX rmATan2(-dy, hMag) stored at +380
    f32 pitchRad = std::atan2(-(f32)dy, hMag);
    s32 pitchAngle = (s32)(pitchRad / PSX_ANGLE_TO_RAD);

    // Determine quadrants (PSX uses sign of dy and dz)
    quadrantYZ = 0;
    if (dy < 0) quadrantYZ = 1;
    if (dz < 0) quadrantYZ = (s16)(quadrantYZ + 2);

    // Adjust pitch for quadrant
    if (quadrantYZ < 2) {
        // Front quadrants: camAngleX = pitch + 16384 (90°)
        camAngleX = pitchAngle + 16384;
        camAngleZ = 0x8000; // 180°
    } else {
        // Back quadrants: camAngleX = 16384 - pitch
        camAngleX = 16384 - pitchAngle;
        camAngleZ = 0;
    }

    // Yaw: angle in XZ plane
    quadrantXZ = 0;
    if (dx < 0) quadrantXZ = 1;
    if (dz < 0) quadrantXZ = (s16)(quadrantXZ + 2);

    // PSX: rmATan2 with quadrant-dependent sign flips
    f32 yawRad;
    switch (quadrantXZ) {
        case 0: // +x, +z
            yawRad = std::atan2(-(f32)dx, (f32)dz);
            camAngleY = (s32)(yawRad / PSX_ANGLE_TO_RAD) + 16384;
            break;
        case 1: // -x, +z
            yawRad = std::atan2(-(f32)dx, (f32)dz);
            camAngleY = (s32)(yawRad / PSX_ANGLE_TO_RAD) + 16384;
            break;
        case 2: // +x, -z
            yawRad = std::atan2(-(f32)dz, -(f32)dx);
            camAngleY = (s32)(yawRad / PSX_ANGLE_TO_RAD) + 0x8000;
            break;
        case 3: // -x, -z
            yawRad = std::atan2(-(f32)dz, -(f32)dx);
            camAngleY = (s32)(yawRad / PSX_ANGLE_TO_RAD) + 0x8000;
            break;
    }

    // Store to orientation
    orientAngles.x = camAngleX;
    orientAngles.y = camAngleY;
    orientAngles.z = camAngleZ;
}

// ============================================================================
// Camera::SetMode (0x80049C44)
// ============================================================================
void Camera::SetMode(CameraMode mode) {
    MARKFUNCTION(0x80049C44);

    currentMode = mode;
    switch (mode) {
        case CAM_MODE_DEFAULT:
            modeFunc = &Camera::DebugCam;
            flags &= ~0x02u; // clear look-at flag
            break;
        case CAM_MODE_FOLLOW:
            modeFunc = &Camera::FollowPath;
            flags |= 0x02u;  // enable look-at
            break;
        case CAM_MODE_RIGID:
            modeFunc = &Camera::RigidCam;
            flags |= 0x02u;
            break;
    }

    // PSX also calls SetMovementTime with current position data after SetMode
    // (updates the movement time based on current global state)
}

// ============================================================================
// Camera::SetFOV (0x8004A500)
// ============================================================================
void Camera::SetFOV(s32 fov) {
    MARKFUNCTION(0x8004A500);
    desiredFOV = fov;
}

// ============================================================================
// Camera::SetCurFOV (0x8004A464)
// Sets curFOV and immediately pushes to tCamera.
// PSX formula: curFOV = ((v*3)<<4 - v)<<3 - v)<<3 = v * 3000
// ============================================================================
void Camera::SetCurFOV(s32 fov) {
    MARKFUNCTION(0x8004A464);
    curFOV = fov * 3000;
    f32 fovRad = (f32)curFOV * PSX_FOV_TO_RAD;
    if (fovRad > 0.01f) {
        p3dCamera.SetFOV(fovRad, PSX_ASPECT);
    }
}

// ============================================================================
// Camera::SetMovementTime (0x8004A400)
// Converts user time values to internal units: input * 1000
// ============================================================================
void Camera::SetMovementTime(const LVector* t) {
    MARKFUNCTION(0x8004A400);
    movementTime.x = t->x * TIME_SCALE;
    movementTime.y = t->y * TIME_SCALE;
    movementTime.z = t->z * TIME_SCALE;
}

// ============================================================================
// Camera::SetTrackingTime (0x8004A39C)
// ============================================================================
void Camera::SetTrackingTime(const LVector* t) {
    MARKFUNCTION(0x8004A39C);
    trackingTime.x = t->x * TIME_SCALE;
    trackingTime.y = t->y * TIME_SCALE;
    trackingTime.z = t->z * TIME_SCALE;
}

// ============================================================================
// Camera::SetLookAtTarget (0x80049DC0)
// ============================================================================
void Camera::SetLookAtTarget(void* thing, u16 mode) {
    MARKFUNCTION(0x80049DC0);
    targetThing = thing;
    if (mode == 1) {
        lookAtMode = mode;
    }
}

// ============================================================================
// Camera::ShakeCamera (0x80049DE4)
// ============================================================================
void Camera::ShakeCamera(s32 frames) {
    MARKFUNCTION(0x80049DE4);
    shakeFrames = frames;
}

// ============================================================================
// Camera::DebugCam (0x80048718) — Mode 0
// PSX: reads pad input, adjusts camera Euler angles + position.
// Angles are 16-bit PSX angle units (65536 = full circle).
// Position delta is rotated by camera orientation before applying.
// ============================================================================
void Camera::DebugCam() {
    MARKFUNCTION(0x80048718);

#if RC_FEATURE_IMPROVED_DEBUG_CAM
    // ------------------------------------------------------------------
    // PC Improved Debug Camera — WASD + mouse-look (LMB drag)
    // Replaces the PSX DebugCam with smooth float-precision controls.
    // Matches the old FreeCamera behaviour that worked well on PC.
    // ------------------------------------------------------------------
    PlatformInput* pi = p3d::input;
    if (!pi) return;

    static Vec3 pos;
    static f32 yaw   = 0.0f;
    static f32 pitch = 0.0f;
    static bool posInited = false;

    // Sync float position from Camera on first call
    if (!posInited) {
        pos.Set((f32)curPos.x, (f32)curPos.y, (f32)curPos.z);
        posInited = true;
    }

    const f32 sensitivity = 0.003f;
    const f32 dt = 1.0f / 60.0f;
    f32 speed = 3000.0f;

    // Mouse rotation (LMB held)
    if (pi->IsMouseButtonDown(pddiInput::MouseLeft)) {
        double mx, my;
        pi->GetMouseDelta(mx, my);
        yaw   += static_cast<f32>(mx) * sensitivity;
        pitch -= static_cast<f32>(my) * sensitivity;
        if (pitch >  1.55f) pitch =  1.55f;
        if (pitch < -1.55f) pitch = -1.55f;
    }

    // Speed boost (Shift)
    if (pi->IsKeyDown(pddiInput::KeyLeftShift))
        speed *= 4.0f;

    f32 spd = speed * dt;

    // Build forward/right vectors from yaw+pitch
    f32 cy = std::cos(yaw), sy = std::sin(yaw);
    f32 cp = std::cos(pitch), sp = std::sin(pitch);

    Vec3 fwd(sy * cp, sp, cy * cp);
    Vec3 right(cy, 0.0f, -sy);

    // Accumulate movement
    if (pi->IsKeyDown('W')) pos += fwd * spd;
    if (pi->IsKeyDown('S')) pos -= fwd * spd;
    if (pi->IsKeyDown('A')) pos -= right * spd;
    if (pi->IsKeyDown('D')) pos += right * spd;
    if (pi->IsKeyDown('Q')) pos.y += spd;
    if (pi->IsKeyDown('E')) pos.y -= spd;

    // Build a proper LookAt view matrix directly
    Vec3 target = pos + fwd;
    Mat4 view = p3dLookAt(pos, target, Vec3(0.0f, 1.0f, 0.0f));
    p3dCamera.SetCameraMatrix(view);
    directMatrix = true;

    // Keep position/curPos in sync for any code that reads them
    position.x = static_cast<s32>(pos.x);
    position.y = static_cast<s32>(pos.y);
    position.z = static_cast<s32>(pos.z);
    curPos = position;
    prevPosition = position;

#else
    // ------------------------------------------------------------------
    // Original PSX DebugCam (0x80048718) — faithful reversal
    // Reads pad input, adjusts Euler angles (±511 per frame) and
    // position (±50 per frame), rotated by camera orientation.
    // ------------------------------------------------------------------
    if (!g_inputManager) return;
    u32 pad = g_inputManager->GetRawButtons(0);

    // PSX: if R3 is down, skip debug camera (guard button)
    if (pad & PsxPad::R3) return;

    // Angle adjustments (PSX: ±511 per frame)
    if (pad & PsxPad::Square)   camAngleY -= 511;
    if (pad & PsxPad::Circle)   camAngleY += 511;
    if (pad & PsxPad::Triangle) camAngleX += 511;
    if (pad & PsxPad::Cross)    camAngleX -= 511;

    // Build local movement delta (PSX: sp+16, sp+20, sp+24)
    s32 ddx = 0, ddy = 0, ddz = 0;

    if (pad & PsxPad::Left)   ddx -= 50;
    if (pad & PsxPad::Right)  ddx += 50;
    if (pad & PsxPad::Up)     ddy += 50;
    if (pad & PsxPad::Down)   ddy -= 50;
    if (pad & PsxPad::Start)  ddz -= 50;
    if (pad & PsxPad::L3)     ddz += 50;

    // If no movement, skip rotation + translation
    if (ddx == 0 && ddy == 0 && ddz == 0) return;

    // Rotate delta by camera angles (PSX: p3dBuildRotMatrixZYX + p3dVecTimesRotMatrix)
    Mat4 rot;
    p3dBuildRotMatrixZYX(camAngleZ, camAngleY, camAngleX, rot);
    Vec3 delta = p3dVecTimesRotMatrix(Vec3((f32)ddx, (f32)ddy, (f32)ddz), rot);

    curPos.x += static_cast<s32>(delta.x);
    curPos.y += static_cast<s32>(delta.y);
    curPos.z += static_cast<s32>(delta.z);
#endif // RC_FEATURE_IMPROVED_DEBUG_CAM
}

// ============================================================================
// Camera::FollowPath (0x80048AC0) — Mode 1
// PSX: 4484 bytes, finds closest camera path nodes, interpolates position
// along spline path, sets target positions. Requires CameraAnchor + path data.
// Stubbed until camera path data is loaded from level.
// ============================================================================
void Camera::FollowPath() {
    MARKFUNCTION(0x80048AC0);
    // TODO: implement camera path follow when CameraAnchor system is reversed
    // For now, does nothing — camera stays at current position
}

// ============================================================================
// Camera::RigidCam (0x8004897C) — Mode 2
// PSX: maintains fixed offset from target Thing. Requires targetThing.
// ============================================================================
void Camera::RigidCam() {
    MARKFUNCTION(0x8004897C);
    if (targetThing == nullptr) return;
    // TODO: implement rigid camera offset when Thing/Character system exists
}

// ============================================================================
// Camera::CameraShake (0x80049DEC)
// Applies random offset to position based on shakeStrength.
// ============================================================================
void Camera::CameraShake() {
    MARKFUNCTION(0x80049DEC);
    shakeFrames--;

    // PSX: random() % shakeStrength per axis, adds to curPos
    if (shakeStrength.x != 0) {
        curPos.x += (std::rand() % shakeStrength.x) - shakeStrength.x / 2;
    }
    if (shakeStrength.y != 0) {
        curPos.y += (std::rand() % shakeStrength.y) - shakeStrength.y / 2;
    }
    if (shakeStrength.z != 0) {
        curPos.z += (std::rand() % shakeStrength.z) - shakeStrength.z / 2;
    }
}
