// pddi.h — Core types and enums for the device driver interface
#ifndef PDDI_H
#define PDDI_H

#include "core.h"

// ARGB packed colour

struct pddiColour {
    u8 r, g, b, a;

    pddiColour() : r(0), g(0), b(0), a(255) {}
    pddiColour(u8 r, u8 g, u8 b, u8 a = 255) : r(r), g(g), b(b), a(a) {}
};

// Enumerations

enum pddiPrimType {
    PDDI_PRIM_TRIANGLES,
    PDDI_PRIM_TRISTRIP,
    PDDI_PRIM_LINES,
    PDDI_PRIM_LINESTRIP,
    PDDI_PRIM_POINTS
};

enum pddiCullMode {
    PDDI_CULL_NONE,
    PDDI_CULL_NORMAL,
    PDDI_CULL_INVERTED
};

enum pddiFilterMode {
    PDDI_FILTER_NONE,
    PDDI_FILTER_BILINEAR,
    PDDI_FILTER_TRILINEAR
};

enum pddiBlendMode {
    PDDI_BLEND_NONE,
    PDDI_BLEND_ALPHA,
    PDDI_BLEND_ADD,
    PDDI_BLEND_SUBTRACT
};

enum pddiCompareMode {
    PDDI_COMPARE_NEVER,
    PDDI_COMPARE_LESS,
    PDDI_COMPARE_LESSEQUAL,
    PDDI_COMPARE_EQUAL,
    PDDI_COMPARE_GREATEREQUAL,
    PDDI_COMPARE_GREATER,
    PDDI_COMPARE_ALWAYS
};

// Clear buffer flags
enum pddiClearFlag {
    PDDI_BUFFER_COLOUR = 0x01,
    PDDI_BUFFER_DEPTH = 0x02,
    PDDI_BUFFER_ALL = PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH
};

// Shader parameter names
namespace PDDI_SP {
    constexpr u32 UVMODE = 0x01;
    constexpr u32 FILTER = 0x02;
    constexpr u32 AMBIENT = 0x03;
    constexpr u32 DIFFUSE = 0x04;
    constexpr u32 EMISSIVE = 0x05;
    constexpr u32 SPECULAR = 0x06;
    constexpr u32 SHININESS = 0x07;
    constexpr u32 BLENDMODE = 0x08;
    constexpr u32 TEXTURE = 0x09;
    constexpr u32 TWOSIDED = 0x0A;
    constexpr u32 ALPHATEST = 0x0B;
    constexpr u32 ISLIT = 0x0D;
}

// Reference-counted base for all pddi objects

class pddiObject {
public:
    pddiObject() : mRefCount(1) {}
    virtual ~pddiObject() = default;

    void AddRef() { ++mRefCount; }
    void Release() { if (--mRefCount <= 0) delete this; }
    int  GetRefCount() const { return mRefCount; }

private:
    int mRefCount;
};

#endif // PDDI_H
