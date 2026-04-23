#include "common.h"
#include "gen/switch.h"
#include "gen/database.h"

static bool SwitchStringEqualsNoCase(const char* lhs, const char* rhs) {
    if (!lhs || !rhs) {
        return false;
    }

    while (*lhs && *rhs) {
        unsigned char left = (unsigned char)*lhs++;
        unsigned char right = (unsigned char)*rhs++;

        if (left >= 'A' && left <= 'Z') {
            left = (unsigned char)(left + ('a' - 'A'));
        }
        if (right >= 'A' && right <= 'Z') {
            right = (unsigned char)(right + ('a' - 'A'));
        }

        if (left != right) {
            return false;
        }
    }

    return *lhs == '\0' && *rhs == '\0';
}

static char* AllocSwitchString(const char* text) {
    if (!text) {
        return nullptr;
    }

    size_t len = strlen(text);
    char* out = new char[len + 1];
    memcpy(out, text, len + 1);
    return out;
}

static void AppendSwitchText(char* dst, size_t dstSize, const char* src) {
    if (!dst || !src || dstSize == 0) {
        return;
    }

    size_t len = strlen(dst);
    if (len >= dstSize - 1) {
        return;
    }

    size_t srcIndex = 0;
    while (src[srcIndex] != '\0' && len + 1 < dstSize) {
        dst[len++] = src[srcIndex++];
    }
    dst[len] = '\0';
}

WDBSwitch::~WDBSwitch() {
    for (s32 index = 0; index < 2; index++) {
        delete[] slots[index].paramText;
        delete[] slots[index].params;
    }
}

bool WDBSwitch::Setup(DBRoot* root) {
    if (!root) {
        return false;
    }

    const DBAttrib* actionAttrib = root->FindAttrib(3);
    if (!actionAttrib) {
        return false;
    }

    slots[0].funcName = actionAttrib->GetAttribString();
    if (SwitchStringEqualsNoCase(slots[0].funcName, "ExitVolume")) {
        return false;
    }

    char combined[256] = {};
    if (const DBAttrib* attrib = root->FindAttrib(4)) {
        AppendSwitchText(combined, sizeof(combined), attrib->GetAttribString());
    }

    for (u32 attribID = 10; attribID < 15; attribID++) {
        const DBAttrib* attrib = root->FindAttrib(attribID);
        if (!attrib) {
            continue;
        }

        if (combined[0] != '\0') {
            AppendSwitchText(combined, sizeof(combined), ",");
        }
        AppendSwitchText(combined, sizeof(combined), attrib->GetAttribString());
    }

    slots[0].paramText = AllocSwitchString(combined[0] != '\0' ? combined : "0");

    if (const DBAttrib* attrib = root->FindAttrib(5)) {
        slots[1].funcName = attrib->GetAttribString();
        if (const DBAttrib* exitParams = root->FindAttrib(6)) {
            slots[1].paramText = AllocSwitchString(exitParams->GetAttribString());
        }
    }

    if (const DBAttrib* attrib = root->FindAttrib(30)) {
        persistent = attrib->value != 0 ? 1u : 0u;
    }

    if (SwitchStringEqualsNoCase(slots[0].funcName, "AsyncLoadGroup")
        || SwitchStringEqualsNoCase(slots[0].funcName, "CharacterModelLoad")
        || SwitchStringEqualsNoCase(slots[0].funcName, "Checkpoint")) {
        persistent = 1;
    }

    return true;
}

bool WDBSwitch::Bind(SwitchFuncResolver resolver) {
    if (!resolver) {
        return false;
    }

    bool matched = false;

    for (s32 index = 0; index < 2; index++) {
        WDBSwitchSlot& slot = slots[index];
        if (!slot.funcName) {
            continue;
        }

        SwitchGameFunc resolvedFunc = nullptr;
        u32 resolvedBucket = 0;
        if (!resolver(slot.funcName, resolvedFunc, resolvedBucket)) {
            continue;
        }

        slot.func = resolvedFunc;
        listBucket = resolvedBucket;
        slot.funcName = nullptr;
        matched = true;

        if (slot.paramText) {
            slot.paramCount = 1;
            for (char* cursor = slot.paramText; *cursor; cursor++) {
                if (*cursor == ',') {
                    *cursor = '\0';
                    slot.paramCount++;
                }
            }

            slot.params = new const char*[slot.paramCount];
            char* token = slot.paramText;
            for (u32 paramIndex = 0; paramIndex < slot.paramCount; paramIndex++) {
                slot.params[paramIndex] = token;
                token += strlen(token) + 1;
            }
        }
    }

    return matched;
}

s32 WDBSwitch::Execute(Thing* thing) {
    if (activeState == 0 && slots[0].func) {
        activeState = slots[0].func(thing, slots[0].paramCount, slots[0].params);
    }
    return activeState;
}

s32 WDBSwitch::Reject(Thing* thing) {
    if (activeState == 1 && slots[1].func) {
        slots[1].func(thing, slots[1].paramCount, slots[1].params);
    }

    activeState = 0;
    return 1;
}

void WDBVolumeSwitch::SetVolume(DBVolume* volume) {
    if (!volume) {
        return;
    }

    centerX = volume->pos.x;
    centerY = volume->pos.y;
    centerZ = volume->pos.z;
    halfX = (volume->bboxMax.x - volume->bboxMin.x) / 2;
    halfY = (volume->bboxMax.y - volume->bboxMin.y) / 2;
    halfZ = (volume->bboxMax.z - volume->bboxMin.z) / 2;
}

bool WDBVolumeSwitch::IsInside(const LVector& pos) const {
    s32 dx = pos.x - centerX;
    if (dx < 0) {
        dx = -dx;
    }
    if (dx >= halfX) {
        return false;
    }

    s32 dz = pos.z - centerZ;
    if (dz < 0) {
        dz = -dz;
    }
    if (dz >= halfZ) {
        return false;
    }

    s32 dy = pos.y - centerY;
    if (dy < 0) {
        dy = -dy;
    }
    return dy < halfY;
}
