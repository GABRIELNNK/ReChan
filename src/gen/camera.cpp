#include "common.h"
#include "gen/camera.h"
#include "gen/control.h"
#include "gen/charmgr.h"
#include "gen/animstruct.h"
#include "ai/thing.h"
#include "p3d/p3dmath.h"
#include "config.h"
#include "p3d/keycode.h"
#include "p3d/input.h"
#include "p3d/context.h"
#include <cstdlib>
#include <cmath>

// PSX math helpers

// PSX FOV push: converts curFOV (= desiredFOV * 3000) to tCamera fovA/fovB.
// PSX: fovA = (87162 * curFOV) >> 16, fovB = (72744 * curFOV) >> 16
static void pushFovToCamera(tCamera* cam, s32 curFOV) {
    cam->SetFOV((s32)((87162LL * curFOV) >> 16), (s32)((72744LL * curFOV) >> 16));
}

static constexpr s32 TIME_SCALE = 1000;
static constexpr s32 MAX_CAM_POS_DELTA = 32;
static constexpr s32 MAX_VIEW_POS_DELTA_X = 32;
static constexpr s32 MAX_VIEW_POS_DELTA_Y = 32;
static constexpr s32 MAX_VIEW_POS_DELTA_Z = 32;
static constexpr s32 RESTING_DIST = 3200;
static constexpr s32 HEIGHT_OFFSET = 2000;
static constexpr s32 SPIN_RATE = 6500;

static s32 rmRandom0() {
    return std::rand();
}

static s32 rmSin16Local(s32 angle) {
    return rmSin16(angle);
}

bool EvalCubic(s32* curValue, s32* accel, s32 target, s32 velocity, s32 time) {
    s32 v7 = *accel;
    s32 v8 = *curValue;
    s32 v9 = v7 + velocity;
    s32 v10 = target - v8;
    s32 v11 = v9 - v10 - v10;
    s32 v12 = 3 * v10 - v9 - v7;

    *curValue = v8 + (s32)(((s64)(v7 + (s32)(((s64)(v12 + (s32)(((s64)v11 * time) >> 16)) * time) >> 16)) * time) >> 16);
    *accel = v7 + (s32)(((s64)(2 * v12 + (s32)(((s64)(3 * v11) * time) >> 16)) * time) >> 16);

    bool crossed;
    if (v8 >= target) {
        crossed = (*curValue < target);
    }
    else {
        crossed = (target < *curValue);
    }

    if (!crossed) {
        return false;
    }

    *curValue = target;
    *accel = velocity;
    return true;
}


// Camera constructor (0x80047AD4)

Camera::Camera() {
    MARKFUNCTION(0x80047AD4);
    // PSX: calls DynamicThing ctor with type 602, creates tMatrixCamera at +404
    // PC: tCamera is directly embedded, constructed by default ctor
    Reset();
}

Camera::~Camera() {
    MARKFUNCTION(0x80047C30);
}


// Camera::Reset (0x80047C5C)

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

    // Velocity magnitudes - PSX init to 1966
    velocityMag.x = 1966;
    velocityMag.y = 1966;
    velocityMag.z = 1966;

    // Default movement/tracking times in Reset are {6,6,6}.
    LVector defaultTime = { 6, 6, 6 };
    SetMovementTime(&defaultTime);
    SetTrackingTime(&defaultTime);

    // FOV
    SetFOV(10); // desiredFOV = 10
    // curFOV = desiredFOV * 3000 (PSX formula)
    curFOV = desiredFOV * 3000;
    fovAccel = 0;
    fovVel = 0;

    // Push FOV to tCamera
    pushFovToCamera(&p3dCamera, curFOV);

    // Twist - PSX 8192 ≈ 45 degrees
    twist = 8192;

    // Camera angles
    camAngleX = 0;
    camAngleY = 0;
    camAngleZ = 0;
    quadrantYZ = 0;
    quadrantXZ = 0;

    // Reset camera flags before selecting mode.
    flags = 0;

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

    // Anim - PSX: inline PurgeAnims/DeleteAsyncAnim logic at end of Reset
    if (cameraAnim) {
        delete cameraAnim;
        cameraAnim = nullptr;
    }
    if (asyncAnimEnum != 0xFFFF) {
        if (g_characterManager) {
            g_characterManager->UnloadAnimationBatch(0, (s32)asyncAnimEnum);
        }
        asyncAnimEnum = 0xFFFF;
        asyncAnim = nullptr;
    }
    cameraAnchor = nullptr;
}


// Camera::Think (0x80047F28)
// Per-frame entry point. Dispatches to current mode function, then Move + Shake.

