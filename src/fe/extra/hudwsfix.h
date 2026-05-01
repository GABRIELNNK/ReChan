#pragma once

#include "gen/config.h"
#include "core.h"

#if FIX_ASPECT_RATIO

struct xcSection;
struct xcOverlayData;

class HUDWidescreenPatches {
public:
    void Initialize(xcSection* section, u8* rawData);
    void BeginUpdate(xcSection* section, u8* rawData);
    void EndUpdate(xcSection* section, u8* rawData);
    void OnLevelLoad(xcSection* section, u8* rawData);
    void OnLevelUnload(u8* rawData);
    void DrawSection(xcSection* section) const;

private:
    struct OverlayAnchor {
        xcOverlayData* overlay = nullptr;
        s8 direction = 0;
        s32 appliedX = 0;
    };

    static constexpr u32 kHubPromptOverlayHash = 0x050794E7u;
    static constexpr s32 kMaxAnchors = 128;

    OverlayAnchor anchors[kMaxAnchors] = {};
    s32 anchorCount = 0;

    static s32 RoundToNearestS32(f32 value);
    static s32 GetShiftX();
    static bool GetOverlayBoundsX(xcOverlayData* overlay, u8* rawData, s32& outMinX, s32& outMaxX);
    static s32 ClassifyOverlayAnchorX(xcOverlayData* overlay, u8* rawData);
    static void ShiftOverlayX(xcOverlayData* overlay, u8* rawData, s32 deltaX);
    static bool StretchHubBottomBarsInOverlay(xcSection* section, xcOverlayData* ovl, u8* rawData, s16 leftX, s16 rightX);

    void Clear();
    void Rebuild(xcSection* section, u8* rawData);
    bool AnchorsLookPreShifted(u8* rawData) const;
    void PrimeAppliedOffsetsIfNeeded(u8* rawData);
    void EnsureInitialized(xcSection* section, u8* rawData);
    void Apply(u8* rawData);
    void Reset(u8* rawData);
    void FixHubPromptBottomBar(xcSection* section, u8* rawData);
};

#endif