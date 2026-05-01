#pragma once

#include "core.h"
#include "gen/cclist.h"

class Thing;
struct DBRoot;
struct DBVolume;

using SwitchGameFunc = s32(*)(Thing*, u32, const char**);
using SwitchFuncResolver = bool(*)(const char* funcName, SwitchGameFunc& outFunc, u32& outBucket);

struct WDBSwitchSlot {
    char* paramText = nullptr;
    const char* funcName = nullptr;
    const char** params = nullptr;
    u32 paramCount = 0;
    SwitchGameFunc func = nullptr;
};

class WDBSwitch : public ccNode {
public:
    ~WDBSwitch() override;

    bool Setup(DBRoot* root);
    bool Bind(SwitchFuncResolver resolver);
    s32 Execute(Thing* thing);
    s32 Reject(Thing* thing);

    virtual bool IsInside(const LVector& pos) const = 0;

    WDBSwitchSlot slots[2];
    s32 activeState = 0;
    u32 listBucket = 0;
    u32 persistent = 0;
};

class WDBVolumeSwitch : public WDBSwitch {
public:
    void SetVolume(DBVolume* volume);
    bool IsInside(const LVector& pos) const override;

private:
    s32 centerX = 0;
    s32 centerY = 0;
    s32 centerZ = 0;
    s32 halfX = 0;
    s32 halfY = 0;
    s32 halfZ = 0;
};