void Camera::Think() {
    MARKFUNCTION(0x80047F28);

    if (cameraAnim == nullptr) {
        if (modeFunc != nullptr) {
            (this->*modeFunc)();
        }

        if (hasCollision != 0) {
            Move();
        }
    }

    // Camera shake
    if (shakeFrames > 0) {
        CameraShake();
    }
}


// Camera::Move (0x80047FD4)
// Interpolates position/target toward their goals using EvalCubic.
// Also interpolates FOV.

void Camera::Move() {
    MARKFUNCTION(0x80047FD4);

    s32 targetFOV = desiredFOV * 3000;
    if (curFOV != targetFOV) {
        // PSX uses velocityMag.x (+344) for FOV time, not movementTime.x (+300)
        EvalCubic(&curFOV, &fovAccel, targetFOV, fovVel, velocityMag.x);
        pushFovToCamera(&p3dCamera, curFOV);
    }

    LVector nextPos = position;

    if (!(flags & 0x01)) {
        s32 deltaX = nextPos.x - curPos.x;
        if (deltaX < 0) {
            deltaX = -deltaX;
        }
        if (deltaX >= MAX_CAM_POS_DELTA) {
            EvalCubic(&nextPos.x, &movementAccel.x, curPos.x, movementVel.x, movementTime.x);
        }

        if (nextPos.y != curPos.y) {
            EvalCubic(&nextPos.y, &movementAccel.y, curPos.y, movementVel.y, movementTime.y);
        }

        s32 deltaZ = nextPos.z - curPos.z;
        if (deltaZ < 0) {
            deltaZ = -deltaZ;
        }
        if (deltaZ >= MAX_CAM_POS_DELTA) {
            EvalCubic(&nextPos.z, &movementAccel.z, curPos.z, movementVel.z, movementTime.z);
        }

        s32 dtx = targetPos.x - prevTargetPos.x;
        if (dtx < 0) {
            dtx = -dtx;
        }
        if (dtx >= MAX_VIEW_POS_DELTA_X) {
            EvalCubic(&targetPos.x, &trackingAccel.x, prevTargetPos.x, trackingVel.x, trackingTime.x);
        }

        s32 dty = targetPos.y - prevTargetPos.y;
        if (dty < 0) {
            dty = -dty;
        }
        if (dty >= MAX_VIEW_POS_DELTA_Y) {
            EvalCubic(&targetPos.y, &trackingAccel.y, prevTargetPos.y, trackingVel.y, trackingTime.y);
        }

        s32 dtz = targetPos.z - prevTargetPos.z;
        if (dtz < 0) {
            dtz = -dtz;
        }
        if (dtz >= MAX_VIEW_POS_DELTA_Z) {
            EvalCubic(&targetPos.z, &trackingAccel.z, prevTargetPos.z, trackingVel.z, trackingTime.z);
        }
    }

    prevPosition = position;
    position = nextPos;

    if (flags & 0x02) {
        LookAtTarget(&targetPos);
    }
}


// Camera::Update (0x800482DC)
// Builds the world-to-camera matrix and pushes it to tCamera.
// Two paths: angle-based (cameraAnim==0) or point-based (cameraAnim!=0).

// Camera::Update (0x800482DC)
// PSX: builds camera-to-world matrix and sets it on tMatrixCamera.
// Two paths: euler angles (cameraAnim==0) or 2-point camera (cameraAnim!=0).

void Camera::Update() {
    MARKFUNCTION(0x800482DC);

    if (cameraAnim != nullptr) {
        // PSX: reads position/target/twist from G_2ptcam (set by t2PointCamFlip)
        LVector camPos, camTarget;
        G_2ptcam.GetPosition(&camPos);
        G_2ptcam.GetTarget(&camTarget);
        u16 camTwist = G_2ptcam.GetTwist();

        LVector headingFixed = {
            camTarget.x - camPos.x,
            camTarget.y - camPos.y,
            camTarget.z - camPos.z,
        };

        rmV3Normalize(&headingFixed, &headingFixed);

        Vec3 heading(
            FIX16_TO_FLOAT(headingFixed.x),
            FIX16_TO_FLOAT(headingFixed.y),
            FIX16_TO_FLOAT(headingFixed.z));

        Vec3 up(0.0f, 1.0f, 0.0f);
        if (headingFixed.x == 0 && headingFixed.z == 0) {
            up = Vec3(1.0f, 0.0f, 0.0f);
        }

        Mat4 headingMatrix;
        p3dFillHeadingMatrix(heading, up, headingMatrix);

        Mat4 twistMatrix;
        p3dBuildRotMatrixZ(ANGLE2RAD(camTwist), twistMatrix);

        Mat4 matrix = twistMatrix * headingMatrix;
        p3dFillTransMatrix(camPos, matrix);

        // PSX: GetFOV from G_2ptcam, SetCameraMatrix + SetFOV on embedded tMatrixCamera
        s32 fovA, fovB;
        G_2ptcam.GetFOV(&fovA, &fovB);
        p3dCamera.SetCameraMatrix(matrix);
        p3dCamera.SetFOV(fovA, fovB);

        // PSX: GetPosition from the tMatrixCamera to sync Camera fields
        LVector matPos;
        p3dCamera.GetPosition(&matPos);
        prevPosition = position;
        position = matPos;
        curPos = matPos;
        prevTargetPos = camTarget;
        targetPos = camTarget;
        return;
    }

    // PSX euler path (cameraAnim == 0)
    // p3dBuildRotMatrixZYX, p3dFillTransMatrix, SetCameraMatrix, UpdateMatrix
    Mat4 matrix;
    p3dBuildRotMatrixZYX(camAngleX, camAngleY, camAngleZ, matrix);
    p3dFillTransMatrix(position, matrix);
    p3dCamera.SetCameraMatrix(matrix);
    p3dCamera.UpdateMatrix();
}


