#pragma once

#include "core.h"
#include "xclib/xccolour.h"

class ColorPulse {
public:
    ColorPulse();
    ColorPulse(const xcColour1555& baseColor, const xcColour1555& endColor, f32 halfCycleDurationSec = 0.5f);

    void SetColors(const xcColour1555& baseColor, const xcColour1555& endColor);
    void SetHalfCycleDuration(f32 halfCycleDurationSec);

    void Start();
    void Update();

    const xcColour1555& GetColor() const { return m_current; }
    u8 GetRed8() const { return m_current.GetRed8(); }
    u8 GetGreen8() const { return m_current.GetGreen8(); }
    u8 GetBlue8() const { return m_current.GetBlue8(); }
    u8 GetAlpha8() const { return m_current.GetAlpha8(); }
    u32 Get8() const { return m_current.Get8(); }

private:
    xcColour1555 m_base;
    xcColour1555 m_end;
    xcColour1555 m_current;
    f32 m_halfCycleDurationSec;
    f32 m_elapsedSec;
};
