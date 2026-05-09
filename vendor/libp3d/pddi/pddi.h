// pddi.h — Core types and enums for the device driver interface
#pragma once

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
    PDDI_BLEND_SUBTRACT,
    PDDI_BLEND_PSX_QUARTER
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

// Vertex format flags (bitfield)
enum pddiVertexFormat {
    PDDI_V_POSITION = 0x01,
    PDDI_V_COLOUR   = 0x02,
    PDDI_V_UV       = 0x04,
    PDDI_V_TEXINFO  = 0x08  // PSX tpage/cba pair
};

// Primitive buffer description
struct pddiPrimBufferDesc {
    pddiPrimType primType;
    u32 vertexFormat;
    u32 vertexCount;
    u32 indexCount;

    pddiPrimBufferDesc()
        : primType(PDDI_PRIM_TRIANGLES), vertexFormat(0)
        , vertexCount(0), indexCount(0) {}

    pddiPrimBufferDesc(pddiPrimType type, u32 format, u32 nVerts, u32 nIndices = 0)
        : primType(type), vertexFormat(format)
        , vertexCount(nVerts), indexCount(nIndices) {}
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
    pddiObject() : refCount(1) {}
    virtual ~pddiObject() = default;

    void AddRef() { ++refCount; }
    void Release() { if (--refCount <= 0) delete this; }
    int  GetRefCount() const { return refCount; }

private:
    int refCount;
};

// Retained-mode primitive buffer
class pddiPrimBuffer : public pddiObject {
public:
    virtual void SetVertexData(const void* data, u32 count) = 0;
    virtual void SetIndices(const u16* indices, u32 count) = 0;
    virtual u32 GetIndexCount() const = 0;
    virtual u32 GetVertexCount() const = 0;
    virtual pddiPrimType GetPrimType() const = 0;

protected:
    virtual ~pddiPrimBuffer() = default;
};
