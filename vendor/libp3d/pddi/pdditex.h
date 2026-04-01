// pdditex.h — pddiTexture: abstract GPU texture interface
#pragma once

#include "pddi/pddi.h"

class pddiTexture : public pddiObject {
public:
    virtual int GetWidth() = 0;
    virtual int GetHeight() = 0;
    virtual int GetBpp() = 0;
    virtual int GetAlphaDepth() = 0;

    // Upload RGBA8 pixel data
    virtual void SetData(int width, int height, int bpp, int alphaDepth,
                         const void* rgba) = 0;

    // Bind for rendering on a texture unit
    virtual void Bind(int unit) = 0;
};