// Camera::LookAtTarget (0x8004850C)
// Computes camera Euler angles from position toward target.

void Camera::LookAtTarget(const LVector* target) {
    MARKFUNCTION(0x8004850C);

    s32 dx = target->x - position.x;
    s32 dy = target->y - position.y;
    s32 dz = target->z - position.z;

    s32 mag = (s32)rmMag2((f32)dx, (f32)dz);
    camAngleX = rmATan216((f32)-dy, (f32)mag);

    quadrantYZ = 0;
    if (dy < 0) {
        quadrantYZ = 1;
    }
    if (dz < 0) {
        quadrantYZ = (s16)(quadrantYZ + 2);
    }

    s32 orientX = camAngleX;

    if (quadrantYZ < 2) {
        camAngleZ = 0x8000;
        camAngleX = camAngleX + 0x4000;
    }
    else {
        camAngleZ = 0;
        camAngleX = 0x4000 - camAngleX;
    }

    quadrantXZ = 0;
    if (dx < 0) {
        quadrantXZ = 1;
    }
    if (dz < 0) {
        quadrantXZ = (s16)(quadrantXZ + 2);
    }

    s32 orientY = orientAngles.y;
    s32 xzQuad = quadrantXZ;
    if (xzQuad == 1) {
        s32 dxFixed = (s32)(-65536LL * dx);
        s32 dzFixed = (s32)((s64)dz << 16);
        camAngleY = rmATan216((f32)dxFixed, (f32)dzFixed) + 0x4000;
        orientY = camAngleY + 0x8000;
    }
    else if (xzQuad < 2) {
        if (quadrantXZ == 0) {
            s32 dxFixed = (s32)(-65536LL * dx);
            s32 dzFixed = (s32)((s64)dz << 16);
            s32 a = rmATan216((f32)dxFixed, (f32)dzFixed);
            camAngleY = a + 0x4000;
            orientY = a - 0x4000;
        }
    }
    else if (xzQuad < 4) {
        s32 dzFixed = (s32)(-65536LL * dz);
        s32 dxFixed = (s32)(-65536LL * dx);
        camAngleY = rmATan216((f32)dzFixed, (f32)dxFixed) + 0x8000;
        orientY = camAngleY;
    }

    orientAngles.x = orientX;
    orientAngles.y = orientY;
    orientAngles.z = camAngleZ;
}


// Camera::SetMode (0x80049C44)

void Camera::SetMode(CameraMode mode) {
    MARKFUNCTION(0x80049C44);

    currentMode = mode;
    switch (mode) {
        case CAM_MODE_DEFAULT:
        {
            modeFunc = &Camera::DebugCam;
            flags &= ~0x02u; // clear look-at flag
            LVector mt = { 6, 6, 6 };
            SetMovementTime(&mt);
            break;
        }
        case CAM_MODE_FOLLOW:
        {
            modeFunc = &Camera::FollowPath;
            flags |= 0x02u;  // enable look-at
            LVector mt = { 8, 10, 8 };
            SetMovementTime(&mt);
            break;
        }
        case CAM_MODE_RIGID:
        {
            modeFunc = &Camera::RigidCam;
            flags |= 0x02u;
            LVector mt = { 6, 6, 6 };
            SetMovementTime(&mt);
            break;
        }
    }
}


// Camera::SetFOV (0x8004A500)

void Camera::SetFOV(s32 fov) {
    MARKFUNCTION(0x8004A500);
    desiredFOV = fov;
}


// Camera::SetCurFOV (0x8004A464)
// Sets curFOV and immediately pushes to tCamera.
// PSX formula: curFOV = ((v*3)<<4 - v)<<3 - v)<<3 = v * 3000

