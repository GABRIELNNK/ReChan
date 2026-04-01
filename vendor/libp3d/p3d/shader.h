// shader.h — tShader: P3D shader entity wrapping pddiBaseShader
#pragma once

#include "p3d/entity.h"
#include "pddi/pddi.h"

class pddiBaseShader;
class tTexture;

class tShader : public tEntity {
public:
    tShader(const char* definition = "simple");
    ~tShader() override;

    pddiBaseShader* GetShader() const { return shader; }

    void SetTexture(u32 param, tTexture* tex);
    void SetInt(u32 param, int value);
    void SetFloat(u32 param, float value);
    void SetColour(u32 param, pddiColour c);

private:
    pddiBaseShader* shader = nullptr;
};
