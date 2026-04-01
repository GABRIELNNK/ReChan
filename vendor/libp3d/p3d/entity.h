// entity.h — tRefCounted + tEntity base classes
#pragma once

#include "core.h"
#include <string>

// UID hash type
using tUID = u32;

inline tUID MakeUID(const char* name) {
    // djb2 hash
    tUID hash = 5381;
    while (*name) {
        hash = ((hash << 5) + hash) + static_cast<u8>(*name);
        ++name;
    }
    return hash;
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
