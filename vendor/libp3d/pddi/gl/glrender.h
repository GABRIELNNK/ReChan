// glrender.h — OpenGL implementation of pddi interfaces
#ifndef GL_RENDER_H
#define GL_RENDER_H

#include "pddi/pddi.h"
#include "pddi/pdditex.h"
#include "pddi/pddishad.h"
#include "pddi/pddidev.h"

struct GLFWwindow;

// glTexture──

class glTexture : public pddiTexture {
public:
    glTexture();
    ~glTexture() override;

    int  GetWidth() override { return mWidth; }
    int  GetHeight() override { return mHeight; }
    int  GetBpp() override { return mBpp; }
    int  GetAlphaDepth() override { return mAlphaDepth; }

    void SetData(int w, int h, int bpp, int alphaDepth, const void* rgba) override;
    void Bind(int unit) override;

    u32 GetGLHandle() const { return mHandle; }

private:
    u32 mHandle = 0;
    int mWidth = 0;
    int mHeight = 0;
    int mBpp = 0;
    int mAlphaDepth = 0;
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

    u32 GetProgram() const { return mProgram; }

private:
    u32           mProgram = 0;
    pddiTexture* mTexture = nullptr;
    pddiColour    mDiffuse = pddiColour(255, 255, 255);
    pddiBlendMode mBlendMode = PDDI_BLEND_NONE;

    void CreateDefaultProgram();
};

// glDisplay──

class glDisplay : public pddiDisplay {
public:
    glDisplay();
    ~glDisplay() override;

    bool  InitDisplay(const pddiDisplayInit& init) override;
    void  SwapBuffers() override;
    int   GetWidth() override { return mWidth; }
    int   GetHeight() override { return mHeight; }
    bool  ShouldClose() override;
    void  PollEvents() override;
    void* GetHandle() override { return mWindow; }

    // Viewport with 4:3 letterboxing
    void GetViewport(int& x, int& y, int& w, int& h) const;

private:
    GLFWwindow* mWindow = nullptr;
    int mWidth = 0;
    int mHeight = 0;

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
};

// glContext──

class glContext : public pddiRenderContext {
public:
    glContext(glDisplay* display);
    ~glContext() override;

    void BeginFrame() override;
    void EndFrame() override;

    void SetClearColour(pddiColour c) override;
    void Clear(int flags) override;

    void SetProjectionMatrix(const Mat4& m) override;
    void SetViewMatrix(const Mat4& m) override;
    void SetWorldMatrix(const Mat4& m) override;

    void SetCullMode(pddiCullMode mode) override;
    void EnableZBuffer(bool enable) override;
    void SetBlendMode(pddiBlendMode mode) override;

    void DrawQuad(pddiBaseShader* shader,
                  float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1) override;

private:
    glDisplay* mDisplay;
    pddiColour mClearColour;
    Mat4       mProjection;
    Mat4       mView;
    Mat4       mWorld;
    u32        mQuadVAO = 0;
    u32        mQuadVBO = 0;

    void InitQuadMesh();
};

// glDevice──

class glDevice : public pddiDevice {
public:
    pddiDisplay* NewDisplay() override;
    pddiRenderContext* NewRenderContext(pddiDisplay* display) override;
    pddiTexture* NewTexture() override;
    pddiBaseShader* NewShader(const char* type) override;
};

#endif // GL_RENDER_H
