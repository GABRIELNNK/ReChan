// inventory.cpp — tInventory implementation
#include "p3d/inventory.h"

tInventory::~tInventory() {
    RemoveAll();
}

void tInventory::Store(tEntity* entity) {
    if (!entity) return;

    const std::string& n = entity->GetName();
    auto it = entities.find(n);
    if (it != entities.end()) {
        it->second->Release();
    }
    entity->AddRef();
    entities[n] = entity;
}

void tInventory::Remove(const std::string& n) {
    auto it = entities.find(n);
    if (it != entities.end()) {
        it->second->Release();
        entities.erase(it);
    }
}

tEntity* tInventory::Find(const std::string& n) {
    auto it = entities.find(n);
    return (it != entities.end()) ? it->second : nullptr;
}

void tInventory::RemoveAll() {
    for (auto& [n, entity] : entities)
        entity->Release();
    entities.clear();
}
