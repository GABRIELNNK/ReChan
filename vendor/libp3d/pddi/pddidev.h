// pddidev.h — pddiDevice, pddiDisplay, pddiRenderContext interfaces
#ifndef PDDI_DEV_H
#define PDDI_DEV_H

#include "pddi/pddi.h"

class pddiTexture;
class pddiBaseShader;

// Display init params

struct pddiDisplayInit {
    int xSize = 960;
    int ySize = 720;
    const char* title = "Pure3D";
};

// pddiDisplay — window/framebuffer management

class pddiDisplay : public pddiObject {
public:
    virtual bool InitDisplay(const pddiDisplayInit& init) = 0;
    virtual void SwapBuffers() = 0;
    virtual int  GetWidth() = 0;
    virtual int  GetHeight() = 0;
    virtual bool ShouldClose() = 0;
    virtual void PollEvents() = 0;
    virtual void* GetHandle() = 0;
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

    // State
    virtual void SetCullMode(pddiCullMode mode) = 0;
    virtual void EnableZBuffer(bool enable) = 0;
    virtual void SetBlendMode(pddiBlendMode mode) = 0;

    // Immediate-mode textured quad (for UI / debug rendering)
    virtual void DrawQuad(pddiBaseShader* shader,
                          float x, float y, float w, float h,
                          float u0 = 0, float v0 = 0,
                          float u1 = 1, float v1 = 1) = 0;
};

// pddiDevice — factory for all pddi objects──

class pddiDevice : public pddiObject {
public:
    virtual pddiDisplay* NewDisplay() = 0;
    virtual pddiRenderContext* NewRenderContext(pddiDisplay* display) = 0;
    virtual pddiTexture* NewTexture() = 0;
    virtual pddiBaseShader* NewShader(const char* type = "simple") = 0;
};

// Create the platform-appropriate device (implemented by GL backend)
pddiDevice* pddiCreate();

#endif // PDDI_DEV_H
