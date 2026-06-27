#include "shadowcsm.h"

#if MODERN_GRAPHICS

#include "core.h"
#include "gen/display.h"
#include "gen/block.h"
#include "gen/camera.h"
#include "gen/envmgr.h"
#include "gen/game.h"
#include "gen/geometry.h"
#include "gen/lights.h"
#include "gen/model.h"
#include "gen/psxmath_helpers.h"
#include "gen/world.h"
#include "p3d/context.h"
#include "p3d/camera.h"
#include "p3d/p3dmath.h"
#include "p3d/vector.h"
#include "pddi/pddi.h"
#include "pddi/pddidev.h"
#include "pddi/pdditex.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

constexpr s32 kCascadeCount = 3;
constexpr s32 kCascadeResolution[2] = { 2048, 4096 }; // Medium, High
constexpr f32 kShadowDistance = 28000.0f;
constexpr f32 kSplitLambda = 0.65f;
constexpr f32 kReceiverPaddingXY = 512.0f;
constexpr f32 kCasterPaddingWorldRadius = 768.0f;
constexpr f32 kCasterPaddingWorldHeight = 7000.0f;
constexpr f32 kCasterPaddingLightNear = 12000.0f;
constexpr f32 kCasterPaddingLightFar = 3000.0f;
constexpr f32 kMinLightHorizontalLenSq = 4096.0f * 4096.0f;
constexpr f32 kCascadePaddingScale[2] = { 1.15f, 1.0f }; // Medium, High
constexpr f32 kCascadeOverlapMin = 768.0f;
constexpr f32 kCascadeOverlapMax = 2500.0f;
constexpr f32 kCascadeOverlapFraction = 0.18f;
constexpr f32 kLevelDepthPadding = 4096.0f;
constexpr f32 kMinLightDepthRange = 16384.0f;

ShadowQuality s_quality = SHADOW_QUALITY_LOW;
pddiRenderTarget* s_cascadeTargets[kCascadeCount] = {};
s32 s_cascadeTargetRes = 0;
Mat4 s_lightVP[kCascadeCount];
bool s_framePrepared = false;
s32 s_casterCount = 0;
f32 s_casterOffsetX = 0.0f;
f32 s_casterOffsetY = 0.0f;
f32 s_casterOffsetZ = 0.0f;
bool s_casterPrepass = false;
bool s_levelLightValid = false;
s32 s_levelLightID = -1;
Vec3 s_levelLightDir = {};

Vec3 TransformPoint(const Mat4& m, const Vec3& p) {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;
    Mat4TransformPoint(m, p.x, p.y, p.z, x, y, z);
    return { x, y, z };
}

Vec3 TransformDir(const Mat4& m, const Vec3& v) {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;
    Mat4TransformDir(m, v.x, v.y, v.z, x, y, z);
    return { x, y, z };
}

bool IsUsableLightDirection(const Vec3& dir) {
    const f32 lenSq = dir.MagnitudeSqr();
    const f32 horizontalLenSq = dir.x * dir.x + dir.z * dir.z;
    return lenSq > 1.0f && horizontalLenSq >= kMinLightHorizontalLenSq;
}

u32 LightBrightness(u32 colour) {
    return (colour & 0xFFu) + ((colour >> 8) & 0xFFu) + ((colour >> 16) & 0xFFu);
}

bool EnsureTargets(s32 resolution) {
    if (s_cascadeTargetRes == resolution) {
        bool allValid = true;
        for (s32 i = 0; i < kCascadeCount; i++) {
            if (!s_cascadeTargets[i] || !s_cascadeTargets[i]->IsValid()) {
                allValid = false;
                break;
            }
        }
        if (allValid) {
            return true;
        }
    }

    ShadowCSM::Shutdown();
    if (!p3d::context) {
        return false;
    }

    for (s32 i = 0; i < kCascadeCount; i++) {
        s_cascadeTargets[i] = p3d::context->CreateRenderTarget(resolution, resolution,
                                                               PDDI_RENDER_TARGET_DEPTH);
        if (!s_cascadeTargets[i]) {
            ShadowCSM::Shutdown();
            return false;
        }
    }

    s_cascadeTargetRes = resolution;
    return true;
}

