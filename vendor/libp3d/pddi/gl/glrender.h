// glrender.h — OpenGL implementation of pddi interfaces
#pragma once

#include "pddi/pddi.h"
#include "pddi/pdditex.h"
#include "pddi/pddishad.h"
#include "pddi/pddidev.h"

struct GLFWwindow;

// glPrimBuffer──

class glPrimBuffer : public pddiPrimBuffer {
public:
    glPrimBuffer(const pddiPrimBufferDesc& desc);
    ~glPrimBuffer() override;

    void SetVertexData(const void* data, u32 count) override;
    void SetIndices(const u16* indices, u32 count) override;
    u32 GetIndexCount() const override { return indexCount; }
    u32 GetVertexCount() const override { return vertexCount; }
    pddiPrimType GetPrimType() const override { return primType; }

    u32 GetVAO() const { return vao; }

private:
    pddiPrimType primType;
    u32 vertexFormat;
    u32 vertexCount = 0;
    u32 indexCount = 0;
    u32 stride = 0;
    u32 vao = 0;
    u32 vbo = 0;
    u32 ebo = 0;

    void SetupVertexAttribs();
};

// glTexture──

class glTexture : public pddiTexture {
public:
    glTexture();
    ~glTexture() override;

    int  GetWidth() override { return width; }
    int  GetHeight() override { return height; }
    int  GetBpp() override { return bpp; }
    int  GetAlphaDepth() override { return alphaDepth; }

    void SetData(int w, int h, int bpp, int alphaDepth, const void* rgba) override;
    void Bind(int unit) override;

    u32 GetGLHandle() const { return handle; }

private:
    u32 handle = 0;
    int width = 0;
    int height = 0;
    int bpp = 0;
    int alphaDepth = 0;
};

// glShader──

class glShader : public pddiBaseShader {
public:
    glShader();
    ~glShader() override;

    const char* GetType() override { return "simple"; }

    void SetTexture(u32 param, pddiTexture* tex) override;
    void SetInt(u32 param, int value) override;
    void SetFloat(u32 param, float value) override;
    void SetColour(u32 param, pddiColour c) override;

    void PreRender() override;
    void PostRender() override;

    u32 GetProgram() const { return program; }

private:
    u32 program = 0;
    pddiTexture* tex = nullptr;
    pddiColour diffuse = pddiColour(255, 255, 255);
    pddiBlendMode blendMode = PDDI_BLEND_NONE;

    void CreateDefaultProgram();
};

// glDisplay

class glDisplay : public pddiDisplay {
public:
    glDisplay();
    ~glDisplay() override;

    bool  InitDisplay(const pddiDisplayInit& init) override;
    void  SwapBuffers() override;
    int   GetWidth() override { return fbWidth; }
    int   GetHeight() override { return fbHeight; }
    bool  ShouldClose() override;
    void  PollEvents() override;

    // Input polling
    bool IsKeyDown(int key) override;
    bool IsMouseButtonDown(int button) override;
    void GetMousePosition(double& x, double& y) override;

    void SetIcon(int w, int h, const unsigned char* rgba) override;

    // Video mode
    int  GetVideoModeCount() override;
    void GetVideoMode(int index, pddiVideoMode& mode) override;
    void SetFullscreen(bool fullscreen) override;
    bool IsFullscreen() override;
    void SetResolution(int w, int h) override;
    void SetVSync(bool enabled) override;
    void SetWindowPos(int x, int y) override;

    void SetTitle(const char* title) override;

    // Cursor
    void ShowCursor(bool visible) override;
    void ClipCursor(bool clip) override;

    // WndProc callback
    void SetWndProc(pddiWndProc proc) override;

    // Overlay
    void SetOverlayCallback(OverlayCallback cb) override;
    void RenderOverlay() override;

    // Viewport
    void GetViewport(int& x, int& y, int& w, int& h) const;

    GLFWwindow* GetWindow() const { return window; }

private:
    GLFWwindow* window = nullptr;
    bool imguiInitialized = false;
    bool imguiFrameStarted = false;
    OverlayCallback overlayCallback = nullptr;
    int fbWidth = 0;
    int fbHeight = 0;
    int windowedX = 100, windowedY = 100;
    int windowedW = 960, windowedH = 720;
    bool cursorVisible = true;
    bool cursorClipped = false;
    bool focused = true;
    pddiWndProc wndProc;

    void UpdateCursorClip();

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void WindowFocusCallback(GLFWwindow* window, int focused);
    static void WindowCloseCallback(GLFWwindow* window);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double x, double y);
    static void ScrollCallback(GLFWwindow* window, double xoff, double yoff);
};

// glContext──

class glContext : public pddiRenderContext {
public:
    glContext(glDisplay* disp);
    ~glContext() override;

    void BeginFrame() override;
    void EndFrame() override;

    void SetClearColour(pddiColour c) override;
    void Clear(int flags) override;

    void SetProjectionMatrix(const Mat4& m) override;
    void SetViewMatrix(const Mat4& m) override;
    void SetWorldMatrix(const Mat4& m) override;
    const Mat4& GetViewMatrix() const override { return viewMatrix; }
    const Mat4& GetProjectionMatrix() const override { return projection; }

    void SetCullMode(pddiCullMode mode) override;
    void EnableZBuffer(bool enable) override;
    void SetBlendMode(pddiBlendMode mode) override;

    void DrawQuad(pddiBaseShader* shader,
                  float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1) override;

    void DrawPrimBuffer(pddiPrimBuffer* buffer) override;

    void SetTexture(pddiTexture* tex) override;
    void SetVRAMHandle(u32 handle) override;

    u32  CreateVRAMTexture(int w, int h, const u16* data) override;
    void DestroyVRAMTexture(u32 handle) override;

    u32 Get3DProgram() const { return program3D; }

private:
    glDisplay* display;
    pddiColour clearColour;
    Mat4 projection;
    Mat4 viewMatrix;
    Mat4 worldMatrix;
    pddiTexture* currentTexture = nullptr;
    u32 vramHandle = 0;
    u32 quadVAO = 0;
    u32 quadVBO = 0;
    u32 program3D = 0;

    // Renderstate cache
    pddiCullMode cachedCullMode = PDDI_CULL_NONE;
    bool cachedZBuffer = false;
    pddiBlendMode cachedBlendMode = PDDI_BLEND_NONE;
    bool stateDirty = true;

    void InitQuadMesh();
    void Init3DShader();
};

// glDevice──

class glDevice : public pddiDevice {
public:
    pddiDisplay* NewDisplay() override;
    pddiRenderContext* NewRenderContext(pddiDisplay* display) override;
    pddiTexture* NewTexture() override;
    pddiPrimBuffer* NewPrimBuffer(const pddiPrimBufferDesc& desc) override;
    pddiBaseShader* NewShader(const char* type) override;
};
