#include "gen/common.h"
#include "extra/colorpulse.h"
#include "gen/time.h"

ColorPulse::ColorPulse()
    : m_base(66, 0, 0)
    , m_end(112, 73, 0)
    , m_current(66, 0, 0)
    , m_halfCycleDurationSec(0.5f)
    , m_elapsedSec(0.0f) {
}

ColorPulse::ColorPulse(const xcColour1555& baseColor, const xcColour1555& endColor, f32 halfCycleDurationSec)
    : m_base(baseColor)
    , m_end(endColor)
    , m_current(baseColor)
    , m_halfCycleDurationSec(halfCycleDurationSec)
    , m_elapsedSec(0.0f) {
}

void ColorPulse::SetColors(const xcColour1555& baseColor, const xcColour1555& endColor) {
    m_base = baseColor;
    m_end = endColor;
    Start();
}

void ColorPulse::SetHalfCycleDuration(f32 halfCycleDurationSec) {
    m_halfCycleDurationSec = halfCycleDurationSec;
}

void ColorPulse::Start() {
    m_elapsedSec = 0.0f;
    m_current = m_base;
}

void ColorPulse::Update() {
    if (!g_time) {
        m_current = m_base;
        return;
    }

    m_elapsedSec += g_time->GetDeltaTime();

    f32 duration = m_halfCycleDurationSec;
    if (duration <= 0.0f) {
        duration = 0.5f;
    }

    f32 progress = m_elapsedSec / duration;
    if (progress > 1.0f) {
        progress = 1.0f;
    }

    const s32 r = (s32)m_base.GetRed8() + (s32)(((s32)m_end.GetRed8() - (s32)m_base.GetRed8()) * progress);
    const s32 g = (s32)m_base.GetGreen8() + (s32)(((s32)m_end.GetGreen8() - (s32)m_base.GetGreen8()) * progress);
    const s32 b = (s32)m_base.GetBlue8() + (s32)(((s32)m_end.GetBlue8() - (s32)m_base.GetBlue8()) * progress);
    m_current.Set8((u8)r, (u8)g, (u8)b);

    if (m_elapsedSec >= duration) {
        while (m_elapsedSec >= duration) {
            m_elapsedSec -= duration;
        }
        m_current = m_base;
    }
}
