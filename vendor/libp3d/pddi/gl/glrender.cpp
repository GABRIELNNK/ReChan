// glrender.cpp - OpenGL implementation of pddi interfaces
#include "pddi/gl/glrender.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>

// Default shader GLSL

static const char* kSimpleVert = R"(
#version 450 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 uProj;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
)";

// 3D vertex-color shader with PSX VRAM palette lookup

static const char* k3DVert = R"(
#version 450 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
layout(location=2) in vec2 aUV;
layout(location=3) in vec2 aTexInfo;
uniform mat4 uMVP;
out vec3 vColor;
out vec2 vUV;
flat out vec2 vTexInfo;
void main() {
    vColor = aColor;
    vUV = aUV;
    vTexInfo = aTexInfo;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* k3DFrag = R"(
#version 450 core
in vec3 vColor;
in vec2 vUV;
flat in vec2 vTexInfo;
uniform usampler2D uVRAM;
uniform int uHasVRAM;
out vec4 FragColor;
void main() {
    float tpageF = vTexInfo.x;
    if (uHasVRAM == 0 || tpageF < 0.0) {
        FragColor = vec4(vColor, 1.0);
        return;
    }

    uint tpage = uint(tpageF);
    uint cba = uint(vTexInfo.y);

    uint tx = tpage & 0xFu;
    uint ty = (tpage >> 4u) & 1u;
    uint depth = (tpage >> 7u) & 3u;

    uint pageX = tx * 64u;
    uint pageY = ty * 256u;

    uint clutX = (cba & 0x3Fu) * 16u;
    uint clutY = (cba >> 6u) & 0x1FFu;

    uint px = uint(mod(vUV.x + 256.0, 256.0));
    uint py = uint(mod(vUV.y + 256.0, 256.0));

    uint clutWord;
    if (depth == 0u) {
        uint wordX = pageX + px / 4u;
        uint word = texelFetch(uVRAM, ivec2(wordX, pageY + py), 0).r;
        uint palIdx = (word >> ((px % 4u) * 4u)) & 0xFu;
        clutWord = texelFetch(uVRAM, ivec2(clutX + palIdx, clutY), 0).r;
    } else if (depth == 1u) {
        uint wordX = pageX + px / 2u;
        uint word = texelFetch(uVRAM, ivec2(wordX, pageY + py), 0).r;
        uint palIdx = (px & 1u) != 0u ? (word >> 8u) & 0xFFu : word & 0xFFu;
        clutWord = texelFetch(uVRAM, ivec2(clutX + palIdx, clutY), 0).r;
    } else {
        clutWord = texelFetch(uVRAM, ivec2(pageX + px, pageY + py), 0).r;
    }

    if (clutWord == 0u) discard;

    // Magenta (R=31,G=0,B=31) is the transparency key
    if ((clutWord & 0x7FFFu) == 0x7C1Fu) discard;

    float r = float(clutWord & 0x1Fu) / 31.0;
    float g = float((clutWord >> 5u) & 0x1Fu) / 31.0;
    float b = float((clutWord >> 10u) & 0x1Fu) / 31.0;

    FragColor = vec4(r, g, b, 1.0) * vec4(vColor, 1.0);
}
)";

static const char* kSimpleFrag = R"(
#version 450 core
in vec2 vUV;
uniform sampler2D uTex;
uniform vec4 uTint;
out vec4 FragColor;
void main() {
    FragColor = texture(uTex, vUV) * uTint;
}
)";

// GL helpers

static u32 CompileGLShader(u32 type, const char* src) {
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "GLSL compile error:\n%s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// glPrimBuffer

glPrimBuffer::glPrimBuffer(const pddiPrimBufferDesc& desc)
    : primType(desc.primType), vertexFormat(desc.vertexFormat)
    , vertexCount(desc.vertexCount), indexCount(desc.indexCount) {
    // Compute stride from vertex format
    stride = 0;
    if (vertexFormat & PDDI_V_POSITION) stride += 3 * sizeof(f32);
    if (vertexFormat & PDDI_V_COLOUR)   stride += 3 * sizeof(f32);
    if (vertexFormat & PDDI_V_UV)       stride += 2 * sizeof(f32);
    if (vertexFormat & PDDI_V_TEXINFO)  stride += 2 * sizeof(f32);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, stride * desc.vertexCount, nullptr, GL_STATIC_DRAW);

    SetupVertexAttribs();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, desc.indexCount * sizeof(u16), nullptr, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

glPrimBuffer::~glPrimBuffer() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void glPrimBuffer::SetVertexData(const void* data, u32 count) {
    vertexCount = count;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, stride * count, data, GL_STATIC_DRAW);
}

