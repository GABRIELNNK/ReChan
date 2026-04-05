// entity.h - tRefCounted + tEntity base classes
#pragma once

#include "core.h"
#include "p3d/hash.h"
#include <string>

// UID hash type
using tUID = u32;

// PSX v11.3: MakeUID calls p3dHash (PJW/hashpjw)
// TENTITY.CPP:114
inline tUID MakeUID(const char* name) {
    return p3dHash(name);
}

// Reference-counted base
class tRefCounted {
public:
    tRefCounted() : refCount(1) {}
    virtual ~tRefCounted() = default;

    void AddRef() { ++refCount; }
    void Release() { if (--refCount <= 0) delete this; }
    int  GetRefCount() const { return refCount; }

private:
    int refCount;
};

// Named entity base class
class tEntity : public tRefCounted {
public:
    tEntity() : uid(0) {}
    virtual ~tEntity() = default;

    void SetName(const char* n) {
        name = n;
        uid = MakeUID(n);
    }

    const std::string& GetName() const { return name; }
    tUID GetUID() const { return uid; }

private:
    std::string name;
    tUID uid;
};
