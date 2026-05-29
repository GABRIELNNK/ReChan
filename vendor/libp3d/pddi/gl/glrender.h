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
    void SetFilterMode(pddiFilterMode mode) override;
    void Bind(int unit) override;

    u32 GetGLHandle() const { return handle; }

private:
    u32 handle = 0;
    int width = 0;
    int height = 0;
    int bpp = 0;
    int alphaDepth = 0;
    pddiFilterMode filterMode = PDDI_FILTER_NONE;
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
    static constexpr s32 kMaxTextureSlots = 16;
    u32 program = 0;
    pddiTexture* texSlots[kMaxTextureSlots] = {};
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
    int   GetWidth() override;
    int   GetHeight() override;
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
    void SetBorderless(bool borderless) override;
    bool IsBorderless() override { return borderless; }
    void SetResolution(int w, int h) override;
    void SetVSync(bool enabled) override;
    void SetMSAA(int samples) override;
    int  GetMSAA() override { return msaaSamples; }
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
    void GetViewport(int& x, int& y, int& w, int& h);

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
    bool borderless = false;
    bool cursorVisible = true;
    bool cursorClipped = false;
    bool focused = true;
    int msaaSamples = 0;
    int maxMsaaSamples = 0;
    pddiWndProc wndProc;

    void UpdateCursorClip();
    int ClampMSAASamples(int samples) const;
    void SyncFramebufferSize();
    void QueryFramebufferSize(int& width, int& height) const;

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

    void SetCameraAspect(float aspect) override { cameraAspect = aspect; }
    float GetCameraAspect() const override { return cameraAspect; }

    void SetClearColour(pddiColour c) override;
    void Clear(int flags) override;

    void SetProjectionMatrix(const Mat4& m) override;
    void SetViewMatrix(const Mat4& m) override;
    void SetWorldMatrix(const Mat4& m) override;
    const Mat4& GetWorldMatrix() const override { return worldMatrix; }
    const Mat4& GetViewMatrix() const override { return viewMatrix; }
    const Mat4& GetProjectionMatrix() const override { return projection; }

    void SetCullMode(pddiCullMode mode) override;
    void EnableZBuffer(bool enable) override;
    void SetBlendMode(pddiBlendMode mode) override;
    void SetDepthClamp(bool enable) override;
    void SetPolygonOffset(bool enable, f32 factor = 0.0f, f32 units = 0.0f) override;
    void SetScissor(int x, int y, int w, int h) override;
    void SetMultisampleEnabled(bool enable) override;
    void ResolveForOverlayPass() override;

    void DrawQuad(pddiBaseShader* shader,
                  float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1) override;

    void DrawPrimBuffer(pddiPrimBuffer* buffer) override;

    void SetTexture(pddiTexture* tex) override;
    void SetVRAMHandle(u32 handle) override;
    void SetTexInfoOverride(bool enabled, u32 texInfoWord) override;

    u32  CreateVRAMTexture(int w, int h, const u16* data) override;
    void DestroyVRAMTexture(u32 handle) override;

    u32 Get3DProgram() const { return program3D; }

    void DrawGouraudQuad(float x0, float y0, float r0, float g0, float b0, float a0,
                         float x1, float y1, float r1, float g1, float b1, float a1,
                         float x2, float y2, float r2, float g2, float b2, float a2,
                         float x3, float y3, float r3, float g3, float b3, float a3);

private:
    glDisplay* display;
    float cameraAspect = 0.0f;
    pddiColour clearColour;
    Mat4 projection;
    Mat4 viewMatrix;
    Mat4 worldMatrix;
    pddiTexture* currentTexture = nullptr;
    u32 vramHandle = 0;
    bool texInfoOverrideEnabled = false;
    u32 texInfoOverrideWord = 0;
    u32 quadVAO = 0;
    u32 quadVBO = 0;
    u32 program3D = 0;
    u32 gouraudVAO = 0;
    u32 gouraudVBO = 0;
    u32 gouraudProgram = 0;
    u32 msaaFbo = 0;
    u32 msaaColorRbo = 0;
    u32 msaaDepthStencilRbo = 0;
    s32 msaaWidth = 0;
    s32 msaaHeight = 0;
    s32 activeMsaaSamples = 0;
    bool usingMsaaFramebuffer = false;
    bool multisampleEnabled = true;
    bool resolvedForOverlay = false;

    // Renderstate cache
    pddiCullMode cachedCullMode = PDDI_CULL_NONE;
    bool cachedZBuffer = false;
    bool cachedDepthClamp = false;
    pddiBlendMode cachedBlendMode = PDDI_BLEND_NONE;
    bool stateDirty = true;
    // Polygon offset override: when true, SetBlendMode(BLEND_NONE) restores these
    // values instead of disabling polygon offset.
    bool polyOffsetOverride = false;
    f32 polyOffsetFactor = 0.0f;
    f32 polyOffsetUnits = 0.0f;

    void InitQuadMesh();
    void InitGouraudMesh();
    void Init3DShader();
    void UpdateMultisampleState();
    void EnsureMSAAFramebuffer(s32 samples, s32 width, s32 height);
    void DestroyMSAAFramebuffer();
};

// glDevice

class glDevice : public pddiDevice {
public:
    pddiDisplay* NewDisplay() override;
    pddiRenderContext* NewRenderContext(pddiDisplay* display) override;
    pddiGamepad* NewGamepad() override;
    pddiTexture* NewTexture() override;
    pddiPrimBuffer* NewPrimBuffer(const pddiPrimBufferDesc& desc) override;
    pddiBaseShader* NewShader(const char* type) override;
};

// glGamepad

class glGamepad : public pddiGamepad {
public:
    void Poll() override;
    bool IsConnected() const override { return connected; }
    bool IsButtonDown(int button) const override;
    float GetAxis(int axis) const override;
    bool SupportsVibration() const override;
    bool SetVibration(float lowFrequency, float highFrequency) override;

private:
    bool connected = false;
    bool buttons[GamepadButton::COUNT] = {};
    float axes[GamepadAxis::COUNT] = {};
    int activeJoystickId = -1;
};
