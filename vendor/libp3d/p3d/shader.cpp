// shader.cpp — tShader implementation
#include "p3d/shader.h"
#include "p3d/texture.h"
#include "p3d/context.h"
#include "pddi/pddidev.h"
#include "pddi/pddishad.h"
#include "pddi/pdditex.h"

tShader::tShader(const char* definition) {
    if (p3d::device)
        shader = p3d::device->NewShader(definition);
}

tShader::~tShader() {
    if (shader) {
        shader->Release();
        shader = nullptr;
    }
}

void tShader::SetTexture(u32 param, tTexture* tex) {
    if (shader)
        shader->SetTexture(param, tex ? tex->GetTexture() : nullptr);
}

void tShader::SetInt(u32 param, int value) {
    if (shader)
        shader->SetInt(param, value);
}

void tShader::SetFloat(u32 param, float value) {
    if (shader)
        shader->SetFloat(param, value);
}

void tShader::SetColour(u32 param, pddiColour c) {
    if (shader)
        shader->SetColour(param, c);
}

void tShader::SetTexture(u32 param, pddiTexture* tex) {
    if (shader)
        shader->SetTexture(param, tex);
}

void tShader::SetInt(const char* param, int value) {
    if (shader)
        shader->SetInt(param, value);
}

void tShader::SetFloat(const char* param, float value) {
    if (shader)
        shader->SetFloat(param, value);
}

void tShader::SetVector(const char* param, float x, float y, float z, float w) {
    if (shader)
        shader->SetVector(param, x, y, z, w);
}
