#include "gen/common.h"
#include "ai/trigger.h"
#include "ai/humanoid.h"
#include "gen/ai.h"
#include "gen/colvol.h"
#include "gen/database.h"
#include "p3d/hash.h"
#include <cstring>
#include <string>
#include <vector>

TriggerThing::TriggerThing(const LVector* pos, u16 type) : Obstacle(pos, type) {
    MARKFUNCTION(0x800A9958);
    targetHashes = nullptr;
    cachedTargets = nullptr;
    targetCount = 0;
    triggerName = nullptr;
}

TriggerThing::~TriggerThing() {
    MARKFUNCTION(0x800A999C);
    delete[] targetHashes;
    targetHashes = nullptr;
    delete[] cachedTargets;
    cachedTargets = nullptr;
    delete[] triggerName;
    triggerName = nullptr;
}

void TriggerThing::AnalyzeMesh(DBRoot* root) {
    MARKFUNCTION(0x800A9A20);

    if (!root) {
        return;
    }

    orientation.x = root->field40;
    orientation.y = root->field44;
    orientation.z = root->field48;

    if (DBVolume* volume = dynamic_cast<DBVolume*>(root)) {
        tagCollisionBox box = { 0x7FFF, 0x7FFF, 0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0 };
        FillCollisionBox(box, *volume);
        SetCollisionBox(box);
    }

    delete[] targetHashes;
    targetHashes = nullptr;
    delete[] cachedTargets;
    cachedTargets = nullptr;
    targetCount = 0;

    const DBAttrib* a6 = root->FindAttrib(6);
    if (a6 && a6->strValue && a6->strValue[0] != '\0') {
        std::vector<u32> hashes;
        std::string tokenList = a6->strValue;
        size_t start = 0;

        while (start <= tokenList.size()) {
            const size_t comma = tokenList.find(',', start);
            const size_t end = (comma == std::string::npos) ? tokenList.size() : comma;
            const size_t len = end - start;
            if (len != 0) {
                hashes.push_back(p3dHash(tokenList.substr(start, len).c_str()));
            }
            if (comma == std::string::npos) {
                break;
            }
            start = comma + 1;
        }

        if (!hashes.empty()) {
            targetCount = static_cast<s32>(hashes.size());
            targetHashes = new u32[targetCount];
            cachedTargets = new Obstacle*[targetCount];
            for (s32 i = 0; i < targetCount; i++) {
                targetHashes[i] = hashes[i];
                cachedTargets[i] = nullptr;
            }
        }
    }

    delete[] triggerName;
    triggerName = nullptr;

    const DBAttrib* a7 = root->FindAttrib(7);
    if (a7 && a7->strValue) {
        const size_t len = std::strlen(a7->strValue);
        triggerName = new char[len + 1];
        std::memcpy(triggerName, a7->strValue, len + 1);
    }

    const DBAttrib* a15 = root->FindAttrib(15);
    if (a15) {
        blockNum = static_cast<u16>(a15->value);
    }
}

void TriggerThing::CreateModel(const char* name) {
    MARKFUNCTION(0x800A9BE8);
    (void)name;
    flags |= TF_MODEL_CREATED;
}

void TriggerThing::DeleteModel() {
    MARKFUNCTION(0x800A9BFC);
    Thing::DeleteModel();
}

void TriggerThing::Reset() {
    MARKFUNCTION(0x800A9C1C);
}

void TriggerThing::Think() {
    MARKFUNCTION(0x800A9C24);
}

void TriggerThing::UpdatePosition() {
    MARKFUNCTION(0x800A9C2C);
}

bool TriggerThing::HandleCollision(Thing* source) {
    MARKFUNCTION(0x800A9C34);

    if (!source || !g_ai || targetCount <= 0 || !targetHashes || !cachedTargets) {
        return false;
    }

    for (s32 i = 0; i < targetCount; i++) {
        Obstacle* target = cachedTargets[i];
        if (!target) {
            ccNode* node = g_ai->moveList.FindNodeCRC(targetHashes[i]);
            if (node) {
                target = static_cast<Obstacle*>(static_cast<Thing*>(node));
                cachedTargets[i] = target;
            }
        }

        if (target) {
            target->TriggerByName(source, triggerName, reinterpret_cast<const char*>(this));
        }
    }

    return true;
}

void TriggerThing::HandlePickupCollision(Thing* pickup) {
    MARKFUNCTION(0x800A9D20);
    HandleCollision(pickup);
}

void TriggerThing::HandleHumanoidCollision(Humanoid* hum) {
    MARKFUNCTION(0x800A9D40);
    HandleCollision(hum);
}