void glPrimBuffer::SetIndices(const u16* indices, u32 count) {
    indexCount = count;
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u16), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void glPrimBuffer::SetupVertexAttribs() {
    u32 offset = 0;
    u32 loc = 0;

    if (vertexFormat & PDDI_V_POSITION) {
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offset);
        glEnableVertexAttribArray(loc);
        offset += 3 * sizeof(f32);
        loc++;
    }
    if (vertexFormat & PDDI_V_COLOUR) {
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offset);
        glEnableVertexAttribArray(loc);
        offset += 3 * sizeof(f32);
        loc++;
    }
    if (vertexFormat & PDDI_V_UV) {
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offset);
        glEnableVertexAttribArray(loc);
        offset += 2 * sizeof(f32);
        loc++;
    }
    if (vertexFormat & PDDI_V_TEXINFO) {
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offset);
        glEnableVertexAttribArray(loc);
        offset += 2 * sizeof(f32);
        loc++;
    }
}

// glTexture

glTexture::glTexture() = default;

glTexture::~glTexture() {
    if (handle)
        glDeleteTextures(1, &handle);
}

void glTexture::SetData(int w, int h, int b, int a, const void* rgba) {
    width = w;
    height = h;
    bpp = b;
    alphaDepth = a;

    if (handle)
        glDeleteTextures(1, &handle);

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void glTexture::Bind(int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, handle);
}

// glShader

glShader::glShader() {
    CreateDefaultProgram();
}

glShader::~glShader() {
    if (program)
        glDeleteProgram(program);
}

void glShader::CreateDefaultProgram() {
    u32 vs = CompileGLShader(GL_VERTEX_SHADER, kSimpleVert);
    u32 fs = CompileGLShader(GL_FRAGMENT_SHADER, kSimpleFrag);
    if (!vs || !fs) 
        return;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "GLSL link error:\n%s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void glShader::SetTexture(u32 param, pddiTexture* t) { tex = t; }
void glShader::SetInt(u32, int) {}
void glShader::SetFloat(u32, float) {}
void glShader::SetColour(u32 param, pddiColour c) {
    diffuse = c;
}

void glShader::PreRender() {
    glUseProgram(program);
    if (tex) {
        tex->Bind(0);
        glUniform1i(glGetUniformLocation(program, "uTex"), 0);
    }
    int tintLoc = glGetUniformLocation(program, "uTint");
    if (tintLoc >= 0) {
        // PSX GPU: output = min(255, (tex * color) >> 7)
        // Divide RGB by 128 (PSX neutral), alpha by 255
        glUniform4f(tintLoc,
            diffuse.r / 128.0f, diffuse.g / 128.0f,
            diffuse.b / 128.0f, diffuse.a / 255.0f);
    }
}

void glShader::PostRender() {
    glUseProgram(0);
}

// glDisplay

glDisplay::glDisplay() = default;

glDisplay::~glDisplay() {
    if (imguiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
    }
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

bool glDisplay::InitDisplay(const pddiDisplayInit& init) {
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW: init failed\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (init.msaa > 0) {
        glfwWindowHint(GLFW_SAMPLES, init.msaa);
    }

    GLFWmonitor* monitor = nullptr;
    int winW = init.xSize;
    int winH = init.ySize;

    if (init.fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        winW = vidMode->width;
        winH = vidMode->height;
        glfwWindowHint(GLFW_RED_BITS, vidMode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, vidMode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, vidMode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, vidMode->refreshRate);
    }

    window = glfwCreateWindow(winW, winH, init.title, monitor, nullptr);
    if (!window) {
        std::fprintf(stderr, "GLFW: window creation failed\n");
        glfwTerminate();
        return false;
    }

    windowedW = init.xSize;
    windowedH = init.ySize;
    if (!init.fullscreen) {
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        if (primaryMonitor) {
            int workX = 0;
            int workY = 0;
            int workW = 0;
            int workH = 0;
            glfwGetMonitorWorkarea(primaryMonitor, &workX, &workY, &workW, &workH);

            int contentW = 0;
            int contentH = 0;
            glfwGetWindowSize(window, &contentW, &contentH);

            int centeredX = workX + (workW - contentW) / 2;
            int centeredY = workY + (workH - contentH) / 2;
            if (centeredX < workX) centeredX = workX;
            if (centeredY < workY) centeredY = workY;

            glfwSetWindowPos(window, centeredX, centeredY);
        }

        glfwGetWindowPos(window, &windowedX, &windowedY);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(init.vsync ? 1 : 0);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowFocusCallback(window, WindowFocusCallback);
    glfwSetWindowCloseCallback(window, WindowCloseCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    if (init.msaa > 0) {
        glEnable(GL_MULTISAMPLE);
    }

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::fprintf(stderr, "GLAD: failed to load OpenGL\n");
        return false;
    }

    std::printf("OpenGL %s on %s\n",
                reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
    imguiInitialized = true;

    return true;
}

void glDisplay::SwapBuffers() {
    glfwSwapBuffers(window);
}

bool glDisplay::ShouldClose() {
    return glfwWindowShouldClose(window);
}

void glDisplay::PollEvents() {
    glfwPollEvents();
    if (imguiInitialized && !imguiFrameStarted) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        imguiFrameStarted = true;
    }
}

void glDisplay::SetOverlayCallback(OverlayCallback cb) {
    overlayCallback = cb;
}

void glDisplay::RenderOverlay() {
    if (!imguiFrameStarted) {
        return;
    }
    if (overlayCallback) {
        overlayCallback();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    imguiFrameStarted = false;
}

bool glDisplay::IsKeyDown(int key) {
    return window && glfwGetKey(window, key) == GLFW_PRESS;
}

bool glDisplay::IsMouseButtonDown(int button) {
    return window && glfwGetMouseButton(window, button) == GLFW_PRESS;
}

void glDisplay::GetMousePosition(double& x, double& y) {
    if (window) {
        glfwGetCursorPos(window, &x, &y);
    } else {
        x = 0;
        y = 0;
    }
}

void glDisplay::SetIcon(int w, int h, const unsigned char* rgba) {
    if (!window || !rgba)
        return;
    GLFWimage img;
    img.width = w;
    img.height = h;
    img.pixels = const_cast<unsigned char*>(rgba);
    glfwSetWindowIcon(window, 1, &img);
}

// Video mode

int glDisplay::GetVideoModeCount() {
    int count = 0;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        glfwGetVideoModes(monitor, &count);
    }
    return count;
}

void glDisplay::GetVideoMode(int index, pddiVideoMode& mode) {
    int count = 0;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor)
        return;
    const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
    if (index < 0 || index >= count)
        return;
    mode.width = modes[index].width;
    mode.height = modes[index].height;
    mode.refreshRate = modes[index].refreshRate;
}

void glDisplay::SetFullscreen(bool fullscreen) {
    if (!window)
        return;

    if (fullscreen == IsFullscreen())
        return;

    if (fullscreen) {
        // Save current windowed position/size
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedW, &windowedH);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0,
                             vidMode->width, vidMode->height,
                             vidMode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, nullptr,
                             windowedX, windowedY,
                             windowedW, windowedH, 0);
    }
}