void BuildFrustumCorners(const Mat4& cameraToWorld, f32 tanHalfX, f32 tanHalfY,
                         f32 nearDepth, f32 farDepth, Vec3* outCorners) {
    const Vec3 camPos = { cameraToWorld.m[12], cameraToWorld.m[13], cameraToWorld.m[14] };
    const Vec3 right = { cameraToWorld.m[0], cameraToWorld.m[1], cameraToWorld.m[2] };
    const Vec3 up = { cameraToWorld.m[4], cameraToWorld.m[5], cameraToWorld.m[6] };
    const Vec3 forward = { cameraToWorld.m[8], cameraToWorld.m[9], cameraToWorld.m[10] };

    const f32 nearX = nearDepth * tanHalfX;
    const f32 nearY = nearDepth * tanHalfY;
    const f32 farX = farDepth * tanHalfX;
    const f32 farY = farDepth * tanHalfY;

    const Vec3 nearCenter = camPos + forward * nearDepth;
    const Vec3 farCenter = camPos + forward * farDepth;

    outCorners[0] = nearCenter + right * -nearX + up * -nearY;
    outCorners[1] = nearCenter + right * nearX + up * -nearY;
    outCorners[2] = nearCenter + right * -nearX + up * nearY;
    outCorners[3] = nearCenter + right * nearX + up * nearY;
    outCorners[4] = farCenter + right * -farX + up * -farY;
    outCorners[5] = farCenter + right * farX + up * -farY;
    outCorners[6] = farCenter + right * -farX + up * farY;
    outCorners[7] = farCenter + right * farX + up * farY;
}

f32 ComputeCascadeSplit(f32 nearDepth, f32 farDepth, s32 cascadeIndex) {
    const f32 ratio = (f32)cascadeIndex / (f32)kCascadeCount;
    const f32 logSplit = nearDepth * std::pow(farDepth / nearDepth, ratio);
    const f32 uniformSplit = nearDepth + (farDepth - nearDepth) * ratio;
    return (logSplit * kSplitLambda) + (uniformSplit * (1.0f - kSplitLambda));
}

f32 ComputeCascadeOverlap(f32 startDepth, f32 endDepth) {
    const f32 range = std::max(endDepth - startDepth, 1.0f);
    return std::min(std::max(range * kCascadeOverlapFraction, kCascadeOverlapMin), kCascadeOverlapMax);
}

bool ExpandLightDepthToLevelBounds(const Mat4& lightView, f32* minZ, f32* maxZ) {
    if (!minZ || !maxZ || !g_game) {
        return false;
    }

    const World* world = g_game->GetWorld();
    if (!world) {
        return false;
    }

    const LVector& levelMin = world->GetLevelMin();
    const LVector& levelMax = world->GetLevelMax();
    if (levelMin.x > levelMax.x || levelMin.y > levelMax.y || levelMin.z > levelMax.z) {
        return false;
    }

    const Vec3 corners[8] = {
        { (f32)levelMin.x, (f32)levelMin.y, (f32)levelMin.z },
        { (f32)levelMax.x, (f32)levelMin.y, (f32)levelMin.z },
        { (f32)levelMin.x, (f32)levelMax.y, (f32)levelMin.z },
        { (f32)levelMax.x, (f32)levelMax.y, (f32)levelMin.z },
        { (f32)levelMin.x, (f32)levelMin.y, (f32)levelMax.z },
        { (f32)levelMax.x, (f32)levelMin.y, (f32)levelMax.z },
        { (f32)levelMin.x, (f32)levelMax.y, (f32)levelMax.z },
        { (f32)levelMax.x, (f32)levelMax.y, (f32)levelMax.z },
    };

    for (const Vec3& corner : corners) {
        const Vec3 lp = TransformPoint(lightView, corner);
        *minZ = std::min(*minZ, lp.z - kLevelDepthPadding);
        *maxZ = std::max(*maxZ, lp.z + kLevelDepthPadding);
    }
    return true;
}

