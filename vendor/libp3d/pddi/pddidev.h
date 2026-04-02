// pddidev.h — pddiDevice, pddiDisplay, pddiRenderContext interfaces
#pragma once

#include "pddi/pddi.h"

class pddiTexture;
class pddiBaseShader;
class pddiPrimBuffer;
struct pddiPrimBufferDesc;

// Display init params

struct pddiDisplayInit {
    int xSize = 960;
    int ySize = 720;
    const char* title = "Pure3D";
};

// pddiDisplay — window/framebuffer management


namespace pddiInput {
    // Letter keys use ASCII: 'A' = 65, 'W' = 87, etc.
    constexpr int KeyLeftShift = 340;
    constexpr int KeyPageUp    = 266;
    constexpr int KeyPageDown  = 267;
    constexpr int KeyF3        = 292;

    constexpr int MouseLeft   = 0;
    constexpr int MouseRight  = 1;
    constexpr int MouseMiddle = 2;
}

class pddiDisplay : public pddiObject {
public:
    virtual bool InitDisplay(const pddiDisplayInit& init) = 0;
    virtual void SwapBuffers() = 0;
    virtual int  GetWidth() = 0;
    virtual int  GetHeight() = 0;
    virtual bool ShouldClose() = 0;
    virtual void PollEvents() = 0;

    // Input polling
    virtual bool IsKeyDown(int key) = 0;
    virtual bool IsMouseButtonDown(int button) = 0;
    virtual void GetMousePosition(double& x, double& y) = 0;
};

// pddiRenderContext — frame management and draw state

class pddiRenderContext : public pddiObject {
public:
    // Frame
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    // Clear
    virtual void SetClearColour(pddiColour c) = 0;
    virtual void Clear(int flags) = 0;

    // Transforms
    virtual void SetProjectionMatrix(const Mat4& m) = 0;
    virtual void SetViewMatrix(const Mat4& m) = 0;
    virtual void SetWorldMatrix(const Mat4& m) = 0;
    virtual const Mat4& GetViewMatrix() const = 0;
    virtual const Mat4& GetProjectionMatrix() const = 0;

    // State
    virtual void SetCullMode(pddiCullMode mode) = 0;
    virtual void EnableZBuffer(bool enable) = 0;
    virtual void SetBlendMode(pddiBlendMode mode) = 0;

    // Immediate-mode textured quad (for UI / debug rendering)
    virtual void DrawQuad(pddiBaseShader* shader,
                          float x, float y, float w, float h,
                          float u0 = 0, float v0 = 0,
                          float u1 = 1, float v1 = 1) = 0;

    // Draw a retained-mode primitive buffer
    virtual void DrawPrimBuffer(pddiPrimBuffer* buffer) = 0;

    // Set texture for 3D primitive rendering (nullptr to disable)
    virtual void SetTexture(pddiTexture* tex) = 0;

    // Set raw VRAM texture handle for PSX VRAM-in-shader lookup (0 to disable)
    virtual void SetVRAMHandle(u32 handle) = 0;

    // Create/destroy a raw R16UI texture for PSX VRAM upload
    virtual u32  CreateVRAMTexture(int w, int h, const u16* data) = 0;
    virtual void DestroyVRAMTexture(u32 handle) = 0;
};

// pddiDevice — factory for all pddi objects──

class pddiDevice : public pddiObject {
public:
    virtual pddiDisplay* NewDisplay() = 0;
    virtual pddiRenderContext* NewRenderContext(pddiDisplay* display) = 0;
    virtual pddiTexture* NewTexture() = 0;
    virtual pddiPrimBuffer* NewPrimBuffer(const pddiPrimBufferDesc& desc) = 0;
    virtual pddiBaseShader* NewShader(const char* type = "simple") = 0;
};

// Create the platform-appropriate device (implemented by GL backend)
pddiDevice* pddiCreate();
