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
#include <vector>

constexpr s32 kCascadeCount = 3;

// Indexed by (ShadowQuality - 1): 0 = Low, 1 = Medium, 2 = High, 3 = Very High.
// SHADOW_QUALITY_OFF never reaches these tables (BeginFrame/SetQuality bail
// out to the legacy blob shadow before any of this is touched).
constexpr s32 kQualityTierCount = 4;
constexpr s32 kActiveCascadeCount[kQualityTierCount] = { 2, 3, 3, 3 };
constexpr s32 kCascadeResolution[kQualityTierCount] = { 1024, 2048, 4096, 8192 };
constexpr f32 kShadowDistance[kQualityTierCount] = { 20000.0f, 28000.0f, 28000.0f, 28000.0f };
constexpr f32 kSplitLambda[kQualityTierCount] = { 0.6f, 0.65f, 0.65f, 0.65f };
// Receiver/caster frustum padding tightens as resolution climbs, raising
// effective texel density for crisper cascade edges; Low keeps the most
// generous padding since it's coarse enough that the extra slack doesn't show.
constexpr f32 kReceiverPaddingXY[kQualityTierCount] = { 512.0f, 384.0f, 320.0f, 256.0f };
constexpr f32 kCasterPaddingWorldRadius[kQualityTierCount] = { 768.0f, 640.0f, 576.0f, 512.0f };
constexpr f32 kCasterPaddingWorldHeight[kQualityTierCount] = { 7000.0f, 7000.0f, 7000.0f, 7000.0f };
constexpr f32 kCasterPaddingLightNear[kQualityTierCount] = { 12000.0f, 12000.0f, 12000.0f, 12000.0f };
constexpr f32 kCasterPaddingLightFar[kQualityTierCount] = { 3000.0f, 3000.0f, 3000.0f, 3000.0f };
constexpr f32 kMinLightHorizontalLenSq = 4096.0f * 4096.0f;
constexpr f32 kCascadePaddingScale[kQualityTierCount] = { 1.25f, 1.15f, 1.0f, 0.95f };
constexpr f32 kCascadeOverlapMin[kQualityTierCount] = { 768.0f, 768.0f, 768.0f, 768.0f };
constexpr f32 kCascadeOverlapMax[kQualityTierCount] = { 2500.0f, 2500.0f, 2500.0f, 2500.0f };
constexpr f32 kCascadeOverlapFraction[kQualityTierCount] = { 0.18f, 0.18f, 0.18f, 0.18f };
constexpr f32 kLevelDepthPadding[kQualityTierCount] = { 4096.0f, 4096.0f, 4096.0f, 4096.0f };
constexpr f32 kMinLightDepthRange[kQualityTierCount] = { 16384.0f, 16384.0f, 16384.0f, 16384.0f };
constexpr pddiRenderTargetFormat kCascadeDepthFormat = PDDI_RENDER_TARGET_DEPTH32F;
// Caster-side polygon offset (glPolygonOffset factor/units) pushes shadow
// casters back in light space to suppress acne. Shrinks as resolution climbs
// (finer texels need less push to avoid acne) to cut peter-panning; Low
// keeps the strongest push since its coarse texels are the most acne-prone.
constexpr f32 kShadowCasterOffsetFactor[kQualityTierCount] = { -2.75f, -1.6f, -1.2f, -1.0f };
constexpr f32 kShadowCasterOffsetUnits[kQualityTierCount] = { -14.0f, -8.0f, -5.0f, -4.0f };
constexpr f32 kMinShadowLightDown = 0.92f;

ShadowQuality s_quality = SHADOW_QUALITY_OFF;
pddiRenderTarget* s_cascadeTargets[kCascadeCount] = {};
s32 s_cascadeTargetRes = 0;
s32 s_activeCascadeCount = 0;
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

struct QueuedBlockCaster {
    pddiPrimBuffer* buffer;
    Vec3 worldMin;
    Vec3 worldMax;
    f32 tx, ty, tz;
};
std::vector<QueuedBlockCaster> s_queuedBlockCasters;