void Camera::SetCurFOV(s32 fov) {
    MARKFUNCTION(0x8004A464);
    curFOV = fov * 3000;
    // PSX: SetFOV(tCamera, (261486000*fov)>>16, (218232000*fov)>>16)
    p3dCamera.SetFOV((s32)((261486000LL * fov) >> 16), (s32)((218232000LL * fov) >> 16));
}


// Camera::SetMovementTime (0x8004A400)
// Converts user time values to internal units: input * 1000

void Camera::SetMovementTime(const LVector* t) {
    MARKFUNCTION(0x8004A400);
    movementTime.x = t->x * TIME_SCALE;
    movementTime.y = t->y * TIME_SCALE;
    movementTime.z = t->z * TIME_SCALE;
}


// Camera::SetTrackingTime (0x8004A39C)

void Camera::SetTrackingTime(const LVector* t) {
    MARKFUNCTION(0x8004A39C);
    trackingTime.x = t->x * TIME_SCALE;
    trackingTime.y = t->y * TIME_SCALE;
    trackingTime.z = t->z * TIME_SCALE;
}


// Camera::SetLookAtTarget (0x80049DC0)

void Camera::SetLookAtTarget(Thing* thing, u16 mode) {
    MARKFUNCTION(0x80049DC0);
    targetThing = thing;
    if (mode == 1) {
        lookAtMode = mode;
    }
}


// Camera::ShakeCamera (0x80049DE4)

void Camera::ShakeCamera(s32 frames) {
    MARKFUNCTION(0x80049DE4);
    shakeFrames = frames;
}


// Camera::DebugCam (0x80048718) - Mode 0
// PSX: reads pad input, adjusts camera Euler angles + position.
// Angles are 16-bit PSX angle units (65536 = full circle).
// Position delta is rotated by camera orientation before applying.

void Camera::DebugCam() {
    MARKFUNCTION(0x80048718);
#if IMPROVED_DEBUG_CAM
    // PC debug camera: mouse look + WASD movement + Q/E vertical + Shift speed
    double mdx = 0, mdy = 0;
    p3d::input->GetMouseDelta(mdx, mdy);

    static constexpr s32 MOUSE_SENSITIVITY = 10;
    if (p3d::input->IsMouseButtonDown(MOUSE_LEFT)) {
        camAngleY += (s32)(mdx * MOUSE_SENSITIVITY);
        camAngleX -= (s32)(mdy * MOUSE_SENSITIVITY);
    }

    s32 speed = 50;
    if (p3d::input->IsKeyDown(KEY_LEFT_SHIFT)) {
        speed = 200;
    }

    s32 ddx = 0;
    s32 ddy = 0;
    s32 ddz = 0;

    if (p3d::input->IsKeyDown(KEY_W)) {
        ddz += speed;
    }
    if (p3d::input->IsKeyDown(KEY_S)) {
        ddz -= speed;
    }
    if (p3d::input->IsKeyDown(KEY_A)) {
        ddx -= speed;
    }
    if (p3d::input->IsKeyDown(KEY_D)) {
        ddx += speed;
    }
    if (p3d::input->IsKeyDown(KEY_E)) {
        ddy += speed;
    }
    if (p3d::input->IsKeyDown(KEY_Q)) {
        ddy -= speed;
    }

    if (ddx != 0 || ddy != 0 || ddz != 0) {
        Mat4 rot;
        p3dBuildRotMatrixZYX(camAngleX, camAngleY, camAngleZ, rot);
        Vec3 delta = p3dVecTimesMatrix(Vec3((f32)ddx, (f32)ddy, (f32)ddz), rot);

        position.x += (s32)delta.x;
        position.y += (s32)delta.y;
        position.z += (s32)delta.z;
    }

    curPos = position;
    prevPosition = curPos;
#else
    if (!g_inputManager) {
        return;
    }

    u32 pad = g_inputManager->GetRawButtons(0);

    if (pad & PsxPad::R3) {
        return;
    }

    // Angle adjustments are driven by D-pad in the original path.
    if (pad & PsxPad::Left) {
        camAngleY -= 511;
    }
    if (pad & PsxPad::Right) {
        camAngleY += 511;
    }
    if (pad & PsxPad::Up) {
        camAngleX += 511;
    }
    if (pad & PsxPad::Down) {
        camAngleX -= 511;
    }

    s32 ddx = 0;
    s32 ddy = 0;
    s32 ddz = 0;

    if (pad & PsxPad::Square) {
        ddx -= 50;
    }
    if (pad & PsxPad::Circle) {
        ddx += 50;
    }
    if (pad & PsxPad::Triangle) {
        ddy += 50;
    }
    if (pad & PsxPad::R1) {
        ddy -= 50;
        ddz -= 50;
    }
    if (pad & PsxPad::R2) {
        ddz += 50;
    }

    if (ddx == 0 && ddy == 0 && ddz == 0) {
        return;
    }

    Mat4 rot;
    p3dBuildRotMatrixZYX(camAngleX, camAngleY, camAngleZ, rot);
    Vec3 delta = p3dVecTimesMatrix(Vec3((f32)ddx, (f32)ddy, (f32)ddz), rot);

    curPos.x += static_cast<s32>(delta.x);
    curPos.y += static_cast<s32>(delta.y);
    curPos.z += static_cast<s32>(delta.z);
#endif
}


