// glrender.cpp — OpenGL implementation of pddi interfaces
#include "pddi/gl/glrender.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
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

    float r = float(clutWord & 0x1Fu) / 31.0;
    float g = float((clutWord >> 5u) & 0x1Fu) / 31.0;
    float b = float((clutWord >> 10u) & 0x1Fu) / 31.0;
    float a = clutWord == 0u ? 0.0 : 1.0;

    FragColor = vec4(r, g, b, a) * vec4(vColor, 1.0);
}
)";

static const char* kSimpleFrag = R"(
#version 450 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
    FragColor = texture(uTex, vUV);
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

// glTexture

glTexture::glTexture() = default;

glTexture::~glTexture() {
    if (mHandle)
        glDeleteTextures(1, &mHandle);
}

void glTexture::SetData(int w, int h, int bpp, int alphaDepth, const void* rgba) {
    mWidth = w;
    mHeight = h;
    mBpp = bpp;
    mAlphaDepth = alphaDepth;

    if (mHandle)
        glDeleteTextures(1, &mHandle);

    glGenTextures(1, &mHandle);
    glBindTexture(GL_TEXTURE_2D, mHandle);
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
    glBindTexture(GL_TEXTURE_2D, mHandle);
}

// glShader

glShader::glShader() {
    CreateDefaultProgram();
}

glShader::~glShader() {
    if (mProgram)
        glDeleteProgram(mProgram);
}

