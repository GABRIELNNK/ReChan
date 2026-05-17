#pragma once

#include "core.h"
#include "gen/utf8text.h"
#include "pc/textmgr.h"

struct TextBackendFontDesc {
    TextFontHandle handle = 0;
    const char* name = nullptr;
    const u8* fileData = nullptr;
    s32 fileSize = 0;
    s32 pixelHeight = 0;
};

class ITextBackend {
public:
    virtual ~ITextBackend() = default;

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsReady() const = 0;

    virtual bool RegisterFont(const TextBackendFontDesc& desc) = 0;
    virtual void UnregisterFont(TextFontHandle handle) = 0;

    virtual TextBounds Measure(Utf8TextView text, const TextRenderState& state) const = 0;
    virtual s32 CountWrappedLines(Utf8TextView text, const TextRenderState& state) const = 0;
    virtual void Draw(Utf8TextView text, f32 x, f32 y, const TextRenderState& state) const = 0;
};

ITextBackend* CreateDefaultTextBackend();