// shader.cpp — tShader implementation
#include "p3d/shader.h"
#include "p3d/texture.h"
#include "p3d/context.h"
#include "pddi/pddidev.h"
#include "pddi/pddishad.h"
#include "pddi/pdditex.h"

tShader::tShader(const char* definition) {
    if (p3d::device)
        mShader = p3d::device->NewShader(definition);
}

tShader::~tShader() {
    if (mShader) {
        mShader->Release();
        mShader = nullptr;
    }
}

void tShader::SetTexture(u32 param, tTexture* tex) {
    if (mShader)
        mShader->SetTexture(param, tex ? tex->GetTexture() : nullptr);
}

void tShader::SetInt(u32 param, int value) {
    if (mShader)
        mShader->SetInt(param, value);
}

void tShader::SetFloat(u32 param, float value) {
    if (mShader)
        mShader->SetFloat(param, value);
}

void tShader::SetColour(u32 param, pddiColour c) {
    if (mShader)
        mShader->SetColour(param, c);
}