// Camera::FollowPath (0x80048AC0) - Mode 1
// PSX: 4484 bytes, finds closest camera path nodes, interpolates position
// along spline path, sets target positions. Requires CameraAnchor + path data.
// Stubbed until camera path data is loaded from level.

void Camera::FollowPath() {
    MARKFUNCTION(0x80048AC0);

    Thing* target = targetThing;
    if (!target) {
        LOG("[Camera] FollowPath: no targetThing");
        return;
    }

    LVector targetWorldPos = {};
    target->GetViewSpot(nullptr, &targetWorldPos);

    // Find two closest camera rail nodes to target position
    DBCameraPathNode* nodeA = nullptr;
    DBCameraPathNode* nodeB = nullptr;

    if (cameraAnchor) {
        s32 dist = cameraAnchor->FindClosestNodes(targetWorldPos, &nodeA, &nodeB);
        (void)dist;
    }

    if (!nodeA) {
        LOG("[Camera] FollowPath: no nodeA! anchor=%p targetWorldPos=(%d,%d,%d)",
            cameraAnchor, targetWorldPos.x, targetWorldPos.y, targetWorldPos.z);
        return;
    }

    if (!nodeB)
        nodeB = nodeA;
    // Read node positions
    // sourcePos (+12) = camera eye position on rail
    // targetPos (+24) = look-at target position on rail
    LVector nodeA_src = nodeA->sourcePos;
    LVector nodeA_tgt = nodeA->targetPos;
    LVector nodeB_src = nodeB->sourcePos;
    LVector nodeB_tgt = nodeB->targetPos;

    // Compute parametric t: projection of targetWorldPos onto segment nodeA_tgt -> nodeB_tgt
    // PSX: uses 16.16 fixed-point (0x10000 = 1.0)
    s32 segX = nodeB_tgt.x - nodeA_tgt.x;
    s32 segY = nodeB_tgt.y - nodeA_tgt.y;
    s32 segZ = nodeB_tgt.z - nodeA_tgt.z;

    s32 toAx = targetWorldPos.x - nodeA_tgt.x;
    s32 toAy = targetWorldPos.y - nodeA_tgt.y;
    s32 toAz = targetWorldPos.z - nodeA_tgt.z;

    s32 toBx = targetWorldPos.x - nodeB_tgt.x;
    s32 toBy = targetWorldPos.y - nodeB_tgt.y;
    s32 toBz = targetWorldPos.z - nodeB_tgt.z;

    s32 dotA = segX * toAx + segY * toAy + segZ * toAz;
    s32 dotB = segX * toBx + segY * toBy + segZ * toBz;
    s32 denom = dotA - dotB;

    s32 t = 0;
    if (denom != 0)
        t = rmDiv16i(dotA, denom);
    if (t < 0) t = 0;
    if (t > FIX16_ONE) t = FIX16_ONE;

    s32 oneMinusT = FIX16_ONE - t;

    // Interpolate node attributes (16.16 lerp)
    s32 fovInterp = (s32)(((s64)nodeA->fov * oneMinusT + (s64)nodeB->fov * t) >> 16);
    s32 angleYInterp = (s32)(((s64)nodeA->camAngleY * oneMinusT + (s64)nodeB->camAngleY * t) >> 16);
    s32 angleXInterp = (s32)(((s64)nodeA->camAngleX * oneMinusT + (s64)nodeB->camAngleX * t) >> 16);
    s32 angleZInterp = (s32)(((s64)nodeA->camAngleZ * oneMinusT + (s64)nodeB->camAngleZ * t) >> 16);
    s32 zoomInterp = (s32)(((s64)nodeA->zoom * oneMinusT + (s64)nodeB->zoom * t) >> 16);
    s32 speedInterp = (s32)(((s64)nodeA->speed * oneMinusT + (s64)nodeB->speed * t) >> 16);
    s32 flagsInterp = (s32)(((s64)(u8)nodeA->flags * oneMinusT + (s64)(u8)nodeB->flags * t) >> 16);

    // Interpolate param offset vectors
    s32 offsetX = (s32)(((s64)nodeA->param0 * oneMinusT + (s64)nodeB->param0 * t) >> 16);
    s32 offsetY = (s32)(((s64)nodeA->param1 * oneMinusT + (s64)nodeB->param1 * t) >> 16);
    s32 offsetZ = (s32)(((s64)nodeA->param2 * oneMinusT + (s64)nodeB->param2 * t) >> 16);

    // PSX: desiredFOV = flagsInterp
    desiredFOV = flagsInterp;

    // Interpolate camera eye position along rail (from targetPos = look-at rail)
    // prevTargetPos = nodeA_tgt + seg * t (the look-at goal position)
    prevTargetPos.x = nodeA_tgt.x + (s32)(((s64)segX * t) >> 16);
    prevTargetPos.y = nodeA_tgt.y + (s32)(((s64)segY * t) >> 16);
    prevTargetPos.z = nodeA_tgt.z + (s32)(((s64)segZ * t) >> 16);

    // Interpolate look-at target along rail (from sourcePos = camera eye rail)
    s32 srcSegX = nodeB_src.x - nodeA_src.x;
    s32 srcSegY = nodeB_src.y - nodeA_src.y;
    s32 srcSegZ = nodeB_src.z - nodeA_src.z;

    curPos.x = nodeA_src.x + (s32)(((s64)srcSegX * t) >> 16);
    curPos.y = nodeA_src.y + (s32)(((s64)srcSegY * t) >> 16);
    curPos.z = nodeA_src.z + (s32)(((s64)srcSegZ * t) >> 16);

    // Compute offset from look-at rail interpolation to player position.
    // PSX: both prevTargetPos and curPos clampings use this same base offset.
    s32 offBaseX = targetWorldPos.x - prevTargetPos.x;
    s32 offBaseY = targetWorldPos.y - prevTargetPos.y;
    s32 offBaseZ = targetWorldPos.z - prevTargetPos.z;

    // Clamp offset for prevTargetPos (look-at goal)
    // PSX: clamp XZ by angleXInterp (+42), Y by angleZInterp (+44)
    s32 offEyeX = offBaseX;
    s32 offEyeY = offBaseY;
    s32 offEyeZ = offBaseZ;

    if (offEyeX > angleXInterp) offEyeX = angleXInterp;
    else if (offEyeX < -angleXInterp) offEyeX = -angleXInterp;

    if (offEyeZ > angleXInterp) offEyeZ = angleXInterp;
    else if (offEyeZ < -angleXInterp) offEyeZ = -angleXInterp;

    if (offEyeY > angleZInterp) offEyeY = angleZInterp;
    else if (offEyeY < -angleZInterp) offEyeY = -angleZInterp;

    prevTargetPos.x += offEyeX;
    prevTargetPos.y += offEyeY;
    prevTargetPos.z += offEyeZ;

    // Clamp offset for curPos (camera eye goal) - same base offset, different limits.
    // PSX: clamp XZ by zoomInterp (+46), Y by speedInterp (+48)
    s32 offTgtX = offBaseX;
    s32 offTgtY = offBaseY;
    s32 offTgtZ = offBaseZ;

    if (offTgtX > zoomInterp) offTgtX = zoomInterp;
    else if (offTgtX < -zoomInterp) offTgtX = -zoomInterp;

    if (offTgtZ > zoomInterp) offTgtZ = zoomInterp;
    else if (offTgtZ < -zoomInterp) offTgtZ = -zoomInterp;

    if (offTgtY > speedInterp) offTgtY = speedInterp;
    else if (offTgtY < -speedInterp) offTgtY = -speedInterp;

    curPos.x += offTgtX;
    curPos.y += offTgtY;
    curPos.z += offTgtZ;

    // Normalize segment direction and apply parallel offset
    s32 normX = segX << 16;
    s32 normY = segY << 16;
    s32 normZ = segZ << 16;
    s32 mag = (s32)rmMag3((f32)normX, (f32)normY, (f32)normZ);
    if (mag) {
        normX = rmDiv16i(normX, mag);
        normY = rmDiv16i(normY, mag);
        normZ = rmDiv16i(normZ, mag);
    }

    // Camera eye parallel offset: segDirNorm * fovInterp + param offset
    s32 parEyeX = (s32)(((s64)normX * fovInterp) >> 16) + offsetX;
    s32 parEyeY = (s32)(((s64)normY * fovInterp) >> 16) + offsetY;
    s32 parEyeZ = (s32)(((s64)normZ * fovInterp) >> 16) + offsetZ;

    prevTargetPos.x += parEyeX;
    prevTargetPos.y += parEyeY;
    prevTargetPos.z += parEyeZ;

    // Look-at target parallel offset: segDirNorm * angleYInterp
    curPos.x += (s32)(((s64)normX * angleYInterp) >> 16);
    curPos.y += (s32)(((s64)normY * angleYInterp) >> 16);
    curPos.z += (s32)(((s64)normZ * angleYInterp) >> 16);

    // PSX: if lookAtMode is set, snap camera and compute angles
    if (lookAtMode) {
        // Copy prevTargetPos to targetPos (look-at target for LookAtTarget)
        targetPos = prevTargetPos;
        LookAtTarget(&targetPos);

        // Copy curPos to position (camera eye)
        prevPosition = position;
        position = curPos;

        SetCurFOV(desiredFOV);

        // Zero all interpolation state (snap to position)
        movementVel = { 0, 0, 0 };
        movementAccel = { 0, 0, 0 };
        trackingVel = { 0, 0, 0 };
        trackingAccel = { 0, 0, 0 };

        lookAtMode = 0;
    }
}


