#pragma once
#include "core.h"

struct xcOverlayData;

// oxOvl (8 bytes on PSX: OXOVL.CPP:98)
// Wrapper around a raw xcOverlayData* with virtual SelfInit.
// PSX layout: +0 xcOverlayData*, +4 vtable (MetroWerks)
class oxOvl {
public:
    xcOverlayData* overlay = nullptr;

    oxOvl();
    virtual ~oxOvl();
    virtual void SelfInit() {}

    void Init(xcOverlayData* ovl);
    void SetVisible(s16 vis);
    bool IsVisible() const;
    bool GetPrimPos(u8* prim, s16& x, s16& y) const;
    bool SetPrimPos(u8* prim, s16 x, s16 y) const;
};
