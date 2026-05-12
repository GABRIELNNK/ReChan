#pragma once

#include "core.h"
#include "xclib/xccolour.h"
#include <cmath>

class TextPulseEffect {
public:
    TextPulseEffect() {
        // Warm amber pulse by default.
        m_low.Set8(160, 96, 48, 255);
        m_high.Set8(255, 216, 128, 255);
        m_current = m_low;
    }

    void SetColors(const xcColour1555& low, const xcColour1555& high) {
        m_low = low;
        m_high = high;
        UpdateCurrent();
    }

    void SetCycleSeconds(f32 seconds) {
        if (seconds <= 0.0f) {
            seconds = 1.0f;
        }
        m_cycleSeconds = seconds;
        WrapElapsed();
        UpdateCurrent();
    }

    void Reset() {
        m_elapsedSeconds = 0.0f;
        UpdateCurrent();
    }

    void Step(f32 dtSeconds) {
        if (dtSeconds <= 0.0f) {
            return;
        }

        m_elapsedSeconds += dtSeconds;
        WrapElapsed();
        UpdateCurrent();
    }

    u32 GetColorABGR() const {
        return m_current.Get8();
    }

private:
    static u8 LerpU8(u8 a, u8 b, f32 t) {
        if (t <= 0.0f) {
            return a;
        }
        if (t >= 1.0f) {
            return b;
        }

        const f32 value = (f32)a + ((f32)b - (f32)a) * t;
        return (u8)(value + 0.5f);
    }

    void WrapElapsed() {
        if (m_cycleSeconds <= 0.0f) {
            m_cycleSeconds = 1.0f;
        }

        while (m_elapsedSeconds >= m_cycleSeconds) {
            m_elapsedSeconds -= m_cycleSeconds;
        }
    }

    void UpdateCurrent() {
        const f32 cycle = (m_cycleSeconds > 0.0f) ? m_cycleSeconds : 1.0f;
        const f32 phase = (cycle > 0.0f) ? (m_elapsedSeconds / cycle) : 0.0f;
        const f32 wave = 0.5f + 0.5f * std::sinf(phase * 6.28318530718f);

        const u8 r = LerpU8(m_low.GetRed8(), m_high.GetRed8(), wave);
        const u8 g = LerpU8(m_low.GetGreen8(), m_high.GetGreen8(), wave);
        const u8 b = LerpU8(m_low.GetBlue8(), m_high.GetBlue8(), wave);
        const u8 a = LerpU8(m_low.GetAlpha8(), m_high.GetAlpha8(), wave);
        m_current.Set8(r, g, b, a);
    }

    xcColour1555 m_low;
    xcColour1555 m_high;
    xcColour1555 m_current;
    f32 m_cycleSeconds = 1.1f;
    f32 m_elapsedSeconds = 0.0f;
};