// Camera::RigidCam (0x8004897C) - Mode 2
// PSX: maintains fixed offset from target Thing. Requires targetThing.

void Camera::RigidCam() {
    MARKFUNCTION(0x8004897C);
    if (targetThing == nullptr) {
        return;
    }

    LVector viewPos = {};
    targetThing->GetViewSpot(&viewPos, &prevTargetPos);

    s32 followAngle = orientAngles.y;
    s32 deltaToZero = -followAngle;
    if (followAngle != 0) {
        if (deltaToZero > 0x8000) {
            deltaToZero += -65535;
        }
        else if (deltaToZero < -32768) {
            deltaToZero += 0xFFFF;
        }

        if (deltaToZero < 0) {
            followAngle = (s16)(followAngle - SPIN_RATE);
            if (-deltaToZero < SPIN_RATE) {
                followAngle = 0;
            }
        }
        else {
            followAngle = (s16)(followAngle + SPIN_RATE);
            if (deltaToZero < SPIN_RATE) {
                followAngle = 0;
            }
        }
    }

    s32 sinYaw = rmSin16Local(followAngle);
    curPos.x = viewPos.x - fixmul16(sinYaw, RESTING_DIST);
    curPos.y = viewPos.y + HEIGHT_OFFSET;

    s32 sinYaw90 = rmSin16Local(followAngle + 0x4000);
    curPos.z = viewPos.z - fixmul16(sinYaw90, RESTING_DIST);
}


