// pddishad.h — pddiBaseShader: abstract material interface
#pragma once

#include "pddi/pddi.h"

class pddiTexture;

class pddiBaseShader : public pddiObject {
public:
    virtual const char* GetType() = 0;
    virtual int GetPasses() { return 1; }

    // Parameter setters
    virtual void SetTexture(u32 param, pddiTexture* tex) = 0;
    virtual void SetInt(u32 param, int value) = 0;
    virtual void SetFloat(u32 param, float value) = 0;
    virtual void SetColour(u32 param, pddiColour c) = 0;

    // Named parameters are used by programmable shaders. Backends own the
    // mapping from these renderer-independent names to their native uniforms.
    virtual void SetInt(const char* param, int value) = 0;
    virtual void SetFloat(const char* param, float value) = 0;
    virtual void SetVector(const char* param, float x, float y, float z, float w) = 0;
    virtual void SetMatrix(const char* param, const float* matrix4x4) = 0;

    // Called by the render context around draw calls
    virtual void PreRender() = 0;
    virtual void PostRender() = 0;
};