bool glDisplay::IsFullscreen() {
    return window && glfwGetWindowMonitor(window) != nullptr;
}

void glDisplay::SetResolution(int w, int h) {
    if (!window)
        return;

    if (IsFullscreen()) {
        GLFWmonitor* monitor = glfwGetWindowMonitor(window);
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, w, h, vidMode->refreshRate);
    } else {
        glfwSetWindowSize(window, w, h);
        windowedW = w;
        windowedH = h;
    }
}

void glDisplay::SetVSync(bool enabled) {
    if (window) {
        glfwSwapInterval(enabled ? 1 : 0);
    }
}

void glDisplay::SetWindowPos(int x, int y) {
    if (window && !IsFullscreen()) {
        glfwSetWindowPos(window, x, y);
        windowedX = x;
        windowedY = y;
    }
}

// Cursor

void glDisplay::ShowCursor(bool visible) {
    cursorVisible = visible;
    if (window) {
        glfwSetInputMode(window, GLFW_CURSOR,
                         visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }
}

void glDisplay::ClipCursor(bool clip) {
    cursorClipped = clip;
    UpdateCursorClip();
}

void glDisplay::UpdateCursorClip() {
    if (!window)
        return;

    if (cursorClipped && focused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported()) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
    } else if (!cursorVisible) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }
}

// WndProc

void glDisplay::SetWndProc(pddiWndProc proc) {
    wndProc = std::move(proc);
}

void glDisplay::GetViewport(int& x, int& y, int& w, int& h) const {
    x = 0;
    y = 0;
    w = fbWidth;
    h = fbHeight;
}

// GLFW callbacks