// Camera::CameraShake (0x80049DEC)
// Applies random offset to position based on shakeStrength.

void Camera::CameraShake() {
    MARKFUNCTION(0x80049DEC);
    shakeFrames--;

    s32 shakeX = 0;
    s32 shakeY = 0;
    s32 shakeZ = 0;

    if (shakeStrength.x != 0) {
        shakeX = rmRandom0() % shakeStrength.x;
    }
    if (shakeStrength.y != 0) {
        shakeY = rmRandom0() % shakeStrength.y;
    }
    if (shakeStrength.z != 0) {
        shakeZ = rmRandom0() % shakeStrength.z;
    }

    prevPosition = position;
    position.x += shakeX;
    position.y += shakeY;
    position.z += shakeZ;
}


// Camera::PurgeAnims (0x80047BE0)
// Called during level unload to clean up camera animation state.
// PSX: separate function body (not inlined DeleteAsyncAnim).

void Camera::PurgeAnims() {
    MARKFUNCTION(0x80047BE0);

    if (cameraAnim) {
        delete cameraAnim;
        cameraAnim = nullptr;
    }
    if (asyncAnimEnum != 0xFFFF) {
        if (g_characterManager) {
            g_characterManager->UnloadAnimationBatch(0, (s32)asyncAnimEnum);
        }
        asyncAnimEnum = 0xFFFF;
        asyncAnim = nullptr;
    }
}


// Camera::UpdateAnim (0x80047EAC)
// Per-frame animation update. Executes the AnimStructure handler,
// retrieves the animated camera position/target, and calls LookAtTarget.

void Camera::UpdateAnim() {
    MARKFUNCTION(0x80047A0C);
    if (cameraAnim) {
        // PSX: ExecuteHandler(cameraAnim, 1) - advances camera animation frame
        cameraAnim->ExecuteHandler(1);

        // PSX: gets view position from targetThing via vtable+72 (GetViewSpot)
        // and stores into prevTargetPos, copies to targetPos, then LookAtTarget.
        if (targetThing) {
            targetThing->GetViewSpot(nullptr, &prevTargetPos);
            targetPos = prevTargetPos;
            LookAtTarget(&targetPos);
        }
    }
}


