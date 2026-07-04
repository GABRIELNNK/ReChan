#pragma once

#include "core.h"
#include "gen/config.h"

#if MODERN_GRAPHICS

struct LVector;
struct Block;
struct DrawableBasic;
class pddiPrimBuffer;

enum ShadowQuality : s32 {
    SHADOW_QUALITY_OFF = 0, // legacy blob shadow, no shadow map
    SHADOW_QUALITY_LOW = 1,
    SHADOW_QUALITY_MEDIUM = 2,
    SHADOW_QUALITY_HIGH = 3,
    SHADOW_QUALITY_VERY_HIGH = 4,
};

class ShadowCSM {
public:
    static ShadowQuality GetQuality();
    static void SetQuality(ShadowQuality quality);

    static void BeginFrame();
    static void DrawCasterIntoCascades(DrawableBasic* drawable, u32 flags);
    static void DrawCasterPrimBufferIntoCascades(pddiPrimBuffer* buffer);
    static void DrawBlockCasterIntoCascades(Block* block, const LVector* drawPos);
    static void SetCasterWorldOffset(f32 x, f32 y, f32 z);

    static void BeginCasterPrepass();
    static void EndCasterPrepass();
    static bool IsCasterPrepass();

    static void Shutdown();

    static bool IsFramePrepared();
    static s32 GetCascadeResolution();
    static s32 GetCasterCount();
    static u32 GetCascadeTextureHandle(s32 index);
    static const char* GetCascadeDepthFormatName();
    static const char* GetFilterQualityName();
};

#endif // MODERN_GRAPHICS