void ComputeLightDepthPlanes(f32 minZ, f32 maxZ, bool includesLevelBounds,
                             f32* nearDist, f32* farDist) {
    if (!nearDist || !farDist) {
        return;
    }

    if (minZ > maxZ) {
        minZ = -kMinLightDepthRange;
        maxZ = -1.0f;
    }

    // Orthographic shadow maps can legitimately have a near plane behind the
    // light eye. This prevents large level geometry from being clipped when it
    // lies outside the old camera-slice-only depth interval.
    f32 nearPlane = -maxZ;
    f32 farPlane = -minZ;
    if (!includesLevelBounds) {
        nearPlane = std::max(1.0f, nearPlane);
    }

    if (farPlane < nearPlane + kMinLightDepthRange) {
        const f32 center = (nearPlane + farPlane) * 0.5f;
        nearPlane = center - kMinLightDepthRange * 0.5f;
        farPlane = center + kMinLightDepthRange * 0.5f;
        if (!includesLevelBounds && nearPlane < 1.0f) {
            farPlane += 1.0f - nearPlane;
            nearPlane = 1.0f;
        }
    }

    *nearDist = nearPlane;
    *farDist = std::max(farPlane, nearPlane + 1.0f);
}

void SnapCascadeBounds(f32* minX, f32* maxX, f32* minY, f32* maxY, s32 resolution) {
    if (!minX || !maxX || !minY || !maxY || resolution <= 0) {
        return;
    }

    const f32 width = *maxX - *minX;
    const f32 height = *maxY - *minY;
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }

    const f32 texelSizeX = width / (f32)resolution;
    const f32 texelSizeY = height / (f32)resolution;
    if (texelSizeX <= 0.0f || texelSizeY <= 0.0f) {
        return;
    }

    const f32 halfWidth = width * 0.5f;
    const f32 halfHeight = height * 0.5f;
    f32 centerX = (*minX + *maxX) * 0.5f;
    f32 centerY = (*minY + *maxY) * 0.5f;
    centerX = std::floor(centerX / texelSizeX) * texelSizeX;
    centerY = std::floor(centerY / texelSizeY) * texelSizeY;

    *minX = centerX - halfWidth;
    *maxX = centerX + halfWidth;
    *minY = centerY - halfHeight;
    *maxY = centerY + halfHeight;
}

Mat4 OrthoReversedZ(f32 left, f32 right, f32 bottom, f32 top, f32 nearDist, f32 farDist) {
    Mat4 o;
    o.m[0] = 2.0f / (right - left);
    o.m[5] = 2.0f / (top - bottom);
    // glClipControl(GL_ZERO_TO_ONE) means clip-space Z is depth-space Z.
    // Map z=-near to 1 and z=-far to 0 for the renderer's reversed-Z path.
    o.m[10] = 1.0f / (farDist - nearDist);
    o.m[12] = -(right + left) / (right - left);
    o.m[13] = -(top + bottom) / (top - bottom);
    o.m[14] = farDist / (farDist - nearDist);
    o.m[15] = 1.0f;
    return o;
}

Vec3 GetShadowLightDirection() {
    const World* world = (g_game != nullptr) ? g_game->GetWorld() : nullptr;
    const s32 levelID = world ? world->GetCurLevelID() : -1;
    if (s_levelLightValid && s_levelLightID == levelID) {
        return s_levelLightDir;
    }

    Vec3 selected = {};
    u32 selectedBrightness = 0;
    if (g_environmentManager) {
        for (s32 i = 0; i < 3; i++) {
            const HardwareLight* light = &g_environmentManager->lighting.originalLights[i];
            Vec3 authored = {
                static_cast<f32>(light->directionX),
                static_cast<f32>(light->directionY),
                static_cast<f32>(light->directionZ),
            };
            const u32 brightness = LightBrightness(light->colour);
            if (brightness > selectedBrightness && IsUsableLightDirection(authored)) {
                selected = authored;
                selectedBrightness = brightness;
            }
        }
    }

    if (selectedBrightness == 0) {
        selected = { 0.35f, -0.85f, 0.25f };
    }

    s_levelLightDir = selected.Normalized();
    if (s_levelLightDir.MagnitudeSqr() <= 0.00001f) {
        s_levelLightDir = { 0.0f, -1.0f, 0.0f };
    }
    s_levelLightID = levelID;
    s_levelLightValid = true;
    return s_levelLightDir;
}

ShadowQuality ShadowCSM::GetQuality() {
    return s_quality;
}

void ShadowCSM::SetQuality(ShadowQuality quality) {
    if (quality < SHADOW_QUALITY_LOW || quality > SHADOW_QUALITY_HIGH) {
        quality = SHADOW_QUALITY_LOW;
    }

    const ShadowQuality previousQuality = s_quality;
    s_quality = quality;

    if (s_quality == SHADOW_QUALITY_LOW) {
        Shutdown();
        if (p3d::context) {
            p3d::context->SetShadowCascades(nullptr, nullptr, nullptr, 0);
            p3d::context->SetReceiveShadows(false);
            p3d::context->SetShadowCasterPass(false, Mat4());
        }
        return;
    }

    if (previousQuality != s_quality) {
        Shutdown();
    }
}

