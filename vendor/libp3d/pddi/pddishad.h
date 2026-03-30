// pddishad.h — pddiBaseShader: abstract material interface
#ifndef PDDI_SHAD_H
#define PDDI_SHAD_H

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

    // Called by the render context around draw calls
    virtual void PreRender() = 0;
    virtual void PostRender() = 0;
};

#endif // PDDI_SHAD_H