Vec3 TransformPoint(const Mat4& m, const Vec3& p) {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;
    Mat4TransformPoint(m, p.x, p.y, p.z, x, y, z);
    return { x, y, z };
}

bool CasterBoundsOverlapCascade(s32 cascadeIndex, const Vec3& worldMin, const Vec3& worldMax) {
    const Vec3 corners[8] = {
        { worldMin.x, worldMin.y, worldMin.z }, { worldMax.x, worldMin.y, worldMin.z },
        { worldMin.x, worldMax.y, worldMin.z }, { worldMax.x, worldMax.y, worldMin.z },
        { worldMin.x, worldMin.y, worldMax.z }, { worldMax.x, worldMin.y, worldMax.z },
        { worldMin.x, worldMax.y, worldMax.z }, { worldMax.x, worldMax.y, worldMax.z },
    };

    f32 minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    f32 maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
    for (const Vec3& corner : corners) {
        const Vec3 lp = TransformPoint(s_lightVP[cascadeIndex], corner);
        minX = std::min(minX, lp.x);
        minY = std::min(minY, lp.y);
        minZ = std::min(minZ, lp.z);
        maxX = std::max(maxX, lp.x);
        maxY = std::max(maxY, lp.y);
        maxZ = std::max(maxZ, lp.z);
    }

    return maxX >= -1.0f && minX <= 1.0f
        && maxY >= -1.0f && minY <= 1.0f
        && maxZ >= 0.0f && minZ <= 1.0f;
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

Vec3 ClampShadowLightTopDown(const Vec3& dir) {
    Vec3 normalized = dir.Normalized();
    if (normalized.MagnitudeSqr() <= 0.00001f) {
        normalized = { 0.35f, -0.85f, 0.25f };
    }

    if (normalized.y <= -kMinShadowLightDown) {
        return normalized;
    }

    Vec3 horizontal = { normalized.x, 0.0f, normalized.z };
    if (horizontal.MagnitudeSqr() <= 0.00001f) {
        horizontal = { 0.35f, 0.0f, 0.25f };
    }
    horizontal = horizontal.Normalized();

    const f32 horizontalScale = std::sqrt(std::max(1.0f - kMinShadowLightDown * kMinShadowLightDown, 0.0f));
    return Vec3(horizontal.x * horizontalScale,
                -kMinShadowLightDown,
                horizontal.z * horizontalScale).Normalized();
}

bool EnsureTargets(s32 resolution, s32 activeCascadeCount) {
    activeCascadeCount = std::min(std::max(activeCascadeCount, 1), kCascadeCount);
    if (s_cascadeTargetRes == resolution && s_activeCascadeCount == activeCascadeCount) {
        bool allValid = true;
        for (s32 i = 0; i < activeCascadeCount; i++) {
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

    for (s32 i = 0; i < activeCascadeCount; i++) {
        s_cascadeTargets[i] = p3d::context->CreateRenderTarget(resolution, resolution,
                                                               kCascadeDepthFormat,
                                                               /*withInstanceId=*/true);
        if (!s_cascadeTargets[i]) {
            ShadowCSM::Shutdown();
            return false;
        }
    }

    s_cascadeTargetRes = resolution;
    s_activeCascadeCount = activeCascadeCount;
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

f32 ComputeCascadeSplit(f32 nearDepth, f32 farDepth, s32 cascadeIndex,
                        s32 activeCascadeCount, s32 qualityIndex) {
    const f32 ratio = (f32)cascadeIndex / (f32)activeCascadeCount;
    const f32 logSplit = nearDepth * std::pow(farDepth / nearDepth, ratio);
    const f32 uniformSplit = nearDepth + (farDepth - nearDepth) * ratio;
    return (logSplit * kSplitLambda[qualityIndex]) + (uniformSplit * (1.0f - kSplitLambda[qualityIndex]));
}

f32 ComputeCascadeOverlap(f32 startDepth, f32 endDepth, s32 qualityIndex) {
    const f32 range = std::max(endDepth - startDepth, 1.0f);
    return std::min(std::max(range * kCascadeOverlapFraction[qualityIndex],
                             kCascadeOverlapMin[qualityIndex]),
                    kCascadeOverlapMax[qualityIndex]);
}

bool ExpandLightDepthRangeToLevelBounds(const Mat4& lightView,
                                        f32* minZ, f32* maxZ,
                                        s32 qualityIndex) {
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
        *minZ = std::min(*minZ, lp.z - kLevelDepthPadding[qualityIndex]);
        *maxZ = std::max(*maxZ, lp.z + kLevelDepthPadding[qualityIndex]);
    }
    return true;
}

void ComputeLightDepthPlanes(f32 minZ, f32 maxZ, bool includesLevelBounds,
                             f32* nearDist, f32* farDist, s32 qualityIndex) {
    if (!nearDist || !farDist) {
        return;
    }

    if (minZ > maxZ) {
        minZ = -kMinLightDepthRange[qualityIndex];
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

    const f32 minLightDepthRange = kMinLightDepthRange[qualityIndex];
    if (farPlane < nearPlane + minLightDepthRange) {
        const f32 center = (nearPlane + farPlane) * 0.5f;
        nearPlane = center - minLightDepthRange * 0.5f;
        farPlane = center + minLightDepthRange * 0.5f;
        if (!includesLevelBounds && nearPlane < 1.0f) {
            farPlane += 1.0f - nearPlane;
            nearPlane = 1.0f;
        }
    }

    *nearDist = nearPlane;
    *farDist = std::max(farPlane, nearPlane + 1.0f);
}

void FitStableSquareBounds(f32* minX, f32* maxX, f32* minY, f32* maxY) {
    if (!minX || !maxX || !minY || !maxY) {
        return;
    }

    const f32 width = *maxX - *minX;
    const f32 height = *maxY - *minY;
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }

    const f32 centerX = (*minX + *maxX) * 0.5f;
    const f32 centerY = (*minY + *maxY) * 0.5f;
    const f32 halfExtent = std::max(width, height) * 0.5f;
    *minX = centerX - halfExtent;
    *maxX = centerX + halfExtent;
    *minY = centerY - halfExtent;
    *maxY = centerY + halfExtent;
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

    s_levelLightDir = ClampShadowLightTopDown(selected);
    s_levelLightID = levelID;
    s_levelLightValid = true;
    return s_levelLightDir;
}

ShadowQuality ShadowCSM::GetQuality() {
    return s_quality;
}

void ShadowCSM::SetQuality(ShadowQuality quality) {
    if (quality < SHADOW_QUALITY_OFF || quality > SHADOW_QUALITY_VERY_HIGH) {
        quality = SHADOW_QUALITY_OFF;
    }

#if defined(RC_PLATFORM_SWITCH)
    // Shadows are force-disabled on Switch, regardless of the persisted
    // setting or menu selection. The cascade shadow map is by far the
    // newest/heaviest 3D subsystem: per-cascade depth + R32UI instance-id
    // render targets (up to 8192x8192 = ~1.5GB at VERY_HIGH, which the
    // Switch's 4GB shared RAM can't back), a whole extra shadow-caster
    // render pass, and the instance-id path whose GLES port drives a
    // float fragment output into an integer colour attachment -- an invalid
    // GL state that mesa's Switch driver can abort() on (a guest-side fatal).
    // Cutting shadows here keeps the first working Switch 3D scene to the
    // core geometry+VRAM-texture path; re-enabling is a later, separate task.
    quality = SHADOW_QUALITY_OFF;
#endif

    const ShadowQuality previousQuality = s_quality;
    s_quality = quality;

    if (s_quality == SHADOW_QUALITY_OFF) {
        Shutdown();
        if (p3d::context) {
            p3d::context->SetShadowCascades(nullptr, nullptr, nullptr, nullptr, nullptr, 0);
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
    s_activeCascadeCount = 0;
    s_framePrepared = false;
    s_casterPrepass = false;
}

void ShadowCSM::BeginFrame() {
    s_framePrepared = false;
    s_casterPrepass = false;
    s_casterCount = 0;

    if (s_quality == SHADOW_QUALITY_OFF) {
        if (p3d::context) {
            p3d::context->SetShadowCascades(nullptr, nullptr, nullptr, nullptr, nullptr, 0);
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

    const s32 qualityIndex = (s32)s_quality - 1;
    const s32 activeCascadeCount = kActiveCascadeCount[qualityIndex];
    const s32 resolution = kCascadeResolution[qualityIndex];
    const f32 paddingScale = kCascadePaddingScale[qualityIndex];
    if (!EnsureTargets(resolution, activeCascadeCount)) {
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
    const f32 shadowFar = std::min(cameraFar, kShadowDistance[qualityIndex]);

    const Vec3 lightDir = GetShadowLightDirection();
    const f32 lx = lightDir.x, ly = lightDir.y, lz = lightDir.z;

    // LookAt's up vector must not be parallel to the light direction.
    f32 upX = 0.0f, upY = 1.0f, upZ = 0.0f;
    if (std::fabs(ly) > 0.999f) {
        upX = 0.0f; upY = 0.0f; upZ = 1.0f;
    }

    f32 splits[kCascadeCount];
    f32 texelWorldSizes[kCascadeCount] = {};
    f32 splitDepths[kCascadeCount + 1];
    splitDepths[0] = cameraNear;
    for (s32 i = 1; i < activeCascadeCount; i++) {
        splitDepths[i] = ComputeCascadeSplit(cameraNear, shadowFar, i,
                                             activeCascadeCount, qualityIndex);
    }
    splitDepths[activeCascadeCount] = shadowFar;

    for (s32 i = 0; i < activeCascadeCount; i++) {
        splits[i] = splitDepths[i + 1];

        const f32 cascadeOverlap = ComputeCascadeOverlap(splitDepths[i], splitDepths[i + 1], qualityIndex);
        const f32 fitNearDepth = (i > 0)
            ? std::max(cameraNear, splitDepths[i] - cascadeOverlap)
            : splitDepths[i];
        const f32 fitFarDepth = (i < activeCascadeCount - 1)
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

        const f32 lightDistance = kShadowDistance[qualityIndex] + kCasterPaddingLightNear[qualityIndex];
        const Vec3 eye = center - Vec3(lx, ly, lz) * lightDistance;
        Mat4 lightView = LookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, upX, upY, upZ);
        const Vec3 casterHeightInLight = TransformDir(lightView, { 0.0f, kCasterPaddingWorldHeight[qualityIndex], 0.0f });
        const f32 casterPadX = (kReceiverPaddingXY[qualityIndex] + kCasterPaddingWorldRadius[qualityIndex] + std::fabs(casterHeightInLight.x)) * paddingScale;
        const f32 casterPadY = (kReceiverPaddingXY[qualityIndex] + kCasterPaddingWorldRadius[qualityIndex] + std::fabs(casterHeightInLight.y)) * paddingScale;
        const f32 casterPadZ = (kCasterPaddingWorldRadius[qualityIndex] + std::fabs(casterHeightInLight.z)) * paddingScale;

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
        minZ -= kCasterPaddingLightNear[qualityIndex] + casterPadZ;
        maxZ += kCasterPaddingLightFar[qualityIndex] + casterPadZ;
        const bool includesLevelBounds = ExpandLightDepthRangeToLevelBounds(lightView,
                                                                            &minZ, &maxZ,
                                                                            qualityIndex);

        FitStableSquareBounds(&minX, &maxX, &minY, &maxY);
        SnapCascadeBounds(&minX, &maxX, &minY, &maxY, resolution);
        texelWorldSizes[i] = (maxX - minX) / (f32)std::max(resolution, 1);

        // The renderer uses reversed-Z with GL_ZERO_TO_ONE clip space
        // (near=1, far=0), matching the main PerspectiveReversedZ path.
        f32 nearDist = 1.0f;
        f32 farDist = kMinLightDepthRange[qualityIndex];
        ComputeLightDepthPlanes(minZ, maxZ, includesLevelBounds, &nearDist, &farDist, qualityIndex);
        Mat4 lightProj = OrthoReversedZ(minX, maxX, minY, maxY, nearDist, farDist);
        s_lightVP[i] = lightProj * lightView;

        p3d::context->SetRenderTarget(s_cascadeTargets[i]);
        // glDepthMask is only ever toggled inside SetBlendMode (see glContext::SetBlendMode);
        // force it back to enabled here since glClear(DEPTH) and the caster draws below
        // are otherwise silently no-ops if some earlier alpha-blended draw left it disabled.
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        p3d::context->Clear(PDDI_BUFFER_DEPTH);
        p3d::context->ClearShadowCasterIdTarget();
        p3d::context->SetRenderTarget(nullptr);
    }

    pddiTexture* depthTextures[kCascadeCount];
    pddiTexture* idTextures[kCascadeCount];
    for (s32 i = 0; i < activeCascadeCount; i++) {
        depthTextures[i] = s_cascadeTargets[i]->GetTexture();
        idTextures[i] = s_cascadeTargets[i]->GetIdTexture();
    }

    p3d::context->SetShadowCascades(depthTextures, s_lightVP, splits, texelWorldSizes, idTextures, activeCascadeCount);
    p3d::context->SetCameraWorldPos(camX, camY, camZ);
    // uShadowLightDir wants the direction FROM a surface TOWARD the light;
    // lightDir here is the direction the light travels (surface-ward), so negate it.
    p3d::context->SetShadowLightDirection(-lx, -ly, -lz);

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

const char* ShadowCSM::GetCascadeDepthFormatName() {
    return (kCascadeDepthFormat == PDDI_RENDER_TARGET_DEPTH32F) ? "D32F" : "D24";
}

const char* ShadowCSM::GetFilterQualityName() {
    if (s_quality == SHADOW_QUALITY_VERY_HIGH) {
        return "3 cascades, rotated Poisson PCF (1.2 texel radius)";
    }
    if (s_quality == SHADOW_QUALITY_HIGH) {
        return "3 cascades, rotated Poisson PCF (1.3 texel radius)";
    }
    if (s_quality == SHADOW_QUALITY_MEDIUM) {
        return "3 cascades, rotated Poisson PCF (1.6 texel radius)";
    }
    if (s_quality == SHADOW_QUALITY_LOW) {
        return "2 cascades, rotated Poisson PCF (2.6 texel radius)";
    }
    return "legacy blob";
}

void BeginShadowCasterDepthBias() {
    if (!p3d::context) {
        return;
    }

    const s32 qualityIndex = (s_quality > SHADOW_QUALITY_OFF) ? (s32)s_quality - 1 : 0;
    p3d::context->SetPolygonOffset(true,
                                   kShadowCasterOffsetFactor[qualityIndex],
                                   kShadowCasterOffsetUnits[qualityIndex]);
}

void EndShadowCasterDepthBias() {
    if (p3d::context) {
        p3d::context->SetPolygonOffset(false);
    }
}

void ShadowCSM::SetCasterWorldOffset(f32 x, f32 y, f32 z) {
    s_casterOffsetX = x;
    s_casterOffsetY = y;
    s_casterOffsetZ = z;
}

void ShadowCSM::BeginCasterPrepass() {
    s_casterPrepass = s_framePrepared;
    s_queuedBlockCasters.clear();
}

void ShadowCSM::EndCasterPrepass() {
    s_casterPrepass = false;
    if (!p3d::context) {
        s_queuedBlockCasters.clear();
        return;
    }

    if (!s_queuedBlockCasters.empty()) {
        p3d::context->SetShadowCasterInstanceId(0);
        for (s32 i = 0; i < s_activeCascadeCount; i++) {
            p3d::context->SetRenderTarget(s_cascadeTargets[i]);
            p3d::context->SetBlendMode(PDDI_BLEND_NONE);
            p3d::context->EnableZBuffer(true);
            BeginShadowCasterDepthBias();
            p3d::context->SetCullMode(PDDI_CULL_NONE);
            p3d::context->SetShadowCasterPass(true, s_lightVP[i]);

            for (const QueuedBlockCaster& caster : s_queuedBlockCasters) {
                if (!CasterBoundsOverlapCascade(i, caster.worldMin, caster.worldMax)) {
                    continue;
                }
                Mat4 blockWorld;
                blockWorld.SetTranslation(caster.tx, caster.ty, caster.tz);
                p3d::context->SetWorldMatrix(blockWorld);
                p3d::context->DrawPrimBuffer(caster.buffer);
            }

            p3d::context->SetShadowCasterPass(false, Mat4());
            EndShadowCasterDepthBias();
            p3d::context->SetRenderTarget(nullptr);
        }
    }
    s_queuedBlockCasters.clear();

    p3d::context->SetRenderTarget(nullptr);
    p3d::context->SetShadowCasterPass(false, Mat4());
    p3d::context->SetReceiveShadows(false);
    p3d::context->EnableZBuffer(true);
    p3d::context->SetPolygonOffset(false);
    p3d::context->SetBlendMode(PDDI_BLEND_NONE);
}

bool ShadowCSM::IsCasterPrepass() {
    return s_casterPrepass;
}

u32 ShadowCSM::GetCascadeTextureHandle(s32 index) {
    if (index < 0 || index >= s_activeCascadeCount || !s_cascadeTargets[index]) {
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

    for (s32 i = 0; i < s_activeCascadeCount; i++) {
        p3d::context->SetRenderTarget(s_cascadeTargets[i]);
        // See the matching comment in BeginFrame: must explicitly re-enable
        // depth writes, they're not implied by EnableZBuffer/SetRenderTarget.
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        p3d::context->EnableZBuffer(true);
        BeginShadowCasterDepthBias();
        p3d::context->SetCullMode(PDDI_CULL_NONE);
        p3d::context->SetShadowCasterPass(true, s_lightVP[i]);
        p3d::context->SetWorldMatrix(casterWorld);

        drawable->Display(flags);

        p3d::context->SetShadowCasterPass(false, Mat4());
        EndShadowCasterDepthBias();
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

    for (s32 i = 0; i < s_activeCascadeCount; i++) {
        p3d::context->SetRenderTarget(s_cascadeTargets[i]);
        p3d::context->SetBlendMode(PDDI_BLEND_NONE);
        p3d::context->EnableZBuffer(true);
        BeginShadowCasterDepthBias();
        p3d::context->SetCullMode(PDDI_CULL_NONE);
        p3d::context->SetShadowCasterPass(true, s_lightVP[i]);
        p3d::context->SetWorldMatrix(casterWorld);

        p3d::context->DrawPrimBuffer(buffer);

        p3d::context->SetShadowCasterPass(false, Mat4());
        EndShadowCasterDepthBias();
        p3d::context->SetRenderTarget(nullptr);
    }

    p3d::context->SetWorldMatrix(savedWorld);
}

void ShadowCSM::DrawBlockCasterIntoCascades(Block* block, const LVector* drawPos) {
    if (!s_framePrepared || !s_casterPrepass || !block || !drawPos || !block->primGeom || !p3d::context) {
        return;
    }

    if (!block->shadowCasterBuffer) {
        block->shadowCasterBuffer = BuildUnculledPrimBufferFromPrimGeom(block->primGeom);
        if (!block->shadowCasterBuffer) {
            return;
        }
    }
    s_casterCount++;

    const f32 tx = static_cast<f32>(drawPos->x);
    const f32 ty = static_cast<f32>(drawPos->y);
    const f32 tz = static_cast<f32>(drawPos->z);
    QueuedBlockCaster entry;
    entry.buffer = block->shadowCasterBuffer;
    entry.worldMin = { tx + static_cast<f32>(block->primGeom->bboxMinX),
                       ty + static_cast<f32>(block->primGeom->bboxMinY),
                       tz + static_cast<f32>(block->primGeom->bboxMinZ) };
    entry.worldMax = { tx + static_cast<f32>(block->primGeom->bboxMaxX),
                       ty + static_cast<f32>(block->primGeom->bboxMaxY),
                       tz + static_cast<f32>(block->primGeom->bboxMaxZ) };
    entry.tx = tx;
    entry.ty = ty;
    entry.tz = tz;
    s_queuedBlockCasters.push_back(entry);
}

#endif // MODERN_GRAPHICS