void ShadowCSM::Shutdown() {
    for (s32 i = 0; i < kCascadeCount; i++) {
        if (s_cascadeTargets[i]) {
            s_cascadeTargets[i]->Release();
            s_cascadeTargets[i] = nullptr;
        }
    }
    s_cascadeTargetRes = 0;
    s_framePrepared = false;
    s_casterPrepass = false;
}

void ShadowCSM::BeginFrame() {
    s_framePrepared = false;
    s_casterPrepass = false;
    s_casterCount = 0;

    if (s_quality == SHADOW_QUALITY_LOW) {
        if (p3d::context) {
            p3d::context->SetShadowCascades(nullptr, nullptr, nullptr, 0);
            p3d::context->SetReceiveShadows(false);
            p3d::context->SetShadowCasterPass(false, Mat4());
        }
        return;
    }
    if (!p3d::context || !p3d::device) {
        return;
    }
    if (!g_display || !g_display->GetCamera() || !g_environmentManager) {
        return;
    }

    const s32 resolution = kCascadeResolution[(s_quality == SHADOW_QUALITY_HIGH) ? 1 : 0];
    const f32 paddingScale = kCascadePaddingScale[(s_quality == SHADOW_QUALITY_HIGH) ? 1 : 0];
    if (!EnsureTargets(resolution)) {
        return;
    }

    Camera* camera = g_display->GetCamera();
    tMatrixCamera* p3dCam = camera->GetP3DCamera();
    if (!p3dCam) {
        return;
    }

    const Mat4& cameraToWorld = p3dCam->GetCameraMatrix();
    const LVector& camPos = camera->GetPosition();
    const f32 camX = (f32)camPos.x, camY = (f32)camPos.y, camZ = (f32)camPos.z;

    ChanProjectionState projState = g_display->GetChanProjectionState();
    f32 tanHalfX = (projState.projectionDistanceX > 0.0f)
        ? ((f32)projState.centerX / projState.projectionDistanceX)
        : 0.45f;
    f32 tanHalfY = (projState.projectionDistanceY > 0.0f)
        ? ((f32)projState.centerY / projState.projectionDistanceY)
        : 0.35f;
    tanHalfX = std::max(tanHalfX, 0.0001f);
    tanHalfY = std::max(tanHalfY, 0.0001f);

    const f32 cameraNear = std::max(p3dCam->GetNearPlane(), 8.0f);
    const f32 cameraFar = std::max(cameraNear + 1.0f, p3dCam->GetFarPlane());
    const f32 shadowFar = std::min(cameraFar, kShadowDistance);

    const Vec3 lightDir = GetShadowLightDirection();
    const f32 lx = lightDir.x, ly = lightDir.y, lz = lightDir.z;

    // LookAt's up vector must not be parallel to the light direction.
    f32 upX = 0.0f, upY = 1.0f, upZ = 0.0f;
    if (std::fabs(ly) > 0.999f) {
        upX = 0.0f; upY = 0.0f; upZ = 1.0f;
    }

    f32 splits[kCascadeCount];
    f32 splitDepths[kCascadeCount + 1];
    splitDepths[0] = cameraNear;
    for (s32 i = 1; i < kCascadeCount; i++) {
        splitDepths[i] = ComputeCascadeSplit(cameraNear, shadowFar, i);
    }
    splitDepths[kCascadeCount] = shadowFar;

    for (s32 i = 0; i < kCascadeCount; i++) {
        splits[i] = splitDepths[i + 1];

        const f32 cascadeOverlap = ComputeCascadeOverlap(splitDepths[i], splitDepths[i + 1]);
        const f32 fitNearDepth = (i > 0)
            ? std::max(cameraNear, splitDepths[i] - cascadeOverlap)
            : splitDepths[i];
        const f32 fitFarDepth = (i < kCascadeCount - 1)
            ? std::min(shadowFar, splitDepths[i + 1] + cascadeOverlap)
            : splitDepths[i + 1];

        Vec3 corners[8];
        BuildFrustumCorners(cameraToWorld, tanHalfX, tanHalfY,
                            fitNearDepth, fitFarDepth, corners);

        Vec3 center = {};
        for (const Vec3& corner : corners) {
            center += corner;
        }
        center *= 1.0f / 8.0f;

        const f32 lightDistance = kShadowDistance + kCasterPaddingLightNear;
        const Vec3 eye = center - Vec3(lx, ly, lz) * lightDistance;
        Mat4 lightView = LookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, upX, upY, upZ);
        const Vec3 casterHeightInLight = TransformDir(lightView, { 0.0f, kCasterPaddingWorldHeight, 0.0f });
        const f32 casterPadX = (kReceiverPaddingXY + kCasterPaddingWorldRadius + std::fabs(casterHeightInLight.x)) * paddingScale;
        const f32 casterPadY = (kReceiverPaddingXY + kCasterPaddingWorldRadius + std::fabs(casterHeightInLight.y)) * paddingScale;
        const f32 casterPadZ = (kCasterPaddingWorldRadius + std::fabs(casterHeightInLight.z)) * paddingScale;

        f32 minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
        f32 maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
        for (const Vec3& corner : corners) {
            const Vec3 lp = TransformPoint(lightView, corner);
            minX = std::min(minX, lp.x);
            minY = std::min(minY, lp.y);
            minZ = std::min(minZ, lp.z);
            maxX = std::max(maxX, lp.x);
            maxY = std::max(maxY, lp.y);
            maxZ = std::max(maxZ, lp.z);
        }

        minX -= casterPadX;
        minY -= casterPadY;
        maxX += casterPadX;
        maxY += casterPadY;
        minZ -= kCasterPaddingLightNear + casterPadZ;
        maxZ += kCasterPaddingLightFar + casterPadZ;
        const bool includesLevelBounds = ExpandLightDepthToLevelBounds(lightView, &minZ, &maxZ);

        SnapCascadeBounds(&minX, &maxX, &minY, &maxY, resolution);

        // The renderer uses reversed-Z with GL_ZERO_TO_ONE clip space
        // (near=1, far=0), matching the main PerspectiveReversedZ path.
        f32 nearDist = 1.0f;
        f32 farDist = kMinLightDepthRange;
        ComputeLightDepthPlanes(minZ, maxZ, includesLevelBounds, &nearDist, &farDist);
        Mat4 lightProj = OrthoReversedZ(minX, maxX, minY, maxY, nearDist, farDist);
        s_lightVP[i] = lightProj * lightView;

        p3d::context->SetRenderTarget(s_cascadeTargets[i]);
        // glDepthMask is only ever toggled inside SetBlendMode (see glContext::SetBlendMode);
        // force it back to enabled here since glClear(DEPTH) and the caster draws below
        // are otherwise silently no-ops if some earlier alpha-blended draw left it disabled.
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        p3d::context->Clear(PDDI_BUFFER_DEPTH);
        p3d::context->SetRenderTarget(nullptr);
    }

    pddiTexture* depthTextures[kCascadeCount];
    for (s32 i = 0; i < kCascadeCount; i++) {
        depthTextures[i] = s_cascadeTargets[i]->GetTexture();
    }

    p3d::context->SetShadowCascades(depthTextures, s_lightVP, splits, kCascadeCount);
    p3d::context->SetCameraWorldPos(camX, camY, camZ);

    s_framePrepared = true;
}

