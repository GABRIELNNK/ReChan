#pragma once

#include "p3d/entity.h"
#include "pddi/pddi.h"

class pddiBaseShader;
class pddiTexture;
class tTexture;

class tShader : public tEntity {
public:
    tShader(const char* definition = "simple");
    ~tShader() override;

    pddiBaseShader* GetShader() const { return shader; }

    void SetTexture(u32 param, tTexture* tex);
    void SetTexture(u32 param, pddiTexture* tex);
    void SetInt(u32 param, int value);
    void SetFloat(u32 param, float value);
    void SetColour(u32 param, pddiColour c);
    void SetInt(const char* param, int value);
    void SetFloat(const char* param, float value);
    void SetVector(const char* param, float x, float y, float z = 0.0f, float w = 0.0f);

private:
    pddiBaseShader* shader = nullptr;
};
