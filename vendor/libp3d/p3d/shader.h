// shader.h — tShader: P3D shader entity wrapping pddiBaseShader
#ifndef P3D_SHADER_H
#define P3D_SHADER_H

#include "p3d/entity.h"
#include "pddi/pddi.h"

class pddiBaseShader;
class tTexture;

class tShader : public tEntity {
public:
    tShader(const char* definition = "simple");
    ~tShader() override;

    pddiBaseShader* GetShader() const { return mShader; }

    void SetTexture(u32 param, tTexture* tex);
    void SetInt(u32 param, int value);
    void SetFloat(u32 param, float value);
    void SetColour(u32 param, pddiColour c);

private:
    pddiBaseShader* mShader = nullptr;
};

#endif // P3D_SHADER_H