bool ShadowCSM::IsFramePrepared() {
    return s_framePrepared;
}

s32 ShadowCSM::GetCascadeResolution() {
    return s_cascadeTargetRes;
}

s32 ShadowCSM::GetCasterCount() {
    return s_casterCount;
}

void ShadowCSM::SetCasterWorldOffset(f32 x, f32 y, f32 z) {
    s_casterOffsetX = x;
    s_casterOffsetY = y;
    s_casterOffsetZ = z;
}

void ShadowCSM::BeginCasterPrepass() {
    s_casterPrepass = s_framePrepared;
}

void ShadowCSM::EndCasterPrepass() {
    s_casterPrepass = false;
    if (p3d::context) {
        p3d::context->SetRenderTarget(nullptr);
        p3d::context->SetShadowCasterPass(false, Mat4());
        p3d::context->SetReceiveShadows(false);
        p3d::context->EnableZBuffer(true);
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
    }
}

bool ShadowCSM::IsCasterPrepass() {
    return s_casterPrepass;
}

u32 ShadowCSM::GetCascadeTextureHandle(s32 index) {
    if (index < 0 || index >= kCascadeCount || !s_cascadeTargets[index]) {
        return 0;
    }
    pddiTexture* tex = s_cascadeTargets[index]->GetTexture();
    return tex ? tex->GetNativeHandle() : 0;
}

