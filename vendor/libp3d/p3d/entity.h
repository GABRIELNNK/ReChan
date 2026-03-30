// entity.h — tRefCounted + tEntity base classes
#ifndef P3D_ENTITY_H
#define P3D_ENTITY_H

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
    tRefCounted() : mRefCount(1) {}
    virtual ~tRefCounted() = default;

    void AddRef() { ++mRefCount; }
    void Release() { if (--mRefCount <= 0) delete this; }
    int  GetRefCount() const { return mRefCount; }

private:
    int mRefCount;
};

// Named entity base class
class tEntity : public tRefCounted {
public:
    tEntity() : mUID(0) {}
    virtual ~tEntity() = default;

    void SetName(const char* name) {
        mName = name;
        mUID = MakeUID(name);
    }

    const std::string& GetName() const { return mName; }
    tUID GetUID() const { return mUID; }

private:
    std::string mName;
    tUID        mUID;
};

#endif // P3D_ENTITY_H