void glDisplay::FramebufferSizeCallback(GLFWwindow* win, int w, int h) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(win));
    if (!self)
        return;
    self->fbWidth = w;
    self->fbHeight = h;
    if (self->wndProc) {
        pddiWndMessage msg{};
        msg.event = PDDI_WND_RESIZE;
        msg.param1 = w;
        msg.param2 = h;
        self->wndProc(msg);
    }
}

void glDisplay::WindowFocusCallback(GLFWwindow* win, int isFocused) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(win));
    if (!self)
        return;
    self->focused = (isFocused != 0);
    self->UpdateCursorClip();
    if (self->wndProc) {
        pddiWndMessage msg{};
        msg.event = PDDI_WND_FOCUS;
        msg.param1 = isFocused;
        self->wndProc(msg);
    }
}

void glDisplay::WindowCloseCallback(GLFWwindow* win) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(win));
    if (!self)
        return;
    if (self->wndProc) {
        pddiWndMessage msg{};
        msg.event = PDDI_WND_CLOSE;
        if (self->wndProc(msg)) {
            glfwSetWindowShouldClose(win, GLFW_FALSE);
        }
    }
}

void glDisplay::KeyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(win));
    if (!self || !self->wndProc)
        return;
    if (action == GLFW_REPEAT)
        return;
    pddiWndMessage msg{};
    msg.event = (action == GLFW_PRESS) ? PDDI_WND_KEYDOWN : PDDI_WND_KEYUP;
    msg.param1 = key;
    msg.param2 = scancode;
    self->wndProc(msg);
}

void glDisplay::MouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(win));
    if (!self || !self->wndProc)
        return;
    pddiWndMessage msg{};
    msg.event = PDDI_WND_MOUSEBUTTON;
    msg.param1 = button;
    msg.param2 = (action == GLFW_PRESS) ? 1 : 0;
    self->wndProc(msg);
}

void glDisplay::CursorPosCallback(GLFWwindow* win, double x, double y) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(win));
    if (!self || !self->wndProc)
        return;
    pddiWndMessage msg{};
    msg.event = PDDI_WND_MOUSEMOVE;
    msg.fparam1 = x;
    msg.fparam2 = y;
    self->wndProc(msg);
}

void glDisplay::ScrollCallback(GLFWwindow* win, double xoff, double yoff) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(win));
    if (!self || !self->wndProc)
        return;
    pddiWndMessage msg{};
    msg.event = PDDI_WND_SCROLL;
    msg.fparam1 = xoff;
    msg.fparam2 = yoff;
    self->wndProc(msg);
}

// glContext

glContext::glContext(glDisplay* disp)
    : display(disp) {
    InitQuadMesh();
    Init3DShader();
}

glContext::~glContext() {
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (program3D) glDeleteProgram(program3D);
}

void glContext::BeginFrame() {
    int vx, vy, vw, vh;
    display->GetViewport(vx, vy, vw, vh);
    glViewport(vx, vy, vw, vh);
    glScissor(vx, vy, vw, vh);
    glEnable(GL_SCISSOR_TEST);
    stateDirty = true;
}

void glContext::EndFrame() {
    glDisable(GL_SCISSOR_TEST);
    glFlush();
}

void glContext::SetClearColour(pddiColour c) { clearColour = c; }

void glContext::Clear(int flags) {
    GLbitfield mask = 0;
    if (flags & PDDI_BUFFER_COLOUR) {
        glClearColor(clearColour.r / 255.0f, clearColour.g / 255.0f,
                     clearColour.b / 255.0f, clearColour.a / 255.0f);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (flags & PDDI_BUFFER_DEPTH)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (mask)
        glClear(mask);
}

void glContext::SetProjectionMatrix(const Mat4& m) { projection = m; }
void glContext::SetViewMatrix(const Mat4& m) { viewMatrix = m; }
void glContext::SetWorldMatrix(const Mat4& m) { worldMatrix = m; }

void glContext::SetCullMode(pddiCullMode mode) {
    if (!stateDirty && mode == cachedCullMode) 
        return;

    cachedCullMode = mode;

    // PSX uses CW winding (left-handed). X-flip in projection preserves CW.
    glFrontFace(GL_CW);

    switch (mode) {
        case PDDI_CULL_NONE:     
            glDisable(GL_CULL_FACE);
            break;
        case PDDI_CULL_NORMAL:  
            glEnable(GL_CULL_FACE); 
            glCullFace(GL_BACK); 
            break;
        case PDDI_CULL_INVERTED: 
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT); 
            break;
    }
}

