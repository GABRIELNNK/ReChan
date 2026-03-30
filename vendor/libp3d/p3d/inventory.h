// inventory.h — tInventory: named asset store
#ifndef P3D_INVENTORY_H
#define P3D_INVENTORY_H

#include "p3d/entity.h"
#include <unordered_map>
#include <string>

class tInventory {
public:
    ~tInventory();

    // Store entity (calls AddRef; overwrites existing)
    void Store(tEntity* entity);

    // Remove by name (calls Release)
    void Remove(const std::string& name);

    // Find by name
    tEntity* Find(const std::string& name);

    // Type-safe find
    template<typename T>
    T* Find(const std::string& name) {
        return dynamic_cast<T*>(Find(name));
    }

    // Remove all entities
    void RemoveAll();

    u32 Count() const { return static_cast<u32>(mEntities.size()); }

    const std::unordered_map<std::string, tEntity*>& GetAll() const { return mEntities; }

private:
    std::unordered_map<std::string, tEntity*> mEntities;
};

#endif // P3D_INVENTORY_H