// Camera::LoadAsyncAnim (0x8004A054)
// Loads a camera animation by enum via CharacterManager.
// Unloads any previously loaded async anim first.
// PSX: creates a 16-byte callback that stores the loaded tAnimation*
// into the camera's asyncAnim field when loading completes.

// PSX: vtable at dword_800CCC98 - camera animation load callback
struct CamAnimCallback : public CharMgrCallback {
    s32 animEnum;
    void** target; // pointer to camera's asyncAnim field

    CamAnimCallback(s32 e, void** t) : animEnum(e), target(t) {}
    void Callback() override {
        if (target && g_characterManager) {
            *target = g_characterManager->GetAnimation(0, animEnum);
        }
        done = 1;
    }
};

void Camera::LoadAsyncAnim(s32 animEnum) {
    MARKFUNCTION(0x8004A054);

    if (asyncAnimEnum != 0xFFFF) {
        if (g_characterManager) {
            g_characterManager->UnloadAnimationBatch(0, (s32)asyncAnimEnum);
        }
        asyncAnim = nullptr;
    }
    asyncAnimEnum = (u16)animEnum;

    CamAnimCallback* cb = new CamAnimCallback(animEnum, &asyncAnim);
    if (g_characterManager) {
        g_characterManager->LoadAnimationBatch(0, (s32)asyncAnimEnum, cb);
    }
    delete cb;
}


// Camera::PlayAsyncAnim (0x8004A0F0)
// Creates AnimStructure from the loaded anim and starts playback.
// PSX: AnimStructure ctor at 0x80070740 (MODEL.CPP:2335), 104 bytes
// PSX: checks GetAnimationType() on the loaded anim:
//   - 0x10002 (65538): single anim -> Attach t2PointCamFlip to G_2ptcam
//   - 0x1000B (65547): composite anim -> Attach each sub-anim's flip
// PSX: then calls StartAnimation on the AnimStructure's drawable

void Camera::PlayAsyncAnim() {
    MARKFUNCTION(0x8004A0F0);

    if (!asyncAnim) {
        return;
    }

    // PSX: cameraAnim = new AnimStructure(3, asyncAnim, 4, nullptr, nullptr)
    cameraAnim = new AnimStructure(3, asyncAnim, 4, nullptr, nullptr);

    // PSX: GetAnimationType on anim, then Attach t2PointCamFlip to G_2ptcam
    // t2PointCamFlip is created inside the AnimStructure constructor for mode 3.
    // The Attach call connects the flip to G_2ptcam so camera pos/target
    // are updated during ExecuteHandler. Since we don't have the full
    // Pure3D flip pipeline yet, the camera animation data won't actually
    // drive position. This will work once tParamFlip is reversed.
}


// Camera::DeleteAsyncAnim (0x8004A220)
// Cleans up camera animation: deletes AnimStructure, unloads animation.

void Camera::DeleteAsyncAnim() {
    MARKFUNCTION(0x8004A220);

    if (cameraAnim) {
        delete cameraAnim;
        cameraAnim = nullptr;
    }
    if (asyncAnimEnum != 0xFFFF) {
        if (g_characterManager) {
            g_characterManager->UnloadAnimationBatch(0, (s32)asyncAnimEnum);
        }
        asyncAnim = nullptr;
        asyncAnimEnum = 0xFFFF;
    }
}


// Camera::GetCameraVector (0x80049F88)
// Returns the camera's forward direction vector.
// PSX extracts row 2 of the 3x3 rotation matrix (forward axis),
// negates X and Z. Values in Q12 scale (4096 = 1.0).

LVector Camera::GetCameraVector() const {
    MARKFUNCTION(0x80049F88);

    LVector result;

    if (cameraAnim) {
        // PSX: GetCameraMatrix(tMatrixCamera+24), extract row 2 of Q12 MATRIX
        // Row 2 in column-major = m[2], m[6], m[10]
        const Mat4& ctw = p3dCamera.GetCameraMatrix();
        result.x = (s32)(-ctw.m[2] * 4096.0f);
        result.y = (s32)(ctw.m[6] * 4096.0f);
        result.z = (s32)(-ctw.m[10] * 4096.0f);
    }
    else {
        // PSX: p3dBuildRotMatrixZYX, extract row 2 (m[2][0..2]) from Q12 MATRIX
        // Row 2 in column-major = m[2], m[6], m[10]
        Mat4 matrix;
        p3dBuildRotMatrixZYX(camAngleX, camAngleY, camAngleZ, matrix);
        result.x = (s32)(-matrix.m[2] * 4096.0f);
        result.y = (s32)(matrix.m[6] * 4096.0f);
        result.z = (s32)(-matrix.m[10] * 4096.0f);
    }

    return result;
}