void glContext::EnableZBuffer(bool enable) {
    if (!stateDirty && enable == cachedZBuffer)
        return;

    cachedZBuffer = enable;
    if (enable)
        glEnable(GL_DEPTH_TEST);
    else       
        glDisable(GL_DEPTH_TEST);
}

void glContext::SetBlendMode(pddiBlendMode mode) {
    if (!stateDirty && mode == cachedBlendMode) 
        return;

    cachedBlendMode = mode;
    switch (mode) {
        case PDDI_BLEND_NONE:
            glDisable(GL_BLEND);
            break;
        case PDDI_BLEND_ALPHA:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case PDDI_BLEND_ADD:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            break;
        case PDDI_BLEND_SUBTRACT:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
            break;
    }
    stateDirty = false;
}

void glContext::DrawQuad(pddiBaseShader* shader,
                         float x, float y, float w, float h,
                         float u0, float v0, float u1, float v1) {
    shader->PreRender();

    auto* s = static_cast<glShader*>(shader);
    glUniformMatrix4fv(glGetUniformLocation(s->GetProgram(), "uProj"),
                       1, GL_FALSE, projection.Data());

    float verts[] = {
        x,     y,     u0, v1,
        x + w, y,     u1, v1,
        x + w, y + h, u1, v0,
        x,     y,     u0, v1,
        x + w, y + h, u1, v0,
        x,     y + h, u0, v0,
    };

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shader->PostRender();
}

void glContext::InitQuadMesh() {
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                          (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void glContext::Init3DShader() {
    u32 vs = CompileGLShader(GL_VERTEX_SHADER, k3DVert);
    u32 fs = CompileGLShader(GL_FRAGMENT_SHADER, k3DFrag);

    if (!vs || !fs)
        return;

    program3D = glCreateProgram();
    glAttachShader(program3D, vs);
    glAttachShader(program3D, fs);
    glLinkProgram(program3D);

    int ok;
    glGetProgramiv(program3D, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program3D, sizeof(log), nullptr, log);
        std::fprintf(stderr, "3D shader link error:\n%s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void glContext::SetTexture(pddiTexture* t) {
    currentTexture = t;
}

void glContext::SetVRAMHandle(u32 h) {
    vramHandle = h;
}

u32 glContext::CreateVRAMTexture(int w, int h, const u16* data) {
    u32 tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, w, h, 0,
                 GL_RED_INTEGER, GL_UNSIGNED_SHORT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void glContext::DestroyVRAMTexture(u32 handle) {
    if (handle) glDeleteTextures(1, &handle);
}

void glContext::DrawPrimBuffer(pddiPrimBuffer* buffer) {
    if (!buffer)
        return;

    glUseProgram(program3D);

    Mat4 mvp = projection * (viewMatrix * worldMatrix);
    glUniformMatrix4fv(glGetUniformLocation(program3D, "uMVP"),
                       1, GL_FALSE, mvp.Data());

    int hasVRAM = vramHandle ? 1 : 0;
    glUniform1i(glGetUniformLocation(program3D, "uHasVRAM"), hasVRAM);
    if (vramHandle) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vramHandle);
        glUniform1i(glGetUniformLocation(program3D, "uVRAM"), 0);
    }

    GLenum glMode = GL_TRIANGLES;
    switch (buffer->GetPrimType()) {
        case PDDI_PRIM_TRIANGLES: glMode = GL_TRIANGLES; break;
        case PDDI_PRIM_TRISTRIP:  glMode = GL_TRIANGLE_STRIP; break;
        case PDDI_PRIM_LINES:     glMode = GL_LINES; break;
        case PDDI_PRIM_LINESTRIP: glMode = GL_LINE_STRIP; break;
        case PDDI_PRIM_POINTS:    glMode = GL_POINTS; break;
    }

    auto* glBuf = static_cast<glPrimBuffer*>(buffer);
    glBindVertexArray(glBuf->GetVAO());
    glDrawElements(glMode, buffer->GetIndexCount(), GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
}

// glDevice

pddiDisplay* glDevice::NewDisplay() { return new glDisplay(); }
pddiRenderContext* glDevice::NewRenderContext(pddiDisplay* d) { return new glContext(static_cast<glDisplay*>(d)); }
pddiTexture* glDevice::NewTexture() { return new glTexture(); }
pddiPrimBuffer* glDevice::NewPrimBuffer(const pddiPrimBufferDesc& desc) { return new glPrimBuffer(desc); }
pddiBaseShader* glDevice::NewShader(const char*) { return new glShader(); }

// Platform factory

pddiDevice* pddiCreate() { return new glDevice(); }
