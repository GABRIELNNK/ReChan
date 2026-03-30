// inventory.cpp — tInventory implementation
#include "p3d/inventory.h"

tInventory::~tInventory() {
    RemoveAll();
}

void tInventory::Store(tEntity* entity) {
    if (!entity) return;

    const std::string& name = entity->GetName();
    auto it = mEntities.find(name);
    if (it != mEntities.end()) {
        it->second->Release();
    }
    entity->AddRef();
    mEntities[name] = entity;
}

void tInventory::Remove(const std::string& name) {
    auto it = mEntities.find(name);
    if (it != mEntities.end()) {
        it->second->Release();
        mEntities.erase(it);
    }
}

tEntity* tInventory::Find(const std::string& name) {
    auto it = mEntities.find(name);
    return (it != mEntities.end()) ? it->second : nullptr;
}

void tInventory::RemoveAll() {
    for (auto& [name, entity] : mEntities)
        entity->Release();
    mEntities.clear();
}