void glShader::CreateDefaultProgram() {
    u32 vs = CompileGLShader(GL_VERTEX_SHADER, kSimpleVert);
    u32 fs = CompileGLShader(GL_FRAGMENT_SHADER, kSimpleFrag);
    if (!vs || !fs) return;

    mProgram = glCreateProgram();
    glAttachShader(mProgram, vs);
    glAttachShader(mProgram, fs);
    glLinkProgram(mProgram);

    int ok;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(mProgram, sizeof(log), nullptr, log);
        std::fprintf(stderr, "GLSL link error:\n%s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void glShader::SetTexture(u32 param, pddiTexture* tex) { mTexture = tex; }
void glShader::SetInt(u32, int) {}
void glShader::SetFloat(u32, float) {}
void glShader::SetColour(u32 param, pddiColour c) {
    if (param == PDDI_SP::DIFFUSE)
        mDiffuse = c;
}

void glShader::PreRender() {
    glUseProgram(mProgram);
    if (mTexture) {
        mTexture->Bind(0);
        glUniform1i(glGetUniformLocation(mProgram, "uTex"), 0);
    }
}

void glShader::PostRender() {
    glUseProgram(0);
}

// glDisplay

glDisplay::glDisplay() = default;

glDisplay::~glDisplay() {
    if (mWindow) {
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
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

    mWindow = glfwCreateWindow(init.xSize, init.ySize, init.title, nullptr, nullptr);
    if (!mWindow) {
        std::fprintf(stderr, "GLFW: window creation failed\n");
        glfwTerminate();
        return false;
    }
    mWidth = init.xSize;
    mHeight = init.ySize;

    glfwMakeContextCurrent(mWindow);
    glfwSetWindowUserPointer(mWindow, this);
    glfwSetFramebufferSizeCallback(mWindow, FramebufferSizeCallback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::fprintf(stderr, "GLAD: failed to load OpenGL\n");
        return false;
    }

    std::printf("OpenGL %s on %s\n",
                reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    return true;
}

void glDisplay::SwapBuffers() { glfwSwapBuffers(mWindow); }
bool glDisplay::ShouldClose() { return glfwWindowShouldClose(mWindow); }
void glDisplay::PollEvents() { glfwPollEvents(); }

void glDisplay::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<glDisplay*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->mWidth = width;
        self->mHeight = height;
    }
}

void glDisplay::GetViewport(int& x, int& y, int& w, int& h) const {
    // 4:3 letterbox inside the window
    constexpr float TARGET_ASPECT = 4.0f / 3.0f;
    float windowAspect = static_cast<float>(mWidth) / static_cast<float>(mHeight);

    if (windowAspect > TARGET_ASPECT) {
        // Pillarbox (bars on sides)
        h = mHeight;
        w = static_cast<int>(mHeight * TARGET_ASPECT);
        x = (mWidth - w) / 2;
        y = 0;
    }
    else {
        // Letterbox (bars on top/bottom)
        w = mWidth;
        h = static_cast<int>(mWidth / TARGET_ASPECT);
        x = 0;
        y = (mHeight - h) / 2;
    }
}

// glContext

glContext::glContext(glDisplay* display)
    : mDisplay(display) {
    InitQuadMesh();
    Init3DShader();
}

glContext::~glContext() {
    if (mQuadVBO) glDeleteBuffers(1, &mQuadVBO);
    if (mQuadVAO) glDeleteVertexArrays(1, &mQuadVAO);
    if (m3DProgram) glDeleteProgram(m3DProgram);
}

void glContext::BeginFrame() {
    int vx, vy, vw, vh;
    mDisplay->GetViewport(vx, vy, vw, vh);
    glViewport(vx, vy, vw, vh);
    glScissor(vx, vy, vw, vh);
    glEnable(GL_SCISSOR_TEST);
}

void glContext::EndFrame() {}

void glContext::SetClearColour(pddiColour c) { mClearColour = c; }

void glContext::Clear(int flags) {
    GLbitfield mask = 0;
    if (flags & PDDI_BUFFER_COLOUR) {
        glClearColor(mClearColour.r / 255.0f, mClearColour.g / 255.0f,
                     mClearColour.b / 255.0f, mClearColour.a / 255.0f);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (flags & PDDI_BUFFER_DEPTH)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (mask)
        glClear(mask);
}

void glContext::SetProjectionMatrix(const Mat4& m) { mProjection = m; }
void glContext::SetViewMatrix(const Mat4& m) { mView = m; }
void glContext::SetWorldMatrix(const Mat4& m) { mWorld = m; }

void glContext::SetCullMode(pddiCullMode mode) {
    switch (mode) {
        case PDDI_CULL_NONE:     glDisable(GL_CULL_FACE); break;
        case PDDI_CULL_NORMAL:   glEnable(GL_CULL_FACE); glCullFace(GL_BACK); break;
        case PDDI_CULL_INVERTED: glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
    }
}

void glContext::EnableZBuffer(bool enable) {
    if (enable) glEnable(GL_DEPTH_TEST);
    else        glDisable(GL_DEPTH_TEST);
}

void glContext::SetBlendMode(pddiBlendMode mode) {
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
}

void glContext::DrawQuad(pddiBaseShader* shader,
                         float x, float y, float w, float h,
                         float u0, float v0, float u1, float v1) {
    shader->PreRender();

    auto* s = static_cast<glShader*>(shader);
    glUniformMatrix4fv(glGetUniformLocation(s->GetProgram(), "uProj"),
                       1, GL_FALSE, mProjection.Data());

    float verts[] = {
        x,     y,     u0, v1,
        x + w, y,     u1, v1,
        x + w, y + h, u1, v0,
        x,     y,     u0, v1,
        x + w, y + h, u1, v0,
        x,     y + h, u0, v0,
    };

    glBindVertexArray(mQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    shader->PostRender();
}

void glContext::InitQuadMesh() {
    glGenVertexArrays(1, &mQuadVAO);
    glGenBuffers(1, &mQuadVBO);
    glBindVertexArray(mQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
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
    if (!vs || !fs) return;

    m3DProgram = glCreateProgram();
    glAttachShader(m3DProgram, vs);
    glAttachShader(m3DProgram, fs);
    glLinkProgram(m3DProgram);

    int ok;
    glGetProgramiv(m3DProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(m3DProgram, sizeof(log), nullptr, log);
        std::fprintf(stderr, "3D shader link error:\n%s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void glContext::SetTexture(pddiTexture* tex) {
    mCurrentTexture = tex;
}

void glContext::SetVRAMHandle(u32 handle) {
    mVRAMHandle = handle;
}

void glContext::DrawPrimBuffer(pddiPrimType type, u32 vao, u32 indexCount) {
    glUseProgram(m3DProgram);

    Mat4 mvp = Mat4Multiply(mProjection, Mat4Multiply(mView, mWorld));
    glUniformMatrix4fv(glGetUniformLocation(m3DProgram, "uMVP"),
                       1, GL_FALSE, mvp.Data());

    int hasVRAM = mVRAMHandle ? 1 : 0;
    glUniform1i(glGetUniformLocation(m3DProgram, "uHasVRAM"), hasVRAM);
    if (mVRAMHandle) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mVRAMHandle);
        glUniform1i(glGetUniformLocation(m3DProgram, "uVRAM"), 0);
    }

    GLenum glMode = GL_TRIANGLES;
    switch (type) {
        case PDDI_PRIM_TRIANGLES: glMode = GL_TRIANGLES; break;
        case PDDI_PRIM_TRISTRIP:  glMode = GL_TRIANGLE_STRIP; break;
        case PDDI_PRIM_LINES:     glMode = GL_LINES; break;
        case PDDI_PRIM_LINESTRIP: glMode = GL_LINE_STRIP; break;
        case PDDI_PRIM_POINTS:    glMode = GL_POINTS; break;
    }

    glBindVertexArray(vao);
    glDrawElements(glMode, indexCount, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
}

// glDevice

pddiDisplay* glDevice::NewDisplay() { return new glDisplay(); }
pddiRenderContext* glDevice::NewRenderContext(pddiDisplay* d) { return new glContext(static_cast<glDisplay*>(d)); }
pddiTexture* glDevice::NewTexture() { return new glTexture(); }
pddiBaseShader* glDevice::NewShader(const char*) { return new glShader(); }

// Platform factory

pddiDevice* pddiCreate() { return new glDevice(); }