void ShadowCSM::DrawCasterIntoCascades(DrawableBasic* drawable, u32 flags) {
    if (!s_framePrepared || !s_casterPrepass || !drawable || !p3d::context) {
        return;
    }

    s_casterCount++;

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    Mat4 casterWorld = savedWorld;
    casterWorld.SetTranslation(savedWorld.GetTransX() + s_casterOffsetX,
                               savedWorld.GetTransY() + s_casterOffsetY,
                               savedWorld.GetTransZ() + s_casterOffsetZ);

    for (s32 i = 0; i < kCascadeCount; i++) {
        p3d::context->SetRenderTarget(s_cascadeTargets[i]);
        // See the matching comment in BeginFrame: must explicitly re-enable
        // depth writes, they're not implied by EnableZBuffer/SetRenderTarget.
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        p3d::context->EnableZBuffer(true);
        p3d::context->SetCullMode(PDDI_CULL_NONE);
        p3d::context->SetShadowCasterPass(true, s_lightVP[i]);
        p3d::context->SetWorldMatrix(casterWorld);

        drawable->Display(flags);

        p3d::context->SetShadowCasterPass(false, Mat4());
        p3d::context->SetRenderTarget(nullptr);
    }

    p3d::context->SetWorldMatrix(savedWorld);
}

void ShadowCSM::DrawCasterPrimBufferIntoCascades(pddiPrimBuffer* buffer) {
    if (!s_framePrepared || !s_casterPrepass || !buffer || !p3d::context) {
        return;
    }

    s_casterCount++;

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    Mat4 casterWorld = savedWorld;
    casterWorld.SetTranslation(savedWorld.GetTransX() + s_casterOffsetX,
                               savedWorld.GetTransY() + s_casterOffsetY,
                               savedWorld.GetTransZ() + s_casterOffsetZ);

    for (s32 i = 0; i < kCascadeCount; i++) {
        p3d::context->SetRenderTarget(s_cascadeTargets[i]);
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        p3d::context->EnableZBuffer(true);
        p3d::context->SetCullMode(PDDI_CULL_NONE);
        p3d::context->SetShadowCasterPass(true, s_lightVP[i]);
        p3d::context->SetWorldMatrix(casterWorld);

        p3d::context->DrawPrimBuffer(buffer);

        p3d::context->SetShadowCasterPass(false, Mat4());
        p3d::context->SetRenderTarget(nullptr);
    }

    p3d::context->SetWorldMatrix(savedWorld);
}

void ShadowCSM::DrawBlockCasterIntoCascades(Block* block, const LVector* drawPos) {
    if (!s_framePrepared || !s_casterPrepass || !block || !drawPos || !block->primGeom || !p3d::context) {
        return;
    }

    pddiPrimBuffer* buffer = BuildUnculledPrimBufferFromPrimGeom(block->primGeom);
    if (!buffer) {
        return;
    }

    s_casterCount++;

    const Mat4 savedWorld = p3d::context->GetWorldMatrix();
    Mat4 blockWorld;
    blockWorld.SetTranslation(static_cast<f32>(drawPos->x),
                              static_cast<f32>(drawPos->y),
                              static_cast<f32>(drawPos->z));

    for (s32 i = 0; i < kCascadeCount; i++) {
        p3d::context->SetRenderTarget(s_cascadeTargets[i]);
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        p3d::context->EnableZBuffer(true);
        p3d::context->SetCullMode(PDDI_CULL_NONE);
        p3d::context->SetShadowCasterPass(true, s_lightVP[i]);
        p3d::context->SetWorldMatrix(blockWorld);

        p3d::context->DrawPrimBuffer(buffer);

        p3d::context->SetShadowCasterPass(false, Mat4());
        p3d::context->SetRenderTarget(nullptr);
    }

    buffer->Release();
    p3d::context->SetWorldMatrix(savedWorld);
}

#endif // MODERN_GRAPHICS
